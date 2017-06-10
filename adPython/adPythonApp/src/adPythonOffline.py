# we need the cv2 lib for reading files and the highgui
import cv2, os, numpy, signal
from optparse import OptionParser

class AdPythonOffline(object):   
    def __init__(self, plugin, **ranges):
        ranges.update(getattr(plugin, "ranges", {}))
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
        
        # Create a window to show the settings
        self.settings_name = 'adPython Settings: %s' % plugin.__class__.__name__
        cv2.namedWindow(self.settings_name, cv2.WINDOW_NORMAL)
        
        # Create a window to show the results in
        self.results_name = 'adPython Results: %s' % plugin.__class__.__name__
        cv2.namedWindow(self.results_name, cv2.WINDOW_NORMAL)
        cv2.setMouseCallback(self.results_name, self.onMouse)
                                
        # now create the sliders on the gui
        self.ignoreSliders = False        
        self.createSliders(ranges)

        # catch CTRL-C
        signal.signal(signal.SIGINT, signal.SIG_DFL)        

        # main loop waiting for user input
        self.main()

    def updateSliders(self):
        self.ignoreSliders = True
        for param in sorted(self.plugin):
            paramvalue = self.plugin[param]            
            # skip non numeric types
            if type(paramvalue) not in (int, float):
                continue        
            # clamp the initial value to range
            index = min(self.paramvalues[param].searchsorted(paramvalue), len(self.paramvalues[param]) - 1)
            self.plugin[param] = type(self.plugin[param])(self.paramvalues[param][index])
            # Override name if param+"Name" exists
            if param + "Name" in self.plugin:
                paramName = self.plugin[param + "Name"]
            else:
                paramName = param  
            # If paramName is -1 then hide it
            if paramName == "-1":
                continue
            # update slider position
            cv2.setTrackbarPos(paramName, self.settings_name, index)    
        self.ignoreSliders = False
                            

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
            # Override name if param+"Name" exists
            if param + "Name" in self.plugin:
                paramName = self.plugin[param + "Name"]
            else:
                paramName = param
            # If paramName is -1 then hide it
            if paramName == "-1":
                continue
            # Now create a slider for it
            cv2.createTrackbar(paramName, self.settings_name, index, 
                len(self.paramvalues[param]) - 1, lambda v, param=param: self.sliderChanged(param, v))
        self.plugin._paramChanged()  

    def onMouse(self, event, x, y, *args):
        if event == 1:
            print "Clicked", x, y
        
    def main(self):    
        firstRun = True
        while True:
            # update the image on screen
            result = self.update()            
            if firstRun:
                firstRun = False
                # move windows to a sensible place
                height = result.shape[0]
                width = result.shape[1]
                if height < 640:
                    width = int(640./height * width)
                    height = 640
                if height > 1024:
                    width = int(1024./height * width)
                    height = 640
                cv2.resizeWindow(self.results_name, width, height)
                cv2.moveWindow(self.results_name, 0, 0)
                cv2.resizeWindow(self.settings_name, 250, height)
                cv2.moveWindow(self.settings_name, width, 0) 
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
        if self.ignoreSliders:
            return
        self.plugin[param] = type(self.plugin[param])(self.paramvalues[param][index])
        self.plugin._paramChanged()
        self.update()
        
    # update the image
    def update(self):
        # read the input array
        src = cv2.imread(self.files[self.findex], self.options.rgb)
        # RGB please!
        if self.options.rgb:
            src = cv2.cvtColor(src, cv2.COLOR_BGR2RGB)
        # pass it to the process function
        result = self.plugin._processArray(src, {})
        # show the input image if nothing returned
        if result is None:
            result = src
        # display and return
        if len(result.shape) == 3:
            result = cv2.cvtColor(result, cv2.COLOR_RGB2BGR)        
        cv2.imshow(self.results_name, result)
        # update the trackbar positions
        self.updateSliders()        
        return result    
                
