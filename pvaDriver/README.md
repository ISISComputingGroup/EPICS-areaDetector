pvaDriver
=================
An 
[EPICS](http://www.aps.anl.gov/epics)
[areaDetector](https://cars.uchicago.edu/software/epics/areaDetector.html)
driver for importing an EPICSv4 NTNDArray via pvAccess into areaDetector.
It does so by creating a monitor on the specified PV.  It can be used with the
NDPluginPva to copy NDArrys from one IOC to another, but can also be used to
allow any EPICS V4 server to send NTNDArrays to the IOC.


Additional information:
* [Documentation](https://cars.uchicago.edu/software/epics/pvaDriverDoc.html).
* [Release notes and links to source and binary releases](RELEASE.md).
