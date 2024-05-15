TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================

DBD += ADEuresysSupport.dbd

USR_INCLUDES += -I$(EURESYS_EGRABBER_INCLUDE)

# This flag significantly speeds up the compiling.
# Comment out for debugging.
USR_CXXFLAGS_Linux += -g0

LIBRARY_IOC_Linux += ADEuresys
LIBRARY_IOC_WIN32 += ADEuresys
ADEuresys_SRCS += EuresysFeature.cpp
ADEuresys_SRCS += ADEuresys.cpp

LIB_LIBS += ADGenICam

include $(ADCORE)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

