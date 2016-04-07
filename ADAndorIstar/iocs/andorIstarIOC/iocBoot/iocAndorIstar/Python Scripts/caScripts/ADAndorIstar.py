#!/usr/bin/python

from __future__ import print_function
import os
import sys
import time

from CaChannel import CaChannel
from CaChannel import CaChannelException
import ca

from ADDriver import ADDriver
	
	
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
		self.DTUInt16 = 0
		self.DTUInt32 = 1
			
		# Local variables
		self.nImageWritten      = 0 # Will save the number of images written
		self.imagesToAcquire    = 1 # Will contain the number of images to acquire
		self.acquisitionStarted = False
		self.acquisitionPaused  = False
		self.acquireDone        = False
		
		# Process Variables for the iStar Camera
		self.pvArraycallbacks         = CaChannel(self.pvPrefixCamera + "ArrayCallbacks");          self.pvList.append(self.pvArraycallbacks)
		self.pvReadMode               = CaChannel(self.pvPrefixCamera + "AndorReadMode");           self.pvList.append(self.pvReadMode)
		self.pvTriggerMode            = CaChannel(self.pvPrefixCamera + "TriggerMode");             self.pvList.append(self.pvTriggerMode)
		self.pvGateMode               = CaChannel(self.pvPrefixCamera + "AndorGateMode");           self.pvList.append(self.pvGateMode)
		self.pvMCPGain                = CaChannel(self.pvPrefixCamera + "AndorMCPGain");            self.pvList.append(self.pvMCPGain)
		self.pvEnableFocusCalculation = CaChannel(self.pvPrefixCamera + "EnableFocusCalculation");  self.pvList.append(self.pvEnableFocusCalculation)
		self.pvFocusValue             = CaChannel(self.pvPrefixCamera + "FocusValue_RBV");          self.pvList.append(self.pvFocusValue)

		self.pvFilePath               = CaChannel(self.pvPrefixCamera + "FilePath");                self.pvList.append(self.pvFilePath)
		self.pvFilePath_RBV           = CaChannel(self.pvPrefixCamera + "FilePath_RBV");            self.pvList.append(self.pvFilePath_RBV)
		self.pvFilePathExists_RBV     = CaChannel(self.pvPrefixCamera + "FilePathExists_RBV");      self.pvList.append(self.pvFilePathExists_RBV)
		self.pvFileName               = CaChannel(self.pvPrefixCamera + "FileName");                self.pvList.append(self.pvFileName)
		self.pvFileName_RBV           = CaChannel(self.pvPrefixCamera + "FileName_RBV");            self.pvList.append(self.pvFileName_RBV)
		self.pvFileNumber             = CaChannel(self.pvPrefixCamera + "FileNumber");              self.pvList.append(self.pvFileNumber)
		self.pvFileNumber_RBV         = CaChannel(self.pvPrefixCamera + "FileNumber_RBV");          self.pvList.append(self.pvFileNumber_RBV)
		self.pvFileTemplate           = CaChannel(self.pvPrefixCamera + "FileTemplate");            self.pvList.append(self.pvFileTemplate)
		self.pvFileTemplate_RBV       = CaChannel(self.pvPrefixCamera + "FileTemplate_RBV");        self.pvList.append(self.pvFileTemplate_RBV)
		self.pvAutoIncrement          = CaChannel(self.pvPrefixCamera + "AutoIncrement");           self.pvList.append(self.pvAutoIncrement)
		self.pvWriteFile              = CaChannel(self.pvPrefixCamera + "WriteFile");               self.pvList.append(self.pvWriteFile)
		self.pvFileFormat             = CaChannel(self.pvPrefixCamera + "FileFormat");              self.pvList.append(self.pvFileFormat)
		self.pvDataType               = CaChannel(self.pvPrefixCamera + "DataType");                self.pvList.append(self.pvDataType)
		self.pvAndorCooler            = CaChannel(self.pvPrefixCamera + "AndorCooler");             self.pvList.append(self.pvAndorCooler)
		self.pvAndorDDGIOC            = CaChannel(self.pvPrefixCamera + "AndorDDGIOC");             self.pvList.append(self.pvAndorDDGIOC)
		self.pvDDGInsertionDelay      = CaChannel(self.pvPrefixCamera + "DDGInsertionDelay");       self.pvList.append(self.pvDDGInsertionDelay)
		self.pvAndorDDGTriggerMode    = CaChannel(self.pvPrefixCamera + "AndorDDGTriggerMode");     self.pvList.append(self.pvAndorDDGTriggerMode)
		self.pvAndorDDGGateDelay      = CaChannel(self.pvPrefixCamera + "AndorDDGGateDelay");       self.pvList.append(self.pvAndorDDGGateDelay)
		self.pvAndorDDGGateWidth      = CaChannel(self.pvPrefixCamera + "AndorDDGGateWidth");       self.pvList.append(self.pvAndorDDGGateWidth)
		self.pvAndorAccumulatePeriod  = CaChannel(self.pvPrefixCamera + "AndorAccumulatePeriod");   self.pvList.append(self.pvAndorAccumulatePeriod)
		self.pvFitsHeaderFileName     = CaChannel(self.pvPrefixCamera + "FitsHeaderFileName");      self.pvList.append(self.pvFitsHeaderFileName)
		
		try :
			for pv in self.pvList[:] :
				#print("ADAndorIstar Connecting " + pv.pvname)
				pv.searchw()
				
			self.pvStatus.add_masked_array_event(
				ca.dbf_type_to_DBR_STS(self.pvStatus.field_type()),
				None,
				ca.DBE_VALUE,
				self.onStatusChanged)

		except CaChannelException, e :
			print("".join(e.args))
			sys.exit(0)
			
			
	def onStatusChanged(self, epics_args, user_args) :
		if(self.acquisitionStarted == False) :
			return
		try :
			if(epics_args["pv_value"] != self.ASIdle) :
				return
			self.pvWriteFile.putw(1)
			self.nImageWritten = self.nImageWritten + 1
			if(self.nImageWritten >= self.imagesToAcquire) :
				self.acquireDone = True
				print(".")
				print("Acquisition Done")
				return
			print(".", end="")
			if(self.acquisitionPaused) :
				return
			self.pvAcquire.putw(self.AAAcquire)

		except CaChannelException, e :
			print("".join(e.args))
			sys.exit(0)

		except :
			sys.exit(0)
			
		
			
	def startAcquire(self, nImages=1) :
		if(nImages < 1) :
			print("Acquire ", nImages, "Are you kidding me ?")
			print("No Acquisition in progress")
			return
		self.imagesToAcquire  = nImages
		while(self.pvStatus.getw() != self.ASIdle) : # Wait for the camera to be redy
			time.sleep(1)
		self.nImageWritten = 0
		self.acquisitionStarted = True
		self.acquireDone = False
		self.pvAcquire.putw(self.AAAcquire)
		print("Acquiring", end="")
		
		
	def pauseAcquisition(self) : 
		self.acquisitionPaused = True
		
		
	def resumeAcquisition(self) : 
		self.acquisitionPaused = False
		self.pvAcquire.putw(self.AAAcquire)
		
		
	def abortAcquisition(self) : 
		self.acquisitionStarted = False
		self.acquisitionPaused  = False
		self.acquireDone        = True
		self.pvAcquire.putw(self.AADone)
		
