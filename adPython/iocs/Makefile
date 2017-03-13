TOP = ..
include $(TOP)/configure/CONFIG

ifeq ($(EPICS_HOST_ARCH), linux-x86)
	DIRS := $(wildcard example example_sim)
else ifeq ($(EPICS_HOST_ARCH), linux-x86_64)
	DIRS :=$(wildcard example example_sim example_zxing)
else ifeq ($(EPICS_HOST_ARCH), win32-x86)
	DIRS :=
endif
DIRS := example

include $(TOP)/configure/RULES_DIRS
