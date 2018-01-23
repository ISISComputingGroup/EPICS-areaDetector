setlocal
call %~dp0..\..\..\..\..\..\..\..\config_env_base.bat
call dllPath.bat
set "PATH=C:\Program Files\Andor SDK;C:\Program Files\Andor SDK\shamrock64;%PATH%"
xcopy /d /y "C:\Program Files\Andor SDK\*.dll" .
xcopy /d /y "C:\Program Files\Andor SDK\shamrock64\*.dll" .
set EPICS_CA_MAX_ARRAY_BYTES=10000000
rem set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;C:\EPICS_ISIS\support\asyn\4-22\opi\medm
rem set EPICS_DISPLAY_PATH=%EPICS_DISPLAY_PATH%;C:\Users\salvato\ISISEPICS\support\areaDetector\R2-1\ADAndor\andorApp\op
rem set DISPLAY=localhost:0.0
REM start medm -x -macro "P=13ANDOR1:, R=cam1:" andorCCD.adl &
..\..\bin\%EPICS_HOST_ARCH%\andorCCDApp st.cmd
pause

