ADCore Releases
===============

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADCore.

Tagged source code and pre-built binary releases prior to R2-0 are included
in the areaDetector releases available via links at
http://cars.uchicago.edu/software/epics/areaDetector.html.

Tagged source code releases from R2-0 onward can be obtained at 
https://github.com/areaDetector/ADCore/releases.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============

R3-2 (November XXX, 2017)
======================
### NDPluginStats
* Previously the X axis for the histogram plot was just the histogram bin number.
  Added code to compute an array of intensity values and a new HistHistogramX_RBV waveform record which
  contains the intensity values for the X axis of the histogram plot.
  This uses a new NDPlotXY.adl medm screen which accepts both X and Y waveform records to plot.
### NDFileHDF5
* Added support for blosc compression library.  The compressors include blosclz, lz4, lz4hc, snappy, zlib, and zstd.
  There is also support for ByteSuffle and BitShuffle.
  ADSupport now contains the blosc library, so it is available for most architectures.  
  The build flags WITH_BLOSC, BLOSC_EXTERNAL, and BLOSC_LIB have been added, similar to other optional libraries.
  Thanks to Xiaoqiang Wang for this addition.
* Changed all output records in NDFileHDF.template to have PINI=YES.  This is how other plugins all work.
### NDPluginOverlay
* Improved the behavior when changing the size of an overlay. Previously the Position was always preserved when 
  the Size was changed. This was not the desired behavior when the user had set the Center rather than Position.
  Now the code remembers whether Position or Center was last set, and preserves the appropriate value when 
  the Size is changed.
* Overlays were previously constrained to fit in image on X=0 and Y=0 edges.  However, the user may want part of 
  the overlay outside the image area. The location of the overlay can now be set anywhere, including negative positions.
  Each pixel in the overlay is now only added if it is within the image area.
* Fixed problems with incorrect drawing and crashing if part of an overlay was outside image area.
* Removed rounding when setting center from position or position from center.
  This was causing the location to change when setting the same center or position.
* Changed cross overlap so that it is drawn symmetrically with the same number of pixels on each side of center.
  This means the actual size is 2*Size/2 + 1, which will be Size+1 if Size is even.
### NDPluginDriver
* Force queueSize to be >=1 when creating queues in createCallbackThreads.  Was crashing when autosave value was 0.
### NDScatter.template
* Removed SCAN=I/O Intr for an output record which was a mistake and could cause crashes.
### pluginTests/Makefile
* Fixed errors with extra parentheses that were preventing include USR_INCLUDES directories from being added.

R3-1 (July 3, 2017)
======================
### GraphicsMagick
* Changes to commonDriverMakefile and commonLibraryMakefile so they work GraphicsMagick both from
  its recent addition to ADSupport R1-3 (GRAPHICSMAGICK_EXTERNAL=NO) and as a
  system library (GRAPHICSMAGIC_EXTERNAL=YES).
* Added support for 32-bit images in NDFileMagick.
* Improved the documentation for NDFileMagick.
### pluginSrc/Makefile, pluginTests/Makefile
* Fixed some problems with XXX_INCLUDE definitions (XXX=HDF5, XML2, SZIP, GRAPHICSMAGICK, BOOST).
### NDPluginDriver
* Fixed limitation where the ArrayPort string was limited to 20 characters.  There is now no limit.
* Fixed problem with the value of the QueueFree record at startup and when the queue size was changed.
  The queue size logic was OK but the displayed value of QueueFree was wrong.
### ADTop.adl
* Add PhotonII detector.
### NDPluginAttrPlot
* Added documentation.
### NDPluginPva
* Added performance measurements to documentation.
### 
### asynNDArrayDriver.h
* Include ADCoreVersion.h from this file so drivers don't need to explicitly include it.


R3-0 (May 5, 2017)
======================
### Requirements
* This release requires EPICS base 3.14.12.4 or higher because it uses the CFG rules which were fixed
  in that release.
### Incompatible changes
* This release is R3-0 rather than R2-7 because a few changes break backwards compatibility.
  * The constructors for asynNDArray driver and NDPluginDriver no longer take a numParams argument.
    This takes advantage of the fact that asynPortDriver no longer requires parameter counting as of R4-31.
    The constructor for ADDriver has not been similarly changed because this would require changing all drivers,
    and it was decided to wait until future changes require changing drivers before doing this.
  * The constructor for NDPluginDriver now takes an additional maxThreads argument.
  * NDPluginDriver::processCallbacks() has been renamed to NDPluginDriver::beginProcessCallbacks().
  * These changes will require minor modifications to any user-written plugins and any drivers that are derived directly
    from asynNDArrayDriver.  
  * All of the plugins in ADCore, and other plugins in the areaDetector project
    (ADPluginEdge, ffmpegServer, FastCCP, ADPCO, ADnED) have had these changes made.
  * The constructors and iocsh configuration commands for NDPluginStdArrays and NDPluginPva have been changed 
    to add the standard maxBuffers argument.  EXAMPLE_commonPlugins.cmd has had these changes made.
    Local startup scripts may need modifications.
### Multiple threads in single plugins (NDPluginDriver, NDPluginBase.template, NDPluginBase.adl, many plugins)
* Added support for multiple threads running the processCallbacks() function in a single plugin.  This can improve
  the performance of the plugin by a large factor.  Linear scaling with up to 5 threads (the largest
  value tested) was observed for most of the plugins that now support multiple threads.
  The maximum number of threads that can be used for the plugin is set in the constructor and thus in the 
  IOC startup script.  The actual number of threads to use can be controlled via an EPICS PV at run time, 
  up to the maximum value passed to the constructor.
  Note that plugins need to be modified to be thread-safe for multiple threads running in a single plugin object.
  The following table describes the support for multiple threads in current plugins.
  
| Plugin               | Supports multiple threads | Comments                                                      |
| ------               | ------------------------- | --------                                                      |
| NDPluginFile         | No                        | File plugins are nearly always limited by the file I/O, not CPU |
| NDPluginAttribute    | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginCircularBuff | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginColorConvert | Yes                       | Multiple threads supported and tested |
| NDPluginFFT          | Yes                       | Multiple threads supported and tested |
| NDPluginGather       | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginOverlay      | Yes                       | Multiple threads supported and tested |
| NDPluginProcess      | No                        | The recursive filter stores results in the object itself, hard to make thread safe |
| NDPluginPva          | No                        | Plugin is very fast, probably not much gain from multiple threads |
| NDPluginROI          | Yes                       | Multiple threads supported and tested |
| NDPluginROIStat      | Yes                       | Multiple threads supported and tested. Note: the time series arrays may be out of order if using multiple threads |
| NDPluginScatter      | No                        | Plugin does not do any computation, no gain from multiple threads |
| NDPluginStats        | Yes                       | Multiple threads supported and tested. Note: the time series arrays may be out of order if using multiple threads |
| NDPluginStdArrays    | Yes                       | Multiple threads supported and tested. Note: the callbacks to the waveform records may be out of order if using multiple threads|
| NDPluginTimeSeries   | No                        | Plugin does not do much computation, no gain from multiple threads |
| NDPluginTransform    | Yes                       | Multiple threads supported and tested. |
| NDPosPlugin          | No                        | Plugin does not do any computation, no gain from multiple threads |

