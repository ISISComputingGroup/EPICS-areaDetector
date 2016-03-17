#!/usr/bin/python

from CaChannel import CaChannel
from CaChannel import CaChannelException
import ca

import time
import sys

#pvprefix = "IMAT-MESSINA-DETECT:IMAT:13ANDOR1:"
pvprefix = "13ANDOR1:"

# Process Variables we are interested in
cameraStatusPV               = pvprefix + "cam1:DetectorState_RBV"
startAcquirePV               = pvprefix + "cam1:Acquire"
exposureTimePV               = pvprefix + "cam1:AcquireTime"
cameraArraycallbacksPV       = pvprefix + "cam1:ArrayCallbacks"

filePathPV                   = pvprefix + "cam1:FilePath"
fileNamePV                   = pvprefix + "cam1:FileName"
fileNumberPV                 = pvprefix + "cam1:FileNumber"
fileTemplatePV               = pvprefix + "cam1:FileTemplate"
autoIncrementPV              = pvprefix + "cam1:AutoIncrement"
saveFilePV                   = pvprefix + "cam1:WriteFile"

fitsFileHeaderFullFileNamePV = pvprefix + "cam1:FitsFileHeaderFullFileName"


# For focusing....
# 13ANDOR1:Focus:ComputeFocusMetrics
# 13ANDOR1:Focus:FocusValue_RBV
# 13ANDOR1:Focus1:EnableCallback

# Create CA Channels for PVs
cameraStatus               = CaChannel(cameraStatusPV)
startAcquire               = CaChannel(startAcquirePV)
exposureTime               = CaChannel(exposureTimePV)
cameraArraycallbacks       = CaChannel(cameraArraycallbacksPV)
filePath                   = CaChannel(filePathPV)
fileName                   = CaChannel(fileNamePV)
fileNumber                 = CaChannel(fileNumberPV)
fileTemplate               = CaChannel(fileTemplatePV)
autoIncrement              = CaChannel(autoIncrementPV)
saveFile                   = CaChannel(saveFilePV)

fitsFileHeaderFullFileName = CaChannel(fitsFileHeaderFullFileNamePV)

isCameraArmed         = False
exposureTimeSeconds   = 1


def eventCB(epics_args, user_args):
	if (epics_args['pv_value'] == 0) and (isCameraArmed == True):# A new image is available
		saveFile.putw(1)
		isCameraArmed = False
		startAcquire.putw(1)
	elif epics_args['pv_value'] == 1:
		isCameraArmed = True
	
#	print "eventCb: Python callback function"
#	print type(epics_args)
#	print epics_args
#	print ca.message(epics_args['status'])
#	print epics_args['pv_value']
#	print ca.alarmSeverityString(epics_args['pv_severity'])
#	print ca.alarmStatusString(epics_args['pv_status'])

		
# Initialize the values of PVs	
def initPVs():
	try:
		cameraArraycallbacks.putw(1)
		filePath.array_put("c:/Images")
		filePath.pend_io()
		fileName.array_put("testImage")
		fileName.pend_io()
		fileTemplate.array_put("%s%s_%3.3d.fits")
		fileTemplate.pend_io()
		fileNumber.putw(0)
		autoIncrement.putw(1)
		fitsFileHeaderFullFileName.array_put("./FitsHeaderParameters.txt")
		fitsFileHeaderFullFileName.pend_io()
		exposureTime.putw(exposureTimeSeconds)

	except CaChannelException as e:
		print e
		sys.exit(0)
		
		
# Attempt to establish a connection to a process variable.
# This method waits for connection to be established or fail with exception.
def connectPVs():
	try:
		cameraStatus.searchw()
		startAcquire.searchw()
		exposureTime.searchw()
		cameraArraycallbacks.searchw()
		filePath.searchw()
		fileName.searchw()
		fileNumber.searchw()
		fileTemplate.searchw()
		autoIncrement.searchw()
		fitsFileHeaderFullFileName.searchw()
		saveFile.searchw()
		
	except CaChannelException as e:
		print e
		sys.exit(0)


def main():
	connectPVs()
	initPVs()
		
	try:
		while(fileNumber.getw() != 10) :
			startAcquire.putw(1)
			while (cameraStatus.getw() != 1) :
				pass
			while (cameraStatus.getw() != 0) :
				pass
			saveFile.putw(1)
			
		
	except CaChannelException as e:
			print e.status
	

if __name__ == "__main__":
    main()		