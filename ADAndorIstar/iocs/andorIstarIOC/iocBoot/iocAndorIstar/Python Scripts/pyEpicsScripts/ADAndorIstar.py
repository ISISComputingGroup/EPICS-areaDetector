#!/usr/bin/python

from __future__ import print_function
from epics import PV
from ADDriver import ADDriver
import os
import sys
import time


class ConnectionError(StandardError) :
   def __init__(self, arg):
      self.args = arg
		
	
class ADAndorIstar(ADDriver) :
  # Constructor example:
	# IMATcamera = AndorIstar(os.getenv("MYPVPREFIX"), "ANDOR1:", "cam1:")
	#
	def __init__(self, pvPrefix, cameraPrefix, cameraR):
		ADDriver.__init__(self, pvPrefix, cameraPrefix, cameraR)
		self.pvPrefixCamera = pvPrefix + cameraPrefix + cameraR
		self.pvList = []
		
		# List of Image Modes
		self.ADImageSingle         = 0
		self.ADImageMultiple       = 1
		self.ADImageContinuous     = 2
		self.ADImageFastKinetics   = 3
		
		# List of Read Modes
		self.ARFullVerticalBinning = 0
		self.ARMultiTrack          = 1
		self.ARRandomTrack         = 2
		self.ARSingleTrack         = 3
		self.ARImage               = 4
		
		# List of Trigger Modes
		self.ATInternal            = 0
		self.ATExternal            = 1
		self.ATExternalStart       = 6
		self.ATExternalExposure    = 7
		self.ATExternalFVP         = 9
		self.ATSoftware            = 10
		
		# List of Gate Modes
		self.AGFireANDedWithGate   = 0
		self.AGGatingFromFireOnly  = 1
		self.AGGatingFromSMBOnly   = 2
		self.AGGateOnContinuously  = 3
		self.AGGateOffContinuously = 4
		self.AGGateUsingDDG        = 5
		
		# List of File Formats
		self.FFTIFF = 0
		self.FFBMP  = 1
		self.FFSIF  = 2
		self.FFEDF  = 3
		self.FFRAW  = 4
		self.FFFITS = 5
		self.FFSPE  = 6
		
		# List of Data Types
		self.DTUInt16 = 3
		self.DTUInt32 = 5
			
		self.startImageNumber = 0 # Will save the starting image number in a sequence of acquisitions
		self.imagesToAcquire  = 1 # Will contain the number of images to acquire
		
		# Process Variables for the iStar Camera
		self.pvArraycallbacks         = PV(self.pvPrefixCamera + "ArrayCallbacks");          self.pvList.append(self.pvArraycallbacks)
		self.pvReadMode               = PV(self.pvPrefixCamera + "AndorReadMode");           self.pvList.append(self.pvReadMode)
		self.pvTriggerMode            = PV(self.pvPrefixCamera + "TriggerMode");             self.pvList.append(self.pvTriggerMode)
		self.pvGateMode               = PV(self.pvPrefixCamera + "AndorGateMode");           self.pvList.append(self.pvGateMode)
		self.pvMCPGain                = PV(self.pvPrefixCamera + "AndorMCPGain");            self.pvList.append(self.pvMCPGain)
		self.pvEnableFocusCalculation = PV(self.pvPrefixCamera + "EnableFocusCalculation");  self.pvList.append(self.pvEnableFocusCalculation)
		self.pvFocusValue             = PV(self.pvPrefixCamera + "FocusValue_RBV");          self.pvList.append(self.pvFocusValue)

		self.pvFilePath               = PV(self.pvPrefixCamera + "FilePath");                self.pvList.append(self.pvFilePath)
		self.pvFilePath_RBV           = PV(self.pvPrefixCamera + "FilePath_RBV");            self.pvList.append(self.pvFilePath_RBV)
		self.pvFileName               = PV(self.pvPrefixCamera + "FileName");                self.pvList.append(self.pvFileName)
		self.pvFileName_RBV           = PV(self.pvPrefixCamera + "FileName_RBV");            self.pvList.append(self.pvFileName_RBV)
		self.pvFileNumber             = PV(self.pvPrefixCamera + "FileNumber");              self.pvList.append(self.pvFileNumber)
		self.pvFileNumber_RBV         = PV(self.pvPrefixCamera + "FileNumber_RBV");          self.pvList.append(self.pvFileNumber_RBV)
		self.pvFileTemplate           = PV(self.pvPrefixCamera + "FileTemplate");            self.pvList.append(self.pvFileTemplate)
		self.pvFileTemplate_RBV       = PV(self.pvPrefixCamera + "FileTemplate_RBV");        self.pvList.append(self.pvFileTemplate_RBV)
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
			for pv in self.pvList[:] :
				if(pv.wait_for_connection() == False) :
					raise ConnectionError("AndorIstar Error: " + pv.pvname + " is NOT connected")

		except ConnectionError, e :
			print("".join(e.args))
			sys.exit(0)

			
	def acquireDone(self, char_value, **kws) :
		if(self.pvStatus.get() != self.ASIdle) :
			print("Status= ", char_value)
			return
		print("Saving")
		self.pvWriteFile.put(1)
		self.presentFileNumber = self.pvFileNumber_RBV.get()
		if(self.presentFileNumber-self.startImageNumber >= self.imagesToAcquire) :
			self.pvStatus.clear_callbacks()
			print("Done acquisition")
			return
		print("Saved. Now Acquiring")
		self.pvAcquire.put(self.AAAcquire)
		
		
			
	def startAcquire(self, nImages=1) :
		if(nImages < 1) :
			print("Acquire ", nImages, "Are you kidding ?")
			return
		self.imagesToAcquire  = nImages
		filePath = self.pvFilePath_RBV.get(as_string=True)
		if(not os.path.isdir(filePath)) :
			print ("Error: Non existing or no path specified. Unable to start")
			return
		fileName = self.pvFileName_RBV.get(as_string=True)
		if(filePath == "") :
			print ("Error: No filename specified. Unable to start")
			return
		self.pvFileFormat.put(self.FFFITS) # Enforce Fits file format 
		fileTemplate = self.pvFileTemplate_RBV.get(as_string=True)
		if(fileTemplate == "") :
			self.pvFileTemplate.put("%s%s_%3.3d.fits") # Default format
		self.pvDataType.put(self.DTUInt16)
		self.startImageNumber = self.pvFileNumber_RBV.get()
		while(self.pvStatus.get() != self.ASIdle) : # Wait for the camera to be redy
			time.sleep(1)
		self.pvStatus.add_callback(self.acquireDone)
		self.pvAcquire.put(self.AAAcquire)
		