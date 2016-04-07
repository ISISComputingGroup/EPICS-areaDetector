#!/usr/bin/python

from __future__ import print_function
import os
import sys
import time

from CaChannel import CaChannel
from CaChannel import CaChannelException
import ca

from ADAndorIstar import ADAndorIstar

		
def initPVs(Camera):
	try:
		print("Connected to: ", Camera.pvManufacturer.getw(), " ", end="")
		print(Camera.pvModel.getw())
		
		Camera.pvFilePath.putw("c:/Images/")          # Path for file saving
		Camera.pvFileName.putw("testImage")           # File name header
		Camera.pvDataType.putw(Camera.DTUInt16)       # 16 bit unsigned images
		Camera.pvFileFormat.putw(Camera.FFFITS)       # Enforce Fits file format 
		Camera.pvFileTemplate.putw("%s%s_%3.3d.fits") # Default File name format
		Camera.pvFileNumber.putw(0)                   # Starting number of files
		Camera.pvAutoIncrement.putw(1)                # File number will be auto ingremented
		
		Camera.pvFitsHeaderFileName.putw("./FitsHeaderParameters.txt")
		
		# Ending any acquisition ongoing
		Camera.pvAcquire.putw(Camera.AADone)		
		while(Camera.pvStatus.getw() != Camera.ASIdle) : # Wait for the camera to be redy
			time.sleep(1)

		Camera.pvImageMode.putw(Camera.ADImageSingle) # We want to acquire as an image
		Camera.pvNumExposures.putw(1)                 # and just a single eposure
		Camera.pvReadMode.putw(Camera.ARImage)        # The readout will be a full image
		Camera.pvGateMode.putw(Camera.AGGateUsingDDG) # We use the MCP as a shutter
		Camera.pvDDGInsertionDelay.putw(0)            # Normal Inserction Delay

		Camera.pvArraycallbacks.putw(1)               # We want array callback to seve the file
		Camera.pvEnableFocusCalculation.putw(0)       # Just to be sure that no focus calculation will be performed
				
	except AttributeError, e :
		print("".join(e.args))
		
		
def main():
	print("Test program for acquiring Energy selective FITS Images")
	IMATCamera = ADAndorIstar(os.getenv("MYPVPREFIX"), "13ANDOR1:", "cam1:")
	print("Initilizing PVs")
	initPVs(IMATCamera)
	if(IMATCamera.pvFilePathExists_RBV.getw() != 1) :
		print("Unexisting destination Path. Exiting...")
		sys.exit(0)
		
	startTOF   = 0.0      # In seconds (converted in picoseconds into the driver)
	endTOF     = 10.0e-3  # In seconds
	deltaTOF   = 1000.0e-6 # In seconds (converted in picoseconds into the driver)
	currentTOF = startTOF
		
	exposureTime = 1  # In seconds

	IMATCamera.pvMCPGain.putw(0)
	IMATCamera.pvAcquireTime.putw(exposureTime)       # In seconds
	IMATCamera.pvAndorDDGIOC.putw(1)                  # We will Integrate On Chip

	########################################################################################################################
	#IMATCamera.pvTriggerMode.putw(IMATCamera.ATExternal)         # The Trigger signal MUST be obtained from T0 but FOR TEST
	#IMATCamera.pvAndorDDGTriggerMode.putw(IMATCamera.ATExternal) # we will use Image Chip and MCP interally triggered
	IMATCamera.pvTriggerMode.putw(IMATCamera.ATInternal)          # The Trigger signal MUST be obtained from T0 but FOR TEST
	IMATCamera.pvAndorDDGTriggerMode.putw(IMATCamera.ATInternal)  # we will use Image Chip and MCP interally triggered
	########################################################################################################################
	
	IMATCamera.pvAndorDDGGateWidth.putw(deltaTOF)
	IMATCamera.pvAndorDDGGateDelay.putw(currentTOF)
	
	while(currentTOF < endTOF-deltaTOF) :
		IMATCamera.startAcquire(1)
		while(IMATCamera.acquireDone != True) :
			time.sleep(1)
		#if current beam intensity was good enough... 
		#  we can go to the next energy value
		currentTOF += deltaTOF

if __name__ == "__main__":
	main()		
		