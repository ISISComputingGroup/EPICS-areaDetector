#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import numpy
import sys
import logging
import time

from pkg_resources import require
require('pyzmq')
require('msgpack-python')
import zmq

import blosc
import msgpack

class Transfer(AdPythonPlugin):

    def __init__(self):
        self.log.setLevel(logging.DEBUG)
        params = dict(tcpport = 34567, num_clients=0, level=6, ratio=0.0)
        AdPythonPlugin.__init__(self, params)
        
        self.zmq_context = zmq.Context()
        self.zmq_socket = self.zmq_context.socket(zmq.PUB)
        self.zmq_socket.bind("tcp://*:%d"%self['tcpport'])
        self.log.debug("HWM: %d", self.zmq_socket.get_hwm())
        self.zmq_socket.set_hwm(2)
        
    def paramChanged(self):
        pass
    
    def processArray(self, arr, attr):
        self.log.debug('packing array with shape: %s', str(arr.shape))
        
        # Pack a description of the array dimensions and datatype into a packet
        msg_array_desc = msgpack.packb({'dtype': str(arr.dtype), 'shape': arr.shape})

        # Compress the data from the array without making a copy (i.e. by passing a read-only
        # pointer to the blosc library.
        arr_ptr = arr.__array_interface__['data'][0]
        msg_array = blosc.compress_ptr(arr_ptr, arr.size, arr.dtype.itemsize, self['level'], True)
        # Calculate the resulting compression ratio
        self['ratio'] = float(len(msg_array)) / float(arr.nbytes)
        self.log.debug("Compressed ratio: %d/%d %.3f", arr.nbytes, len(msg_array), self['ratio'])

        # Pack the NDAttribute dictionary into a packet
        self.log.debug('packing attributes')
        msg_attr = msgpack.packb(attr)
        self.log.debug('    result dict length: %d', len(msg_attr))
        
        # Send the packets in a multipart zmq message.
        self.log.debug('sending multipart message')
        tracker = self.zmq_socket.send_multipart([msg_array_desc, msg_array, msg_attr], copy=False, track=True)
        
        # Wait for ZMQ to report that it has completed. We can possibly ignore this step - but then we would be
        # relying on the ZMQ buffering rather than our areaDetector buffers - and that is much less configurable.
        #self.log.debug('waiting for send to complete')
        #try:
        #    tracker.wait(1.0)
        #except zmq.NotDone:
        #    self.log.exception('Timeout when waiting to complete transfer: %s', str(zmq.NotDone))
        
        # All done, ready for new frame!
        self.log.debug('Processing done!')
        return None
        
if __name__=="__main__":
    Focus().runOffline()
