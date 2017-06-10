#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
from adPythonMorph import Morph
import logging, cv2
import numpy as np
import random
#from pkg_resource import require
#require("matplotlib")

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
                      t_d = 78.0,
                      # Contours
                      c_epsilon = 8,
                      c_min = 100,
                      c_max = 1800,
                      # Output
                      step = 5,                      
                      )
        # make a detector and get its param types
        detector_formats = ["","Grid","Pyramid"]        
        detector_types = ["FAST","STAR","SIFT","SURF","ORB","MSER","GFTT","HARRIS","Dense","SimpleBlob"]
        ########## Change these indexes
        detector_format = 0
        detector_type = 5
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

    def drawMatches(self, img1, kp1, img2, kp2, matches):
	"""
	My own implementation of cv2.drawMatches as OpenCV 2.4.9
	does not have this function available but it's supported in
	OpenCV 3.0.0

	This function takes in two images with their associated 
	keypoints, as well as a list of DMatch data structure (matches) 
	that contains which keypoints matched in which images.

	An image will be produced where a montage is shown with
	the first image followed by the second image beside it.

	Keypoints are delineated with circles, while lines are connected
	between matching keypoints.

	img1,img2 - Grayscale images
	kp1,kp2 - Detected list of keypoints through any of the OpenCV keypoint 
		  detection algorithms
	matches - A list of matches of corresponding keypoints through any
		  OpenCV keypoint matching algorithm
	"""
	matchesMask = [[0,0] for i in xrange(len(matches))]

	# Create a new output image that concatenates the two images together
	# (a.k.a) a montage
	rows1 = img1.shape[0]
	cols1 = img1.shape[1]
	rows2 = img2.shape[0]
	cols2 = img2.shape[1]

	out = np.zeros((max([rows1,rows2]),cols1+cols2,3), dtype='uint8')

	# Place the first image to the left
	out[:rows1,:cols1,:] = np.dstack([img1, img1, img1])

	# Place the next image to the right of it
	out[:rows2,cols1:cols1+cols2,:] = np.dstack([img2, img2, img2])
        
	# For each pair of points we have between both images
	# draw circles, then connect a line between them
	for i,(m,n) in enumerate(matches):
	    if m.distance < (self["t_d"]/100)*n.distance:
		matchesMask[i]=[1,0]
		# Get the matching keypoints for each of the images
		img1_idx = m.queryIdx
		img2_idx = m.trainIdx

		# x - columns
		# y - rows
		(x1,y1) = kp1[img1_idx].pt
		(x2,y2) = kp2[img2_idx].pt

		# Draw a small circle at both co-ordinates
		# radius 4
		# colour blue
		# thickness = 1
		cv2.circle(out, (int(x1),int(y1)), 4, (255, 0, 0), 1)   
		cv2.circle(out, (int(x2)+cols1,int(y2)), 4, (255, 0, 0), 1)

		# Draw a line in between the two points
		# thickness = 1
		# colour blue
		cv2.line(out, (int(x1),int(y1)), (int(x2)+cols1,int(y2)), (255, 0, 0), 1)

	return out
	# Show the i
	#cv2.namedWindow("Matched Features", 1)
	#cv2.resizeWindow("Matched Features", 300, 300)
	#cv2.imshow("Matched Features", out)
	#cv2.waitKey()
	#cv2.destroyAllWindows()

    def processArray(self, arr, attr={}):
        arr = cv2.resize(arr, (1280, 960))
        #arr = cv2.resize(arr, (600, 600))
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

	#################################################
        img1 = gray

	img2 = cv2.imread("/home/fmq68384/Python/OpenCV/DescriptorMatcher/Crystal_2.jpg",0) # trainImage
	img2 = cv2.resize(img2, (600, 600))
	# Initiate SIFT detector
	sift = cv2.SIFT()

	# find the keypoints and descriptors with SIFT
	kp1, des1 = sift.detectAndCompute(img1,None)
	kp2, des2 = sift.detectAndCompute(img2,None)

	# FLANN parameters
	FLANN_INDEX_KDTREE = 0
	index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
	search_params = dict(checks=50)   # or pass empty dictionary

	#flann = cv2.FlannBasedMatcher(index_params,search_params)
	#matches = flann.knnMatch(des1,des2,k=2)
	bf = cv2.BFMatcher()
	matches = bf.knnMatch(des1,des2, k=2)

	self.dest = self.drawMatches(img1,kp1,img2,kp2,matches)

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

