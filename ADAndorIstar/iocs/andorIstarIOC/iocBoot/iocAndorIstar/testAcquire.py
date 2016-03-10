#!/usr/bin/python

from __future__ import print_function
from epics import ca, PV

import os
import time
import sys

pvprefixCamera = os.getenv("MYPVPREFIX") + "13ANDOR1:"

# Process Variables we are interested in
cameraImageModePV            = pvprefixCamera + "cam1:ImageMode"
cameraReadModePV             = pvprefixCamera + "cam1:AndorReadMode"
cameraMCPGainPV              = pvprefixCamera + "cam1:AndorMCPGain"
cameraStatusPV               = pvprefixCamera + "cam1:DetectorState_RBV"
cameraStartAcquirePV         = pvprefixCamera + "cam1:Acquire"
cameraExposureTimePV         = pvprefixCamera + "cam1:AcquireTime"
cameraInsertionDelayPV       = pvprefixCamera + "cam1:DDGInsertionDelay"
cameraGateWidthPV            = pvprefixCamera + "cam1:AndorDDGGateWidth"
cameraGateDelayPV            = pvprefixCamera + "cam1:AndorDDGGateDelay"
cameraArraycallbacksPV       = pvprefixCamera + "cam1:ArrayCallbacks"
cameraTriggerModePV          = pvprefixCamera + "cam1:TriggerMode"
cameraDDGTriggerModePV       = pvprefixCamera + "cam1:AndorDDGTriggerMode"
cameraGateModePV             = pvprefixCamera + "cam1:AndorGateMode"
cameraIntegrateOnChipPV      = pvprefixCamera + "cam1:AndorDDGIOC"

filePathPV                   = pvprefixCamera + "cam1:FilePath"
fileNamePV                   = pvprefixCamera + "cam1:FileName"
fileNumberPV                 = pvprefixCamera + "cam1:FileNumber"
fileTemplatePV               = pvprefixCamera + "cam1:FileTemplate"
autoIncrementPV              = pvprefixCamera + "cam1:AutoIncrement"
saveFilePV                   = pvprefixCamera + "cam1:WriteFile"

fitsHeaderFileNamePV         = pvprefixCamera + "cam1:FitsHeaderFileName"


# For focusing....in the future
# pvprefixCamera + "Focus:ComputeFocusMetrics"
# pvprefixCamera + "Focus:FocusValue_RBV"
# pvprefixCamera + "Focus1:EnableCallback"

# Create CA Channels for PVs
cameraStatus               = PV(cameraStatusPV)
cameraImageMode            = PV(cameraImageModePV)
cameraReadMode             = PV(cameraReadModePV)
cameraTriggerMode          = PV(cameraTriggerModePV)
cameraStartAcquire         = PV(cameraStartAcquirePV)
cameraExposureTime         = PV(cameraExposureTimePV)
cameraInsertionDelay       = PV(cameraInsertionDelayPV)
cameraGateWidth            = PV(cameraGateWidthPV)
cameraGateDelay            = PV(cameraGateDelayPV)
cameraMCPGain              = PV(cameraMCPGainPV)
cameraDDGTriggerMode       = PV(cameraDDGTriggerModePV)
cameraGateMode             = PV(cameraGateModePV)
cameraIntegrateOnChip      = PV(cameraIntegrateOnChipPV)

cameraArraycallbacks       = PV(cameraArraycallbacksPV)

filePath                   = PV(filePathPV)
fileName                   = PV(fileNamePV)
fileNumber                 = PV(fileNumberPV)
fileTemplate               = PV(fileTemplatePV)
fileAutoIncrement          = PV(autoIncrementPV)
fileSave                   = PV(saveFilePV)

fitsHeaderFileName         = PV(fitsHeaderFileNamePV)

