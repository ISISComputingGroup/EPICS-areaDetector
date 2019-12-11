ADmarCCD Releases
==================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADmarCCD.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
https://cars.uchicago.edu/software/epics/areaDetector.html.

Tagged source code releases from R2-0 onward can be obtained at 
https://github.com/areaDetector/ADmarCCD/releases.

Tagged prebuilt binaries from R2-0 onward can be obtained at
https://cars.uchicago.edu/software/pub/ADmarCCD.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R2-3 (2-July-2018)
----
* Added support for new PVs in ADCore R3-3 in opi files (NumQueuedArrays, etc.)
* Added ADBuffers.adl to main medm screen.
* Changed configure/RELEASE files for compatibility with areaDetector R3-3.
* Improved op/*/autoconvert/* files with better medm files and better converters.
* Added NDDriverVersion information.


R2-2 (21-February-2017)
----
* Updated medm screen for larger version of ADSetup in ADCore R2-6


R2-1 (18-April-2015)
----
* Fixed problems stopping acquisition in normal and double-correlation modes. 
* Changes for compatibility with ADCore R2-2.


R2-0 (20-March-2014)
----
* Moved the repository to [Github](https://github.com/areaDetector/ADmarCCD).
* Re-organized the directory structure to separate the driver library from the example IOC application.
* Added support for the features available on the new high-speed (-HS series) 
  detectors from Rayonix using Version 2 of their server protocol.  These features include
  gating, high-speed series acquisition with either external trigger or internal clock, and support
  for different readout modes.
  Added the following new records:
    - GateMode
    - ReadoutMode
    - ServerMode_RBV
    - ReadoutMode
    - SeriesFileTemplate
    - SeriesFileDigits
    - SeriesFileFirst
    - MarSeriesStatus_RBV


R1-9-1 and earlier
------------------
Release notes are part of the
[areaDetector Release Notes](https://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
