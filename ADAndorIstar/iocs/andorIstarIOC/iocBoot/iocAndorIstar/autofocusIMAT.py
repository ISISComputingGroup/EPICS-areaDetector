#!/usr/bin/python

from __future__ import print_function
from epics import PV

import time
import sys
import os
import math

pvPrefixGalil  = os.getenv("MYPVPREFIX") + "MOT:DMC01:"
pvprefixCamera = os.getenv("MYPVPREFIX") + "13ANDOR1:"

exposureTimeSeconds   = 0.003
ADImageSingle         = 0
ARImage               = 4
ATInternal            = 0
AGGateOnContinuously  = 3
GainDDG               = 0

#pvPrefixGalil = "IMAT-PC:salvato:MOT:DMC01:"
#pvprefixCamera = "NUR:Administrator:13ANDOR1:"

# Process Variables we are interested in:
# For iStar Camera
cameraStatusPV               = pvprefixCamera + "cam1:DetectorState_RBV"
cameraStartAcquirePV         = pvprefixCamera + "cam1:Acquire"
cameraExposureTimePV         = pvprefixCamera + "cam1:AcquireTime"
cameraArraycallbacksPV       = pvprefixCamera + "cam1:ArrayCallbacks"
cameraImageModePV            = pvprefixCamera + "cam1:ImageMode"
cameraReadModePV             = pvprefixCamera + "cam1:AndorReadMode"
cameraTriggerModePV          = pvprefixCamera + "cam1:TriggerMode"
cameraGateModePV             = pvprefixCamera + "cam1:AndorGateMode"
cameraMCPGainPV              = pvprefixCamera + "cam1:AndorMCPGain"

imageCallbacksPV             = pvprefixCamera + "image1:EnableCallbacks"
imageDataPV                  = pvprefixCamera + "image1:ArrayData"
imageColumnsPV               = pvprefixCamera + "image1:ArraySize0_RBV"
imageRowsPV                  = pvprefixCamera + "image1:ArraySize1_RBV"

# For focusing with the focusing plugin....(sometime in the future...)
# 13ANDOR1:Focus:ComputeFocusMetrics
# 13ANDOR1:Focus:FocusValue_RBV
# 13ANDOR1:Focus1:EnableCallback

# For Newport Lens Carrier
pvLensCarrierVMAX = pvPrefixGalil + "MTR0101.VMAX"
pvLensCarrierCNEN = pvPrefixGalil + "MTR0101.CNEN"
pvLensCarrierWLP  = pvPrefixGalil + "MTR0101_WLP_CMD"
pvLensCarrierHOMR = pvPrefixGalil + "MTR0101.HOMR"
pvLensCarrierDMOV = pvPrefixGalil + "MTR0101.DMOV"
pvLensCarrierAPOS = pvPrefixGalil + "MTR0101.VAL"
pvLensCarrierMRES = pvPrefixGalil + "MTR0101.MRES"

# For Laser light source
pvLaserControlBit = pvPrefixGalil + "Galil0Bo0_CMD"
pvLaserStatusBit  = pvPrefixGalil + "Galil0Bo0_STATUS.VAL"


#Create PVs
cameraStatus           = PV(cameraStatusPV)
cameraStartAcquire     = PV(cameraStartAcquirePV)
cameraExposureTime     = PV(cameraExposureTimePV)
cameraArraycallbacks   = PV(cameraArraycallbacksPV)
cameraImageMode        = PV(cameraImageModePV)
cameraReadMode         = PV(cameraReadModePV)
cameraTriggerMode      = PV(cameraTriggerModePV)
cameraGateMode         = PV(cameraGateModePV)
cameraMCPGain          = PV(cameraMCPGainPV)

imageCallbaks          = PV(imageCallbacksPV)
imageData              = PV(imageDataPV)
imageColumns           = PV(imageColumnsPV)
imageRows              = PV(imageRowsPV)

lensCarrierVMAX        = PV(pvLensCarrierVMAX)
lensCarrierCNEN        = PV(pvLensCarrierCNEN)
lensCarrierWLP         = PV(pvLensCarrierWLP)
lensCarrierHOMR        = PV(pvLensCarrierHOMR)
lensCarrierDMOV        = PV(pvLensCarrierDMOV)
lensCarrierAPOS        = PV(pvLensCarrierAPOS)
lensCarrierMRES        = PV(pvLensCarrierMRES)

