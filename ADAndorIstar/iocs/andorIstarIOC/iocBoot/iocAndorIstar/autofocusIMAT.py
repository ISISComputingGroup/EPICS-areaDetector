#!/usr/bin/python

from __future__ import print_function
from epics import PV

import time
import sys
import os
import math


#####################################
#####################################

AT_IMAT = False

#####################################
#####################################


if AT_IMAT == True :
# At IMAT...
	pvPrefixGalilCrate  = os.getenv("MYPVPREFIX") + "MOT:DMC01:"
	pvPrefixGalilMotor  = os.getenv("MYPVPREFIX") + "MOT:"
	pvprefixCamera      = os.getenv("MYPVPREFIX") + "13ANDOR1:"
else :
# At Messina...
	pvPrefixGalilCrate  = "IMAT:SALVATO:MOT:DMC01:"
	pvPrefixGalilMotor  = "IMAT:SALVATO:MOT:"
	pvprefixCamera      = "NUR:ADMINISTRATOR:13ANDOR1:"

# Constants...
lCarrierMotor       = "MTR0101"

ExposureTimeSeconds   = 0.003
ADImageSingle         = 0
ARImage               = 4
ATInternal            = 0
AGGateOnContinuously  = 3
GainDDG               = 0

NormallyOpenSwitch    = 0
NormallyCloseSwitch   = 1

LA_Stepper            = 3
BaseSpeed             = 4.0
MaxSpeed              = 4.0
Resolution            = 0.005


# Process Variables we are interested in:
# For iStar Camera
cameraStatusPV               = pvprefixCamera + "cam1:DetectorState_RBV"
cameraStartAcquirePV         = pvprefixCamera + "cam1:Acquire"
cameraExposureTimePV         = pvprefixCamera + "cam1:AcquireTime"
cameraImageModePV            = pvprefixCamera + "cam1:ImageMode"
cameraReadModePV             = pvprefixCamera + "cam1:AndorReadMode"
cameraTriggerModePV          = pvprefixCamera + "cam1:TriggerMode"
cameraGateModePV             = pvprefixCamera + "cam1:AndorGateMode"
cameraMCPGainPV              = pvprefixCamera + "cam1:AndorMCPGain"
cameraEnableFocusPV          = pvprefixCamera + "cam1:EnableFocusCalculation"
cameraFocusValuePV           = pvprefixCamera + "cam1:FocusValue_RBV"

# For Newport Lens Carrier
pvLensCarrierMTYP = pvPrefixGalilMotor + lCarrierMotor + "_MTRTYPE_CMD"
pvLensCarrierVBAS = pvPrefixGalilMotor + lCarrierMotor + ".VBAS"
pvLensCarrierVMAX = pvPrefixGalilMotor + lCarrierMotor + "_VMAX_SP"
pvLensCarrierCNEN = pvPrefixGalilMotor + lCarrierMotor + ".CNEN"
pvLensCarrierWLP  = pvPrefixGalilMotor + lCarrierMotor + "_WLP_CMD"
pvLensCarrierHOMR = pvPrefixGalilMotor + lCarrierMotor + ".HOMR"
pvLensCarrierHOMF = pvPrefixGalilMotor + lCarrierMotor + ".HOMF"
pvLensCarrierDMOV = pvPrefixGalilMotor + lCarrierMotor + ".DMOV"
pvLensCarrierAPOS = pvPrefixGalilMotor + lCarrierMotor + ".VAL"
pvLensCarrierMRES = pvPrefixGalilMotor + lCarrierMotor + ".MRES"

# For Limit Switches configuration
pvLimitSwitchType = pvPrefixGalilCrate + "LIMITTYPE_CMD"
pvHomeSwitchType  = pvPrefixGalilCrate + "HOMETYPE_CMD"

# For Laser light source
pvLaserControlBit = pvPrefixGalilCrate + "Galil0Bo0_CMD"
pvLaserStatusBit  = pvPrefixGalilCrate + "Galil0Bo0_STATUS.VAL"

# Now we can...
# Create PVs
cameraStatus           = PV(cameraStatusPV)
cameraStartAcquire     = PV(cameraStartAcquirePV)
cameraExposureTime     = PV(cameraExposureTimePV)
cameraImageMode        = PV(cameraImageModePV)
cameraReadMode         = PV(cameraReadModePV)
cameraTriggerMode      = PV(cameraTriggerModePV)
cameraGateMode         = PV(cameraGateModePV)
cameraMCPGain          = PV(cameraMCPGainPV)
cameraEnableFocus      = PV(cameraEnableFocusPV)
cameraFocusValue       = PV(cameraFocusValuePV)

lensCarrierMTYP        = PV(pvLensCarrierMTYP)
lensCarrierVBAS        = PV(pvLensCarrierVBAS)
lensCarrierVMAX        = PV(pvLensCarrierVMAX)
lensCarrierCNEN        = PV(pvLensCarrierCNEN)
lensCarrierWLP         = PV(pvLensCarrierWLP)
lensCarrierHOMR        = PV(pvLensCarrierHOMR)
lensCarrierHOMF        = PV(pvLensCarrierHOMF)
lensCarrierDMOV        = PV(pvLensCarrierDMOV)
lensCarrierAPOS        = PV(pvLensCarrierAPOS)
lensCarrierMRES        = PV(pvLensCarrierMRES)

