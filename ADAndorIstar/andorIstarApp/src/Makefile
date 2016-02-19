TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

LIBRARY_IOC_WIN32 += andorIstar
LIBRARY_IOC_Linux += andorIstar
LIB_SRCS += andorIstar.cpp

# (Gabriele Salvato) LIB_SRCS_WIN32 += shamrock.cpp
LIB_SRCS_WIN32 += shamrockDummy.cpp

LIB_SRCS_Linux += shamrockDummy.cpp

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
  LIB_LIBS += ATMCD32M
  LIB_LIBS += cfitsio
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
  LIB_LIBS += atmcd64m
  LIB_LIBS += cfitsio
endif

# (Gabriele Salvato) LIB_LIBS_WIN32 += ShamrockCIFm

DATA+=GREY.PAL

DBD += andorIstarSupport.dbd
DBD += shamrockSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

