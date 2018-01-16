#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) configure
DIRS := $(DIRS) siliconSoftwareSupport
DIRS := $(DIRS) cameralinkApp
pcoApp_DEPEND_DIRS += siliconSoftwareSupport
ifeq ($(BUILD_IOCS), YES)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
iocs_DEPEND_DIRS += cameralinkApp
endif
include $(TOP)/configure/RULES_TOP