exposureTimeSeconds   = 10
ADImageSingle         = 0
ARImage               = 4
ATInternal            = 0
AGGateOnContinuously  = 3
GainDDG               = 0

		
# Initialize the values of PVs	
def initPVs():
	try:
		filePath.put("c:/Images")
		fileName.put("testImage")
		fileTemplate.put("%s%s_%3.3d.fits")
		fileNumber.put(0)
		fileAutoIncrement.put(1)
		fitsHeaderFileName.put("./FitsHeaderParameters.txt")
		
		cameraImageMode.put(ADImageSingle)
		cameraReadMode.put(ARImage)
		cameraArraycallbacks.put(1)
		cameraExposureTime.put(exposureTimeSeconds)
		cameraInsertionDelay.put(0)
		cameraTriggerMode.put(ATInternal)
		cameraDDGTriggerMode.put(ATInternal)
		cameraGateMode.put(AGGateOnContinuously)
		cameraIntegrateOnChip.put(0)
		cameraMCPGain.put(GainDDG)

	except :
		print (e)
		sys.exit(0)
		
		
class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
		
# Attempt to establish a connection to a process variable.
# This method waits for connection to be established or fail.
def connectPVs():
	try:
		if(cameraImageMode.wait_for_connection() == False) :
			raise ConnectionError("cameraImageMode")
		if(cameraReadMode.wait_for_connection() == False) :
			raise ConnectionError("cameraReadMode")
		if(cameraStatus.wait_for_connection() == False) :
			raise ConnectionError("cameraStatus")
		if(cameraStartAcquire.wait_for_connection() == False) :
			raise ConnectionError("cameraStartAcquire")
		if(cameraExposureTime.wait_for_connection() == False):
			raise ConnectionError("cameraExposureTime")
		if(cameraArraycallbacks.wait_for_connection() == False):
			raise ConnectionError("cameraArraycallbacks")
		if(cameraInsertionDelay.wait_for_connection() == False):
			raise ConnectionError("cameraInsertionDelay")
		if(cameraTriggerMode.wait_for_connection() == False):
			raise ConnectionError("cameraTriggerMode")
		if(cameraDDGTriggerMode.wait_for_connection() == False):
			raise ConnectionError("cameraDDGTriggerMode")
		if(cameraGateMode.wait_for_connection() == False) :
			raise ConnectionError("cameraGateMode")
		if(cameraIntegrateOnChip.wait_for_connection() == False) :
			raise ConnectionError("cameraIntegrateOnChip")
		if(cameraMCPGain.wait_for_connection() == False) :
			raise ConnectionError("cameraMCPGain")

		if(filePath.wait_for_connection() == False) :
			raise ConnectionError("filePath")
		if(fileName.wait_for_connection() == False) :
			raise ConnectionError("fileName")
		if(fileNumber.wait_for_connection() == False) :
			raise ConnectionError("fileNumber")
		if(fileTemplate.wait_for_connection() == False) :
			raise ConnectionError("fileTemplate")
		if(fileAutoIncrement.wait_for_connection() == False) :
			raise ConnectionError("fileAutoIncrement")
		if(fitsHeaderFileName.wait_for_connection() == False) :
			raise ConnectionError("fitsHeaderFileName")
		if(fileSave.wait_for_connection() == False) :
			raise ConnectionError("fileSave")

		return True
		
	except ConnectionError, e :
		print("Unable to Connect "+"".join(e.args) + "PV")
		return False

def main():
	print("Test program for acquiring 10 FITS Images")
	print("stored into the directory: ", end="")
	fp = filePath.get(None, True)
	print("%s" % fp)
	print("Connecting to the Camera IOC PVs")
	if(connectPVs() == False) :
		print("Not all the needed PVs are available: exiting")
		sys.exit(0)
	print("Initilizing PVs")
	initPVs()
	print("Start acquiring", end="")	
	try:
		while(fileNumber.get() < 9) :
			cameraStartAcquire.put(1)
			while (cameraStatus.get() != 1) :
				pass
			while (cameraStatus.get() != 0) :
				pass
			fileSave.put(1)
			print(".", end="")
		print("Done")
			
	except :
		print ("Error...exiting")
	

if __name__ == "__main__":
    main()		