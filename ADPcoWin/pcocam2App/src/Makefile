TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module
# -------------------------------

LIBRARY_IOC += pcocam2

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxx.h

# xxxRecord.dbd will be installed into <top>/dbd
DBD += pcocam2Support.dbd
DBD += pcocam2HardwareSupport.dbd

# The following are compiled and added to the support library
pcocam2_SRCS += Pco.cpp
pcocam2_SRCS += DllApi.cpp
pcocam2_SRCS += NDArrayException.cpp
pcocam2_SRCS += AsynException.cpp
pcocam2_SRCS += StateMachine.cpp
pcocam2_SRCS += TraceStream.cpp
pcocam2_SRCS += GangServer.cpp
pcocam2_SRCS += GangClient.cpp
pcocam2_SRCS += GangConnection.cpp
pcocam2_SRCS += GangConfig.cpp
pcocam2_SRCS += GangMemberConfig.cpp
pcocam2_SRCS += GangServerConfig.cpp
pcocam2_SRCS += SocketProtocol.cpp
pcocam2_SRCS += PerformanceMonitor.cpp
pcocam2_SRCS += PcoException.cpp
pcocam2_SRCS_WIN32 += PcoApi.cpp
pcocam2_SRCS += SimulationApi.cpp
pcocam2_SRCS += TakeLock.cpp
pcocam2_SRCS += FreeLock.cpp
pcocam2_SRCS += IntegerParam.cpp
pcocam2_SRCS += DoubleParam.cpp
pcocam2_SRCS += StringParam.cpp
pcocam2_SRCS += AsynParam.cpp
pcocam2_SRCS += ADDriverEx.cpp
pcocam2_SRCS += NdArrayRef.cpp

# Include path to vendor headers
USR_INCLUDES_WIN32 += -I../include/

# These are the vendor libraries
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS += $(wildcard ../lib/*.lib)
BIN_INSTALLS += $(wildcard ../dll/*.dll)
USR_CFLAGS += -DDEBUG  /Zi /O2
USR_CPPFLAGS += -DDEBUG /Zi /O2
USR_LDFLAGS += /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF
else ifeq (windows-x64-debug, $(findstring windows-x64-debug, $(T_A)))
LIB_INSTALLS += $(wildcard ../lib64/*.lib)
BIN_INSTALLS += $(wildcard ../dll64/*.dll)
USR_CFLAGS += /wd4290
USR_CPPFLAGS += /wd4290
#USR_LDFLAGS += /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS += $(wildcard ../lib64/*.lib)
BIN_INSTALLS += $(wildcard ../dll64/*.dll)
USR_CFLAGS += /O2 /wd4290
USR_CPPFLAGS += /O2 /wd4290
USR_LDFLAGS += /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF
else
USR_CPPFLAGS += -DDEBUG -O0
USR_CFLAGS += -DDEBUG  -O0
endif

#switch off optimization for debugging
#HOST_OPT=NO

# We need to link against the EPICS Base libraries
pcocam2_LIBS += $(EPICS_BASE_IOC_LIBS)



include $(TOP)/configure/RULES
