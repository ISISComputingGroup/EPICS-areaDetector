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
R1-9 (July 20, 2023)
----------------------
* Added XML, database and OPI files for many new cameras:
  - AVT Manta GT1290, Alvium 1800 U-811
  - Basler acA1300-60gm, acA1920-25gm, acA640-90gm, acA1300-75gm, acA3088-16gm, acA640-300gm
  - FLIR A25, ORX_10G_123S6M, ORX_10G_245S8M, ORX_10G_310S9M, ORX_10G_71S7M, SC6700
  - Point Grey GS3_U3_51S5M, GS3_U3_89S6M_C
* ADGenICam
  - Fix problem where startCapture() could be called when the camera was already acquiring.
    This caused resource leaks in ADAravis, and perhaps other drivers.
* Improved Python scripts to process XML files.  Added support for IntReg and String features.
  Thanks to Xiaoqiang Wang for this.
* Changed documentation link on Github pages.

R1-8 (9-June-2021)
-------------------
* Improvements to allow the asyn parameter datatype and the GenICam feature datatype to be different. 
  Specifically, one can be an integer and the other a double.
  - This is needed for the areaDetector Gain feature (double) on GenICam cameras
    that do not implement the GenICam Gain feature (also a double), but rather only implement
    the GainRaw or GainRawChannelA features (both of which are integers).
  - Some older Allied Vision Technologies/Prosilica cameras such as the GC1380H require this.
  - This change was done in a general way, not treating Gain as a special case, so it could
    be useful for other GenICam features in the future.
  - The flag std=c++11 was added when compiling on Linux.  This needed for the new version of ADGenICam.cpp
    which uses extended initializer lists.

R1-7-1 (26-May-2021)
-------------------
* Fixed a memory leak due to calling epicsStrDup() in GenICamFeature.cpp and never freeing that memory.
  The epicsStrDup() call was not needed and has been removed.
* Made the treatment of enum features internally consistent.
  If the feature is not implemented or not available or not readable then there is a single enum state, "N.A.".
  Previously it used isReadable() when reading enums in ADGenICam.cpp, 
  but isWritable() when doing callbacks in GenICamFeature.cpp.
* Improved the logic to eliminate unneeded callbacks when it has been set to N.A.

R1-7 (15-November-2020)
-------------------
* Added bool leftShift argument to decompressMono12Packed() and decompressMono12p().
  In R1-6 these functions always left shifted the output by 4 bits, and bits 0-3 were 0.
  This 4-bit left-shift operation is now optional.
* Added 3 new FLIR cameras.

R1-6 (2-October-2020)
-------------------
* Added new methods ADGenICam::decompressMono12Packed and ADGenICam::decompressMono12p.
  These can be called from drivers to convert images from Mono12Packed or Mono12p
  to UInt16.  These are now used by ADAravis.
  ADSpinnaker and ADVimba currently use methods in the vendor SDKs for this function,
  but they could also be changed to use these methods.
* Changed the `GenICamFeature::report()` function to print additional information:
  - For Integer features it now prints the minimum, maximum, and increment.
  - For Double features it now prints the minimum and maximum.
  
  These values are useful in determining the allowed range and step size for features.
  This information can be printed using the iocsh command:
```
asynReport 2 [driverName]
```
  The output is quite lengthy, so it can be useful to send it to a file like this:
```
asynReport 2 [driverName] > myFeatures.txt
```

R1-5 (20-September-2020)
-------------------
* Changed the code and the Makefiles to avoid using shareLib.h from EPICS base.
  Added new header file ADGenICamAPI.h that is used to control whether
  functions, classes, and variables are defined internally to the library or externally. 
  This is the mechanism now used in EPICS base 7.
  It makes it much easier to avoid mistakes in the order of include files that cause external 
  functions to be accidentally exported in the DLL or shareable library. 
  This should work on all versions of base, and have no impact on user code.
* Added fix for cameras that don't support GenICam feature AcquisitionMode=MultiFrame.
* Added 0.1 second delay between the pausing of acquisition and setting camera parameter, and
  0.1 second delay after setting a parameter and resuming acquisition.
* Added support for new cameras:
  - AVT_Mako_G158C.xml
  - Basler_piA640_210gm.xml
  - FLIR_BFS_70S7M.xml

R1-4 (9-April-2020)
-------------------
* Added logic to pause and resume acquisition when any of the following parameters are changed:
  - ADMinX, ADMinY 
  - ADSizeX, ADSizeY
  - ADBinX, ADBinY
  - ADNumImages
  - GCPixelFormat

  Previously changing these parameters while acquiring had no effect.
* Don't call setImageParams when ADImageMode, ADNumImages, or NDDataType change.
  This is not necessary and writes to the binning features which can have undesired side effects.
* Added xml, template, and OPI files for a number of additional Allied Vision Technologies/Prosilica 
  and Basler cameras.

R1-3 (24-February-2020)
------------------------
* Fix an error when creating the enum choices for GenICam features that are not writable.
  This cause the readback value to be invalid because it was not one of the allowed enum values.
  This generated errors in medm (and probably other OPIs) when opening screens containing these records.
* Added xml, template, and OPI files for a number of additional Allied Vision Technologies/Prosilica 
  and FLIR/Point Grey cameras.
* Fixed addCamera.sh to specify that Python 3 should be used.
* Added autoconverted .bob files for Phoebus Display Manager.

R1-2 (5-January-2020)
------------------------
* Change GenICam integer feature support from 32-bit integers to 64-bit integers, which is what GenICam specifies.  
  This requires asyn R4-38 which adds asynInt64 support for the ai, ao, longin, and longout records.
  With EPICS base 3.16.1 and later (including EPICS 7) int64in and int64 out records can be used for these features.  
  On older versions of base ai and ao records must be used, which limits exact representation of the features to 52 bits.
* Changed makeDb.py to use int64in and int64out records for GenICam integer features if the --devInt64 option flag is used.
  If this option flag is not used then ai and ao records are used for these features.
  The --devInt64 flag is recommended if running on EPICS base 3.16.1 or later, including EPICS 7.
* Changed makeDb.py and makeAdl.py to use Python 3, rather than Python 2.
* Added addCamera.sh which is a simple script that runs both makeDb.py and makeAdl.py.
  It is run from the top-level ADGenICam directory and is passed the name of the camera,
  i.e. the name of the XML file without the path and without the .xml extension.
  It can be edited to enable or disable the --devInt64 flag for makeDb.py.
* Added updateCameras.sh which runs addCamera.sh for each camera in the xml directory. 
  It builds the databases and medm screens for all of the cameras.
* Fixed problems with some EPICS PVs not getting their initial values from the camera correctly.
  This showed up the first time the IOC was run if autosave was being used, or every time the
  IOC was run if autosave was not being used.
* Added a number of new AVT cameras.

R1-1 (20-October-2019)
----------------------
* Changed ADSerialNumber to use either DeviceSerialNumber or DeviceID feature.  
  AVT uses DeviceID for at least some of its cameras.
* Changed makeDb.py to remove the autosaveFields info nodes because they were not useful.
* Rebuilt all the template files using the new version of makeDb.py so they no longer have the autosaveFields tags.
* Added new camera xml, template and OPI files for AVT_GC1380CH and PGR_PGE_23S6C cameras.

R1-0 (12-August-2019)
----
* Initial release

