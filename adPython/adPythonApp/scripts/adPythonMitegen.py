#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
from adPythonMorph import Morph
import logging, cv2
import numpy as np
import random

class Mitegen(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        #self.log.setLevel(logging.DEBUG) 
        params = dict(
                      micron_pix = 3.67, # 5.13 for LD_*                                  
                      # Morphology
                      m_operation = 3,
                      m_ksize = 4,    
                      m_iters = 1,                      
                      # Threshold
                      t_ksize = 9,
                      t_c = 5,
                      # Contours for cross finding
                      curve_epsilon = 8,
                      ar_err = 0.15,
                      # Canny        
                      canny_thresh = 110,
                      # Output
                      step = 7,                      
                      ltype = -1,
                      lsize = -1, 
                      lx = -1.,
                      ly = -1.,
                      ch = -1.                                                          
                      )
        # import a morphology plugin to do filtering
        self.morph = Morph()        
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # pass the morph ones to the morph plugin
        for p in self.morph:
            self.morph[p] = self["m_"+p]
        self.morph.paramChanged()

    def is_cross(self, cnt):
        x, y, w, h = cv2.boundingRect(cnt)
        # cross is 200 microns width, so allow between half and double width
        w_min = 100 / self['micron_pix']
        w_max = 400 / self['micron_pix']   
        # cross height aspect ratio is 1.5     
        ar = 1.5
        if w > w_min and w < w_max and \
            h > w_min / ar and h <  w_max / ar and \
            abs(w / float(h) - ar) < self["ar_err"]:
            # got a contour that might be a cross
            # check that all the points are in a cross shape
            central_x = (x + w * (0.5 - self["ar_err"]), x + w * (0.5 + self["ar_err"]))
            central_y = (y + h * (0.5 - self["ar_err"]), y + h * (0.5 + self["ar_err"]))     
            for pt in cnt:
                px, py = pt[0][0], pt[0][1]
                if (px < central_x[0] or px > central_x[1]) and \
                    (py < central_y[0] or py > central_y[1]):
                    # px and py outside central part
                    return False
            # passed cross checks
            return True
        else:
            # contour doesn't match width and aspect ratio params
            return False

    def rotated(self, x, y):
        newx = x * np.cos(self.angle) + y * np.sin(self.angle)
        newy = x * np.sin(self.angle) + y * np.cos(self.angle)
        return newx, newy

    def get_dots(self, canny, coords):        
        cx, cy, cw, ch = self.cross_params
        sums, pts, tot = {}, {}, 0
        for i, coord in enumerate(coords):
            # get dims of ROI of cross image
            x, y = self.rotated(coord[0] * (cw + 8) / 5., coord[1] * (ch + 8) /3)
            x, y = int(x + cx - 2), int(y + cy - 1)
            w, h = int(cw / 5. - 4), int(ch / 3. - 5)
            # get sum of it
            sums[i] = canny[y:y+w, x:x+w].sum() / 255            
            pts[i] = (x, y, w, h)
        thresh = 0.8 * max(sums.values())
        tot = 0            
        for i, s in sums.items():
            x, y, w, h = pts[i]
            if self["step"] == 6:
                cv2.rectangle(self.dest, (x, y), (x + w, y + h), (255, 0, 0))              
            # if sum > thresh then draw a dot
            if s > thresh:
                tot += 2**i
                if self["step"] == 6:
                    cv2.circle(self.dest, (x + w/2, y + h/2), h/2, (0, 255, 0), 2)                
        return tot

    def lookup_lsize(self, lsize):
        # lookup loop size based on type
        if lsize >= 9:
            # something's gone wrong
            return -1
        if self["ltype"] == 4:
            # microloops
            return (10, 20, 35, 50, 75, 100, 150, 200, 300)[lsize]
        elif self["ltype"] == 1:
            # M2
            return (10, 20, 30, 50, 75, 100, 150, 200, 300)[lsize]            
        else:
            # not supported
            return -1

    def processArray(self, arr, attr={}):
        # convert to grey
        gray = cv2.cvtColor(arr, cv2.COLOR_RGB2GRAY)
        if self["step"] <= 0: 
            return gray       
        
        # threshold
        thresh = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
            cv2.THRESH_BINARY_INV, self["t_ksize"]*2+1, self["t_c"])
        if self["step"] <= 1: 
            return thresh     

        # morphological operation
        morph = self.morph.processArray(thresh)
        if self["step"] <= 2: 
            return morph
                       
        # If we are expected to output an image then copy it so we can write
        if self["step"] in range(3, 8): 
            self.dest = arr.copy()                             
        # Find contours
        raw, _ = cv2.findContours(morph, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
        # approx contour with a polygon at most curve_epsilon from raw contour
        approx = [cv2.approxPolyDP(r, self["curve_epsilon"], True) for r in raw]
        if self["step"] == 3:
            cv2.drawContours(self.dest, approx, -1, (0, 255, 0), 1, cv2.CV_AA)         
            return self.dest
                
        # Find cross shapes in the approx contours
        crosses = [raw[i] for i, a in enumerate(approx) if self.is_cross(a)]
        # if we didn't find anything then just return
        if not crosses:
            print "No crosses found in image"
            return        
        # Draw the raw versions of all the crosses
        if self["step"] == 4:
            cv2.drawContours(self.dest, crosses, -1, (0, 255, 0), 1, cv2.CV_AA)         
            return self.dest        
        
        # Select the smallest cross
        def bounding_area(cross):
            x, y, w, h = cv2.boundingRect(cross)
        small = sorted(crosses, key=bounding_area)[0]
        self.cross_params = cv2.boundingRect(small)
        # Calculate angle of cross
        moment = cv2.moments(small)
        self.angle = 0.5*np.arctan((2*moment['mu11'])/(moment['mu20']-moment['mu02']))         
                    
        # now go back to morph image and do a canny edge detect on it
        canny = cv2.Canny(gray, self["canny_thresh"], 2*self["canny_thresh"], 5);
        if self["step"] <= 5: 
            return canny
        
        # Count the number of white pixels in each dots location
        # These dots are the loop type
        self["ltype"] = self.get_dots(canny, [(1,0), (1,2), (3,2), (3,0)])
        # These dots are the loop size
        lsize = self.get_dots(canny, [(0,0), (0,2), (4,2), (4,0)])       
        if self["step"] <= 6:
            return self.dest
        # lookup loop size based on type
        self["lsize"] = self.lookup_lsize(lsize)
        if self["lsize"] == -1:
            print "Loop size %d and type %d not supported" % (lsize, self["ltype"])
            return 
            
        # Now draw the loop, we assume the holder is mounted roughly level and 870 microns between loop and cross
        cx, cy, cw, ch = self.cross_params        
        lx, ly = self.rotated(-870. / self['micron_pix'] + cw / 2.,  ch / 2.)
        lx, ly = int(lx + cx), int(ly + cy)
        lr = int(self["lsize"] / 2. / self['micron_pix'] + 0.5)
        # These are the results in microns
        self['lx'] = lx * self['micron_pix']
        self['ly'] = ly * self['micron_pix']
        self['ch'] = ch * self['micron_pix']        
        if self["step"] <= 7:
            cv2.circle(self.dest, (lx, ly), lr, (0, 255, 0), 2)
            return self.dest
        


if __name__=="__main__":
    Mitegen().runOffline(
        canny_thresh=200, 
        m_operation=11, 
        step=9, 
        ar_err = (0, 0.3, 0.01), 
        micron_pix=(3, 7, 0.01), 
        s_thresh=(0,1,0.01),
        lx = 1000,
        ly = 1000,
        ch = 1000,)

