TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = PICamApp
PROD_IOC_Linux += $(PROD_NAME)
PROD_IOC_WIN32 += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += PICamSupport.dbd

# Specify link locations of installed PICAM and genicam software
$(PROD_NAME)_LDFLAGS_Linux += -L/opt/pleora/ebus_sdk/x86_64/lib/genicam/bin/Linux64_x64 -Wl,-rpath,/opt/pleora/ebus_sdk/x86_64/lib/genicam/bin/Linux64_x64
$(PROD_NAME)_LDFLAGS_Linux += -L/opt/pleora/ebus_sdk/x86_64/lib -Wl,-rpath,/opt/pleora/ebus_sdk/x86_64/lib

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp

# Add locally compiled object code
$(PROD_NAME)_LIBS += ADPICam
$(PROD_NAME)_LIBS_WIN32 += Picam

$(PROD_NAME)_SYS_LIBS_Linux += picam pida pidi picc piac

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

