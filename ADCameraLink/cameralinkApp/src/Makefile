TOP=../..
include $(TOP)/configure/CONFIG

#----------------------------------------
#  
#
#PCO PCO PCO
#
#
#=============================

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
  LIBRARY_IOC_WIN32 = cameralink
endif
LIBRARY_IOC_Linux = cameralink

#LIB_SRCS    +=legacystubs.c

LIB_SRCS    += camLinkSerial.cpp

LIB_SRCS    += ccd_exception.cpp
LIB_SRCS    += coreco.cpp
LIB_SRCS    += siswSerialPort.cpp
LIB_SRCS    += stopWatch.cpp
LIB_SRCS    += siliconSoftware.cpp
LIB_SRCS    += softwareGrabber.cpp
LIB_SRCS    += cl_com_port2.cpp
LIB_SRCS    += sw_com_port.cpp
LIB_SRCS    += logfile.cpp
LIB_SRCS    += ADCameralink.cpp

LIB_LIBS += fglib5
LIB_LIBS += display_lib

INC += ADCameralink.h
INC += ccd_exception.h
INC += comportInterface.h
INC += coreco.h
INC += logfile.h
INC += stopWatch.h
INC += siswSerialPort.h
INC += cl_com_port2.h
INC += siliconSoftware.h
INC += grabberInterface.h
INC += sw_com_port.h

#one or the other
#USR_CXXFLAGS += -DUSE_SAP
#USR_CXXFLAGS_Linux += -DUSE_SWGRAB
USR_CXXFLAGS += -DUSE_SISW
#use sisw serioal, sw grabber images
USR_CXXFLAGS_Linux += -DUSE_SISW_DBGSER

#let logfile.cpp use asynPrint instead of printf
USR_CXXFLAGS += -D LOGFILE_USE_ASYN

USR_CXXFLAGS_Linux += -D MEUNIX
USR_CXXFLAGS_Linux += -D LINUX

#to use internal serial port code, do not define this
#define to use seperate asynDruver for seruial port.
USR_CXXFLAGS += -D USEASYNSERIAL

#define to use the log_file class in camLinkSrc2, else it uses a null sort of class that does notbhgin
USR_CXXFLAGS += -D USELOGFILE


#when we build debug.. we need this line?
#!! for debvug only
PROD_LDFLAGS_WIN32 = /NODEFAULTLIB:LIBCMT.lib

PROD_LDFLAGS_WIN32 += /NODEFAULTLIB:msvcrt.lib

PROD_LDFLAGS_WIN32 += /VERBOSE:LIB

DBD += camLinkSerialSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE
