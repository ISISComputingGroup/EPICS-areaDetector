TOP = ../..
include $(TOP)/configure/CONFIG
#ARCH = windows-x64-static
#ARCH = windows-x64-debug
#ARCH = windows-x64
#ARCH = linux-x86_64
#ARCH = vxWorks-ppc32
ARCH = $(EPICS_HOST_ARCH)
TARGETS = envPaths cdCommands dllPath.bat runIOC.bat
include $(TOP)/configure/RULES.ioc
