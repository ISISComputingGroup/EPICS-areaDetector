#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import cv2
import numpy
import logging

class Focus(AdPythonPlugin):
    def __init__(self):
        # Set a debug logging level in the local logger
        #self.log.setLevel(logging.DEBUG)    
        params = dict(ksize = 3, prefilter = 0, iters = 1,
                      sum = 0.0, filtered_mean = 0.0, filtered_stddev = 0.0)
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        ksize = self["ksize"]
        self.element = cv2.getStructuringElement(cv2.MORPH_OPEN, (ksize, ksize))
        self.log.info('Changed parameter, ksize=%s', str(ksize))

    def processArray(self, arr, attr):
        # got a new image to process
        self.log.debug("arr size: %s", arr.shape)
        self.log.debug("parameters: %s", str(self._params))
        
        if self['prefilter'] > 0:
            dest = cv2.morphologyEx(arr, cv2.MORPH_OPEN, self.element)
        else:
            dest = arr
        dest = cv2.morphologyEx(dest, cv2.MORPH_GRADIENT, 
                                self.element, iterations = self['iters'])
        #hist = numpy.histogram(dest, bins = self.params['bins'], range = (100,5000))
        #correcthist = (hist[0], hist[1][:-1])
        meanstddev = cv2.meanStdDev(dest)
        self.log.debug("mean stddev: %s", str(meanstddev))
        self['filtered_mean'] = meanstddev[0][0][0]
        self['filtered_stddev'] = meanstddev[1][0][0]
        self['sum'] = cv2.sumElems(dest)[0]
        
        return dest

if __name__=="__main__":
    Focus().runOffline()
