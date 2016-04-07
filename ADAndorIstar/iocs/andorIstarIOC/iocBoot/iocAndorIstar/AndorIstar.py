#!/usr/bin/python

from __future__ import print_function
from epics import PV
import os
import sys


class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
		
class ADDriver :
  # Area Detector Base Class
	#
	def __init__(self, pvPrefix, cameraPrefix, cameraR):
		self.pvPrefixCamera = pvPrefix + cameraPrefix + cameraR
		self.pvList = []
		# Process Variables ADDriver
		self.pvManufacturer                 = PV(self.pvPrefixCamera + "Manufacturer_RBV");       self.pvList.append(self.pvManufacturer)
		self.pvModel                        = PV(self.pvPrefixCamera + "Model_RBV");              self.pvList.append(self.pvModel)
		self.pvSizeX                        = PV(self.pvPrefixCamera + "SizeX_RBV");              self.pvList.append(self.pvSizeX)
		self.pvSizeY                        = PV(self.pvPrefixCamera + "SizeY_RBV");              self.pvList.append(self.pvSizeY)
		self.pvAcquireTime                  = PV(self.pvPrefixCamera + "AcquireTime");            self.pvList.append(self.pvAcquireTime)
		self.pvImageMode                    = PV(self.pvPrefixCamera + "ImageMode");              self.pvList.append(self.pvImageMode)
		self.pvAcquire                      = PV(self.pvPrefixCamera + "Acquire");                self.pvList.append(self.pvAcquire)
		self.pvTemperature                  = PV(self.pvPrefixCamera + "Temperature");            self.pvList.append(self.pvTemperature)
		self.pvTemperatureActual            = PV(self.pvPrefixCamera + "TemperatureActual");      self.pvList.append(self.pvTemperatureActual)
		try :
			for pv in self.pvList[:] :
				#print("ADDriver   Connecting " + pv.pvname)
				if(pv.wait_for_connection() == False) :
					raise ConnectionError("ADDriver Error: " + pv.pvname + " is NOT connected")

		except ConnectionError, e :
			print("".join(e.args))
			sys.exit(0)
		
	
