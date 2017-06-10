#!/usr/bin/env dls-python
from adPythonPlugin import AdPythonPlugin
import logging, numpy, cv2

class Rotate(AdPythonPlugin):
    def __init__(self):
        # The default logging level is INFO.
        # Comment this line to set debug logging off
        #self.log.setLevel(logging.DEBUG) 
        # Make some generic parameters
        params = dict(clip = 0,
                      rotCentreX = -1,
                      rotCentreY = -1,
                      rotAngle = 0.0,
                      fillBlue = 0,
                      fillGreen = 0,
                      fillRed = 0)
        AdPythonPlugin.__init__(self, params)
        
    def paramChanged(self):
        # one of our input parameters has changed
        # just log it for now, do nothing.
        self.log.debug("Parameter has been changed %s", self)

    def processArray(self, arr, attr={}):        
        # Called when the plugin gets a new array
        # arr is a numpy array
        # attr is an attribute dictionary that will be attached to the array
        
        angle=self["rotAngle"]
        clipping=self["clip"]
        centreX=self["rotCentreX"]
        centreY=self["rotCentreY"]
        fillColour=(self["fillRed"],self["fillGreen"],self["fillBlue"]) # (Blue, Green, Red)
        #fillColour=(255,255,255) # white
        #fillColour=(0,0,0) # black
       
        # get the dimensions of the source image
        height, width = arr.shape[:2]
        if centreX<0 or clipping==2: 
            # assume rotation about centre if user hasn't specified or if no clipping was specified
            centreX = width/2
        if centreY<0 or clipping==2: 
            # assume rotation about centre if user hasn't specified or if no clipping was specified
            centreY = height/2
        # get the rotation matrix
        # NB Rotation is about a pixel in the image not about an arbitrary point between pixels therefore rotations of 90 degress
        #    multiples often show a line of fill along an edge
        rotationMatrix = cv2.getRotationMatrix2D((centreX,centreY), -angle, 1.0);

        if clipping==1:
            # Clip to square of size equal to the smallest X or Y dimension of the source image
            # perform rotation using affine transform
            rot = cv2.warpAffine(arr, rotationMatrix, (width, height), borderValue=fillColour)
            # calculate desired output image size
            newWidth = min(width,height)
            newHeight = newWidth  
            # clip the image               
            out=rot[(height/2-newHeight/2):(height/2+newHeight/2), (width/2-newWidth/2):(width/2+newWidth/2)].copy()
        elif clipping==2:
            # No clippig which results in image size increasing in both X and Y dimensions
            # grab sine and cos from the rotation matrix
            cos = numpy.abs(rotationMatrix[0, 0])
            sin = numpy.abs(rotationMatrix[0, 1])
            # compute the new bounding dimensions of the image
            newWidth = int((height * sin) + (width * cos))
            newHeight = int((height * cos) + (width * sin))
            # adjust the rotation matrix to take into account translation
            rotationMatrix[0, 2] += (newWidth / 2) - centreX
            rotationMatrix[1, 2] += (newHeight / 2) - centreY
            # perform rotation using affine transform
            out = cv2.warpAffine(arr, rotationMatrix, (newWidth, newHeight), borderValue=fillColour)
        else:
            # keep source image dimensions
            # perform rotation using affine transform
            out = cv2.warpAffine(arr, rotationMatrix, (width, height), borderValue=fillColour)
     
        # Stamp the output array with the rotation angle by writing
        # to the attribute dictionary. 
        attr["angle"] = angle
        # update readback PV's
        self["rotAngle"] = angle
        self["rotCentreX"] = centreX
        self["rotCentreY"] = centreY
        self.log.debug("Array processed, angle: %d", attr["angle"])
        # return the resultant array.
        return out

if __name__=="__main__":
    Rotate().runOffline(
        clip=(0,2),      # This has range 0..2
        rotCentreX=(0,10000),
        rotCentreY=(0,10000),
        rotAngle=(-360,360,.001),# This has range -360 to 360 step 0.01
        fillBlue=0,
        fillGreen=0,
        fillRed=0)
        
