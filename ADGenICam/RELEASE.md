ADGenICam Releases
==================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADGenICam.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R1-1 (20-October-2019)
----
* Changed ADSerialNumber to use either DeviceSerialNumber or DeviceID feature.  
  AVT uses DeviceID for at least some of its cameras.
* Changed makeDb.py to remove the autosaveFields info nodes because they were not useful.
* Rebuilt all the template files using the new version of makeDb.py so they no longer have the autosaveFields tags.
* Added new camera xml, template and OPI files for AVT_GC1380CH and PGR_PGE_23S6C cameras.

R1-0 (12-August-2019)
----
* Initial release

