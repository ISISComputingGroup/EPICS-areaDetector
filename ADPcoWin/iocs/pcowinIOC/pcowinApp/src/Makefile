TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = pcowinApp

# Only build for Windows
PROD_IOC_WIN32 += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += base.dbd
$(PROD_NAME)_DBD += asyn.dbd
$(PROD_NAME)_DBD += busySupport.dbd
$(PROD_NAME)_DBD += pcowinSupport.dbd
$(PROD_NAME)_DBD += pcowinHardwareSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp

# Add locally compiled object code
$(PROD_NAME)_LIBS += pcowin PCO_CDlg Pco_conv SC2_Cam SC2_DLG

# Other library dependencies
$(PROD_NAME)_LIBS += ADBase
$(PROD_NAME)_LIBS += busy

# System libraries
$(PROD_NAME)_SYS_LIBS += nafxcw
$(PROD_NAME)_SYS_LIBS += Comctl32
$(PROD_NAME)_SYS_LIBS += Winspool
$(PROD_NAME)_SYS_LIBS += Comdlg32
$(PROD_NAME)_SYS_LIBS += windowscodecs

# Include pco .dll files
BIN_INSTALLS_WIN32 += $(wildcard $(ADPCOWIN)/bin/$(EPICS_HOST_ARCH)/*.dll)

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

