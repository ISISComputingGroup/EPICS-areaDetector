#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import numpy


class PowerMean(AdPythonPlugin):
    '''Computes the mean power of a sequence of updates.'''

    def __init__(self):
        # The only parameter is the sample count for averaging.
        params = dict(count = 1)
        AdPythonPlugin.__init__(self, params)

        self.seen = 0

    def processArray(self, arr, attr={}):
        if self.seen and self.data.shape == arr.shape:
            self.data += arr * arr
            self.seen += 1
        else:
            self.data = arr * arr
            self.seen = 1

        if self.seen >= self['count']:
            result = numpy.sqrt(self.data / self.seen)
            self.seen = 0
            self.data = None
            return result
