#!/usr/bin/env dls-python
#import sys
#sys.path.insert(0, "/dls_sw/work/tools/RHEL6-x86_64/OpenCV/prefix/lib/python2.7/site-packages")

from adPythonPlugin import AdPythonPlugin
import logging, numpy, cv2, pydmtx
import math
#from pydmtx import DataMatrix

print cv2.__version__

def angle_cos(p0, p1, p2):
    d1, d2 = (p0-p1).astype('float'), (p2-p1).astype('float')
    return numpy.dot(d1, d2) / numpy.sqrt( numpy.dot(d1, d1)*numpy.dot(d2, d2) )

def dist_between(p0, p1):
    d = (p0-p1).astype('float') 
    return numpy.sqrt(numpy.dot(d, d))

class DataMatrix(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        self.log.setLevel(logging.DEBUG) 
        # Make some generic parameters
        # You can change the Name fields on the EDM screen here
        # Hide them by making their name -1
#        params = dict(int1 = 101,      int1Name = "Threshold block size",
#                      int2 = 13,      int2Name = "Threshold C",
#                      int3 = 3,      int3Name = "Kernel size",
#                      double1 = 0.35, double1Name = "Angle cos",
#                      double2 = 6.0, double2Name = "Curve epsilon",
#                      double3 = 15.0, double3Name = "Length diff")
        self.dm=pydmtx.DataMatrix()
        params = dict(int1 = 35, int2 = 13, int3 = 3, double1 = 0.35, double2 = 6.0, double3 = 15.0,
                      dp = 2.2, minDist = 100, minRadius = 50, maxRadius = 100, param1 = 500,
                      step = 6, bc1 = "", bc2 = "", bc3 = "", bc4 = "", bc5 = "", bc6 = "", bc7 = "", bc8 = "",
                      bc9 = "", bc10 = "", bc11 = "", bc12 = "", bc13 = "", bc14 = "", bc15 = "", bc16 = "",
                      message = "")
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # just log it for now, do nothing.
        self.log.debug("Parameter has been changed %s", self)

    def processArray(self, arr, attr={}):
        # reset barcode strings
        for index in range(1,17):
            self["bc"+str(index)] = ""

        # Turn the picture gray
        if arr.ndim == 3:
            gray = cv2.cvtColor(arr, cv2.COLOR_RGB2GRAY)
        else:
            gray = arr.copy()
        if self["step"] <= 0: 
          return gray

        ################################################
        # Execute the circle finding
        circles = cv2.HoughCircles(gray, cv2.cv.CV_HOUGH_GRADIENT, self["dp"], self["minDist"], minRadius = self["minRadius"], maxRadius = self["maxRadius"], param1 = self["param1"])
        # if we find circles
        if circles is not None:
            if self["step"] <= 1:
                out = arr.copy()
                
            xs, ys, rs = [], [], []
            ccs = []
            for circle in circles[0]:
                if self["step"] <= 1:
                    cv2.circle(out, (circle[0], circle[1]), circle[2], (200, 200, 200), 15)
                    
                # Store the center of each circle
                cc = [circle[0], circle[1]]
                ccs.append(cc)
            
            nccs = numpy.array(ccs, 'int32')
            c_center, radius = cv2.minEnclosingCircle(nccs)
            #print "Center: " + str(c_center)
            #print "Radius: " + str(c_radius)

            crs = []
            for cc in ccs:
                dx = cc[0] - c_center[0]
                dy = cc[1] - c_center[1]
                dr = math.sqrt(dx*dx + dy*dy)
                cr = [dr, cc[0], cc[1]]
                crs.append(cr)
                #print "Circle dr: " + str(dr)
            
            if self["step"] <= 1:
                return out

            if len(crs) < 18:
                self["message"] = "Failed to find all circles"
                return arr

            #print str(crs)
            scrs = crs.sort()
            marker = crs[6]
            #print str(crs[6])
            c_angle = math.atan2((c_center[1]-marker[2]), (marker[1]-c_center[0]))
            c_radius = marker[0]
            #print "Angle: " + str(c_angle/(2*math.pi)*360)
        
        ####################################
        
        #printdir(arr)
        #c_center, c_radius, c_angle = processCircles(arr.copy())
        
        if c_center == None or c_radius == None or c_angle == None:
            return arr
        #print "Center of puck: " + str(c_center)
        #print "Angle of marker: " + str(c_angle)
        #print "Radius of marker: " + str(c_radius)
        
        # copy the array output
        out = arr.copy()
        # Do an adaptive threshold to get a black and white image which factors in lighting
        thresh = cv2.adaptiveThreshold(gray, 255.0, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY, self["int1"], self["int2"])
        if self["step"] <= 2:
          return thresh

        # Morphological filter to get rid of noise
        element = cv2.getStructuringElement(cv2.MORPH_RECT, (self["int3"], self["int3"]))
        morph = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, element, iterations=1)
        if self["step"] <= 3:
          return morph
        
        m = morph.copy()  
        # Find squares in the image
        contours, hierarchy = cv2.findContours(morph, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
        squares = []
        barcodes = []
        #print"Qty of contours: " + str(len(contours))
        index = 0
        av_dist = 0


        if self["step"] <= 4:
            #print"About to enter for loop"
            cv2.drawContours(out, contours, -1, (255,0,0), 3)
            return out

        for rawcnt in contours:
            #print"Rawcnt: " + str(rawcnt)
            index = index + 1
            #print"Index: " + str(index)
            cnt = cv2.approxPolyDP(rawcnt, self["double2"], True)
            cnt = cnt.reshape(-1, 2)
            #print"Here 1"
            #print"Reshaped: " + str(cnt)
            l = len(cnt)
            if l < 4:
                continue
            # find distances between successive points
            dists = [(dist_between(cnt[i], cnt[(i+1)%l]),i) for i in xrange(l)]
            dists.sort(reverse=True)            
            #print"Distances dists: " + str(dists)
            # if longest 2 line lengths are about the same and segments are 
            # next to each other           
            #print"Here 2"
            (d0, i0), (d1, i1) = dists[0], dists[1]
            #print"Distances d0: " + str(d0)
            #print"Distances d1: " + str(d1)
            #print"Distances i0: " + str(i0)
            #print"Distances i1: " + str(i1)
            if abs(d0 - d1) < self["double3"] and abs(i0 - i1) in (1, l-1) and \
                d0 > self["int1"] and d1 > self["int1"]:
                # Find out which point is first in contour
                if (i0 < i1 and i1 < l-1) or (i1 == 0 and i0 == l-1):
                    first = i0
                else:
                    if i0 < i1 and i1 == l-1 and (i1-i0) == 1:
                        first = i0
                    else:
                        first = i1
                # Get cos of the angle of the corner                
                pts = [cnt[first], cnt[(first+1)%l], cnt[(first+2)%l]]
                #print "L: " + str(l)
                #print "pts: " + str(pts)
                ac = angle_cos(*pts)
                #print"Here 3"
                if abs(ac) < self["double1"]:                      
                    #print"Here 3a"
                    # work out the rotation of the lines
                    #print "pts[0][0] - pts[1][0]: " + str(pts[0][0] - pts[1][0])
                    #print "pts[1][1]-pts[0][1]: " + str(pts[1][1]-pts[0][1])
                    #print "pts[2][0] - pts[1][0]: " + str(pts[2][0] - pts[1][0])
                    #print "pts[1][1]-pts[2][1]: " + str(pts[1][1]-pts[2][1])
                    if (pts[1][1]-pts[0][1]) != 0:
                        angle0 = numpy.degrees(numpy.arctan((pts[0][0] - pts[1][0])/float(pts[1][1]-pts[0][1])))
                    else:
                        angle0 = 90.0
                    if (pts[1][1]-pts[2][1]) != 0:
                        angle2 = numpy.degrees(numpy.arctan((pts[2][0] - pts[1][0])/float(pts[1][1]-pts[2][1])))
                    else:
                        angle2 = 90.0
                    #print"Here 3b"
                    # rotate vector by 90 degrees clockwise to get last point
                    dx, dy = pts[1][0] - pts[0][0], pts[1][1] - pts[0][1]
                    p3 = [pts[0][0] - dy, pts[0][1] + dx]
                    #print"Here 3c"
                    pts.append(p3)
                    #print"Here 3d"
                    pts = numpy.array(pts, 'int32')
                    #print str(pts)
                    # now work out the bounding rectangle of those points
                    #print "Before bouding rect"
                    x, y, w, h = cv2.boundingRect(pts.reshape((-1, 1, 2)))
                    #print"Bounding rect:"
                    #print"  x:" + str(x)
                    #print"  y:" + str(y)
                    #print"  w:" + str(w)
                    #print"  h:" + str(h)
                    #print"Here 4"
                    # and take a roi of it
                    pad = self["int1"] / 10
                    #print"Pad: " + str(pad)
                    #print"Here 4a"
                    threshroi = thresh[y-pad:y+h+pad, x-pad:x+w+pad]
                    origroi = gray[y-pad:y+h+pad, x-pad:x+w+pad]
                    
                    #print str(origroi)
                    if self["step"] <= 5:
                        return threshroi.copy()
                    #print"Here 4b"
                    #print"pts: " + str(pts)
                    #print"x-pad: " + str(x-pad)
                    #print"y-pad: " + str(y-pad)
                    newpts = pts-(x-pad, y-pad)
                    testpts = newpts.copy()
                    #print "testpts: " + str(testpts), type(testpts)
                    #print "New pts: " + str(newpts)
                    #testpts = [[0,0],[0,10],[10,10],[10,0]]
                    # work out the rotated bounding rectangle

                    #try:
                    #    print "minAreaRect: " + str(cv2.minAreaRect(testpts))
                    #except:
                    #    print "Caught an exception!!!"
                    #import pdb; pdb.set_trace()
                    #print str(testpts)
                    center, size, angle = cv2.minAreaRect(testpts)                
                    #print str(center) + ", " + str(size) + ", " + str(angle)
                    #print"Here 4c"
                    if angle0 > angle2:
                        if pts[1][1]-pts[0][1] < 0:
                            angle += 180
                    else:
                        if pts[1][1]-pts[0][1] < 0:
                            angle += 90
                        else:
                            angle += 270  
                    #print"Here 4d"
                    # get the rotation matrix                            
                    M = cv2.getRotationMatrix2D(center, angle, 1.0);
                    #print"Here 4e"
                    # perform affine transform
                    l = max(*threshroi.shape)
                    #print"Here 4f"
                    rot = cv2.warpAffine(threshroi, M, (l, l), borderValue=(255,255,255))
                    #return rot
                    # now decode it
                    #print"Here 5"
                    sym = self.dm.decode(l, l, buffer(rot.tostring()), max_count = 1)
                    #print"Here 6"
                    #printsym
                    if sym:
                        #print sym
                        #print str(dists[0])
                        #print str(dists[1])
                        #print str(dists[2])
                        #print str(dists[3])
                        #print "Barcode found: " + str(center[0]+x+pad) + "," + str(center[1]+y+pad)
                        #bcr = math.sqrt((center[0]+x+pad-1400)*(center[0]+x+pad-1400) + (center[1]+y+pad-645)*(center[1]+y+pad-645))
                        #av_dist = av_dist + bcr
                        bcc = [(center[0]+x+pad), (center[1]+y+pad)]
                        #print "Barcode center: " + str(bcc)
                        bcn = findNumber(bcc, c_center, c_radius, c_angle)
                        barcode = [bcn, sym]
                        barcodes.append(barcode)
                        #print "Radius from middle: " + str(bcr)
                        #print "[" + str(bcn) + "] " + sym
                        #if sym == "D8":
                        #    print str(pts)
                        cv2.putText(out, str(bcn) + ": " + sym, (int(center[0]+x+pad), int(center[1]+y+pad)), cv2.FONT_HERSHEY_PLAIN, 3, (0,0,0), 3)
                        squares.append(pts)
                    #else:
                    #    return cv2.warpAffine(origroi, M, (l, l), borderValue=(255,255,255))                     
        barcodes.sort()
        #print str(sbc)
        self["message"] = "Decoded " + str(len(barcodes)) + " barcodes"
        for barcode in barcodes:
            print "[" + str(barcode[0]) + "] " + barcode[1]
            if barcode[0] > 0:
                self["bc"+str(barcode[0])] = barcode[1]
        
        # Draw squares on the image    
        cv2.drawContours(out, squares, -1, (0,0,0), 3)
        
        # Mask out the area of these polys   
        #cv2.imwrite("/scratch/U/datamatrix_results.jpg", cv2.cvtColor(out, cv2.COLOR_RGB2BGR) )
        return out
        
def findNumber(bc, center, radius, angle):
    dx = bc[0] - center[0]
    dy = bc[1] - center[1]
    dr = math.sqrt(dx*dx + dy*dy)
    da = math.atan2((center[1]-bc[1]), (bc[0]-center[0]))
    #print "Barcode raw angle: " + str(da/(2*math.pi)*360.0)
    da = (da-angle)/(2*math.pi)*360.0
    if da > 180.0:
        da = da - 360.0
    if da < -180.0:
        da = da + 360.0
    if dr < radius:
        # The pin is in the inner ring
        #print "Barcode [inner] angle: " + str(da)
        if da > 150 or da < -150:
            return 1
        if da > 78 and da < 138:
            return 5
        if da > 6 and da < 66:
            return 4
        if da > -66 and da < -6:
            return 3
        if da > -138 and da < -78:
            return 2
    else:
        # The pin is in the outer ring
        #print "Barcode [outer] angle: " + str(da)
        if da > 170 or da < -170:
            return 6
        if da > 137 and da < 157:
            return 16
        if da > 105 and da < 125:
            return 15
        if da > 72 and da < 92:
            return 14
        if da > 39 and da < 59:
            return 13
        if da > 6 and da < 26:
            return 12
        if da > -26 and da < -6:
            return 11
        if da > -59 and da < -39:
            return 10
        if da > -92 and da < -72:
            return 9
        if da > -125 and da < -105:
            return 8
        if da > -157 and da < -137:
            return 7
    return 0

        

if __name__=="__main__":
    DataMatrix().runOffline(double1=(0, 1, 0.01), double3=(0,300))

