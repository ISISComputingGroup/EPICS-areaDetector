set PATH=c:/Programmi/Andor iStar/Drivers;%PATH%
set EPICS_CA_MAX_ARRAY_BYTES=10000000
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;C:\EPICS_ISIS\support\asyn\4-22\opi\medm
set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;C:\Users\salvato\ISISEPICS\support\areaDetector\R2-1\ADAndor\andorApp\op
set DISPLAY=localhost:0.0
REM start medm -x -macro "P=13ANDOR1:, R=cam1:" andorCCD.adl &
..\..\bin\win32-x86\andorCCDApp st.cmd
pause

