#!/usr/bin/python

from __future__ import print_function
import sys

from CaChannel import CaChannel
from CaChannel import CaChannelException

		
class ADDriver :
  # Area Detector Base Class
	#
	def __init__(self, pvPrefix, cameraPrefix, cameraR):
		self.pvPrefixCamera = pvPrefix + cameraPrefix + cameraR
		self.pvList = []

		# List of Status Values
		self.ASIdle         = 0
		self.ASAcquire      = 1
		self.ASReadout      = 2
		self.ASCorrect      = 3
		self.ASSaving       = 4
		self.ASAborting     = 5
		self.ASError        = 6
		self.ASWaiting      = 7
		self.ASInitializing = 8
		self.ASDisconnected = 9
		self.ASAborted      = 10

		# List of acquire modes
		self.AADone    = 0
		self.AAAcquire = 1


		# Process Variables ADDriver
		self.pvManufacturer      = CaChannel(self.pvPrefixCamera + "Manufacturer_RBV");       self.pvList.append(self.pvManufacturer)
		self.pvModel             = CaChannel(self.pvPrefixCamera + "Model_RBV");              self.pvList.append(self.pvModel)
		self.pvStatus            = CaChannel(self.pvPrefixCamera + "DetectorState_RBV");      self.pvList.append(self.pvStatus)
		self.pvSizeX             = CaChannel(self.pvPrefixCamera + "SizeX_RBV");              self.pvList.append(self.pvSizeX)
		self.pvSizeY             = CaChannel(self.pvPrefixCamera + "SizeY_RBV");              self.pvList.append(self.pvSizeY)
		self.pvAcquireTime       = CaChannel(self.pvPrefixCamera + "AcquireTime");            self.pvList.append(self.pvAcquireTime)
		self.pvImageMode         = CaChannel(self.pvPrefixCamera + "ImageMode");              self.pvList.append(self.pvImageMode)
		self.pvNumExposures      = CaChannel(self.pvPrefixCamera + "NumExposures");           self.pvList.append(self.pvNumExposures)
		self.pvAcquire           = CaChannel(self.pvPrefixCamera + "Acquire");                self.pvList.append(self.pvAcquire)
		self.pvTemperature       = CaChannel(self.pvPrefixCamera + "Temperature");            self.pvList.append(self.pvTemperature)
		self.pvTemperatureActual = CaChannel(self.pvPrefixCamera + "TemperatureActual");      self.pvList.append(self.pvTemperatureActual)
				
		try :
			for pv in self.pvList[:] :
				print("ADDriver   Connecting " + pv.pvname)
				pv.searchw()

		except CaChannelException, e :
			print("".join(e.args))
			sys.exit(0)
