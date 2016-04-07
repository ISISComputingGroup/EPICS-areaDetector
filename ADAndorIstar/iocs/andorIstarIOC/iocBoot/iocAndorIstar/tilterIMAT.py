#!/usr/bin/python

from __future__ import print_function
from epics import PV

import time
import sys
import os
import math

#pvPrefixGalilCrate  = os.getenv("MYPVPREFIX") + "MOT:DMC01:"
#pvPrefixGalilMotor  = os.getenv("MYPVPREFIX") + "MOT:"
#pvprefixCamera      = os.getenv("MYPVPREFIX") + "13ANDOR1:"

pvPrefixGalilCrate  = "IMAT:SALVATO:MOT:DMC01:"
pvPrefixGalilMotor  = "IMAT:SALVATO:MOT:"
pvprefixCamera      = "NUR:ADMINISTRATOR:13ANDOR1:"

tilterMotor         = "MTR0102"

exposureTimeSeconds   = 0.003
ADImageSingle         = 0
ARImage               = 4
ATInternal            = 0
AGGateOnContinuously  = 3
GainDDG               = 0

# Process Variables we are interested in:
# For iStar Camera
cameraStatusPV         = pvprefixCamera + "cam1:DetectorState_RBV"
cameraStartAcquirePV   = pvprefixCamera + "cam1:Acquire"
cameraExposureTimePV   = pvprefixCamera + "cam1:AcquireTime"
cameraArraycallbacksPV = pvprefixCamera + "cam1:ArrayCallbacks"
cameraImageModePV      = pvprefixCamera + "cam1:ImageMode"
cameraReadModePV       = pvprefixCamera + "cam1:AndorReadMode"
cameraTriggerModePV    = pvprefixCamera + "cam1:TriggerMode"
cameraGateModePV       = pvprefixCamera + "cam1:AndorGateMode"
cameraMCPGainPV        = pvprefixCamera + "cam1:AndorMCPGain"
cameraEnableFocusPV    = pvprefixCamera + "cam1:EnableFocusCalculation"
cameraFocusValuePV     = pvprefixCamera + "cam1:FocusValue_RBV"

imageCallbacksPV       = pvprefixCamera + "image1:EnableCallbacks"
imageDataPV            = pvprefixCamera + "image1:ArrayData"
imageColumnsPV         = pvprefixCamera + "image1:ArraySize0_RBV"
imageRowsPV            = pvprefixCamera + "image1:ArraySize1_RBV"

# For Newport Lens Carrier
pvTilterMotorMTYP      = pvPrefixGalilMotor + tilterMotor + "_MTRTYPE_CMD"
pvTilterMotorVMAX      = pvPrefixGalilMotor + tilterMotor + "_VMAX_SP"
pvTilterMotorVBAS      = pvPrefixGalilMotor + tilterMotor + ".VBAS"
pvTilterMotorCNEN      = pvPrefixGalilMotor + tilterMotor + ".CNEN"
pvTilterMotorWLP       = pvPrefixGalilMotor + tilterMotor + "_WLP_CMD"
pvTilterMotorHOMR      = pvPrefixGalilMotor + tilterMotor + ".HOMR"
pvTilterMotorHOMF      = pvPrefixGalilMotor + tilterMotor + ".HOMF"
pvTilterMotorDMOV      = pvPrefixGalilMotor + tilterMotor + ".DMOV"
pvTilterMotorAPOS      = pvPrefixGalilMotor + tilterMotor + ".VAL"
pvTilterMotorMRES      = pvPrefixGalilMotor + tilterMotor + ".MRES"

# For Laser light source
pvLaserControlBit      = pvPrefixGalilCrate + "Galil0Bo0_CMD"
pvLaserStatusBit       = pvPrefixGalilCrate + "Galil0Bo0_STATUS.VAL"


# Create PVs
cameraStatus           = PV(cameraStatusPV)
cameraStartAcquire     = PV(cameraStartAcquirePV)
cameraExposureTime     = PV(cameraExposureTimePV)
cameraArraycallbacks   = PV(cameraArraycallbacksPV)
cameraImageMode        = PV(cameraImageModePV)
cameraReadMode         = PV(cameraReadModePV)
cameraTriggerMode      = PV(cameraTriggerModePV)
cameraGateMode         = PV(cameraGateModePV)
cameraMCPGain          = PV(cameraMCPGainPV)
cameraEnableFocus      = PV(cameraEnableFocusPV)
cameraFocusValue       = PV(cameraFocusValuePV)

imageCallbaks          = PV(imageCallbacksPV)
imageData              = PV(imageDataPV)
imageColumns           = PV(imageColumnsPV)
imageRows              = PV(imageRowsPV)

tilterMotorMTYP        = PV(pvTilterMotorMTYP)
tilterMotorVMAX        = PV(pvTilterMotorVMAX)
tilterMotorVBAS        = PV(pvTilterMotorVBAS)
tilterMotorCNEN        = PV(pvTilterMotorCNEN)
tilterMotorWLP         = PV(pvTilterMotorWLP)
tilterMotorHOMR        = PV(pvTilterMotorHOMR)
tilterMotorHOMF        = PV(pvTilterMotorHOMF)
tilterMotorDMOV        = PV(pvTilterMotorDMOV)
tilterMotorAPOS        = PV(pvTilterMotorAPOS)
tilterMotorMRES        = PV(pvTilterMotorMRES)

