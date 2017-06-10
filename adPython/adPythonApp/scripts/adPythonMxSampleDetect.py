#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin

from inspect import getargspec
from itertools import chain
import numpy as np
import cv2


# Unify the interface to morphological operations using a decorator such that
# all operations are called as `fn(arr, (param1, param2, ...))`. This makes it
# easier to expose functions with different numbers of arguments to the end
# user in a generic way.

def unified_interface(function):
    def wrapper(arr, params):
        n_params = len(getargspec(function).args) - 1  # (arr is not a param.)
        required_params = params[:n_params]  # Throw away unwanted params.
        return function(arr, *required_params)
    return wrapper


@unified_interface
def erode(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.erode(arr, element, iterations=iterations)


@unified_interface
def dilate(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.dilate(arr, element, iterations=iterations)


# `_morph` suffix to avoid name collision.
@unified_interface
def open_morph(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.morphologyEx(
        arr, cv2.MORPH_OPEN, element, iterations=iterations)


@unified_interface
def close(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.morphologyEx(
        arr, cv2.MORPH_CLOSE, element, iterations=iterations)


@unified_interface
def gradient(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.morphologyEx(
        arr, cv2.MORPH_GRADIENT, element, iterations=iterations)


@unified_interface
def top_hat(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.morphologyEx(
        arr, cv2.MORPH_TOPHAT, element, iterations=iterations)


@unified_interface
def black_hat(arr, ksize, iterations):
    element = cv2.getStructuringElement(cv2.MORPH_RECT, (ksize, ksize))
    return cv2.morphologyEx(
        arr, cv2.MORPH_BLACKHAT, element, iterations=iterations)


@unified_interface
def blur(arr, ksize):
    return cv2.blur(arr, (ksize, ksize))


@unified_interface
def gaussian_blur(arr, ksize):
    # Kernel size should be odd.
    if not ksize % 2: ksize += 1
    return cv2.GaussianBlur(arr, (ksize, ksize), 0)


@unified_interface
def median_blur(arr, ksize):
    if not ksize % 2: ksize += 1
    return cv2.medianBlur(arr, ksize)
    

@unified_interface
def canny_edge_detect(arr, upper_threshold, lower_threshold):
    # (Upper and lower threshold arguments commute.)
    return cv2.Canny(arr, upper_threshold, lower_threshold)


# List of candidate preprocessing functions.
# Order must match that in mbb* records.
pp_candidates = [
    erode,
    dilate,
    open_morph,
    close,
    gradient,
    top_hat,
    black_hat,
    blur,
    gaussian_blur,
    median_blur,
    lambda arr, params: arr,  # The "identity" process.
]


# A substitute for "None" which can fit into an np.int32 array/waveform record.
# EDM plot can't handle negative integers, so best to use 0 rather than -1.
none_value = 0


def locate_sample(edge_arr, params):
    # Straight port of Tom Cobb's algorithm from the original (adOpenCV) 
    # mxSampleDetect.

    direction, min_tip_height = params[:2]

    # Index into edges_arr like [y, x], not [x, y]!
    height, width = edge_arr.shape

    tip_y, tip_x = None, None
    top = [None]*width
    bottom = [None]*width

    rows = xrange(height)
    if direction == 1:
        columns = xrange(width)
    else:
        assert direction == -1
        columns = reversed(xrange(width))

    for x in columns:
        for y in rows:

            if not edge_arr[y, x]:
                continue
                
            if top[x] is None:
                top[x] = y

            bottom[x] = y

        # Look for the first non-narrow region between top and bottom edges.
        if tip_x is None and top[x] is not None \
        and abs(top[x] - bottom[x]) > min_tip_height:
            
            # Move backwards to where there were no edges at all...
            while top[x] is not None:
                x += -direction
                if x == -1 or x == width:
                    # (In this case the sample is off the edge of the picture.)
                    break
            x += direction # ...and forward one step. x is now at the tip.

            tip_x = x
            tip_y = int(round(0.5*(top[x] + bottom[x])))

            # Zero the edge arrays to the left (right) of the tip.
            if direction == 1:
                top[:x] = [None for _ in xrange(x)]
                bottom[:x] = [None for _ in xrange(x)]
            else:
                assert direction == -1
                top[x:] = [None for _ in xrange(x)]
                bottom[x:] = [None for _ in xrange(x)]

    # Prepare for export to PVs.
    top = np.asarray(
        [none_value if t is None else t for t in top], dtype=np.int32)
    bottom = np.asarray(
        [none_value if b is None else b for b in bottom], dtype=np.int32)
    if tip_y is None or tip_x is None:
        tip_y, tip_x = -1, -1

    return (tip_y, tip_x), (top, bottom)


def draw_circle(arr, (y, x), color, radius=5):
    cv2.circle(arr, (x, y), radius, color)


def draw_edges(arr, edges, color):
    for x, y in chain(*map(enumerate, edges)):
        if y != none_value: arr[y, x] = color


class MxSampleDetect(AdPythonPlugin):

    def __init__(self):

        # Default values. All params are integers in this case.
        params = dict(
            preprocess=0,  # Choose from the list of candidate functions.
            pp_param1=3,  # Generic parameter for preprocessing.
            pp_param2=1,  # Another. (Meaning imbued by use.)
            canny_upper=100,  # Thresholds for Canny edge detection.
            canny_lower=50,
            close_ksize=5,  # Kernel size for "close" operation.
            close_iterations=1,
            scan_direction=+1,  # +1:LtR, -1:RtL
            min_tip_height=5,
            tip_x=-1,  # Pixel positions of detected tip.
            tip_y=-1,  # (Not really parameters...)
            top=np.asarray([0], dtype=np.int32),  # Edge waveforms.
            bottom=np.asarray([0], dtype=np.int32),  # (Not parameters either.)
            out_arr=0,  # Which array to put downstream.
            draw_circle=0,  # Annotation options.
            draw_edges=0,
            force_color=0,  # Expand colour depth to show colour annotations?
        )

        AdPythonPlugin.__init__(self, params)

    def processArray(self, arr, attr={}):
        # Get a greyscale version of the input.
        if arr.ndim == 3:
            gray_arr = cv2.cvtColor(arr, cv2.COLOR_BGR2GRAY)
        else:
            assert arr.ndim == 2
            gray_arr = arr

        # Preprocess the array. (Use the greyscale one.)
        pp_params = (self['pp_param1'], self['pp_param2'])
        pp_arr = pp_candidates[self['preprocess']](gray_arr, pp_params)

        # (Could do a remove_dirt step here if wanted.)

        # Find some edges.
        canny_params = (self['canny_upper'], self['canny_lower'])
        edge_arr = canny_edge_detect(pp_arr, canny_params)

        # Do a "close" image operation. (Add other options?)
        close_params = (self['close_ksize'], self['close_iterations'])
        closed_arr = close(edge_arr, close_params)

        # Find the sample.
        location_params = (self['scan_direction'], self['min_tip_height'])
        tip, edges = locate_sample(closed_arr, location_params)

        # Write our results to PVs.
        self['tip_y'], self['tip_x'] = tip
        self['top'], self['bottom'] = edges

        # Select whichever array the user wants passed down to others in the
        # image processing chain.
        out = (arr, gray_arr, pp_arr, edge_arr, closed_arr)[self['out_arr']]

        # Find out which annotation(s) the user wants (if any).
        do_circle, do_edges = self['draw_circle'], self['draw_edges']

        # Expand the output colour depth to enable colour annotations if
        # required and requested to do so. Choose the colour for annotations.
        if self['force_color'] and (do_circle or do_edges):
            color = [255, 0, 0]
            if out.ndim == 2:
                out = cv2.cvtColor(out, cv2.COLOR_GRAY2RGB)
        else:
            color = [255, 0, 0] if out.ndim == 3 else 255

        # If we're annotating the original, we need to make a writable copy.
        if out is arr and (do_circle or do_edges): out = arr.copy()

        # Do the annotations.
        if do_circle: draw_circle(out, tip, color)
        if do_edges: draw_edges(out, edges, color)

        return out


if __name__ == '__main__':
    # This script can be run offline with
    # `PYTHONPATH+=../src/ dls-python adPythonMxSampleDetect.py`.

    # Args passed to .runOffline define ranges for sliders.
    MxSampleDetect().runOffline(
        preprocess=11,
        pp_param1=200,
        pp_param2=200,
        canny_upper=256,
        canny_lower=256,
        close_ksize=200,
        close_iterations=200,
        min_tip_height=100,
        tip_x=500,
        tip_y=500,
        out_arr=5,
    )
