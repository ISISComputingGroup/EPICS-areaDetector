TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for the source files that define template functions.
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

DBD += NDPluginEdge.dbd

INC += NDPluginEdge.h

LIBRARY_IOC += NDPluginEdge

NDPluginEdge_SRCS += NDPluginEdge.cpp

USR_INCLUDES += -I$(OPENCV_INCLUDE)

NDPluginEdge_LIBS       += NDPlugin

ifdef OPENCV_LIB
  opencv_core_DIR += $(OPENCV_LIB)
  NDPluginEdge_LIBS       += opencv_core opencv_imgproc
else
  NDPluginEdge_SYS_LIBS   += opencv_core opencv_imgproc
endif

include $(AREA_DETECTOR)/ADCore/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

