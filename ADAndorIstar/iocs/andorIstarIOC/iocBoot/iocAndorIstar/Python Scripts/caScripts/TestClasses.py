#!/usr/bin/python

from __future__ import print_function
import os
import sys
import time

from CaChannel import CaChannel
from CaChannel import CaChannelException
from ADAndorIstar import ADAndorIstar

		
def initPVs(Camera):
#	try:
		print("Connected to: ", Camera.pvManufacturer.getw(), " ", end="")
		print(Camera.pvModel.getw())
		
		Camera.pvFilePath.putw("c:/Images/")
		Camera.pvFileName.putw("testImage")
		Camera.pvDataType.putw(Camera.DTUInt16)
		Camera.pvFileFormat.putw(Camera.FFFITS)
		Camera.pvFileTemplate.putw("%s%s_%3.3d.fits")
		Camera.pvFileNumber.putw(0)
		Camera.pvAutoIncrement.putw(1)
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
		Camera.pvDDGInsertionDelay.putw(0)            #
		Camera.pvAndorDDGIOC.putw(0)                  # We not Integrate On Chip

		Camera.pvTriggerMode.putw(Camera.ATInternal)         # Image Chip and MCP interally triggered
		Camera.pvAndorDDGTriggerMode.putw(Camera.ATInternal) #
		
		Camera.pvArraycallbacks.putw(1)
		Camera.pvEnableFocusCalculation.putw(0)
		
	# except AttributeError, e :
		# print("".join(e.args))
		
		
def main():
	IMATCamera = ADAndorIstar(os.getenv("MYPVPREFIX"), "ANDOR1:", "cam1:")
	print("Initilizing PVs")
	initPVs(IMATCamera)
	print("Test program for acquiring 10 FITS Images")
	print("stored into the directory: ", end="")
	fp = IMATCamera.pvFilePath.getw(None, True)
	print("%s" % fp)
	print("Connecting to the Camera IOC PVs")

	print("Start acquiring", end="")
	try:
		for i in range(0, 10) :
			IMATCamera.pvAcquire.putw(IMATCamera.AAAcquire)
			while (IMATCamera.pvStatus.getw() != IMATCamera.ASAcquire) :
				pass
			while (IMATCamera.pvStatus.getw() != IMATCamera.ASIdle) :
				pass
			IMATCamera.pvWriteFile.putw(1)
			print(".", end="")
		print("Done")
		IMATCamera.pvArraycallbacks.putw(0)	
		
	except NameError, e:
		print("Error: ", end="")
		print("".join(e.args))
		
	except AttributeError, e:
		print("Error: ", end="")
		print("".join(e.args))
		
	except :
		print("Unexpected Error: Exiting.")


if __name__ == "__main__":
	main()		
		