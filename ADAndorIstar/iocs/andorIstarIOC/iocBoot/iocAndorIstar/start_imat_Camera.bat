@echo off
set OLD_PATH=%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADAndorIstar\bin\win32-x86;%PATH%
set PATH=%ProgramFiles(x86)%\Andor iStar\Drivers;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADAndorIstar\lib\win32-x86;%PATH%
set PATH=%ProgramFiles%\Andor iStar\Drivers;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADAndorIstar\andorIstarApp\cfitsio\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADBinaries\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADCore\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\asyn\master\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\oncrpc\master\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\calc\master\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\sscan\master\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\busy\master\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\autosave\master\bin\win32-x86;%PATH%
set PATH=C:\Program Files\Hummingbird\Connectivity\14.00\Exceed;%PATH%

set EPICS_CA_MAX_ARRAY_BYTES=10000000

set EPICS_DISPLAY_PATH=%EPICS_KIT_ROOT%\support\areaDetector\master\ADAndorIstar\andorIstarApp\op\adl
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\areaDetector\master\ADCore\ADApp\op\adl
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\asyn\master\opi\medm
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\autosave\master\asApp\op\adl

set DISPLAY=localhost:0.0
@echo on

start medm -x -macro "P=%MYPVPREFIX%13ANDOR1:, R=cam1:" Andor.adl &

..\..\bin\win32-x86\andorIstarApp st.cmd

@echo off
set PATH=%OLD_PATH%
@echo on

