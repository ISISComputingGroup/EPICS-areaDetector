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
		
		exposureTime = 1
		
		Camera.pvImageMode.putw(Camera.ADImageSingle) # We want to acquire as an image
		Camera.pvNumExposures.putw(1)                 # and just a single eposure
		Camera.pvReadMode.putw(Camera.ARImage)        # The readout will be a full image
		
		Camera.pvMCPGain.putw(0)
		
		Camera.pvAcquireTime.putw(exposureTime)       # In seconds
		Camera.pvAndorDDGGateWidth.putw(exposureTime) # In seconds (converted in picoseconds into the driver)
		
		Camera.pvAndorDDGGateDelay.putw(0.0)          # In seconds (converted in picoseconds into the driver)
		Camera.pvGateMode.putw(Camera.AGGateUsingDDG) # We use the MCP as a shutter

		Camera.pvDDGInsertionDelay.putw(0)            # Normal Inserction Delay
		Camera.pvAndorDDGIOC.putw(0)                  # We do not Integrate On Chip

		Camera.pvTriggerMode.putw(Camera.ATInternal)         # Image Chip and MCP interally triggered
		Camera.pvAndorDDGTriggerMode.putw(Camera.ATInternal) #
		
		Camera.pvArraycallbacks.putw(1)
		Camera.pvEnableFocusCalculation.putw(0)
				
	except AttributeError, e :
		print("".join(e.args))
		
		
def main():
	print("Test program for acquiring 20 FITS Images")
	IMATCamera = ADAndorIstar(os.getenv("MYPVPREFIX"), "13ANDOR1:", "cam1:")
	print("Initilizing PVs")
	initPVs(IMATCamera)

	if(IMATCamera.pvFilePathExists_RBV.getw() != 1) :
		print("Unexisting destination Path. Exiting...")
		sys.exit(0)
		
	IMATCamera.startAcquire(20)

	while(IMATCamera.acquireDone != True) :
		time.sleep(1)	

if __name__ == "__main__":
	main()		
		