laserControlBit        = PV(pvLaserControlBit)
laserStatusBit         = PV(pvLaserStatusBit)


class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
		
# Attempt to establish a connection to the process variables.
def connectPVs():
	try :
		if(cameraExposureTime.wait_for_connection() == False):
			raise ConnectionError("Error: cameraExposureTime not connected")
		if(cameraImageMode.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraImageMode not connected")
		if(cameraReadMode.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraReadMode not connected")
		if(cameraTriggerMode.wait_for_connection() == False):
			raise ConnectionError("Error: cameraTriggerMode not connected")
		if(cameraMCPGain.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraMCPGain not connected")
		if(cameraArraycallbacks.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraArraycallbacks not connected")
			
		if(imageCallbaks.wait_for_connection() == False) :
			raise ConnectionError("Error: imageCallbaks not connected")
	
		if(laserControlBit.wait_for_connection() == False) :
			raise ConnectionError("Error: laserControlBit not connected")
		if(lensCarrierVMAX.wait_for_connection() == False) :
			raise ConnectionError("Error: lensCarrierVMAX not connected")
		if(lensCarrierWLP.wait_for_connection() == False) :
			raise ConnectionError("Error: lensCarrierWLP not connected")
		if(lensCarrierMRES.wait_for_connection() == False) :
			raise ConnectionError("Error: lensCarrierMRES not connected")

		return True

	except ConnectionError, e :
		print("".join(e.args))
		return False
		
		
def initPVs():
	cameraMCPGain.put(GainDDG)
	cameraExposureTime.put(exposureTimeSeconds)
	cameraImageMode.put(ADImageSingle)
	cameraReadMode.put(ARImage)
	cameraGateMode.put(AGGateOnContinuously)
	cameraTriggerMode.put(ATInternal)
	cameraGateMode.put(AGGateOnContinuously)
	cameraArraycallbacks.put(1)# Ensure Array Callbacks are enabled
	
	imageCallbaks.put(1)       # Ensure Image Callbacks are enabled

	laserControlBit.put(0)    # Ensure Laser is switched off
	lensCarrierVMAX.put(4.0)  # Max speed of Lenses in mm/s
	lensCarrierWLP.put("Off") # No Wrong Limits Protection
	lensCarrierMRES.put(0.005)# mm per step
	return True

	
def waitLensInPosition():
	while lensCarrierDMOV.get() != 1:# Wait until Move Done
		time.sleep(1)
	
	
def waitLaserOn():
	while laserStatusBit.get() != 1:# Wait until Laser is switched ON
		time.sleep(1)

		
def waitNewimageReady():
	waitCameraAcquiring()
	print ("Camera is acquiring.", end="")
	while cameraStatus.get() != 0:
		time.sleep(1)
		print(".", end="")
	print ("Done acquiring")

		
def waitCameraAcquiring():
	while cameraStatus.get() != 1:
		time.sleep(1)

		
def focusMetric(image, nRows, nColumns):
# Contrast Based algorithm:
# Xu et al, 
# "Robust Automatic Focus Algorithm for Low Contrast images 
#  Using a New Contrast Measure"
# Sensors 2011, 11, 8281-8294
	dContrast = 0.0
	rimage = list()
	for i in range(0, nRows*nColumns):
		rimage.append(image[i]/65536.0)
	for j in range(1, nRows-2) :
		RowM1 = (j-1) * nColumns
		Row   =   j   * nColumns
		RowP1 = (j+1) * nColumns;
        
		for k in range(1, nColumns-2) :
			ref = rimage[Row+k]
			gXY  = abs(ref - rimage[Row+k-1])
			gXY += abs(ref - rimage[Row+k+1])
			gXY += abs(ref - rimage[RowM1+k])
			gXY += abs(ref - rimage[RowP1+k])
			dContrast += gXY*gXY;
      
	return dContrast

		
def f(x):# Fake focus value for testing purpose
	center = 25.05
	return -(x-center)*(x-center)

	
# Golden Section Search for the MINIMUM of a function 
# Adapted from:
# NUMERICAL RECIPES IN C:
#	THE ART OF SCIENTIFIC COMPUTING 
# (ISBN 0-521-43108-5)
def goldenSearch(_x0, _x3, _Tolerance):
	C = 0.5*(3.0-math.sqrt(5.0))# the golden ratio
	R = 1.0 - C	
	Tolerance = _Tolerance
	x0 = _x0
	x3 = _x3
	x1 = x0 + R * (x3 - x0)
	x2 = x1 + C * (x3 - x1)
	
	laserControlBit.put(0)# Ensure Laser is switched off

	lensCarrierCNEN.put("Enable")
	lensCarrierAPOS.put(x1)
	waitLensInPosition()
	laserControlBit.put(1)
	waitLaserOn()# Ensure Laser is switched on
	cameraStartAcquire.put(1)
	waitNewimageReady()
	laserControlBit.put(0)
	nImageRows    = imageRows.get()
	nImageColumns = imageColumns.get()
	image = list(imageData.get())
	f1 = -focusMetric(image, nImageRows, nImageColumns)
	#f1 = f(x1)#fake value for testing purpose
	print ("At position %f Focus value is %f" % (x1, -f1))

	lensCarrierCNEN.put("Enable")
	lensCarrierAPOS.put(x2)
	waitLensInPosition()
	laserControlBit.put(1)
	waitLaserOn()
	cameraStartAcquire.put(1)
	waitNewimageReady()
	laserControlBit.put(0)
	image = list(imageData.get())
	f2 = -focusMetric(image, nImageRows, nImageColumns)
	#f2 = f(x2)#fake value for testing purpose
	print ("At position %f Focus value is %f" % (x2, -f2))

	while (abs(x3-x0) > Tolerance) :
		if (f2 < f1) :
			x0 = x1
			x1 = x2
			x2 = R * x1 + C * x3
			f1 = f2
			lensCarrierCNEN.put("Enable")
			lensCarrierAPOS.put(x2)
			waitLensInPosition()
			laserControlBit.put(1)
			waitLaserOn()
			cameraStartAcquire.put(1)
			waitNewimageReady()
			laserControlBit.put(0)
			image = list(imageData.get())
			f2 = -focusMetric(image, nImageRows, nImageColumns)
			#f2 = f(x2) #fake value for testing purpose
			print ("At position %f Focus value is %f" % (x2, -f2))
		else :
			x3 = x2
			x2 = x1
			x1 = R * x2 + C * x0
			f2 = f1
			lensCarrierCNEN.put("Enable")
			lensCarrierAPOS.put(x1)
			waitLensInPosition()
			laserControlBit.put(1)
			waitLaserOn()
			cameraStartAcquire.put(1)
			waitNewimageReady()
			laserControlBit.put(0)
			image = list(imageData.get())
			f1 = -focusMetric(image, nImageRows, nImageColumns)
			#f1 = f(x1) #fake value for testing purpose
			print ("At position %f Focus value is %f" % (x1, -f1))

	lensCarrierCNEN.put("Enable")
	if (f1 < f2) :
		lensCarrierAPOS.put(x1)
		print ("Focus found at %f value %f" % (x1, -f1))
	else :
		lensCarrierAPOS.put(x2)
		print ("Focus found at %f value %f" % (x2, -f2))


def main():
	try:
		xMin = 0.0
		xMax = 25.5
		xTol = 0.02
		print ("Checking PVs connection...")
		if(connectPVs() == False) :
			print("PVs not connected ... Autofocus failed. Exiting")
			sys.exit(0)
		print ("Initializing PVs...")
		if(initPVs() == False) :
			print("Unable to initialize PV values ... Autofocus failed. Exiting")
			sys.exit(0)
		# Ensure Lens carrier is homed.
		print ("Homing lens carrier.", end="")
		lensCarrierCNEN.put("Enable")      # Enable motor Power
		lensCarrierHOMR.put(1)             # Start Reverse Homing procedure
		while lensCarrierDMOV.get() != 1 : # Wait until Homed
			time.sleep(1)
			print (".", end="")
		print ("Lens carrier at Home: Start focusing !")
		goldenSearch(xMin, xMax, xTol)
	except:
		pass
		
if __name__ == "__main__":
    main()		
