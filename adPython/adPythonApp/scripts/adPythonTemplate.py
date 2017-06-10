#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import logging, numpy

try:
    # If we have the opencv library then we can use an add that
    # saturates the datatype rather than overflowing
    from cv2 import add
except ImportError:
    # otherwise we will use the numpy array add operation which is 
    # faster but will overflow
    from numpy import add    

class Template(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        self.log.setLevel(logging.DEBUG) 
        # Make some generic parameters
        # You can change the Name fields on the EDM screen here
        # Hide them by making their name -1
        params = dict(int1 = 1,      int1Name = "Array offset",
                      int2 = 2,      int2Name = "Array Sum",
                      int3 = 3,      int3Name = "-1",
                      double1 = 1.0, double1Name = "Double 1",
                      double2 = 2.0, double2Name = "Double 2",
                      double3 = 3.0, double3Name = "-1")
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # just log it for now, do nothing.
        self.log.debug("Parameter has been changed %s", self)

    def processArray(self, arr, attr={}):        
        # Called when the plugin gets a new array
        # arr is a numpy array
        # attr is an attribute dictionary that will be attached to the array
        # In our example we will take our array and add int1 to it.
        try:
            out = add(arr, self["int1"])           
        except TypeError:
            # early versions of opencv didn't let you add a scalar to an array
            out = add(arr, numpy.zeros(arr.shape, arr.dtype) + self["int1"])                        
        # Stamp the output array with the sum of the resultant array by writing
        # to the attribute dictionary. Note that we convert from the numpy
        # uint64 type to a python integer as we only handle python integers,
        # doubles and strings in the C code for now
        attr["sum"] = int(out.sum())
        self["int2"] = attr["sum"]
        self.log.debug("Array processed, sum: %d", attr["sum"])        
        # return the resultant array.
        return out

if __name__=="__main__":
    Template().runOffline(
        int1=256,            # This has range 0..255
        int2=(100,200),      # This has range 100..199
        double1=(0,5,0.001), # This has range 0, 0.001, 0.002 ... 4.999
        double2=numpy.logspace(1, 5, 3)) # This is 10^1, 10^3, 10^5

