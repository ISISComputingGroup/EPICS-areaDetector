@echo off
set OLD_PATH=%PATH%
set PATH=%ProgramFiles(x86)%\Andor iStar\Drivers;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADAndorIstar\andorIstarApp\cfitsio\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADBinaries\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADAndorIstar\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADCore\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\asyn\4-26\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\oncrpc\2\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\calc\3-4\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\sscan\2-8\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\busy\1-6-1\bin\win32-x86;%PATH%
set PATH=%EPICS_KIT_ROOT%\support\autosave\R5_6_1\bin\win32-x86;%PATH%
set PATH=C:\Program Files\Hummingbird\Connectivity\14.00\Exceed;%PATH%

set EPICS_CA_MAX_ARRAY_BYTES=10000000

set EPICS_DISPLAY_PATH=%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADAndorIstar\andorIstarApp\op\adl
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\areaDetector\R2-1\ADCore\ADApp\op\adl
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\asyn\4-26\opi\medm
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;%EPICS_KIT_ROOT%\support\autosave\R5_6_1\asApp\op\adl

set DISPLAY=localhost:0.0
@echo on

start medm -x -macro "P=13ANDOR1:, R=cam1:" Andor.adl &

..\..\bin\win32-x86\andorIstarApp st.cmd

@echo off
set PATH=%OLD_PATH%
@echo on

