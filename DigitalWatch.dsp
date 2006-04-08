# Microsoft Developer Studio Project File - Name="DigitalWatch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DigitalWatch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DigitalWatch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DigitalWatch.mak" CFG="DigitalWatch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DigitalWatch - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DigitalWatch - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DigitalWatch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/Release"
# PROP Intermediate_Dir "obj/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I ".\src" /I ".\src\OSD" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 d3d9.lib d3dx9.lib ddraw.lib dxguid.lib libcmt.lib libcimt.lib strmbase.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib oleaut32.lib uuid.lib winmm.lib version.lib comdlg32.lib ws2_32.lib shell32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libcd.lib" /nodefaultlib:"libcmtd.lib" /nodefaultlib:"msvcrtd.lib" /out:"bin/DigitalWatch.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DigitalWatch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj/Debug"
# PROP Intermediate_Dir "obj/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I ".\src" /I ".\src\OSD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /YX"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 d3d9.lib d3dx9.lib ddraw.lib dxguid.lib libcmtd.lib libcimtd.lib strmbasd.lib kernel32.lib user32.lib gdi32.lib ole32.lib advapi32.lib oleaut32.lib uuid.lib winmm.lib version.lib dsnetifc.lib comdlg32.lib ws2_32.lib shell32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libcmtd.lib" /nodefaultlib:"msvcrtd.lib" /out:"bin/DigitalWatch.exe" /pdbtype:sept /libpath:"lib/"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "DigitalWatch - Win32 Release"
# Name "DigitalWatch - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\DigitalWatch.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWThread.cpp
# End Source File
# Begin Source File

SOURCE=.\src\stdafx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\DigitalWatch.h
# End Source File
# Begin Source File

SOURCE=.\src\DWThread.h
# End Source File
# Begin Source File

SOURCE=.\src\Globals.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\stdafx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\DigitalWatch.ico
# End Source File
# Begin Source File

SOURCE=.\DigitalWatch.rc
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# End Group
# Begin Group "AppWindows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\DigitalWatchWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DigitalWatchWindow.h
# End Source File
# End Group
# Begin Group "TvControl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\TVControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TVControl.h
# End Source File
# End Group
# Begin Group "DirectShow"

# PROP Default_Filter ""
# Begin Group "Graph"

# PROP Default_Filter ""
# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Group "DVB-T"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\BDACard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDACard.h
# End Source File
# Begin Source File

SOURCE=.\src\BDACardCollection.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDACardCollection.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSource.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSourceTuner.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSourceTuner.h
# End Source File
# Begin Source File

SOURCE=.\src\DVBMpeg2DataParser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DVBMpeg2DataParser.h
# End Source File
# Begin Source File

SOURCE=.\src\DVBTChannels.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DVBTChannels.h
# End Source File
# Begin Source File

SOURCE=.\src\DVBTFrequencyList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DVBTFrequencyList.h
# End Source File
# End Group
# Begin Group "TSFileSource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\TSFileSource\ITSFileSink.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\ITSFileSource.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\MediaFormats.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSinkGuids.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\TSFileSource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\TSFileSource.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\TSFileSourceGuids.h
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\TSFileStreamList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TSFileSource\TSFileStreamList.h
# End Source File
# End Group
# Begin Group "TimeShifting"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\BDADVBTimeShift.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTimeShift.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTimeShiftTuner.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTimeShiftTuner.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\DWSource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWSource.h
# End Source File
# End Group
# Begin Group "Sinks"

# PROP Default_Filter ""
# Begin Group "DVB_T"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\BDADVBTSink.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSink.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkDSNet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkDSNet.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkFile.h
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkTShift.cpp
# End Source File
# Begin Source File

SOURCE=.\src\BDADVBTSinkTShift.h
# End Source File
# Begin Source File

SOURCE=.\src\IMP2Requant.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\src\DWDecoders.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWDecoders.h
# End Source File
# Begin Source File

SOURCE=.\src\DWGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWGraph.h
# End Source File
# Begin Source File

SOURCE=.\src\DWMediaTypes.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWMediaTypes.h
# End Source File
# Begin Source File

