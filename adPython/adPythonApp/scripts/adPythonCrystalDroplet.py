#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
from adPythonMorph import Morph
import logging, cv2
import numpy as np
import random

# from opencv/include/opencv2/core/core.hpp
INT=0
BOOLEAN=1
REAL=2
STRING=3
MAT=4
MAT_VECTOR=5
ALGORITHM=6
FLOAT=7
UNSIGNED_INT=8
UINT64=9
SHORT=10 
COLOR=11

class CrystalDroplet(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        self.log.setLevel(logging.DEBUG)         
        params = dict(
                      # Morphology
                      m_operation = 3,
                      m_ksize = 4,    
                      m_iters = 1,                      
                      # Threshold
                      t_ksize = 999,
                      t_c = 5,
                      # Contours
                      c_epsilon = 8,
                      c_min = 100,
                      c_max = 1800,
                      # Output
                      step = 4,                      
                      )
        # make a detector and get its param types
        detector_formats = ["","Grid","Pyramid"]        
        detector_types = ["FAST","STAR","SIFT","SURF","ORB","MSER","GFTT","HARRIS","Dense","SimpleBlob"]
        ########## Change these indexes
        detector_format = 0
        detector_type = 0
        ###############
        self.det = cv2.FeatureDetector_create(detector_formats[detector_format] + detector_types[detector_type])
        self.ranges = {}
        for param in self.det.getParams():
            typ = self.det.paramType(param)
            d_param = "d_" + param
            if typ in (FLOAT, REAL):
                params[d_param] = float(self.det.getDouble(param))
                print param, params[d_param]
                step = params[d_param] / 10.0
                if step > 1:
                    step = 1
                elif step > 0.1:
                    step = 0.1
                elif step > 0.01:
                    step = 0.01
                elif step > 0.001:
                    step = 0.001                    
                else:
                    step = 0.01
                    params[d_param] = 1.0
                self.ranges[d_param] = (0, min(params[d_param]*4, 1000), step)
            elif typ == BOOLEAN:
                params[d_param] = int(self.det.getBool(param))
                self.ranges[d_param] = 2
            elif typ in (UNSIGNED_INT, UINT64, SHORT):
                params[d_param] = int(self.det.getInt(param))
                self.ranges[d_param] = (params[d_param]+1)
        # import a morphology plugin to do filtering
        self.morph = Morph()        
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # pass the morph ones to the morph plugin
        for p in self.morph:
            self.morph[p] = self["m_"+p]
        self.morph.paramChanged()
        for p in self:
            if p.startswith("d_"):
                param = p[2:]
                typ = self.det.paramType(param)
                if typ in (FLOAT, REAL):
                    self.det.setDouble(param, self[p])
                elif typ == BOOLEAN:
                    self.det.setBool(param, self[p])
                elif typ in (UNSIGNED_INT, UINT64, SHORT):
                    self.det.setInt(param, self[p])

    def processArray(self, arr, attr={}):
        arr = cv2.resize(arr, (1280, 960))
        self.dest = arr
        
        # convert to grey
        gray = cv2.cvtColor(arr, cv2.COLOR_RGB2GRAY)
        if self["step"] <= 0: 
            return gray       
       
        # morphological operation
        morph = self.morph.processArray(gray)
        if self["step"] <= 1: 
            return morph

        # threshold
        thresh = cv2.adaptiveThreshold(morph, 255, cv2.ADAPTIVE_THRESH_MEAN_C, 
            cv2.THRESH_BINARY, self["t_ksize"], self["t_c"])        
        if self["step"] <= 2: 
            return thresh        
        
        # Find contours
        raw, heirarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
        # approx contour with a polygon at most curve_epsilon from raw contour
        approx = [cv2.approxPolyDP(r, self["c_epsilon"], True) for r in raw]
        
        # look at the contours
        used = []
        for cont, heir in zip(approx, heirarchy[0]):
            x, y, w, h = cv2.boundingRect(cont)
            # if width and height in range
            if w > self["c_min"] and w < self["c_max"] and \
                    h > self["c_min"] and h < self["c_max"] and \
                    x > self["c_min"] and x < gray.shape[0] - self["c_min"]:
                used.append(heir)
                cv2.drawContours(self.dest, [cont], -1, (0, 255, 0), 1, cv2.CV_AA)
                
        if self["step"] <= 3:
            return self.dest
        
        # find some blobs
        kp = self.det.detect(morph, None)
        self.dest = cv2.drawKeypoints(self.dest, kp, color=(255, 0, 0))
        if self["step"] <= 4:        
            return self.dest

        return self.dest  
                           

if __name__=="__main__":
    CrystalDroplet().runOffline(
        corners = 1000,
        canny_thresh=200, 
        t_ksize = 1000,
        m_operation=11, 
        step=9, 
        ar_err = (0, 0.3, 0.01), 
        micron_pix=(3, 7, 0.01), 
        s_thresh=(0,1,0.01),
        lx = 1000,
        ly = 1000,
        hessianThreshold = 800,
        ch = 1000,
        qualityLevel = (0, 0.1, 0.001),
        c_max = 2000)

