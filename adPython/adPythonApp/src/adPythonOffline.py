# we need the cv2 lib for reading files and the highgui
import cv2, os, numpy
from optparse import OptionParser

class AdPythonOffline(object):   
    def __init__(self, plugin, **ranges):
        self.plugin = plugin
        # first see if we passed any images in the commandline
        parser = OptionParser("""usage: %prog [files...]
        
This program will test the adPythonPlugin with a set of offline files. 
For each f in files:
- if f is a directory then add any png, jpg or tiff files within it to images
- otherwise add f to images
Display a gui that lets the user interactively change parameters on each image
- hit n for next image
- hit p for previous image
- hit q to quit""")
        parser.add_option("-r", "--rgb", dest="rgb", help="Allow rgb images to be passed straight through without converting them to greyscale", default=False, action="store_true")
        self.options, args = parser.parse_args()
        # If no files, load the test image
        if not args:
            self.files = [os.path.realpath(os.path.join(__file__, "..", "..", 
                "..", "test.jpg"))]
        else:
            self.files = []
            for a in args:
                # if argument is a dir add image files in it
                if os.path.isdir(a):
                    for f in os.listdir(a):
                        if f.split(".")[-1] in ("png", "jpg", "tif", "tiff"):
                            self.files.append(os.path.join(a, f))
                # otherwise add it without checking
                else:
                    self.files.append(a)
        self.findex = 0            
        
        # Create a window to show the results in
        self.window_name = 'adPythonOffline: %s' % plugin.__class__.__name__
        cv2.namedWindow(self.window_name)
        
        # now create the sliders on the gui
        self.createSliders(ranges)

        # main loop waiting for user input
        self.main()

    def createSliders(self, ranges):
        # create a trackbar for each parameter
        self.paramvalues = {}
        for param in sorted(self.plugin):
            paramvalue = self.plugin[param]
            # skip non numeric types
            if type(paramvalue) not in (int, float):
                continue
            # if no range information, make some up
            if param in ranges:
                paramrange = ranges[param]
            else:
                paramrange = 100
            # create range information. If we have a numpy array then use it,
            if type(paramrange) == numpy.ndarray:
                self.paramvalues[param] = paramrange
            # otherwise if we have a tuple or list just pass *it to numpy.arange                
            elif type(paramrange) in (list, tuple):                
                self.paramvalues[param] = numpy.arange(*paramrange)
            # otherwise pass it directly through
            else:
                self.paramvalues[param] = numpy.arange(paramrange)
            # clamp the initial value to range
            index = min(self.paramvalues[param].searchsorted(paramvalue), len(self.paramvalues[param]) - 1)
            self.plugin[param] = type(self.plugin[param])(self.paramvalues[param][index])
            # Now create a slider for it
            cv2.createTrackbar(param, self.window_name, index, 
                len(self.paramvalues[param]) - 1, lambda v, param=param: self.sliderChanged(param, v))
        self.plugin.paramChanged()  
        
    def main(self):        
        while True:
            # update the image on screen
            self.update()
            # see what the user wants to do next
            key = cv2.waitKey(0)
            if key & 0xFF == ord('q') or key == -1: # q or close window
                break
            elif key & 0xFF == ord('n'):
                self.findex += 1
                if self.findex >= len(self.files):
                    self.findex = 0                
            elif key & 0xFF == ord('p'):
                self.findex -= 1
                if self.findex < 0:
                    self.findex = len(self.files) - 1
                    
    # called when slider value changed
    def sliderChanged(self, param, index):
        self.plugin[param] = type(self.plugin[param])(self.paramvalues[param][index])
        self.plugin.paramChanged()
        self.update()
        
    # update the image
    def update(self):
        # read the input array
        src = cv2.imread(self.files[self.findex], self.options.rgb)
        # pass it to the process function
        result = self.plugin.processArray(src, {})
        cv2.imshow(self.window_name, result)
                
