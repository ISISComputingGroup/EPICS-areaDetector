#!/usr/bin/python

from __future__ import print_function
from epics import PV
from ADAndorIstar import ADAndorIstar
import os
import sys
import time

		
def initPVs(Camera):
	try:
		print("Connected to: ", Camera.pvManufacturer.get(), " ", end="")
		print(Camera.pvModel.get())
		
		Camera.pvFilePath.put("c:/Images/")
		Camera.pvFileName.put("testImage")
		Camera.pvDataType.put(Camera.DTUInt16)
		Camera.pvFileFormat.put(Camera.FFFITS)
		Camera.pvFileTemplate.put("%s%s_%3.3d.fits")
		Camera.pvFileNumber.put(0)
		Camera.pvAutoIncrement.put(1)
		Camera.pvFitsHeaderFileName.put("./FitsHeaderParameters.txt")
		
		exposureTime = 1
		
		Camera.pvImageMode.put(Camera.ADImageSingle) # We want to acquire as an image
		Camera.pvNumExposures.put(1)                 # and just a single eposure
		Camera.pvReadMode.put(Camera.ARImage)        # The readout will be a full image
		
		Camera.pvMCPGain.put(0)
		
		Camera.pvAcquireTime.put(exposureTime)       # In seconds
		Camera.pvAndorDDGGateWidth.put(exposureTime) # In seconds (converted in picoseconds into the driver)
		Camera.pvAndorDDGGateDelay.put(0.0)          # In seconds (converted in picoseconds into the driver)
		Camera.pvGateMode.put(Camera.AGGateUsingDDG) # We use the MCP as a shutter
		Camera.pvDDGInsertionDelay.put(0)            #
		Camera.pvAndorDDGIOC.put(0)                  # We not Integrate On Chip

		Camera.pvTriggerMode.put(Camera.ATInternal)         # Image Chip and MCP interally triggered
		Camera.pvAndorDDGTriggerMode.put(Camera.ATInternal) #
		
		Camera.pvArraycallbacks.put(1)
		Camera.pvEnableFocusCalculation.put(0)
		
	except AttributeError, e :
		print("".join(e.args))
		
		
def main():
	IMATCamera = ADAndorIstar(os.getenv("MYPVPREFIX"), "ANDOR1:", "cam1:")
	print("Initilizing PVs")
	initPVs(IMATCamera)
	print("Test program for acquiring 10 FITS Images")
	print("stored into the directory: ", end="")
	fp = IMATCamera.pvFilePath.get(None, True)
	print("%s" % fp)
	print("Connecting to the Camera IOC PVs")

	print("Start acquiring", end="")
	try:
		for i in range(0, 10) :
			IMATCamera.pvAcquire.put(IMATCamera.AAAcquire)
			while (IMATCamera.pvStatus.get() != IMATCamera.ASAcquire) :
				pass
			while (IMATCamera.pvStatus.get() != IMATCamera.ASIdle) :
				pass
			IMATCamera.pvWriteFile.put(1)
			print(".", end="")
		print("Done")
		IMATCamera.pvArraycallbacks.put(0)	
		
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
		