#!/usr/bin/python

from __future__ import print_function
from epics import PV
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
		self.pvManufacturer      = PV(self.pvPrefixCamera + "Manufacturer_RBV");       self.pvList.append(self.pvManufacturer)
		self.pvModel             = PV(self.pvPrefixCamera + "Model_RBV");              self.pvList.append(self.pvModel)
		self.pvStatus            = PV(self.pvPrefixCamera + "DetectorState_RBV");      self.pvList.append(self.pvStatus)
		self.pvSizeX             = PV(self.pvPrefixCamera + "SizeX_RBV");              self.pvList.append(self.pvSizeX)
		self.pvSizeY             = PV(self.pvPrefixCamera + "SizeY_RBV");              self.pvList.append(self.pvSizeY)
		self.pvAcquireTime       = PV(self.pvPrefixCamera + "AcquireTime");            self.pvList.append(self.pvAcquireTime)
		self.pvImageMode         = PV(self.pvPrefixCamera + "ImageMode");              self.pvList.append(self.pvImageMode)
		self.pvNumExposures      = PV(self.pvPrefixCamera + "NumExposures");           self.pvList.append(self.pvNumExposures)
		self.pvAcquire           = PV(self.pvPrefixCamera + "Acquire");                self.pvList.append(self.pvAcquire)
		self.pvTemperature       = PV(self.pvPrefixCamera + "Temperature");            self.pvList.append(self.pvTemperature)
		self.pvTemperatureActual = PV(self.pvPrefixCamera + "TemperatureActual");      self.pvList.append(self.pvTemperatureActual)
		try :
			for pv in self.pvList[:] :
				#print("ADDriver   Connecting " + pv.pvname)
				if(pv.wait_for_connection() == False) :
					raise ConnectionError("ADDriver Error: " + pv.pvname + " is NOT connected")

		except ConnectionError, e :
			print("".join(e.args))
			sys.exit(0)
