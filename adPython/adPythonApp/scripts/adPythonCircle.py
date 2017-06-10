#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import logging, numpy, cv2

class Circle(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        self.log.setLevel(logging.DEBUG) 
        # Make some generic parameters
        # You can change the Name fields on the EDM screen here
        # Hide them by making their name -1
        params = dict(dp = 1.0, minDist = 10, minRadius = 120, maxRadius = 150,
            drawCircles = 1, param1 = 120, x = 0.0, y = 0.0)
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # just log it for now, do nothing.
        self.log.debug("Parameter has been changed %s", self)

    def processArray(self, arr, attr={}):        
        # Called when the plugin gets a new array
        # arr is a numpy array
        # attr is an attribute dictionary that will be attached to the array
        # convert to grey
        if len(arr.shape) == 3:
            gray = cv2.cvtColor(arr, cv2.COLOR_RGB2GRAY)
        else:
            # already gray
            gray = arr
        # run circle finding on it        
        circles = cv2.HoughCircles(gray, cv2.cv.CV_HOUGH_GRADIENT, self["dp"], 
            self["minDist"], minRadius = self["minRadius"], 
            maxRadius = self["maxRadius"], param1 = self["param1"])
        # if we find circles
        if circles is not None:
            xs, ys, rs = [], [], []
            for circle in circles[0]:                            
                # store the x and y centre positions and radius
                xs.append(circle[0])
                ys.append(circle[1])
                rs.append(circle[2])
            # calc the mean of all circle centres and store to a param
            x = numpy.mean(xs)
            y = numpy.mean(ys)
            self["x"] = x
            self["y"] = y
            # draw on the output image if asked to            
            if self["drawCircles"]:
                # copy the array so we can write on it
                out = arr.copy()
                for x, y, radius in zip(xs, ys, rs):
                    # draw a circle on it
                    cv2.circle(out, (x, y), radius, (0, 0, 255))
                # draw a cross for the centre position
                x, y = int(x), int(y)        
                cv2.line(out, (x-10, y), (x+10, y), (0, 0, 255))
                cv2.line(out, (x, y-10), (x, y+10), (0, 0, 255))                
                return out

if __name__=="__main__":
    Circle().runOffline(
        maxRadius = 500, dp = (0, 5, 0.1), param1 = 400)

