/**
 *	TVControl.h
 *	Copyright (C) 2003-2004 Nate
 *	Copyright (C) 2004 RAvenGEr
 *
 *	This file is part of DigitalWatch, a free DTV watching and recording
 *	program for the VisionPlus DVB-T.
 *
 *	DigitalWatch is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	DigitalWatch is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with DigitalWatch; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef TVCONTROL_H
#define TVCONTROL_H

/*#include "DVBInput.h"
#include "FilterGraphManager.h"
#include "OSD.h"
#include "ControlBar.h"*/
#include "KeyMap.h"
#include "DWGraph.h"
#include "DWSource.h"
#include "LogMessage.h"
#include "ParseLine.h"
#include <vector>

#define TIMER_AUTO_HIDE_CURSOR			999
#define TIMER_DISABLE_POWER_SAVING		998
#define TIMER_RECORDING_TIMELEFT		997

#define FT_UNKNOWN             0
#define FT_CHANNEL_CHANGE      1
#define FT_BEFORE_START        2
#define FT_AFTER_START         3
#define FT_ANY                 4

class TVControl : public LogMessageCaller
{
public:
	TVControl();
	~TVControl();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise();
	HRESULT Destroy();

	HRESULT AlwaysOnTop(int nAlwaysOnTop = 1);	//0=off, 1=on, 2=toggle
	HRESULT Fullscreen(int nFullScreen = 1);	//0=off, 1=on, 2=toggle

	HRESULT SetSource(LPWSTR wszSourceName);

	HRESULT VolumeUp(int value);
	HRESULT VolumeDown(int value);
	HRESULT SetVolume(int value);
	HRESULT Mute(int nMute);

	HRESULT SetColorControls(int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma);

	HRESULT Zoom(int percentage);
	HRESULT ZoomIn(int percentage);
	HRESULT ZoomOut(int percentage);
	HRESULT ZoomMode(int mode);
	HRESULT AspectRatio(int width, int height);

	HRESULT ShowMenu(LPWSTR szMenuName);
	HRESULT ExitMenu(long nNumberOfMenusToExit = 1);

	HRESULT ShowOSDItem(LPWSTR szName, long secondsToShowFor = -1);
	HRESULT HideOSDItem(LPWSTR szName);
	HRESULT ToggleOSDItem(LPWSTR szName);

	HRESULT Exit();
	
	HRESULT Key(int nKeycode, BOOL bShift, BOOL bCtrl, BOOL bAlt);
	HRESULT ExecuteCommands(LPCWSTR command);

public:
	BOOL ShowCursor(BOOL bAllowHide = TRUE);
	BOOL HideCursor();

	HRESULT OnSizing(long fwSide, LPRECT rect);
	HRESULT OnSize();
	HRESULT OnTimer(int wParam);

private:
	HRESULT ExecuteGlobalCommand(ParseLine* command);

	KeyMap m_globalKeyMap;

	DWGraph *m_pFilterGraph;
	DWSource *m_pActiveSource;
	std::vector<DWSource *> m_sources;
};

/*
	int GetFunctionType(char* strFunction);

	BOOL SetChannel(int nNetwork, int nProgram = -1);
	BOOL ManualChannel(int nFrequency, int nVPid, int nAPid, BOOL bAC3);
	BOOL NetworkUp();
	BOOL NetworkDown();
	BOOL ProgramUp();
	BOOL ProgramDown();
	BOOL LastChannel();

	BOOL TVPlaying(int nPlaying);	//0=off, 1=on, 2=toggle
	BOOL Recording(int nRecording, char* filename = NULL);	//0=off, 1=on, 2=toggle
	BOOL RecordingTimer(int nAddTime);
	BOOL RecordingPause(int nPause);
	//RecordingTimeoutAdd

	BOOL VolumeUp(int value);
	BOOL VolumeDown(int value);
	BOOL SetVolume(int nMute);
	BOOL Mute(int value);

	BOOL VideoDecoderEntry(int nIndex);
	BOOL AudioDecoderEntry(int nIndex);
	BOOL ResolutionEntry(int nIndex);
	//#ColorControlsEntry(Index) 

	BOOL Resolution(int left, int top, int width, int height, BOOL bMove, BOOL bResize);
	BOOL SetColorControls(int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma);

	BOOL Zoom(int percentage);
	BOOL ZoomIn(int percentage);
	BOOL ZoomOut(int percentage);
	BOOL ZoomMode(int mode);*/

/*	BOOL AspectRatio(int width, int height);

	BOOL ShowFilterProperties();


	BOOL TimeShift(int nMode);
	BOOL TimeShiftJump(int nSeconds);
	//BOOL TimeShiftNow();

	//Menu() 
	//ChannelsMenu() 
	//VideoDecodersMenu() 
	//AudioDecodersMenu() 
	//ResolutionMenu() 
	//#OverlayControlsMenu() 
	//ExitMenu() 

	//CursorUp() 
	//CursorDown() 
	//CursorLeft() 
	//CursorRight() 
	//Select() 
*/
/*
	BOOL ControlBarMouseDown(int x, int y);
	BOOL ControlBarMouseUp(int x, int y);
	BOOL ControlBarMouseMove(int x, int y);

	AppData* m_pAppData;
	DVBInput* m_pDVBInput;
	FilterGraphManager* m_pFilterGraph;
	OSD *m_pOSD;
	ControlBar *m_pControlBar;
*/

#endif