class AndorIstar(ADDriver) :
  # Constructor example:
	# IMATcamera = AndorIstar(os.getenv("MYPVPREFIX"), "13ANDOR1:", "cam1:")
	#
	def __init__(self, pvPrefix, cameraPrefix, cameraR):
		ADDriver.__init__(self, pvPrefix, cameraPrefix, cameraR)
		self.pvPrefixCamera = pvPrefix + cameraPrefix + cameraR
		self.pvList = []
		
		self.ADImageSingle         = 0
		self.ARImage               = 4
		self.ATInternal            = 0
		self.AGGateOnContinuously  = 3

		# Process Variables for the iStar Camera
		self.pvStatus                 = PV(self.pvPrefixCamera + "DetectorState_RBV");       self.pvList.append(self.pvStatus)
		self.pvArraycallbacks         = PV(self.pvPrefixCamera + "ArrayCallbacks");          self.pvList.append(self.pvArraycallbacks)
		self.pvReadMode               = PV(self.pvPrefixCamera + "AndorReadMode");           self.pvList.append(self.pvReadMode)
		self.pvTriggerMode            = PV(self.pvPrefixCamera + "TriggerMode");             self.pvList.append(self.pvTriggerMode)
		self.pvGateMode               = PV(self.pvPrefixCamera + "AndorGateMode");           self.pvList.append(self.pvGateMode)
		self.pvMCPGain                = PV(self.pvPrefixCamera + "AndorMCPGain");            self.pvList.append(self.pvMCPGain)
		self.pvEnableFocusCalculation = PV(self.pvPrefixCamera + "EnableFocusCalculation");  self.pvList.append(self.pvEnableFocusCalculation)
		self.pvFocusValue             = PV(self.pvPrefixCamera + "FocusValue_RBV");          self.pvList.append(self.pvFocusValue)

		self.pvFilePath               = PV(self.pvPrefixCamera + "FilePath");                self.pvList.append(self.pvFilePath)
		self.pvFileName               = PV(self.pvPrefixCamera + "FileName");                self.pvList.append(self.pvFileName)
		self.pvFileNumber             = PV(self.pvPrefixCamera + "FileNumber");              self.pvList.append(self.pvFileNumber)
		self.pvFileTemplate           = PV(self.pvPrefixCamera + "FileTemplate");            self.pvList.append(self.pvFileTemplate)
		self.pvAutoIncrement          = PV(self.pvPrefixCamera + "AutoIncrement");           self.pvList.append(self.pvAutoIncrement)
		self.pvWriteFile              = PV(self.pvPrefixCamera + "WriteFile");               self.pvList.append(self.pvWriteFile)
		self.pvFileFormat             = PV(self.pvPrefixCamera + "FileFormat");              self.pvList.append(self.pvFileFormat)
		self.pvDataType               = PV(self.pvPrefixCamera + "DataType");                self.pvList.append(self.pvDataType)
		self.pvAndorCooler            = PV(self.pvPrefixCamera + "AndorCooler");             self.pvList.append(self.pvAndorCooler)
		self.pvAndorDDGIOC            = PV(self.pvPrefixCamera + "AndorDDGIOC");             self.pvList.append(self.pvAndorDDGIOC)
		self.pvDDGInsertionDelay      = PV(self.pvPrefixCamera + "DDGInsertionDelay");       self.pvList.append(self.pvDDGInsertionDelay)
		self.pvAndorDDGTriggerMode    = PV(self.pvPrefixCamera + "AndorDDGTriggerMode");     self.pvList.append(self.pvAndorDDGTriggerMode)
		self.pvAndorDDGGateDelay      = PV(self.pvPrefixCamera + "AndorDDGGateDelay");       self.pvList.append(self.pvAndorDDGGateDelay)
		self.pvAndorDDGGateWidth      = PV(self.pvPrefixCamera + "AndorDDGGateWidth");       self.pvList.append(self.pvAndorDDGGateWidth)
		self.pvAndorAccumulatePeriod  = PV(self.pvPrefixCamera + "AndorAccumulatePeriod");   self.pvList.append(self.pvAndorAccumulatePeriod)
		self.pvFitsHeaderFileName     = PV(self.pvPrefixCamera + "FitsHeaderFileName");      self.pvList.append(self.pvFitsHeaderFileName)
		try :
			# print("AndorIstar.connectPVs()")
			# print(self.pvList[:])
			for pv in self.pvList[:] :
				#print("AndorIstar Connecting " + pv.pvname)
				if(pv.wait_for_connection() == False) :
					raise ConnectionError("AndorIstar Error: " + pv.pvname + " is NOT connected")

		except ConnectionError, e :
			print("".join(e.args))
			sys.exit(0)
			
		
def initPVs(Camera):
	try:
		print("Connected to: ", Camera.pvManufacturer.get(), " ", end="")
		print(Camera.pvModel.get())
		
		Camera.pvFilePath.put("c:/Images")
		Camera.pvFileName.put("testImage")
		Camera.pvFileTemplate.put("%s%s_%3.3d.fits")
		Camera.pvFileNumber.put(0)
		Camera.pvAutoIncrement.put(1)
		Camera.pvFitsHeaderFileName.put("./FitsHeaderParameters.txt")
		
		
		Camera.pvMCPGain.put(0)
		Camera.pvAcquireTime.put(0.001)
		Camera.pvImageMode.put(Camera.ADImageSingle)
		Camera.pvReadMode.put(Camera.ARImage)
		Camera.pvGateMode.put(Camera.AGGateOnContinuously)
		Camera.pvTriggerMode.put(Camera.ATInternal)
		Camera.pvAndorDDGTriggerMode.put(Camera.ATInternal)
		Camera.pvArraycallbacks.put(0)# Start with Array Callbacks disabled
		Camera.pvEnableFocusCalculation.put(0)
		Camera.pvDDGInsertionDelay.put(0)
		Camera.pvAndorDDGIOC.put(0)
		
	except AttributeError, e :
		print("".join(e.args))
		
		
def main():
	IMATCamera = AndorIstar(os.getenv("MYPVPREFIX"), "13ANDOR1:", "cam1:")
	print("Test program for acquiring 10 FITS Images")
	print("stored into the directory: ", end="")
	fp = IMATCamera.pvFilePath.get(None, True)
	print("%s" % fp)
	print("Initilizing PVs")
	initPVs(IMATCamera)
	print("Connecting to the Camera IOC PVs")

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

		