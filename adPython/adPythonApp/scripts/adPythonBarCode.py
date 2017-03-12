#!/usr/bin/env dls-python
import numpy
import sys
import logging

from adPythonPlugin import AdPythonPlugin
import cv2
import zbar

class BarCodeSymbol:
    def __init__(self):
        self.data = ''
        self.type = ''
        self.polygon = []
        self.quality = 0
        
    def __str__(self):
        s = "<BarCodeSymbol type=\'%s\', data=\'%s\', quality=%d, points:%d>" % (self.type, self.data, self.quality, len(self.polygon))
        return s
    
    def points_to_polygon(self, points):
        '''Takes a set of points in lists or tuples: ((x1,y1), (x2,y2), ... (xn,yn))
        and converts it into a shape that OpenCV can treat as points in a polygon.'''
        points = numpy.array(points, numpy.int32)
        self.polygon = points.reshape((-1, 1, 2))
    
    @staticmethod
    def quality_sorted(obj_list):
        return sorted( obj_list, key=lambda x: x.quality, reverse=True )
        
class BarCodeDecoder:
    '''BarDecoder base class to provide a common interface for 
    implementing various barcode libraries decode support'''
    def __init__(self, logger='BarCodeDecoder'):
        self.log = logging.getLogger(logger)
        self.symbols = []
        
    def __str__(self):
        s = "< BarDecoder symbols: %s >" % str([str(s) for s in self.symbols])
        return s
        
    def decode(self, array, threshold = 50):
        '''Decode the barcode symbols from a 2D array
        The input array should be a numpy 2D array of unsigned 8 bpp elements.
        Returns the number of symbols decoded from the array.'''
        raise NotImplementedError

class ZBarDecoder(BarCodeDecoder):
    '''Use ZBar to decode a range of barcode types'''
    def __init__(self, logger='ZBarDecoder'):
        BarCodeDecoder.__init__(self, logger)
        self._scanner = zbar.ImageScanner()
        self._scanner.parse_config('enable')
    
    def decode(self, array, threshold = 50):
        self.symbols = []
        zimg = zbar.Image( array.shape[0], array.shape[1], 'Y800', array.tostring() )
        self._scanner.scan(zimg)
        for zb_symbol in zimg:
            symbol = BarCodeSymbol()
            symbol.type = str(zb_symbol.type)
            symbol.data = zb_symbol.data
            symbol.quality = int(zb_symbol.quality)
            symbol.points_to_polygon(zb_symbol.location)
            self.symbols.append(symbol)
        self.symbols = BarCodeSymbol.quality_sorted( self.symbols )
        return len(self.symbols)


class BarCode(AdPythonPlugin):
    SCANNER_ALL= 0
    SCANNER_ZBAR = 1

    def __init__(self):
        #self.log.setLevel(logging.DEBUG)
        params = dict(data = "", type = "", count = 0, quality = 0, threshold = 25, 
                      busy = 0, scanner = self.SCANNER_ZBAR)
        AdPythonPlugin.__init__(self, params)
        self.scanners = { self.SCANNER_ZBAR: ZBarDecoder() }
        self._busy = 0
        
    def paramChanged(self):
        # Check if user has made busy record busy
        if self['busy'] == 1 and (self['busy'] != self._busy):
            self.log.debug('Starting scan!')
            self._busy = 1
            self['count'] = 0
            self['data'] = ""
            self['type'] = ""
            self['quality'] = 0
    
    def processArray(self, arr, attr):
        # If the user has not started scanning then abort immediately
        if not self['busy'] == 1: return None
        
        # Work out which of the decoders to use - or all of them
        if self['scanner'] == self.SCANNER_ALL: 
            decoders = self.scanners.values()
        else: 
            decoders = [self.scanners[self['scanner']]]
        
        # Run through the decoders and check if they find barcodes.
        # Break out of the loop as soon as a decoder has a hit
        count = 0
        for decoder in decoders:
            count = decoder.decode( arr, threshold = self['threshold'] )
            if count > 0: break
        
        # Abort if no barcodes was found
        if count == 0: return None
        
        self.log.debug('Found %d symbols (%s)', count, str(decoder))

        # Find the best quality result of the scan
        symbol = decoder.symbols[0]
        
        # Abort if the quality of the scan was below desired threshold
        if symbol.quality < self['threshold']:
            self.log.warning('The detection quality was below threshold (%d < %d) (False match) %s', 
                             symbol.quality, self['threshold'], str(decoder))
            return None
        
        self.log.info('Symbol found: %s', str(symbol))
        # Update the user parameters with the symbol data and details
        self['count'] = count
        self['data'] = symbol.data
        self['type'] = symbol.type
        self['quality'] = symbol.quality
        self['busy'] = 0
     
        # Take a copy of the input array in order to return a new array
        dest = numpy.array(arr)
        
        # If a polygon of the scan position of the barcode is available, draw it
        # on top of the output array
        if len(symbol.polygon) > 0:
            self.log.debug("Drawing polygon on output: %s", str(dest.shape))
            cv2.polylines(dest, [symbol.polygon], True, 0, 5)
        
        return dest
        
if __name__=="__main__":
    Focus().runOffline()
