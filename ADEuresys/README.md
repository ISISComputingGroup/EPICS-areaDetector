An 
[EPICS](http://www.aps.anl.gov/epics)
[areaDetector](https://github.com/areaDetector/areaDetector/blob/master/README.md)
driver for CoaXPress frame grabbers from 
[Euresys](http://euresys.com) using their eGrabbera SDK.
These can control CoaXPress cameras from many vendors.

This driver derives from the [ADGenICam](https://github.com/areaDetector/ADGenICam) base class.

ADEuresys runs on both Windows and Linux.

 - The user must install the EGrabber package locally on both Windows and Linux.
 - configure/CONFIG_SITE or CONFIG_SITE.$(EPICS_HOST_ARCH).Common must be edited to point to the local install.

Additional information:
* [Documentation](https://areadetector.github.io/areaDetector/ADEuresys/ADEuresys.html)
* [Release notes](RELEASE.md)
