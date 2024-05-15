ADPICam
=============
An [EPICS](http://www.aps.anl.gov/epics/) [areaDetector](https://github.com/areaDetector/areaDetector/blob/master/README.md) 
driver for Cameras from Princeton Instruments that support the PICAM library.  

[PICam](ftp://ftp.princetoninstruments.com/public/Manuals/Princeton%20Instruments/PICam%20User%20Manual.pdf)

This driver is based on the PICAM Virtual Camera Library. It runs on 64 bit Windows (Vista, 7 & 8) and 64 bit linux. Detectors supported by the vendor PICAM driver library include:
* PI-MAX3
* PI-MAX4, PI-MAX4:RF, PI-MAX4:EM
* PIoNIR/NIRvana
* NIRvana-LN
* PIXIS, PIXIS-XB, PIXIS-XF, PIXIS-XO
* ProEM
* ProEM+
* PyLoN
* PyLoN-IR
* Quad-RO

This detector has been tested with a Quad-RO camera and to some degree with the 
PIXIS Demo camera (soft camera in the library).  Most notably missing from the 
library so far are the Pulse and Modulation Parameters used mostly in the 
PI-MAX3 & 4 cameras.     

### Building on Linux

ADPICam has been built on RHEL 8 and CentOS 7, and tested with a PI-MTE3 detector on RHEL 8.

Before building ADPICam on linux, you must first install the PICam SDK from Princeton Instruments ([available via a support request on their site](https://www.princetoninstruments.com/contact-software/)):

```
sudo bash Picam_SDK-v5.12.2.run
```

This should install libraries into `/opt/pleora` and `/opt/PrincetonInstrumnets`, along with some links in `/usr/local/lib`. To build ADPICam you will need to add read permissions to these locations to the specified user, or build and run as root. Additionally, make sure that /usr/local/lib is in your system library search path:

```
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

From there, you can build with

```
make
```

Additional information:
* [Documentation](https://areaDetector.github.io/areaDetector/ADPICam/PICamDoc.html).
* [Release notes and links to source and binary releases](RELEASE.md).
