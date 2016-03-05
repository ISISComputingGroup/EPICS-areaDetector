TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# OS-specific files in os/ARCH
INC_WIN32 += fitsio.h
INC_WIN32 += longnam.h 

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
  ifeq ($(STATIC_BUILD), NO)
    FITS_LIBS = $(addprefix ../os/win32-x86-shared/, cfitsio.lib)
    FITS_BINS = $(addprefix ../os/win32-x86-shared/, cfitsio.dll)
    LIB_INSTALLS_WIN32   += $(FITS_LIBS)
    BIN_INSTALLS_WIN32   += $(FITS_BINS)
  endif

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
  ifeq ($(STATIC_BUILD), NO)
    FITS_LIBS = $(addprefix ../os/windows-x64-shared/, cfitsio.lib)
    FITS_BINS = $(addprefix ../os/windows-x64-shared/, cfitsio.dll)
    LIB_INSTALLS_WIN32   += $(FITS_LIBS)
    BIN_INSTALLS_WIN32   += $(FITS_BINS)
  endif

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