SOURCE=.\src\FilterPropList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FilterPropList.h
# End Source File
# End Group
# Begin Group "Filters"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\DWDump.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DWDump.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\dsnetifc.h
# End Source File
# Begin Source File

SOURCE=.\src\FilterGraphTools.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FilterGraphTools.h
# End Source File
# Begin Source File

SOURCE=.\src\StreamFormats.h
# End Source File
# Begin Source File

SOURCE=.\src\SystemDeviceEnumerator.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SystemDeviceEnumerator.h
# End Source File
# End Group
# Begin Group "OSD"

# PROP Default_Filter ""
# Begin Group "OSD Data"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\OSD\DWOSDData.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDData.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDDataItem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDDataItem.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\IDWOSDDataList.h
# End Source File
# End Group
# Begin Group "OSD Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\OSD\DWOSDButton.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDButton.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDControl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDControl.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDGroup.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDImage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDImage.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDLabel.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDList.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOSDWindows.h
# End Source File
# End Group
# Begin Group "DirectDraw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWDirectDraw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWDirectDraw.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWDirectDrawScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWDirectDrawScreen.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWOverlayCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWOverlayCallback.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWRendererDirectDraw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWRendererDirectDraw.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWSurfaceRendererDirectDraw.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DirectDraw\DWSurfaceRendererDirectDraw.h
# End Source File
# End Group
# Begin Group "VMR9Bitmap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\OSD\VMR9Bitmap\DWRendererVMR9Bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\VMR9Bitmap\DWRendererVMR9Bitmap.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\VMR9Bitmap\DWSurfaceRendererVMR9Bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\VMR9Bitmap\DWSurfaceRendererVMR9Bitmap.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\OSD\DWOnScreenDisplay.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWOnScreenDisplay.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWRenderer.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurface.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurface.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurfaceRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurfaceRenderer.h
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurfaceText.cpp
# End Source File
# Begin Source File

SOURCE=.\src\OSD\DWSurfaceText.h
# End Source File
# End Group
# Begin Group "Driect3D"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\DirectXCommon\d3dfont.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DirectXCommon\d3dfont.h
# End Source File
# Begin Source File

SOURCE=.\src\DirectXCommon\d3dutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DirectXCommon\d3dutil.h
# End Source File
# Begin Source File

SOURCE=.\src\DirectXCommon\dxutil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DirectXCommon\dxutil.h
# End Source File
# End Group
# Begin Group "Data"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\AppData.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AppData.h
# End Source File
# Begin Source File

SOURCE=.\src\FileReader.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FileReader.h
# End Source File
# Begin Source File

SOURCE=.\src\FileWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FileWriter.h
# End Source File
# Begin Source File

SOURCE=.\src\KeyMap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\KeyMap.h
# End Source File
# Begin Source File

SOURCE=.\src\XMLDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\src\XMLDocument.h
# End Source File
# End Group
# Begin Group "General"

# PROP Default_Filter ""
# Begin Group "Logging"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\LogMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\LogMessage.h
# End Source File
# Begin Source File

SOURCE=.\src\LogMessageWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\LogMessageWriter.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\GlobalFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\src\GlobalFunctions.h
# End Source File
# Begin Source File

SOURCE=.\src\ParseLine.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ParseLine.h
# End Source File
# Begin Source File

SOURCE=.\src\ReferenceCountingClass.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ReferenceCountingClass.h
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\docs\Configuration.txt
# End Source File
# Begin Source File

SOURCE=.\docs\Functions.txt
# End Source File
# Begin Source File

SOURCE=.\docs\History.txt
# End Source File
# Begin Source File

SOURCE=.\docs\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\docs\TODO.txt
# End Source File
# End Group
# Begin Group "Config Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bin\Channels.ini
# End Source File
# End Group
# Begin Group "Installer"

# PROP Default_Filter ""
# End Group
# Begin Group "MemoryLeakDetect"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\dbgint.h
# End Source File
# Begin Source File

SOURCE=.\src\MemLeakDetect.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MemLeakDetect.h
# End Source File
# End Group
# End Target
# End Project
