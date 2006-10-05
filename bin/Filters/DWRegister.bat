cd Filters

%WINDIR%\system32\regsvr32.exe TSFileSource.ax /S /u
rem copy TSFileSource.ax %WINDIR%\system32
%WINDIR%\system32\regsvr32.exe TSFileSource.ax /s

%WINDIR%\system32\regsvr32.exe DSNet.ax /S /u
copy DSNet.ax %WINDIR%\system32
%WINDIR%\system32\regsvr32.exe DSNet.ax /s

rem regsvr32 MpaDecFilter.ax
rem regsvr32 Mpeg2DecFilter.ax
rem regsvr32 MpgMux.ax
rem Pause