* Added a new endProcessCallbacks method so derived classes do not need to each implement the logic to call 
  downstream plugins.  This method supports optionally sorting the output callbacks by the NDArray::UniqueId
  value.  This is very useful when running multiple threads in the plugin, because these are likely to do
  their output callbacks in the wrong order.  The base class will sort the output NDArrays to be in the correct
  order when possible.  The sorting capability is also useful for the new NDPluginGather plugin, even though
  it only uses a single thread.
* Renamed NDPluginDriver::processCallbacks() to NDPluginDriver::beginProcessCallbacks(). This makes it clearer
  that this method is intended to be called at the beginning of processCallbacks() in the derived class.
  NDPluginDriver::processCallbacks() is now a pure virtual function, so it must be implemented in the derived
  class.
* Added new parameter NDPluginProcessPlugin and new bo record ProcessPlugin.  NDPluginDriver now stores
  the last NDArray it receives.  If the ProcessPlugin record is processed then the plugin will execute
  again with this last NDArray.  This allows modifying plugin behaviour and observing the results
  without requiring the underlying detector to collect another NDArray.  If the plugin is disabled then
  the NDArray is released and returned to the pool.
* Moved many of the less commonly used PVs from NDPluginBase.adl to new file NDPluginBaseFull.adl.  This reduces the screen
  size for most plugin screens, and hides the more obscure PVs from the casual user.  The More related display widget in
  NDPluginBase.adl can now load both the asynRecord.adl and the NDPluginBaseFull.adl screens.
* iocBoot/EXAMPLE_commonPlugins.cmd now accepts an optional MAX_THREADS environment variable.  This defines
  the maximum number of threads to use for plugins that can run in multiple threads.  The default is 5.

### NDPluginScatter
* New plugin NDPluginScatter is used to distribute (scatter) the processing of NDArrays to multiple downstream plugins.
  It allows multiple intances of a plugin to process NDArrays in parallel, utilizing multiple cores 
  to increase throughput. It is commonly used together with NDPluginGather, which gathers the outputs from 
  multiple plugins back into a single stream. 
  This plugin works differently from other plugins that do callbacks to downstream plugins.
  Other plugins pass each NDArray that they generate of all downstream plugins that have registered for callbacks.
  NDPluginScatter does not do this, rather it passes each NDArray to only one downstream plugin.
  The algorithm for chosing which plugin to pass the next NDArray to can be described as a modified round-robin.
  The first NDArray is passed to the first registered callback client, the second NDArray to the second client, etc. 
  After the last client the next NDArray goes to the first client, and so on. The modification to strict round-robin 
  is that if client N input queue is full then an attempt is made to send the NDArray to client N+1,
  and if this would fail to client N+2, etc. If no clients are able to accept the NDArray because their queues are 
  full then the last client that is tried (N-1) will drop the NDArray. Because the "last client" rotates according 
  to the round-robin schedule the load of dropped arrays will be uniform if all clients are executing at the same
  speed and if their queues are the same size.

### NDPluginGather
* New plugin NDPluginGather is used to gather NDArrays from multiple upstream plugins and merge them into a single stream. 
  When used together with NDPluginScatter it allows multiple intances of a plugin to process NDArrays
  in parallel, utilizing multiple cores to increase throughput.
  This plugin works differently from other plugins that receive callbacks from upstream plugins.
  Other plugins subscribe to NDArray callbacks from a single upstream plugin or driver. 
  NDPluginGather allows subscribing to callbacks from any number of upstream plugins. 
  It combines the NDArrays it receives into a single stream which it passes to all downstream plugins. 
  The EXAMPLE_commonPlugins.cmd and medm files in ADCore allow up to 8 upstream plugins, but this number can 
  easily be changed by editing the startup script and operator display file.

### NDPluginAttrPlot
 * New plugin that caches NDAttribute values for an acquisition and exposes values of the selected ones to the EPICS 
   layer periodically.  Written by Blaz Kranjc from Cosylab.  No documentation yet, but it should be coming soon.

### asynNDArrayDriver, NDFileNexus
* Changed XML file parsing from using TinyXml to using libxml2.  TinyXml was originally used because libxml2 was not
  available for vxWorks and Windows.  libxml2 was already used for NDFileHDF5 and NDPosPlugin, originally using pre-built
  libraries for Windows in ADBinaries.  ADSupport now provides libxml2, so it is available for all platforms, and
  there is no need to continue building and using TinyXml.  This change means that libxml2 is now required, and so
  the build option WITH_XML2 is no longer used.  XML2_EXTERNAL is still used, depending on whether the version
  in ADSupport or an external version of the library should be used.  The TinyXml source code has been removed from
  ADCore.
* Added support for macro substitution in the XML files used to define NDAttributes.  There is a new NDAttributesMacros
  waveform record that contains the macro substitution strings, for example "CAMERA=13SIM1:cam1:,ID=ID34:".
* Added a new NDAttributesStatus mbbi record that contains the status of reading the attributes XML file.
  It is used to indicate whether the file cannot be found, if there is an XML syntax error, or if there is a
  macro substitutions error.

### PVAttribute
* Fixed a race condition that could result in PVAttributes not being connected to the channel.  This was most likely
  to occur for local PVs in the areaDetector IOC where the connection callback would happen immediately, before the
  code had been initialized to handle the callback. The race condition was introduced in R2-6.
  
### NDFileHDF5
* Fixed a problem with PVAttributes that were not connected to a PV.  Previously this generated errors from the HDF5
  library because an invalid datatype of -1 was used.  Now the data type for such disconnected attributes is set to
  H5T_NATIVE_FLOAT and the fill value is set to NAN.  No data is written from such attributes to the file, so the
  fill value is used.

### Plugin internals
* All plugins were modified to no longer count the number of parameters that they define, taking advantage of
  this feature that was added to asynPortDriver in asyn R4-31.
* All plugins were modified to call NDPluginDriver::beginProcessCallbacks() rather than 
  NDPluginDriver::processCalbacks() at the beginning of processCallbacks() as described above.
* Most plugins were modified to call NDPluginDriver::endProcessCallbacks() near the end of processCallbacks().
  This takes care of doing the NDArray callbacks to downstream plugins, and sorting the output NDArrays if required.
  It also handles the logic of caching the last NDArray in this->pArrays[0].  This significantly simplifies the code
  in the derived plugin classes.
* Previously all plugins were releasing the asynPortDriver lock when calling doCallbacksGenericPointer().
  This was based on a very old observation of a deadlock problem if the the lock was not released.  Releasing
  the lock causes serious problems with plugins running multiple threads, and probably was never needed.  Most
  plugins no longer call doCallbacksGenericPointer() directly because it is now done in NDPluginDriver::endProcessCallbacks().
  The lock is no longer released when calling doCallbacksGenericPointer().  The simDetector driver has also been modified
  to no longer release the lock when calling plugins with doCallbacksGenericPointer(), and all other drivers should be
  modified as well.  It is not really a problem with drivers however, since the code doing those callbacks is normally
  only running in a single thread.

