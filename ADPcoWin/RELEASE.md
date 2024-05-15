PCO releases
============

This driver is hosted at https://github.com/areaDetector/ADPcoWin.

Releases between 4-1 and 6-0 were only performed on a separate branch
from master (specifically created for Diamond Light Source).


Release notes
=============

6-0 (July 26, 2021)
---
* First new tag on master incorporating changes from 5.X releases.

5-6
---
* Fix timestamp calculation.
* Add call to updateTimeStamp required for ADCore >2.0.
* Update Readme.

5-5
---
* Fix compiler warnings.
* Dependency bump.
* Tidy-up DLS example IOC.

5-4
---
* Add acquire event so acquire readback is high after camera starts acquisition.

5-3
---
* Dependency updates.

5-2
---
* Now increment arrayCounter before setting uniqueID to be 1-indexed.
* Minor tweaks to OPI screens.

5-1
---
* Remove sscan dependency.
* Remove rsync buttons on EDM screens.

5-0
---
* Updated for EPICS R3.14.12.7 and ADCore 3-0.
* Add Windows debug build.

4-3
---
* Re-add DLL for CameraLink cameras.
* Fix example IOC.

4-2
---
* Rename module to ADPcoWin from pcocam2.

4-1
---
* Add Apache V2 License.

4-0-1
-----
* Add OPI screens.

4-0
---
* Add CLHS DLL for PCO.edge 5.5 CLHS.
* Add global reset shutter option to template for Edge cameras.
* Add support for new firmware version structure.

3-3-1
-----
* Archive CCD temperature.

3-3
---
* Disable building example IOC and documentation by default.

3-2
---
* Fix minor typos.

3-1
---
* Refinements to ROI and symmetry.
* Now supports setting ROI using percentage.

3-0-4
-----
* Updated to areaDetector 2: ADCore 2-3-dls3, requiring ADBinaries 2-2dls2.
* Added a standalone example IOC.
* Added Camera performance statistics PVs.
* Added PV for "confimred stop" command which calls back once stop is complete.
* Make only 16-bit mode available and remove image reversal.

2-3-3
-----
* Updated area detector to 1-9dls19.
* Removed 32 bit windows build.
* Made the options in the Pixel Rate widget in the EDM GUI update when the camera connects.
* Added wait for entry into recording state for an arm command.
* Fixed faulty frame number tracking when bit alignment is LSB.
* Additional status PVs added.
* Fix for a problem where memory fills up.
* Fixed issue when exiting from a hardware ROI or binning.
* Now drop garbage frames created by the PCO4000 when acquiring without arming.
* Added busy record to arm mode, so that when arming the Channel Access callback is not made until the arm is complete.
* Added connection state PV and 20 second startup delay.
* Fix some race conditions.
* Add ability to poll buffers.

1-8
---
* Spurious frames created by the PCO4000 when arm is pressed removed.
* Support for cooling set point for the cameras that have this feature.
* Added external trigger only mode.
* Camera reported min/max exposure times now enforced.
* Hardware binning and ROI faults fixed.  Working well with the PCO Edge. On the 4000 and Dimax, returning to full frame requires a camera reboot. Work still in progress.
* Added experimental reboot command that only works with the Edge.
* Removed automatic selection of the fastest pixel clock rate.
* Fixed fault with Edge in its fastest clock rate where the pixels 'wrapped round' causing a faulty image.
* Added camlink large gap mode
* Fixed frame number tracking that detects missing frames.

1-3
---
* Initial public version.
* Most development done with the PCO Edge camera.
* Tested with the PCO Dimax and PCO 4000 cameras, with the following known issues. With the Dimax, the ROI feature can lock the camera up, so don't use. Also with the Dimax, recovery from an acquisition time that is longer than the
maximum allowed currently requires the IOC to be restarted.
The ROI feature has not been tested with the PCO4000. 