laserControlBit        = PV(pvLaserControlBit)
laserStatusBit         = PV(pvLaserStatusBit)


class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
		
# Attempt to establish a connection to the process variables.
def connectPVs():
	try :
		if(cameraExposureTime.wait_for_connection() == False):
			raise ConnectionError("Error: " + pvCameraExposureTime + " not connected")
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
		if(cameraEnableFocus.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraEnableFocus not connected")
		if(cameraFocusValue.wait_for_connection() == False) :
			raise ConnectionError("Error: cameraFocusValue not connected")
			
		if(imageCallbaks.wait_for_connection() == False) :
			raise ConnectionError("Error: imageCallbaks not connected")
	
		if(tilterMotorMTYP.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvTilterMotorMTYP + " not connected")
		if(laserControlBit.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLaserControlBit + " not connected")
		if(tilterMotorVMAX.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pcTilterMotorVMAX + " not connected")
		if(tilterMotorVBAS.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pcTilterMotorVBAS + " not connected")
		if(tilterMotorWLP.wait_for_connection() == False) :
			raise ConnectionError("Error: tilterMotorWLP not connected")
		if(tilterMotorMRES.wait_for_connection() == False) :
			raise ConnectionError("Error: tilterMotorMRES not connected")

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
	cameraArraycallbacks.put(1) # Ensure Array Callbacks are enabled
	cameraEnableFocus.put(1)
	imageCallbaks.put(1)        # Ensure Image Callbacks are enabled

	tilterMotorMTYP.put(5)      # Motor Type: Rev LA Stepper
	laserControlBit.put(0)      # Ensure Laser is switched off
	tilterMotorMRES.put(0.00027)# mm per step
	tilterMotorVMAX.put(0.5)    # Max speed of Lenses in mm/s
	tilterMotorVBAS.put(0.4)    # Max speed of Lenses in mm/s
	tilterMotorWLP.put("Off")   # No Wrong Limits Protection (DO NOT TOUCH !)
	return True

	
def waitLensInPosition():
	while tilterMotorDMOV.get() != 1:# Wait until Move Done
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

	tilterMotorCNEN.put("Enable")
	tilterMotorAPOS.put(x1)
	waitLensInPosition()
	laserControlBit.put(1)
	waitLaserOn()# Ensure Laser is switched on
	cameraStartAcquire.put(1)
	waitNewimageReady()
	laserControlBit.put(0)
	#f1 = f(x1)#fake value for testing purpose
	f1 = -cameraFocusValue.get()
	print ("At position %f Focus value is %f" % (x1, -f1))

	tilterMotorCNEN.put("Enable")
	tilterMotorAPOS.put(x2)
	waitLensInPosition()
	laserControlBit.put(1)
	waitLaserOn()
	cameraStartAcquire.put(1)
	waitNewimageReady()
	laserControlBit.put(0)
	#f2 = f(x2)#fake value for testing purpose
	f2 = -cameraFocusValue.get()
	print ("At position %f Focus value is %f" % (x2, -f2))

	while (abs(x3-x0) > Tolerance) :
		if (f2 < f1) :
			x0 = x1
			x1 = x2
			x2 = R * x1 + C * x3
			f1 = f2
			tilterMotorCNEN.put("Enable")
			tilterMotorAPOS.put(x2)
			waitLensInPosition()
			laserControlBit.put(1)
			waitLaserOn()
			cameraStartAcquire.put(1)
			waitNewimageReady()
			laserControlBit.put(0)
			#f2 = f(x2) #fake value for testing purpose
			f2 = -cameraFocusValue.get()
			print ("At position %f Focus value is %f" % (x2, -f2))
		else :
			x3 = x2
			x2 = x1
			x1 = R * x2 + C * x0
			f2 = f1
			tilterMotorCNEN.put("Enable")
			tilterMotorAPOS.put(x1)
			waitLensInPosition()
			laserControlBit.put(1)
			waitLaserOn()
			cameraStartAcquire.put(1)
			waitNewimageReady()
			laserControlBit.put(0)
			#f1 = f(x1) #fake value for testing purpose
			f1 = -cameraFocusValue.get()
			print ("At position %f Focus value is %f" % (x1, -f1))

	tilterMotorCNEN.put("Enable")
	if (f1 < f2) :
		tilterMotorAPOS.put(x1)
		print ("Focus found at %f value %f" % (x1, -f1))
	else :
		tilterMotorAPOS.put(x2)
		print ("Focus found at %f value %f" % (x2, -f2))


def main():
	try:
		xMin = -12
		xMax = 0
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
		tilterMotorCNEN.put("Enable")      # Enable motor Power
		tilterMotorHOMF.put(1)             # Start Forward Homing procedure
		while tilterMotorDMOV.get() != 1 : # Wait until Homed
			time.sleep(1)
			print (".", end="")
		print ("Lens carrier at Home: Start focusing !")
		goldenSearch(xMin, xMax, xTol)
		cameraEnableFocus.put(0)
	except:
		print("Got an Exception") 
		pass
		
if __name__ == "__main__":
    main()		