### commonLibraryMakefile, commonDriverMakefile
* These files are now installed in the top-level $(ADCORE)/cfg directory.  External software that uses these files
  (e.g. plugins and drivers not in ADCore) should be changed to use this location rather than $(ADCORE)/ADApp/ since
  the location of the files in the source tree could change in the future.

### NDOverlayN.template
* Removed PINI=YES from CenterX and CenterY records.  Only PositionX/Y should have PINI=YES, otherwise
  the behavior depends on the order of execution with SizeX/Y.

### Viewers
* The ADCore/Viewers directory containing the ImageJ and IDL viewers has been moved to its own 
[ADViewers repository](https://github.com/areaDetector/ADViewers).
* It now contains a new ImageJ EPICS_NTNDA_Viewer plugin written by Tim Madden and Marty Kraimer.  
  It is essentially identical to EPICS_AD_Viewer.java except that it displays NTNDArrays from the NDPluginPva plugin, 
  i.e. using pvAccess to transport the images rather than NDPluginStdArrays which uses Channel Access.
* EPICS_AD_Viewer.java has been changed to work with the new ProcessPlugin feature in NDPluginDriver by monitoring
  ArrayCounter rather than UniqueId.
  


R2-6 (February 19, 2017)
========================

### NDPluginDriver, NDPluginBase.template, NDPluginBase.adl
* If blockCallbacks is non-zero in constructor then it no longer creates a processing thread.
  This saves resources if the plugin will only be used in blocking mode.  If the plugin is changed
  to non-blocking mode at runtime then the thread will be created then.
* Added new parameter NDPluginExecutionTime and new ai record ExecutionTime_RBV.  This gives the execution
  time in ms the last time the plugin ran.  It works both with BlockingCallbacks=Yes and No.  It is very
  convenient for measuring the performance of the plugin without having to run the detector at high
  frame rates.

### NDArrayBase.template
* Added new longout record NDimensions and new waveform record Dimensions to control the NDArray
  dimensions.  These were needed for NDDriverStdArrays, and may be useful for other drivers.
  Previously there were only input records (NDimensions_RBV and Dimensions_RBV) 
  for these parameters.

### NDPluginSupport.dbd
* Build this file in Makefile, remove from source so it is easier to maintain correctly.

### NDPluginROI
* Added CollapseDims to optionally collapse (remove) output array dimensions whose value is 1.
  For example an output array that would normally have dimensions [1, 256, 256] would be
  [256, 256] if CollapseDims=Enable.
  
### pluginTests
* Added ROIPluginWrapper.cpp to test the CollapseDims behavior in NDPluginROI.

### NDPluginTransform
* Set the NDArraySize[X,Y,Z] parameters appropriately after the transformation.  This is also done
  by the ROI plugin, and is convenient for clients to see the sizes, since the transform can
  swap the X and Y dimensions. 

### NDPluginOverlay
* Added new Ellipse shape to draw elliptical or circular overlays.
* Improved efficiency by only computing the coordinates of the overlay pixels when the overlay
  definition changes or the image format changes.  The pixel coordinates are saved in a list.
  This is particularly important for the new Ellipse shape because it uses trigonometric functions 
  to compute the pixel coordinates. When neither the overlay definition or the image format changes 
  it now just sets the pixel values for each pixel in the list.
* Added CenterX and CenterY parameter for each overlay.  One can now specify the overlay location
  either by PositionX and PositionY, which defines the position of the upper left corner of the
  overlay, or by CenterX and CenterY, which define the location of the center of the overlay.
  If CenterX/Y is changed then PositionX/Y will automatically update, and vice-versa.
* Changed the meaning of SizeX and SizeY for the Cross overlay shape.  Previously the total size
  of a Cross overlay was SizeX\*2 and SizeY\*2.  It is now SizeX and SizeY.  This makes it consistent
  with the Rectangle and Overlay shapes, i.e. drawing each of these shapes with the same PositionX
  and SizeX/Y will result in shapes that overlap in the expected manner.
* Slightly changed the meaning of SizeX/Y for the Cross and Rectangle shapes.  Previously the total
  size of the overlay was SizeX and SizeY.  Now it is SizeX+1 and SizeY+1, i.e. the overlay extends
  +-SizeX/2 and +-SizeY/2 pixels from the center pixel.  This preserves symmetry when WidthX/Y is 1,
  and the previous behavior is difficult to duplicate for the Ellipse shape.

### NDPluginStats
* Extensions to compute Centroid
  * Added calculations of 3rd and 4th order image moments, this provides skewness and kurtosis.
  * Added eccentricity and orientation calculations.
* Changed Histogram.
  * Previously the documentation stated that all values less than or equal to HistMin will 
    be in the first bin of the histogram, and all values greater than or equal to histMax will 
    be in last bin of the histogram.  This was never actually implemented; values outside the range
    HistMin:HistMax were not included in the histogram at all.
  * Rather than change the code to be consistent with the documentation two new records were added,
    HistBelow and HistAbove.  HistBelow contains the number of values less than HistMin,
    while HistAbove contains the number of values greater than HistMax.  This was done
    because adding a large number of values to the first and last bins of the histogram would change
    the entropy calculation, and also make histogram plots hard to scale nicely.

### NDPluginPos
* Added NDPos.adl medm file.
* Removed NDPosPlugin.dbd from NDPluginSupport.dbd because it can only be built if WITH_XML2 is set.

### NDPluginPva
* The pvaServer is no longer started in the plugin code, because that would result in running multiple servers
  if multiple NDPluginPva plugins were loaded.  Now the command "startPVAServer" must be added to the IOC startup
  script if any NDPluginPva plugins are being used.

### NDPluginFile
* If the NDArray contains an attribute named FilePluginClose and the attribute value is non-zero then
  the current file will be closed.

### NDFileTIFF
* If there is an NDAttribute of type NDAttrString named TIFFImageDescription then this attribute is written 
  to the TIFFTAG_IMAGEDESCRIPTION tag in the TIFF file.  Note that it will also be written to a user
  tag in the TIFF file, as with all other NDAttributes.  This is OK because some data processing code
  may expect to find the information in one location or the other.
* Added documentation on how the plugin writes NDAttributes to the TIFF file.

### NDAttribute
* Removed the line `#define MAX_ATTRIBUTE_STRING_SIZE 256` from NDAttribute.h because it creates the
  false impression that there is a limit on the size of string attributes.  There is not.
  Some drivers and plugins may need to limit the size, but they should do this with local definitions.
  The following files were changed to use local definitions, with these symbolic names and values:

  | File                           | Symbolic name             | Value |
  | ------------------------------ | ------------------------- | ----- |
  | NDFileHDF5AttributeDataset.cpp | MAX_ATTRIBUTE_STRING_SIZE | 256   |
  | NDFileNetCDF.cpp               | MAX_ATTRIBUTE_STRING_SIZE | 256   |
  | NDFileTIFF.cpp                 | STRING_BUFFER_SIZE        | 2048  |

### paramAttribute
* Changed to read string parameters using the asynPortDriver::getStringParam(int index, std::string&amp;) 
  method that was added in asyn R4-31.  This removes the requirement that paramAttribute specify a maximum
  string parameters size, there is now no limit.
  
### PVAttribute
* Fixed problem where a PV that disconnected and reconnected would cause multiple CA subscriptions.
  Note that if the data type of the PV changes on reconnect that the data may not be correct because
  the data type of a PVAttribute is not allowed to change.  The application must be restarted in this
  case.
  
### asynNDArrayDriver, NDArrayBase.template, NDPluginBase.adl, ADSetup.adl, all plugin adl files
* Added 2 new parameters: NDADCoreVersion, NDDriverVersion and new stringin records ADCoreVersion_RBV and
  DriverVersion_RBV.  These show the version of ADCore and of the driver or plugin that the IOC was
  built with.  Because NDPluginBase.adl grew larger all of the other plugin adl files have changed
  their layouts.

### ADDriver, ADBase.template, ADSetup.adl
* Added 3 new parameters: ADSerialNumber, ADFirmwareVersion, and ADSDKVersion and new stringin records 
  SerialNumber_RBV, FirmwareVersion_RBV, and SDKVersion_RBV. These show the serial number and firmware
  version of the detector, and the version of the vendor SDK library that the IOC was built with.
  Because ADSetup.adl grew larger all driver adl files need to change their layouts.  This has been done
  for ADBase.adl in ADCore.  New releases of driver modules will have the changed layouts.

### pvaDriver
* Moved the driver into its own repository areaDetector/pvaDriver.  The new repository contains
  both the driver library from ADCore and the example IOC that was previously in ADExample.
  
### NDArray.cpp
* Print the reference count in the report() method.

### iocBoot/EXAMPLE_commonPlugins.cmd
* Add commented out line to call startPVAServer if the EPICS V4 NDPva plugin is loaded.
  Previously the plugin itself called startPVAServer, but this can result in the function 
  being called multiple times, which is not allowed.

### Viewers/ImageJ
* Improvements to EPICS_AD_Viewer.java 
  * Automatically set the contrast when a new window is created. This eliminates the need to 
    manually set the contrast when changing image size, data type, and color mode in many cases.
  * When the image window is automatically closed and reopened because the size, data type, 
    or color mode changes the new window is now positioned in the same location as the window 
    that was closed.
* New ImageJ plugin called GaussianProfiler.java.  It was written by Noumane Laanait when he
  was at the APS (currently at ORNL).  This is similar to the standard ImageJ Plot Profile tool in Live
  mode, but it also fits a Gaussian peak to the profile, and prints the fit parameters centroid, FWHM,
  amplitude, and background.  It is very useful for focusing x-ray beams, etc.
* New ImageJ plugin called EPICS_AD_Controller.java.  This plugin allows using the ImageJ ROI tools
  (rectangle and oval) to graphically define the following:
  * The readout region of the detector/camera
  * The position and size of an ROI (NDPluginROI)
  * The position and size of an overlay (NDPluginOverlay)

  The plugin chain can include an NDPluginTransform plugin which changes the image orientation and an
  NDPluginROI plugin that changes the binning, size, and X/Y axes directions.  The plugin corrects
  for these transformations when defining the target object.  Chris Roehrig from the APS wrote an
  earlier version of this plugin.

### Dependencies
* This release requires asyn R4-31 or later because it uses new features in asynPortDriver.


R2-5 (October 28, 2016)
========================
### ADSupport
* Added a new repository ADSupport to areaDetector.  This module contains the source code for all 3rd party
  libraries used by ADCore.  The libraries that were previously built in ADCore have been moved
  to this new repository.  These are ADCore/ADApp/netCDFSrc, nexusSrc, and tiffSupport/tiffSrc, and
  tiffSupport/jpegSrc.  ADSupport also replaces the ADBinaries repository.  ADBinaries contained prebuilt
  libraries for Windows for xml2, GraphicsMagick, and HDF5.  It was becoming too difficult to maintain
  these prebuilt libraries to work with different versions of Visual Studio, and also with 32/64 bit, 
  static dynamic, debug/release, and to work with MinGW.  These libraries are now built from source code
  using the EPICS build system in ADSupport.
* The libraries in ADSupport can be built for Windows (Visual Studio or MinGW, 32/64 bit, static/dynamic), 
  Linux (currently Intel architectures only, 32/64 bit), Darwin, and vxWorks 
  (currently big-endian 32-bit architectures only).  
* Previously the only file saving plugin that was supported on vxWorks was netCDF.  Now all file saving 
  plugins are supported on vxWorks 6.x (TIFF, JPEG, netCDF, HDF5, Nexus).  HDF5 and Nexus are not supported
  on vxWorks 5.x because the compiler is too old.
* All 3rd party libraries are now optional.  For each library XXX there are now 4 Makefile variables that control
  support for that library.  XXX can be JPEG, TIFF, NEXUS, NETCDF, HDF5, XML2, SZIP, and ZLIB.
  - WITH_XXX   If this is YES then drivers or plugins that use this library will be built.  If NO then
    drivers and plugins that use this library will not be built.
  - XXX_EXTERNAL  If this is YES then the library is not built in ADSupport, but is rather assumed to be found
    external to the EPICS build system.  If this is NO then the XXX library will be built in ADSupport.
  - XXX_DIR  If this is defined and XXX_EXTERNAL=YES then the build system will search this directory for the 
    XXX library.
  - XXX_INCLUDE If this is defined then the build system will search this directory for the include files for
    the XXX library.
* ADSupport does not currently include support for GraphicsMagick.  This means that GraphicsMagick is not
  currently supported on Windows.  It can be used on Linux and Darwin if it is installed external to areaDetector
  and GRAPHICSMAGICK_EXTERNAL=YES.
* All EPICS modules except base and asyn are now optional.  Previously 
  commonDriverSupport.dbd included "calcSupport.dbd", "sscanSupport.dbd", etc.
  These dbd and libraries are now only included if they are defined in a RELEASE
  file.


### NDPluginPva
* New plugin for exporting NDArrays as EPICS V4 NTNDArrays.  It has an embedded EPICSv4 server to serve the NTNDArrays
  to V4 clients.
  When used with pvaDriver it provides a mechanism for distributing plugin processing across multiple  processes 
  and multiple machines. Thanks to Bruno Martins for this.

### pvaDriver
* New driver for importing an EPICS V4 NTNDArray into areaDetector.  It works by creating a monitor on the 
  specified PV and doing plugin callbacks each time the array changes.  
  When used with NDPluginPva it provides a mechanism for distributing plugin processing across multiple processes 
  and multiple machines.  Thanks to Bruno Martins for this.

### NDFileHDF5
* Added support for Single Writer Multiple Reader (SWMR).  This allows HDF5 files to be read while they are still be
  written.  This feature was added in HDF5 1.10.0-patch1, so this release or higher is required to use the 
  SWMR support in this plugin.
  The file plugin allows selecting whether SWMR support is enabled, and it is disabled by default.
  Files written with SWMR support enabled can only be read by programs built with HDF 1.10 or higher, so SWMR should not
  be enabled if older clients are to read the files.  SWMR is only supported on local, GPFS, and Lustre file systems. 
  It is not supported on NFS or SMB file systems, for example.  Thanks to Alan Greer for this.
  * NOTE: we discovered shortly before releasing ADSupport R1-0 and ADCore R2-5 that the
    Single Writer Multiple Reader (SWMR) support in HDF5 1.10.0-patch1 was broken.
    It can return errors if any of the datasets are of type H5_C_S1 (fixed length strings).
    We were able to reproduce the errors with a simple C program, and sent that to the HDF Group.
    They quickly produced a new unreleased version of HDF5 called 1.10-swmr-fixes that fixed the problem.

    The HDF5 Group plans to release 1.10.1, hopefully before the end of 2016.  That should be
    the first official release that will correctly support SWMR.

    As of the R1-0 release ADSupport contains 2 branches. 
    - master contains the HDF5 1.10.0-patch1 release from the HDF5 Group with only the minor changes
      required to build with the EPICS build system, and to work on vxWorks and mingw.
      These changes are documented in README.epics.  This version should not be used with SWMR
      support enabled because of the known problems described above.
    - swmr-fixes contains the 1.10-swmr-fixes code that the HDF Group provided.
      We had to make some changes to this code to get it to work on Windows.
      It is not an official release, but does appear to correctly support SWMR.
      Users who would like to begin to use SWMR before HDF5 1.10.1 is released can use
      this branch, but must be aware that it is not officially supported. 

* NDAttributes of type NDAttrString are now saved as 1-D array of strings (HDF5 type H5T_C_S1) rather
  than a 2-D array of 8-bit integers (HDF5 type H5T_NATIVE_CHAR), which is the datatype used prior to R2-5.  
  H5T_NATIVE_CHAR is really intended to be an integer data type, and so most HDF5 utilities (h5dump, HDFView, etc.) display
  these attributes by default as an array of integer numbers rather than as a string.  

### NDPosPlugin
* New plugin attach positional information to NDArrays in the form of NDAttributes. This plugin accepts an XML
  description of the position data and then attaches each position to NDArrays as they are passed through the plugin.
  Each position can contain a set of index values and each index is attached as a separate NDAttribute.  
  The plugin operates using a FIFO and new positions can be added to the queue while the plugin is actively 
  attaching position data.  When used in conjunction with the HDF5
  writer it is possible to store NDArrays in an arbitrary pattern within a multi-dimensional dataset.

### NDPluginDriver
* Added the ability to change the QueueSize of a plugin at run-time. This can be very useful,
  particularly for file plugins where an acquisition of N frames is overflowing the queue,
  but increasing the queue can fix the problem. This will be even more useful in ADCore R3-0
  where we plan to eliminate Capture mode in NDPluginFile. Being able to increase the queue does
  everything that Capture mode did, but has the additional advantage that in Capture mode the
  NDArray memory is not allocated from the NDArrayPool, so there is no check on allocating too
  many arrays or too much memory. Using the queue means that arrays are allocated from the pool,
  so the limits on total number of arrays and total memory defined in the constructor will be obeyed.
  This is very important in preventing system freezes if the user accidentally tries allocate all the
  system memory, which can effectively crash the computer.
* Added a new start() method that must be called after the plugin object is created.  This requires
  a small change to all plugins.  The plugins in ADCore/ADApp/pluginSrc can be used as examples.  This
  change was required to prevent a race condition when the plugin only existed for a very short
  time, which happens in the unit tests in ADCore/ADApp/pluginsTests.

### NDPluginTimeSeries
* New plugin for time-series data.  The plugin accepts input arrays of dimensions
  [NumSignals] or [NumSignals, NewTimePoints].  The plugin creates NumSignals 1-D
  arrays of dimension [NumTimPoints], each of which is the time-series for one signal.
  On each callback the new time points are appended to the existing time series arrays.
  The plugin can operate in one of two modes.  In Fixed Length mode the time-series arrays
  are cleared when acquisition starts, and new time points are appended until 
  NumTimePoints points have been received, at which point acquisition stops and further
  callbacks are ignorred.  In Circular Buffer mode when NumTimePoints samples are received
  then acquisition continues with the new time points replacing the oldest ones in the
  circular buffer.  In this mode the exported NDArrays and waveforms always contain the latest 
  NumTimePoints samples, with the first element of the array containing the oldest time
  point and the last element containing the most recent time point.    
* This plugin is used by R7-0 and later of the 
  [quadEM module](https://github.com/epics-modules/quadEM).
  It should also be useful for devices like ADCs, transient digitizers, and other devices
  that produce time-series data on one or more input signals.  
* There is a new ADCSimDetector test application in 
  [areaDetector/ADExample](https://github.com/areaDetector/ADExample) 
  that tests and demonstrates this plugin.  This test application simulates a buffered 
  ADC with 8 input waveform signals.

### NDPluginFFT
* New plugin to compute 1-D or 2-D Fast Fourier transforms.  It exports 1-D or 2-D
  NDArrays containing the absolute value of the FFT.  It creates 1-D waveform
  records of the input, and the real, imaginary, and absolute values of the first row of the FFT.
  It also creates 1-D waveform records of the time and frequency axes, which are useful if the 1-D
  input represents a time-series. The waveform records are convenient for plotting in OPI screens. 
* The FFT algorithm requires that the input array dimensions be a power of 2, but the plugin
  will pad the array to the next larger power of 2 if the input array does not meet this
  requirement.  
* The simDetector test application in 
  [areaDetector/ADExample](https://github.com/areaDetector/ADExample) 
  has a new simulation mode that generates images based on the sums and/or products of 4 sine waves.
  This application is useful for testing and demonstrating the NDPluginFFT plugin.

### NDPluginStats and NDPluginROIStat
* Added waveform record containing NDArray timestamps to time series data arrays. Thanks to
  Stuart Wilkins for this.

### NDPluginCircularBuff
* Initialize the TriggerCalc string to "0" in the constructor to avoid error messages during iocInit
  if the string has not been set to a valid value that is stored with autosave.

### asynNDArrayDriver
* Fixed bug in FilePath handling on Windows. If the file path ended in "/" then it would incorrectly
  report that the directory did not exist when it did.
* Added asynGenericPointerMask to interrupt mask in constructor.  Should always have been there.
* Added asynDrvUserMask to interface mask in constructor.  Should always have been there.

### NDArrayBase.template, NDPluginDriver.cpp
* Set ArrayCallbacks.VAL to 1 so array callbacks are enabled by default.

### NDPluginBase.template
* Changed QueueSize from longin to longout, because the plugin queue size can now be changed at runtime.
  Added longin QueueSize_RBV.
* Changed EnableCallbacks.VAL to $(ENABLED=0), allowing enabling callbacks when loading database,
  but default remains Disable.
* Set the default value of the NDARRAY_ADDR macro to 0 so it does not need to be defined in most cases.
  
### ADApp/op/adl
* Fixed many medm adl files so text fields have correct string/decimal, width and aligmnent attributes to
  improve autoconversion to other display managers.

### nexusSrc/Makefile
* Fixed so it will work when hdf5 and sz libraries are system libraries.
  Used same logic as commonDriverMakefile

### iocBoot
* Deleted commonPlugins.cmd and commonPlugin_settings.req.  These were accidentally restored before the R2-4
  release after renaming them to EXAMPLE_commonPlugins.cmd and EXAMPLE_commonPlugin_settings.req.

### ImageJ EPICS_ADViewer
* Changed to work with 1-D arrays, i.e. nx>0, ny=0, nz=0.  Previously it did not work if ny=0.  This
  is a useful enhancement because the ImageJ Dynamic Profiler can then be used to plot the 1-D array.


R2-4 (September 21, 2015)
========================
### Removed simDetector and iocs directory. 
Previously the simDetector was part of ADCore, and there was an iocs directory that built the simDetector 
application both as part of an IOC and independent of an IOC. This had 2 disadvantages:

1. It prevented building the simDetector IOC with optional plugins that reside in separate
   repositories, such as ffmpegServer and ADPluginEdge.  This is because ADCore needs to
   be built before the optional plugins, but by then the simDetector IOC is already built
   and cannot use the optional plugins.
2. It made ADCore depend on the synApps modules required to build an IOC, not just the
   EPICS base and asyn that are required to build the base classes and plugins.
   It was desirable to minimize such dependencies in ADCore.
  
For these reasons the simDetector driver and IOC code have been moved to a new repository
called ADExample.  This repository is just like any other detector repository.
This solves problem 1 above because the optional plugins can now be built after ADCore
but before ADExample. 

### NDAttribute
* Fixed problem that the sourceType property was never set.

### NDRoiStat[.adl, .edl, ui, .opi]
* Fixed problem with ROI numbers when calling related displays.

### ADApp/pluginTests/
* New directory with unit tests.

### XML schema
* Moved the XML schema files from the iocBoot directory to a new XML_schema directory.

### iocBoot
* Moved commonPlugin_settings.req from ADApp/Db to iocBoot.  
  Renamed commonPlugins.cmd to EXAMPLE_commonPlugins.cmd and commonPlugin_settings.req to
  EXAMPLE_commonPlugin_settings.req.  These files must be copied to commonPlugins.cmd and
  commonPlugin_settings.req respectively.  This was done because these files are typically
  edited locally, and so should not be in git. 
  iocBoot now only contains EXAMPLE_commonPlugins.cmd and EXAMPLE_commonPlugin_settings.req.  
  EXAMPLE_commonPlugins.cmd adds ADCore/iocBoot to the autosave search path.
  
### ADApp
* commonLibraryMakefile has been changed to define xxx_DIR and set LIB_LIBS+ = xxx if xxx_LIB is defined.  
  If xxx_LIB is not defined then xxx_DIR is not defined and it sets LIB_SYS_LIBS += xxx.  
  xxx includes HDF5, SZIP, and OPENCV. 
  commonDriverMakefile has been changed similarly for PROD_LIBS and PROD_SYS_LIBS.
  This allows optional libraries to either searched in the system location or a user-defined location 
  without some conflicts that could previously occur.


R2-3 (July 23, 2015)
========================
### devIocStats and alive modules
* The example iocs in ADCore and other drivers can now optionally be built with the 
  devIocStats module, which provides very useful resource utilization information for the IOC.
  devIocStats runs on all supported architectures.
  The OPI screen can be loaded from Plugins/Other/devIocStats.
  It is enabled by default in areaDetector/configure/EXAMPLE_RELEASE_PRODS.local. 
* The synApps alive module can also be built into detector IOCs.  It provdes status information
  about the IOC to a central server.  It is disabled by default in 
  areaDetector/configure/EXAMPLE_RELEASE_PRODS.local.  areaDetector/INSTALL_GUIDE.md has been
  updated to describe what needs to be done to enable or disable these optional modules.

### NDPluginFile
* Fixed a serious performance problem.  The calls to openFile() and closeFile() in the derived file 
  writing classes were being done with the asynPortDriver mutex locked.
  This meant that driver callbacks were blocked from putting entries on the queue during this time, 
  slowing down the drivers and potentially causing them to drop frames.  
  Changed openFileBase() and closeFileBase() to unlock the mutex during these operations.  
  Testing with the simDetector and ADDexela shows that the new version no longer slows
  down the driver or drops frames at the driver level when saving TIFF files or netCDF files 
  in Single mode.  This required locking the mutex in the derived file writing classes when
  they access the parameter library.

### NDPluginROIStat
* Added time-series support for each of the statistics in each ROI.  This is the same as the
  time-series support in the NDPluginStats and NDPluginAttribute plugins.

### NDFileHDF5
* Bug fixes: 
  * When writing files in Single mode if NumCapture was 0 then the chunking was computed 
    incorrectly and the files could be much larger than they should be.
  * There was a problem with the HDF5 istorek parameter not being set correctly.

### commonDriverMakefile
* Include SNCSEQ libraries if SNCSEQ is defined.  This must be defined if the CALC module
  was built with SNCSEQ support.
* Optionally include DEVIOCSTATS and ALIVE libraries and dbd files if these are defined.


R2-2 (March 23, 2015)
========================
### Compatibility
* This release requires at least R4-26 of asyn because it uses the info(asyn:READOUT,"1") tag
  in databases to have output records update on driver callbacks.
* This release requires R2-2 of areaDetector/ADBinaries
* This release requires R2-2 of areaDetector/areaDetector because of changes to EXAMPLE_CONFIG_SITE.local.
* Detector IOC startup scripts will need a few minor changes to work with this release of ADCore.
  iocBoot/iocSimDetector/st.cmd should be used as an example.
  - The environment variable EPICS_DB_INCLUDE_PATH must be defined and must include $(ADCORE)/db
  - The environment variable CBUFFS must be defined to specify the number of frames buffered in the
    NDPluginCircularBuff plugin, which is loaded by commonPlugins.cmd.
  - When loading NDStdArrays.template NDARRAY_PORT must be specified.  NDPluginBase should no longer be loaded, 
    this is now done automatically via an include in NDStdArrays.template.
  - Example lines:
  ```
   epicsEnvSet("CBUFFS", "500")
   epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
   dbLoadRecords("NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,
                 TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int8,FTVL=UCHAR,NELEMENTS=3145728")
  ```
  
### NDPluginROIStat
* New plugin that supports multiple regions-of-interest with simple statistics on each.
  It is more efficient and convenient than the existing NDPluginROI and NDPluginStats when many 
  regions of interest with simple statistics are needed.  Written by Matthew Pearson.
  
### NDPluginCircularBuff
* New plugin that implements a circular buffer.  NDArrays are stored in the buffer until a trigger
  is received.  When a trigger is received it outputs a configurable number of pre-trigger and post-trigger
  NDArrays.  The trigger is based on NDArray attributes using a user-defined calculation.  Written by Edmund Warrick.
  
### NDPluginAttribute
* New plugin that exports attributes of NDArrays as EPICS PVs.  Both scalar (ai records) and time-series
  arrays (waveform records) are exported.  Written by Matthew Pearson.
  
### NDPluginStats
* Added capability to reset the statistics to 0 (or other user-defined values) with a new
  $(P)$(R)Reset sseq records in NDStats.template.  The reset behavior can be changed by 
  configuring these records.

### NDPluginROI
* Remember the requested ROI size and offset.  If the requested values cannot be satisfied due
  to constraints such as binning or the input array size, then use the requested values when the
  constraints no longer apply.

### NDPluginOverlay
* Bug fix: the vertical line in the cross overlay was not drawn correctly if WidthX was greater than 1.

### NDPluginFile
* Created the NDFileCreateDir parameter. This allows file writers to create a controlled number
  of directories in the path of the output file.
* Added the NDFileTempSuffix string parameter. When this string is not empty it will be
  used as a temporary suffix on the output filename while the file is being written. When writing
  is complete, the file is closed and then renamed to have the suffix removed. The rename operation 
  is atomic from a filesystem view and can be used by monitoring applications like rsync or inotify
  to kick off processing applications. 
  
### NDFileHDF5
* Created a separate NDFileHDF5.h file so the class can be exported to other applications.
* Updated the HDF5 library for Windows in ADBinaries from 1.8.7 to 1.8.14.  The names of the libraries has changed,
  so Makefiles in ADCore needed to be changed.  There is a new flag, HDF5_STATIC_BUILD which can be set to
  NO or YES depending on which version of the HDF5 library should be used.  This symbol is now defined in
  areaDetector/configure/EXAMPLE_CONFIG_SITE.local to be YES for static builds and NO for dynamic builds.
  In principle one can use the dynamic version of the HDF5 library when doing a static build, and vice-versa.
  In practice this has not been tested.
* Bug fixes: 
  * The NDArrayPort could not be changed after iocInit.
  * Hardlinks did not work for NDAttribute datasets.
  * Failing to specify a group with "ndattr_default=true" would crash the IOC.
  * NDAttribute datasets of datatype NDAttrString would actually be HDF5 attributes,
    not datasets.  They would only store a single string value, not an array of string values which they 
    should if the file contains multiple NDArrays.
  * NDAttribute datasets were pre-allocated to the size of NumCapture, rather than 
    growing with the number of NDArrays in the file.  This meant that there was no way to specify that the HDF5 file 
    in Stream mode should continue to grow forever until Capture was set to 0.  Specifying NumCapture=0 did not work, 
    it crashed the IOC.  Setting NumCapture to a very large number resulted in wasted file space, 
    because all datasets except the detector dataset were pre-allocated to this large size.
  * HDF5 attributes defined in the XML layout file for NDAttribute datasets and constant datasets
    did not get written to the HDF5 file.
  * The important NDArray properties uniqueID, timeStamp, and epicsTS did not get written to the HDF5 file.
  * NDAttribute datasets had two "automatic" HDF5 attributes, "description" and "name".  These do not provide
    a complete description of the source of the NDAttribute data, and the HDF5 attribute names are prone
    to name conflicts with user-defined attributes.  "description" and "source" have been renamed to 
    NDAttrDescription and NDAttrSource. Two additional automatic attributes have been added, 
    NDAttrSourceType and NDAttrName, which now completely define the source of the NDAttribute data.

### Version information
* Added a new include file, ADCoreVersion.h that defines the macros ADCORE_VERION, ADCORE_REVISION, and
  ADCORE_MODIFICATION.  The release number is ADCORE_VERSION.ADCORE_REVISION.ADCORE_MODIFICATION.
  This can be used by drivers and plugins to allow them to be used with different releases of ADCore.

### Plugins general
* Added epicsShareClass to class definitions so classes are exported with Windows DLLs.
* Install all plugin header files so the classes can be used from other applications.

### netCDFSupport
* Fixes to work on vxWorks 6.9 from Tim Mooney.

### NDAttribute class
* Changed the getValue() method to do type conversion if the requested datatype does not match the
  datatype of the attribute.

### Template files
* Added a new template file NDArrayBase.template which contains records for all of the 
  asynNDArrayDriver parameters except those in NDFile.template.  Moved records from ADBase.template
  and NDPluginBase.template into this new file.  Made all template files "include" the files from the
  parent class, rather than calling dbLoadRecords for each template file.  This simplifies commonPlugins.cmd.
  A similar include mechanism was applied to the *_settings.req files, which simplifies commonPlugin_settings.req.
* Added a new record, $(P)$(R)ADCoreVersion_RBV, that is loaded for all drivers and plugins. 
  This record contains the ADCore version number. This can be used by Channel Access clients to alter their
  behavior depending on the version of ADCore that was used to build this driver or plugin.
  The record contains the string ADCORE_VERSION.ADCORE_REVISION.ADCORE_MODIFICATION, i.e. 2.2.0 for this release.

* Added the info tag "autosaveFields" to allow automatic creation of autosave files.
* ADBase.template
  - Added optional macro parameter RATE_SMOOTH to smooth the calculated array rate.
    The default value is 0.0, which does no smoothing.  Range 0.0-1.0, larger values 
    result in more smoothing.
  
### ImageJ Viewer
* Bug fixes from Lewis Muir.

### simDetector driver
* Created separate simDetector.h file so class can be exported to other applications.
      
### iocs/simDetectorNoIOC
* New application that demonstrates how to instantiate a simDetector driver
  and a number of plugins in a standalone C++ application, without running an EPICS IOC.
  If asyn and ADCore are built with the EPICS_LIBCOM_ONLY flag then this application only
  needs the libCom library from EPICS base and the asyn library.  It does not need any other
  libraries from EPICS base or synApps.

### Makefiles
* Added new build variable $(XML2_INCLUDE), which replaces hardcoded /usr/include/libxml2 in
  several Makefiles.  $(XML2_INCLUDE) is normally defined in 
  $(AREA_DETECTOR)/configure/CONFIG_SITE.local, typically to be /usr/include/libxml2.
* Changes to support the EPICS_LIBCOM_ONLY flag to build applications that depend only
  on libCom and asyn.
    
    
R2-1 (October 17, 2014)
=======================
### NDPluginFile
* Added new optional feature "LazyOpen" which, when enabled and in "Stream" mode, will defer 
  file creation until the first frame arrives in the plugin. This removes the need to initialise
  the plugin with a dummy frame before starting capture.  

### NDFileHDF5
* Added support for defining the layout of the HDF5 file groups, dataset and attributes in an XML
  definition file. This was a collaboration between DLS and APS: Ulrik Pedersen, Alan Greer, 
  Nicholas Schwarz, and Arthur Glowacki. See project pages: 
  [AreaDetector HDF5 XML Layout](http://confluence.diamond.ac.uk/x/epF-AQ)
  [HDF5 Writer Plugin](https://confluence.aps.anl.gov/x/d4GG)

### NDFileTiff
* All NDArray attributes are now written as TIFF ASCII file tags, up to a maximum of 490 tags.
  Thanks to Matt Pearson for this.

### NDPluginOverlay
* Added support for text overlays. Thanks to Keith Brister for this.
* Added support for line width in rectangle and cross overlays.  Thanks to Matt Pearson for this.
* Fixed problem with DrawMode=XOR. This stopped working back in 2010 when color support was added.

### NDPluginTransform
* Complete rewrite to greatly improve simplicity and efficiency.  It now supports 8 transformations
  including the null transformation.  Performance improved by a factor of 13 to 85 depending
  on the transformation.  Thanks to Chris Roehrig for this.

### Miscellaneous
* Added a new table to the 
  [top-level documentation] (http://cars.uchicago.edu/software/epics/areaDetector.html).
  This contains for each module, links to:
  - Github repository
  - Documentation
  - Release Notes
  - Directory containing pre-built binary files
* Added support for cygwin32 architecture.  This did not work in R2-0.


R2-0 (April 4, 2014)
====================
* Moved the repository to [Github](https://github.com/areaDetector/ADCore).
* Re-organized the directory structure to separate the driver library from the example 
  simDetector IOC application.
* Moved the pre-built libraries for Windows to the new ADBinaries repository.
* Removed pre-built libraries for Linux.  Support libraries such as HDF5 and GraphicsMagick 
  must now be present on the build system computer.
* Added support for dynamic builds on win32-x86 and windows-x64. 

### NDArray and asynNDArrayDriver
* Split NDArray.h and NDArray.cpp into separate files for each class: 
  NDArray, NDAttribute, NDAttributeList, and NDArrayPool.
* Changed all report() methods to have a FILE *fp argument so output can go to a file. 
* Added a new field, epicsTS, to the NDArray class. The existing timeStamp field is a double,
  which is convenient because it can be easily displayed and interpreted.  However, it cannot
  preserve all of the information in an epicsTimeStamp, which this new field does.
  This is the definition of the new epicsTS field.
```
epicsTimeStamp epicsTS;  /**< The epicsTimeStamp; this is set with
                           * pasynManager->updateTimeStamp(), 
                           * and can come from a user-defined timestamp source. */
```
* Added 2 new asynInt32 parameters to the asynNDArrayDriver class.
    - NDEpicsTSSec contains NDArray.epicsTS.secPastEpoch
    - NDEpicsTSNsec contains NDArray.epicsTS.nsec

* Added 2 new records to NDPluginBase.template.
    - $(P)$(R)EpicsTSSec_RBV contains the current NDArray.epicsTS.secPastEpoch
    - $(P)$(R)EpicsTSNsec_RBV contains the current NDArray.epicsTS.nsec

* The changes in R2-0 for enhanced timestamp support are described in 
[areaDetectorTimeStampSupport](http://cars.uchicago.edu/software/epics/areaDetectorTimeStampSupport.html).

### NDAttribute
* Added new attribute type, NDAttrSourceFunct. 
  This type of attribute gets its value from a user-defined C++ function. 
  It can thus be used to get any type of metadata. Previously only EPICS PVs 
  and driver/plugin parameters were available as metadata. 
* Added a new example file, ADSrc/myAttributeFunctions.cpp, that demonstrates 
  how to write user-defined attribute functions. 
* Made all contents of attribute XML files be case-sensitive. It was announced in R1-7 that 
  datatype and dbrtype strings would need to be upper-case. This is now enforced. 
  In addition attribute names are now case-sensitive. 
  Any mix of case is allowed, but the NDAttributeList::find() method is now case sensitive. 
  This was done because it was found that the epicsStrCaseCmp function was significantly 
  reducing performance with long attribute lists. 
* Removed the possibility to change anything except the datatype and value of an attribute once 
  it is created. The datatype can only be changed from NDAttrUndefined to one of the actual values.
* Added new setDataType() method, removed dataType from setValue() method.
* Added getName(), getDescription(), getSource(), getSourceInfo(), getDataType() methods.
* Fixed logic problem with PVAttribute.  Previously it could update the actual attribute value
  whenever a channel access callback arrived.  It now caches the callback value in private data and
  only copied it to the value field when updateValue() is called.
* Changed constructor to have 6 required paramters, added sourceType and pSource.

### NDPluginDriver 
* This is the base class from which all plugins derive. Added the following calls
  to the NDPluginDriver::processCallbacks() method:
    - setTimeStamp(&pArray->epicsTS);
    - setIntegerParam(NDEpicsTSSec, pArray->epicsTS.secPastEpoch);
    - setIntegerParam(NDEpicsTSNsec, pArray->epicsTS.nsec);

  It calls setTimeStamp with the epicsTS field from the NDArray that was passed to the
  plugin. setTimeStamp sets the internal timestamp in pasynManager. This is the timestamp
  that will be used for all callbacks to device support and read() operations in this
  plugin asynPortDriver. Thus, the record timestamps will come from the NDArray passed
  to the plugin if the record TSE field is -2. It also sets the new asynNDArrayDriver
  NDEpicsTSSec and NDEpicsTSNsec parameters to the fields from the NDArray.epicsTS.
  These records can then be used to monitor the EPICS timestamp in the NDArray even
  if TSE is not -2.

### NDPluginOverlay. 
* Fixed bug in the cross overlay that caused lines not to display if the cross was 
  clipped to the image boundaries. The problem was attempting to store a signed value in a size_t variable. 

### NDPluginROI. 
* Make 3-D [X, Y, 1] arrays be converted to 2-D even if they are not RGB3. 

### NDPluginStats. 
* Fixed bug if a dimension was 1; this bug was introduced when changing dimensions to size_t. 

### NDFileNetCDF. 
* Changes to work on vxWorks 6.8 and above.
* Writes 2 new variables to every netCDF file for each NDArray. 
  - epicsTSSec contains NDArray.epicsTS.secPastEpoch. 
  - epicsTSNsec contains NDArray.epicsTS.nsec. 
    
  Note that these variables are arrays of length numArrays,
  where numArrays is the number of NDArrays (images) in the file. It was not possible
  to write the timestamp as a single 64-bit value because the classic netCDF file
  format does not support 64-bit integers.

### NDFileTIFF. 
* Added 3 new TIFF tags to each TIFF file:</p>
  - Tag=65001, field name=NDUniqueId, field_type=TIFF_LONG, value=NDArray.uniqueId.
  - Tag=65002, field name=EPICSTSSec, field_type=TIFF_LONG, value=NDArray.epicsTS.secPastEpoch.
  - Tag=65003, field name=EPICSTSNsec, field_type=TIFF_LONG, value=NDArray.epicsTS.nsec.

  It was not possible to write the timestamp as a single 64-bit value because TIFF
  does not support 64-bit integer tags. It does have a type called TIFF_RATIONAL which
  is a pair of 32-bit integers. However, when reading such a tag what is returned
  is the quotient of the two numbers, which is not what is desired.

### NDPluginAttribute. 
* New plugin that allows trending and publishing an NDArray attribute over channel access.


R1-9-1 and earlier
==================
Release notes are part of the
[areaDetector Release Notes](http://cars.uchicago.edu/software/epics/areaDetectorReleaseNotes.html).
