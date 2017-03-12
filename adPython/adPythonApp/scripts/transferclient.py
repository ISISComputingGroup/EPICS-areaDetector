#!/usr/bin/env dls-python
import sys
import logging
import time

from pkg_resources import require
require('numpy')
require('pyzmq')

sys.path.insert(0, '/dls_sw/work/tools/RHEL6-x86_64/python-blosc/prefix/lib/python2.7/site-packages')
sys.path.insert(0, '/dls_sw/work/tools/RHEL6-x86_64/msgpack-python/prefix/lib/python2.7/site-packages')

import numpy as np
import zmq
import blosc
import msgpack
import cv2

logging.basicConfig(format='%(asctime)s %(levelname)8s %(name)8s %(filename)s:%(lineno)d: %(message)s', level=logging.INFO)

class TransferClient:
    def __init__(self, host, port):
        self.log = logging.getLogger('TransferClient')
        self.log.setLevel(logging.DEBUG)

        self.zmq_context = zmq.Context()
        self.zmq_socket = self.zmq_context.socket(zmq.SUB)
        publisher = "tcp://%s:%s"%(str(host), str(port))
        self.log.info("Binding to publisher: \'%s\'"%publisher)
        self.zmq_socket.connect(publisher)
        self.zmq_socket.setsockopt_string(zmq.SUBSCRIBE, u"")
        
    def run(self):
        self.log.info("Starting receive loop")
        while True:
            arr, attr = self.receive_msg()
            
    def run_display(self, winname='blosc compressed stream'):
        self.log.info("Starting receive loop with window: \"%s\"",str( winname ))
        self.log.info("Press \'q\' to quit")
        cv2.namedWindow(winname, flags=1)
        cv2.waitKey(20)
        while True:
            self.log.debug("Waiting for frame")
            arr, attr = self.receive_msg()
            self.log.debug("Drawing frame %s", str(arr.shape))
            cv2.imshow(winname, arr)
            self.log.debug("Attributes: %s", str(attr))
            key = cv2.waitKey(20)
            if key == 1048689:
                self.log.debug("User quit")
                break
        cv2.destroyWindow(winname)
    
    def receive_msg(self):
        # receive (blocking) messages
        self.log.debug("Waiting for multipart message")
        frames = self.zmq_socket.recv_multipart(flags=0, copy=False, track=False)
        self.log.debug("    Multipart message received. Length: %d", len(frames))
        arr_desc = msgpack.unpackb(frames[0].bytes)
        self.log.debug('Array description: %s', str(arr_desc))
        self.log.debug("Unpacking numpy array from bytes")
        # Create an empty numpy array placeholder to unpack the compressed array into
        arr = np.empty(arr_desc['shape'], dtype=arr_desc['dtype'])
        dest_arr_ptr = arr.__array_interface__['data'][0]
        # Unfortunately the access to Frame.bytes makes a copy of the compressed data.
        # As we only read the compressed data it is not strictly necessary to make
        # a copy, however it seems impossible to get a string object out without making a copy...
        # We would have to modify the blosc python bindings to add a decompress_ptr function which 
        # would work when given a python memoryview object (i.e. a pointer)
        compressed_bytes = frames[1].bytes
        blosc.decompress_ptr(compressed_bytes, dest_arr_ptr)
        
        self.log.debug("    unpacked array: shape: %s", str(arr.shape))
        attr = msgpack.unpackb(frames[2].bytes)
        self.log.debug("    unpacked attributes: %s", str(attr))
        return arr, attr
        
if __name__=="__main__":
    host = sys.argv[1]
    port = sys.argv[2]
    tc = TransferClient(host, port)
    
    # run (block) the receive loop
    #tc.run()
    tc.run_display(":".join([host,port]))
    