limitSwitchType        = PV(pvLimitSwitchType)
homeSwitchType         = PV(pvHomeSwitchType)

laserControlBit        = PV(pvLaserControlBit)
laserStatusBit         = PV(pvLaserStatusBit)


# A class to handle CA Connection exceptions
class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
		
# Attempt to establish a connection to the process variables.
def connectPVs():
	try :
		if(cameraExposureTime.wait_for_connection() == False):
			raise ConnectionError("Error: " + pvCameraExposureTime + " not connected")
		if(cameraImageMode.wait_for_connection() == False) :
			raise ConnectionError("Error: " + CameraImageMode + " not connected")
		if(cameraReadMode.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvCameraReadMode + " not connected")
		if(cameraTriggerMode.wait_for_connection() == False):
			raise ConnectionError("Error: " + pvCameraTriggerMode + " not connected")
		if(cameraMCPGain.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvCameraMCPGain + " not connected")
		if(cameraEnableFocus.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvCameraEnableFocus + " not connected")
		if(cameraFocusValue.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvCameraFocusValue + " not connected")
	
		if(laserControlBit.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLaserControlBit + " not connected")

		if(limitSwitchType.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLimitSwitchType + " not connected")
		if(homeSwitchType.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvHomeSwitchType + " not connected")

		if(lensCarrierMTYP.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLensCarrierMTYP + " not connected")
		if(lensCarrierVBAS.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLensCarrierVBAS + " not connected")
		if(lensCarrierVMAX.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLensCarrierVMAX + " not connected")
		if(lensCarrierWLP.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLensCarrierWLP + " not connected")
		if(lensCarrierMRES.wait_for_connection() == False) :
			raise ConnectionError("Error: " + pvLensCarrierMRES + " not connected")

		return True

	except ConnectionError, e :
		print("".join(e.args))
		return False
		
		
def initPVs():
	laserControlBit.put(0)    # Ensure Laser is switched off

	cameraMCPGain.put(GainDDG)
	cameraExposureTime.put(ExposureTimeSeconds)
	cameraImageMode.put(ADImageSingle)
	cameraReadMode.put(ARImage)
	cameraGateMode.put(AGGateOnContinuously)
	cameraTriggerMode.put(ATInternal)
	cameraGateMode.put(AGGateOnContinuously)

#	limitSwitchType.put(NormallyOpenSwitch)# To be carefully checked <<<<<<<<<<<<<<<<<<<<
#	homeSwitchType.put(NormallyOpenSwitch) # To be carefully checked <<<<<<<<<<<<<<<<<<<<
	limitSwitchType.put(NormallyCloseSwitch)# To be carefully checked <<<<<<<<<<<<<<<<<<<<
	homeSwitchType.put(NormallyCloseSwitch) # To be carefully checked <<<<<<<<<<<<<<<<<<<<
	
	lensCarrierMTYP.put(LA_Stepper) # LA Stepper
	lensCarrierMRES.put(Resolution) # mm per step
	lensCarrierVBAS.put(BaseSpeed)  # Base speed of Lenses in mm/s
	lensCarrierVMAX.put(MaxSpeed)   # Max speed of Lenses in mm/s
	lensCarrierWLP.put("Off")       # No Wrong Limits Protection (Why ?)
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

		
def f(x):# Fake focus value for test
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
	#f1 = f(x1)#fake value for testing
	f1 = -cameraFocusValue.get()
	print ("At position %f Focus value is %f" % (x1, -f1))

	lensCarrierCNEN.put("Enable")
	lensCarrierAPOS.put(x2)
	waitLensInPosition()
	laserControlBit.put(1)
	waitLaserOn()
	cameraStartAcquire.put(1)
	waitNewimageReady()
	laserControlBit.put(0)
	#f2 = f(x2)#fake value for testing
	f2 = -cameraFocusValue.get()
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
			#f2 = f(x2) #fake value for testing purpose
			f2 = -cameraFocusValue.get()
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
			#f1 = f(x1) #fake value for testing purpose
			f1 = -cameraFocusValue.get()
			print ("At position %f Focus value is %f" % (x1, -f1))

	lensCarrierCNEN.put("Enable")
	if (f1 < f2) :
		lensCarrierAPOS.put(x1)
		print ("Focus found at %f value %f" % (x1, -f1))
	else :
		lensCarrierAPOS.put(x2)
		print ("Focus found at %f value %f" % (x2, -f2))


def main():
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
	lensCarrierHOMR.put(1)             # Start Reverse Homing procedure << To be checked !
	while lensCarrierDMOV.get() != 1 : # Wait until Homed
		time.sleep(1)
		print (".", end="")
	print ("Lens carrier at Home: Start focusing !")
	cameraEnableFocus.put(1)# Enable focus calculation by the Andor driver
	goldenSearch(xMin, xMax, xTol)
	cameraEnableFocus.put(0)
	laserControlBit.put(0)  # Just to be sure...
		
if __name__ == "__main__":
    main()		
