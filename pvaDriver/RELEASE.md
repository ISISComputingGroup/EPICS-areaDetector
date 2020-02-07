pvaDriver Releases
==================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/pvaDriver.

Tagged source code releases can be obtained at 
https://github.com/areaDetector/pvaDriver/releases.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R1-6 (February XXX, 2019)
====================
* Improve NTNDArray exception handling.


R1-5 (July 2, 2018)
====================
* Added support for new PVs in ADCore R3-3 in opi files (NumQueuedArrays, EmptyFreeList, etc.)
* Improved op/*/autoconvert/* files with better medm files and better converters.
* Minor change to work with ADCore R3-3.


R1-4 (May 24, 2018)
====================
* Changes to work with EPICS base 7.0, i.e. EPICS_PVA_MAJOR_VERSION >= 6.
* Removed unlocking around doCallbacksGenericPointer, not needed and can be unsafe.


R1-3 (July 5, 2017)
====================
* Change startup scripts in iocPvaDriver.  st.cmd now loads envPaths and st_base.cmd, so it can be
  run with no copying of files.  st.cmd.linux and st.cmd.windows can still be used for conveniently
  switching host architectures in the same directory by copying envPaths to envPaths.linux or envPaths.windows
  as appropriate.
* Changed layout of medm screen for ADCore R3-0.


R1-2 (May 9, 2017)
====================
* Reject invalid PV name when creating monitor.


R1-1 (February 22, 2017)
====================
* Added new parameters ADSerialNumber, ADFirmwareVersion, ADSDKVersion, NDDriverVersion. 
  Minor change to driver and medm screen.


R1-0 (November 22, 2016)
========================
* Initial release as a separate repository.  Prior to R1-0 pvaDriver was contained in the ADCore
repository (R2-5 of ADCore only).

