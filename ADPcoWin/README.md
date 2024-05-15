ADWinPco
========

EPICS areaDetector driver for PCO cameras.

Credits and licensing
---------------------

Original development of source code in this module from Diamond Light Source. Released under the Apache V2 license. See LICENSE.

The camera drivers are provided by PCO and redistributed here with permission from the vendor in binary, unmodified form. The drivers are downloaded from the 
[PCO support website](https://www.pco.de/support/interface/scmos-cameras/)


Supported cameras
-----------------

This driver has been tested with the following combination of cameras and framegrabbers:

| Camera model  | Framegrabber   | Interface     |
|---------------|----------------|---------------|
| pco.4000      | microEnable IV | CameraLink    |
| pco.1600      | microEnable IV | CameraLink    |
| pco.dimax     | microEnable IV | CameraLink    |
| pco.edge CL   | microEnable IV | CameraLink    |
| pco.edge CLHS | microEnable V  | CameraLink HS |

Most cameras should work as long as the DLLs/libs have been added for the interface used. These are available from the PCO support website above.

Additional features of some cameras such as the dimax ring buffer may not be supported.

Operating systems
-----------------

We have tested this driver with Windows 10 and Windows Server 2012.

Additional information
----------------------
* [Release notes](RELEASE.md)