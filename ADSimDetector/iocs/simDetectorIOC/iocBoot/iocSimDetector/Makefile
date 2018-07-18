TOP = ../..
include $(TOP)/configure/CONFIG
#ARCH = win32-x86
#ARCH = win32-x86-mingw
#ARCH = windows-x64-static
#ARCH = windows-x64-debug
#ARCH = windows-x64
#ARCH = cygwin-x86
#ARCH = linux-x86
#ARCH = linux-x86-rhel6
#ARCH = linux-x86_64
ARCH = $(EPICS_HOST_ARCH)
#ARCH = solaris-sparc
#ARCH = vxWorks-ppc32
TARGETS = envPaths cdCommands dllPath.bat runIOC.bat
include $(TOP)/configure/RULES.ioc
