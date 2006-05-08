/**
 *	TVControl.cpp
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

#include "StdAfx.h"
#include <time.h>
#include "TVControl.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "DWSource.h"
#include "BDADVBTSource.h"
#include "TSFileSource/TSFileSource.h"
#include "BDADVBTimeShift.h"
#include "../resource.h"

#include <process.h>
#include <math.h>
#include <shlobj.h>

void CommandQueueThread(void *pParam)
{
	TVControl *tv;
	tv = (TVControl *)pParam;
	tv->StartCommandQueueThread();
}

//////////////////////////////////////////////////////////////////////
// TVControl
//////////////////////////////////////////////////////////////////////
TVControl::TVControl()
{
	m_mainThreadId = GetCurrentThreadId();
	m_hCommandProcessingStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hCommandProcessingDoneEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_pFilterGraph = NULL;
	m_pActiveSource = NULL;

	m_windowInitialiseState = WIS_UNDEFINED;
}
TVControl::~TVControl(void)
{
	Destroy();
	CloseHandle(m_hCommandProcessingStopEvent);
	CloseHandle(m_hCommandProcessingDoneEvent);
}

void TVControl::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock sourcesLock(&m_sourcesLock);

	LogMessageCaller::SetLogCallback(callback);

	std::vector<DWSource *>::iterator it = m_sources.begin();
	for ( ; it != m_sources.end() ; it++ )
	{
		DWSource *source = *it;
		source->SetLogCallback(callback);
	}
	m_globalKeyMap.SetLogCallback(callback);
	if (m_pFilterGraph)
		m_pFilterGraph->SetLogCallback(callback);

}

HRESULT TVControl::Initialise()
{
	(log << "Initialising TVControl\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;
	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%sKeys.xml", g_pData->application.appPath);
	if FAILED(hr = m_globalKeyMap.LoadFromFile((LPWSTR)&file))
		return hr;

	m_pFilterGraph = new DWGraph();
	m_pFilterGraph->SetLogCallback(m_pLogCallback);
	m_pFilterGraph->Initialise();
	m_pActiveSource = NULL;

	CAutoLock sourcesLock(&m_sourcesLock);
	DWSource *source;

	source = new BDADVBTSource();
	source->SetLogCallback(m_pLogCallback);
	m_sources.push_back(source);
	(log << "Added Source - " << source->GetSourceType() << "\n").Write();

	source = new TSFileSource();
	source->SetLogCallback(m_pLogCallback);
	m_sources.push_back(source);
	(log << "Added Source - " << source->GetSourceType() << "\n").Write();

	source = new BDADVBTimeShift();
	source->SetLogCallback(m_pLogCallback);
	m_sources.push_back(source);
	(log << "Added Source - " << source->GetSourceType() << "\n").Write();

	hr = ShowMenu(L"MainMenu");
	(log << "Showing Main Menu : " << hr << "\n").Write();

	(log << "Starting command processing thread\n").Write();
	ResetEvent(m_hCommandProcessingStopEvent);
	ResetEvent(m_hCommandProcessingDoneEvent);
	unsigned long result = _beginthread(CommandQueueThread, 0, (void *) this);
	if (result == -1L)
	{
		(log << "  Thread failed to start.\n").Write();
		return E_FAIL;
	}

	(log << "Starting Timers\n").Write();
	if (g_pData->settings.application.disableScreenSaver)
	{
		SetTimer(g_pData->hWnd, TIMER_DISABLE_POWER_SAVING, 30000, NULL);
	}
	SetTimer(g_pData->hWnd, TIMER_RECORDING_TIMELEFT, 1000, NULL);

	indent.Release();
	(log << "Finished Initialising TVControl\n").Write();

	return S_OK;
}

HRESULT TVControl::Destroy()
{
	(log << "Destroying TVControl\n").Write();
	LogMessageIndent indent(&log);

	(log << "Stopping Timers\n").Write();
	KillTimer(g_pData->hWnd, TIMER_DISABLE_POWER_SAVING);
	KillTimer(g_pData->hWnd, TIMER_RECORDING_TIMELEFT);

	(log << "Stoping command queue processing thread\n").Write();
	SetEvent(m_hCommandProcessingStopEvent);
	DWORD result;
	do
	{
		result = WaitForSingleObject(m_hCommandProcessingDoneEvent, 1000);
	} while (result == WAIT_TIMEOUT);
	if (result != WAIT_OBJECT_0)
	{
		DWORD err = GetLastError();
		(log << "WaitForSingleObject error: " << (int)err << "\n").Write();
	}

	(log << "Cleaning up sources\n").Write();
	CAutoLock sourcesLock(&m_sourcesLock);

	UnloadSource();

	std::vector<DWSource *>::iterator it = m_sources.begin();
	for ( ; it != m_sources.end() ; it++ )
	{
		DWSource * source = *it;
		source->Destroy();
		delete source;
	}
	m_sources.clear();

	if (m_pFilterGraph)
	{
		m_pFilterGraph->Destroy();
		delete m_pFilterGraph;
		m_pFilterGraph = NULL;
	}

	indent.Release();
	(log << "Finished Destroying TVControl\n").Write();

	return S_OK;
}

HRESULT TVControl::Load(LPWSTR pCmdLine)
{
	CAutoLock sourcesLock(&m_sourcesLock);
	HRESULT hr;

	if (pCmdLine[0] == '\0')
	{
		(log << "No Command line arguments specified\n").Write();
		return S_FALSE;
	}

	(log << "Loading Command line: " << pCmdLine << "\n").Write();

	// Check for quotes and trim them if they're there.
	if (pCmdLine[0] == L'"')
	{
		pCmdLine++;

		int i=0;
		while ((pCmdLine[i] != L'"') && (pCmdLine[i] != 0))
		{
			if (pCmdLine[i] == L'\\')
				i++;
			i++;
		}
		pCmdLine[i] = 0;

		(log << "After quotes trimmed: " << pCmdLine << "\n").Write();
	}

	std::vector<DWSource *>::iterator it = m_sources.begin();
	for ( ; it < m_sources.end() ; it++ )
	{
		DWSource *source = *it;
		if (source->CanLoad(pCmdLine))
		{
			if (source != m_pActiveSource)
			{
				if (m_pActiveSource)
				{
					//TODO: create a m_pActiveSource->DisconnectFromGraph() method
					m_pActiveSource->Destroy();
					m_pActiveSource = NULL;
				}

				if FAILED(hr = source->Initialise(m_pFilterGraph))
					return (log << "Failed to initialise new active source: " << hr << "\n").Write();

				m_pActiveSource = source;
			}
			if FAILED(hr = source->Load(pCmdLine))
				return (log << "Failed to load command \"" << pCmdLine << "\" in new active source: " << hr << "\n").Write();

			ExitMenu(-1);

			return S_OK;
		}
	}

	return S_FALSE;
}

HRESULT TVControl::AlwaysOnTop(int nAlwaysOnTop)
{
	long bAlwaysOnTop = ((nAlwaysOnTop == 1) || ((nAlwaysOnTop == 2) && !g_pData->values.window.bAlwaysOnTop));

	g_pData->values.window.bAlwaysOnTop = bAlwaysOnTop;

	BOOL bResult = FALSE;

	if (g_pData->values.window.bAlwaysOnTop)
		bResult = ::SetWindowPos(g_pData->hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	else
		bResult = ::SetWindowPos(g_pData->hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	//if (bResult)
	//	m_pOSD->ShowItem("AlwaysOnTop");
	return (bResult ? S_OK : E_FAIL);
}

HRESULT TVControl::Fullscreen(int nFullScreen)
{
	long bFullScreen = ((nFullScreen == 1) || ((nFullScreen == 2) && !g_pData->values.window.bFullScreen));

	g_pData->values.window.bFullScreen = bFullScreen;

	WINDOWPLACEMENT wPlace;
	GetWindowPlacement(g_pData->hWnd, &wPlace);

	if (g_pData->values.window.bFullScreen)
		wPlace.showCmd = SW_MAXIMIZE;
	else
		wPlace.showCmd = SW_RESTORE;

	BOOL bResult = SetWindowPlacement(g_pData->hWnd, &wPlace);

	//if (bResult)
	//	m_pOSD->ShowItem("Fullscreen");
	return (bResult ? S_OK : E_FAIL);
}

HRESULT TVControl::SetSource(LPWSTR wszSourceName)
{
	CAutoLock sourcesLock(&m_sourcesLock);
	HRESULT hr;

	std::vector<DWSource *>::iterator it = m_sources.begin();
	for ( ; it < m_sources.end() ; it++ )
	{
		DWSource *source = *it;
		LPWSTR pType = source->GetSourceType();

		if (_wcsicmp(wszSourceName, pType) == 0)
		{
			if (m_pActiveSource)
			{
				if (m_pActiveSource && m_pActiveSource->IsRecording())
				{
					g_pTv->ShowOSDItem(L"Recording", 5);
					g_pOSD->Data()->SetItem(L"warnings", L"Recording In Progress");
					g_pTv->ShowOSDItem(L"Warnings", 5);
					g_pOSD->Data()->SetItem(L"recordingicon", L"R");
					g_pTv->ShowOSDItem(L"RecordingIcon", 100000);
					return S_FALSE;
				}

				//TODO: create a m_pActiveSource->DisconnectFromGraph() method
				m_pActiveSource->Destroy();
				m_pActiveSource = NULL;
			}

			if FAILED(hr = source->Initialise(m_pFilterGraph))
				return (log << "Failed to initialise source: " << hr << "\n").Write();

			m_pActiveSource = source;
			if FAILED(hr = source->Load(NULL))
				return (log << "Failed to load NULL command: " << hr << "\n").Write();

			return S_OK;
		}
	}

	return S_FALSE;
}

HRESULT TVControl::UnloadSource()
{
	if (m_pActiveSource)
	{
		//TODO: create a m_pActiveSource->DisconnectFromGraph() method
		m_pActiveSource->Destroy();
		m_pActiveSource = NULL;
	}
	return S_OK;
}

HRESULT TVControl::VolumeUp(int value)
{
	HRESULT hr;
	long volume;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->GetVolume(m_pActiveSource->GetGraphBuilder(), volume);
	else
		hr = m_pFilterGraph->GetVolume(volume);

	if FAILED(hr)
		return (log << "Failed to retrieve volume: " << hr << "\n").Write(hr);

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->SetVolume(m_pActiveSource->GetGraphBuilder(), volume+value);
	else
		hr = m_pFilterGraph->SetVolume(volume+value);

	if FAILED(hr)
		return (log << "Failed to set volume: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("Volume");
	return S_OK;
}

HRESULT TVControl::VolumeDown(int value)
{
	HRESULT hr;
	long volume;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->GetVolume(m_pActiveSource->GetGraphBuilder(), volume);
	else
		hr = m_pFilterGraph->GetVolume(volume);

	if FAILED(hr)
		return (log << "Failed to retrieve volume: " << hr << "\n").Write(hr);

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->SetVolume(m_pActiveSource->GetGraphBuilder(), volume-value);
	else
		hr = m_pFilterGraph->SetVolume(volume-value);

	if FAILED(hr)
		return (log << "Failed to set volume: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("Volume");
	return S_OK;
}

HRESULT TVControl::SetVolume(int value)
{
	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->SetVolume(m_pActiveSource->GetGraphBuilder(), value);
	else
		hr = m_pFilterGraph->SetVolume(value);

	if FAILED(hr)
		return (log << "Failed to set volume: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("Volume");
	return hr;
}

HRESULT TVControl::Mute(int nMute)
{
	g_pData->values.audio.bMute = ((nMute == 1) || ((nMute == 2) && !g_pData->values.audio.bMute));

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->Mute(m_pActiveSource->GetGraphBuilder(), g_pData->values.audio.bMute);
	else
		hr = m_pFilterGraph->Mute(g_pData->values.audio.bMute);

	if FAILED(hr)
		return (log << "Failed to set mute: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("Mute");
	return S_OK;
}

HRESULT TVControl::SetColorControls(int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma)
{
	if (nBrightness <     0) nBrightness =     0;
	if (nBrightness > 10000) nBrightness = 10000;
	if (nContrast   <     0) nContrast   =     0;
	if (nContrast   > 20000) nContrast   = 20000;
	if (nHue        <  -180) nHue        =  -180;
	if (nHue        >   180) nHue        =   180;
	if (nSaturation <     0) nSaturation =     0;
	if (nSaturation > 20000) nSaturation = 20000;
	if (nGamma      <     1) nGamma      =     1;
	if (nGamma      >   500) nGamma      =   500;

//	HRESULT hr = m_pFilterGraph->SetColorControls(nBrightness, nContrast, nHue, nSaturation, nGamma);
	HRESULT hr = m_pActiveSource->GetFilterGraph()->SetColorControls(m_pActiveSource->GetGraphBuilder(), nBrightness, nContrast, nHue, nSaturation, nGamma);
	if FAILED(hr)
		return (log << "Failed to set color controls: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("ColorControls");
	return S_OK;
}

HRESULT TVControl::Zoom(int percentage)
{
	g_pData->values.video.zoom = percentage;
	if (g_pData->values.video.zoom < 10)	//Limit the smallest zoom to 10%
		g_pData->values.video.zoom = 10;

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else
		hr = m_pFilterGraph->RefreshVideoPosition();

	if FAILED(hr)
		return (log << "Failed to RefreshVideoPosition: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("Zoom");
	return S_OK;
}

HRESULT TVControl::ZoomIn(int percentage)
{
	g_pData->values.video.zoom += percentage;

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else
		hr = m_pFilterGraph->RefreshVideoPosition();

	if FAILED(hr)
		return (log << "Failed to RefreshVideoPosition: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("ZoomIn");
	return S_OK;
}

HRESULT TVControl::ZoomOut(int percentage)
{
	g_pData->values.video.zoom -= percentage;
	if (g_pData->values.video.zoom < 10)	//Limit the smallest zoom to 10%
		g_pData->values.video.zoom = 10;

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else
		hr = m_pFilterGraph->RefreshVideoPosition();

	if FAILED(hr)
		return (log << "Failed to RefreshVideoPosition: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("ZoomOut");
	return S_OK;
}

HRESULT TVControl::ZoomMode(int mode)
{
	const int maxModes = 2;
	if (mode >= maxModes)
		return (log << "zoommode " << mode << " is not a valid mode\n").Write(E_FAIL);
	if (mode < 0)
	{
		mode = g_pData->values.video.zoomMode + 1;
		if (mode >= maxModes)
			mode = 0;
	}
	g_pData->values.video.zoomMode = mode;

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else
		hr = m_pFilterGraph->RefreshVideoPosition();

	if FAILED(hr)
		return (log << "Failed to RefreshVideoPosition: " << hr << "\n").Write(hr);

	//m_pOSD->ShowItem("ZoomMode");
	return S_OK;
}

HRESULT TVControl::AspectRatio(int nOverride, int width, int height)
{
	if (nOverride == 0)
		g_pData->values.video.aspectRatio.bOverride = FALSE;

	if (width <= 0)
		return (log << "Zero or negative width supplied: " << width << "\n").Write(E_FAIL);
	if (height <= 0)
		return (log << "Zero or negative height supplied: " << height << "\n").Write(E_FAIL);

	g_pData->values.video.aspectRatio.bOverride = nOverride;
	g_pData->values.video.aspectRatio.width = width;
	g_pData->values.video.aspectRatio.height = height;

	HRESULT hr;

	if(m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else
		hr = m_pFilterGraph->RefreshVideoPosition();

	if FAILED(hr)
		return (log << "Failed to RefreshVideoPosition: " << hr << "\n").Write(hr);

	//OSD "AspectRatio"
	return S_OK;
}

HRESULT TVControl::ShowMenu(LPWSTR szMenuName)
{
	HRESULT hr = S_FALSE;
	hr = g_pOSD->ShowMenu(szMenuName);
	return hr;
}

HRESULT TVControl::ExitMenu(long nNumberOfMenusToExit)
{
	HRESULT hr = S_FALSE;
	hr = g_pOSD->ExitMenu(nNumberOfMenusToExit);
	return hr;
}

HRESULT TVControl::ShowOSDItem(LPWSTR szName, long secondsToShowFor)
{
	DWOSDWindow *overlay = g_pOSD->Overlay();
	if (overlay)
	{
		DWOSDControl *control = overlay->GetControl(szName);
		if (control)
		{
			control->Show(secondsToShowFor);
			return S_OK;
		}
	}

	return S_FALSE;
}

HRESULT TVControl::HideOSDItem(LPWSTR szName)
{
	DWOSDWindow *overlay = g_pOSD->Overlay();
	if (overlay)
	{
		DWOSDControl *control = overlay->GetControl(szName);
		if (control)
		{
			control->Hide();
			return S_OK;
		}
	}

	return S_FALSE;
}

HRESULT TVControl::ToggleOSDItem(LPWSTR szName)
{
	DWOSDWindow *overlay = g_pOSD->Overlay();
	if (overlay)
	{
		DWOSDControl *control = overlay->GetControl(szName);
		if (control)
		{
			control->Toggle();
			return S_OK;
		}
	}

	return S_FALSE;
}

HRESULT TVControl::SetWindowPos(int nLeft, int nTop, int nWidth, int nHeight, BOOL bMove, BOOL bResize)
{
	if (m_windowInitialiseState == WIS_UNDEFINED)
	{
		m_windowInitialiseState = WIS_INITIALISING;
		// This seems to make the client area and border width initialise
		::SetWindowPos(g_pData->hWnd, NULL, 0, 0, 720, 408, SWP_NOMOVE | SWP_NOZORDER);
		m_windowInitialiseState = WIS_INITIALISED;
	}

	if (g_pData->values.window.bFullScreen)
		return S_FALSE;
	HRESULT hr;

	RECT gwr, gcr;
	GetWindowRect(g_pData->hWnd, &gwr);
	GetClientRect(g_pData->hWnd, &gcr);
	POINT point;

	point.x = gwr.left;
	point.y = gwr.top;
	::ScreenToClient(g_pData->hWnd, &point);
	int left = gcr.left - point.x;
	int top  = gcr.top  - point.y;

	point.x = gwr.right;
	point.y = gwr.bottom;
	::ScreenToClient(g_pData->hWnd, &point);
	int right  = point.x - gcr.right;
	int bottom = point.y - gcr.bottom;

	int width  = nWidth  + (left + right);
	int height = nHeight + (top + bottom);
	left = nLeft - left;
	top  = nTop  - top;

	long flags = SWP_NOZORDER | (bMove ? 0 : SWP_NOMOVE) | (bResize ? 0 : SWP_NOSIZE);
	hr = (::SetWindowPos(g_pData->hWnd, NULL, left, top, width, height, flags) ? S_OK : E_FAIL);

	return hr;
}

HRESULT TVControl::Exit()
{

	if (m_pActiveSource && m_pActiveSource->IsRecording())
	{
		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Recording In Progress");
		g_pTv->ShowOSDItem(L"Warnings", 5);
		g_pOSD->Data()->SetItem(L"recordingicon", L"R");
		g_pTv->ShowOSDItem(L"RecordingIcon", 100000);
		return S_FALSE;
	}
/*
	if (m_pFilterGraph->IsRecording())
	{
		return_FALSE_SHOWMSG("Cannot exit while recording");
	}
*/
//	KillTimer(g_pData->hWnd, 996);

	// We can't call DestroyWindow from a different thread so we set send a WM_QUIT
	// message to the main thread instead
	return (PostThreadMessage(m_mainThreadId, WM_QUIT, 0, 0) ? S_OK : E_FAIL);
}

HRESULT TVControl::OnKey(int nKeycode, BOOL bShift, BOOL bCtrl, BOOL bAlt)
{
	LPWSTR command = new wchar_t[100];
	swprintf(command, L"Key(%i, %i, %i, %i)", nKeycode, bShift, bCtrl, bAlt);
	ExecuteCommandsQueue(command);
	delete[] command;
	return S_OK;
}

HRESULT TVControl::Key(int nKeycode, BOOL bShift, BOOL bCtrl, BOOL bAlt)
{
	if (nKeycode >= 0)
		log << "TVControl::Key - ";
	else
		log << "TVControl::Mouse - ";
	log.Write();

	if (bShift) log << L"shift ";
	if (bCtrl ) log << L"ctrl ";
	if (bAlt  ) log << L"alt ";

	if (nKeycode == MOUSE_LDBLCLICK)
		log << L"Left Double Click";
	else if (nKeycode == MOUSE_RCLICK)
		log << L"Right Click";
	else if (nKeycode == MOUSE_MCLICK)
		log << L"Middle Click";
	else if (nKeycode == MOUSE_SCROLL_UP)
		log << L"Scroll Up";
	else if (nKeycode == MOUSE_SCROLL_DOWN)
		log << L"Scroll Down";
	else
	{
		LPTSTR keyName = new TCHAR[100];
		GetKeyNameText((MapVirtualKey(nKeycode, 0) << 16), keyName, 100);

		log << keyName << " (" << nKeycode << ")";
		delete[] keyName;
	}

	HideOSDItem(L"UnknownKey");
	g_pOSD->Data()->SetItem(L"LastKey", log.GetBuffer());

	log << "\n";
	log.Write();

	LogMessageIndent indent(&log);


	HRESULT hr;
	LPWSTR command = NULL;

	if FAILED(hr = g_pOSD->GetKeyFunction(nKeycode, bShift, bCtrl, bAlt, &command))
	{
		if (m_pActiveSource)
		{
			hr = m_pActiveSource->GetKeyFunction(nKeycode, bShift, bCtrl, bAlt, &command);
		}
		if FAILED(hr)
		{
			hr = m_globalKeyMap.GetFunction(nKeycode, bShift, bCtrl, bAlt, &command);
		}
	}

	if SUCCEEDED(hr)
	{
		ExecuteCommandsImmediate(command);
		delete command;
		return S_OK;
	}

	ShowOSDItem(L"UnknownKey", 5);
/*
	g_pData->KeyPress.Set(nKeycode, bShift, bCtrl, bAlt);
	m_pOSD->ShowItem("KeyPress");
*/
	return S_FALSE;
}

HRESULT TVControl::ExecuteCommandsImmediate(LPCWSTR command)
{
	HRESULT hr = S_FALSE;
	if (command == NULL)
		return hr;

	(log << "TVControl Execute '" << command << "'\n").Write();
	LogMessageIndent indent(&log);

	//Make a copy of the command in case executing it changes the pointer that was passed in.
	LPWSTR pCurr = NULL;
	strCopy(pCurr, command);
	LPWSTR pCommand = pCurr;

	while (wcslen(pCurr) > 0)
	{
		ParseLine parseLine;
		parseLine.IgnoreRHS();
		if (parseLine.Parse(pCurr) == FALSE)
			return (log << "TVControl::ExecuteCommandImmediate - Parse error in function: " << pCurr << "\n").Show(E_FAIL);

		hr = ExecuteGlobalCommand(&parseLine);
		if (hr == S_FALSE)	//S_FALSE if the ExecuteFunction didn't handle the function
		{
			hr = g_pOSD->ExecuteCommand(&parseLine);
			if (hr == S_FALSE)
			{
				if (m_pActiveSource)
				{
					hr = m_pActiveSource->ExecuteCommand(&parseLine);
				}
			}
		}
		if (hr == S_FALSE)
		{
			(log << "Function '" << parseLine.LHS.Function << "' called but has no implementation.\n").Write();
		}
		pCurr += parseLine.GetLength();
		skipWhitespaces(pCurr);
	}

	delete[] pCommand;

	indent.Release();
	(log << "Finished Execute : " << hr << "\n").Write();

	return hr;
}

HRESULT TVControl::ExecuteGlobalCommand(ParseLine* command)
{
	(log << "TVControl::ExecuteGlobalCommand - " << command->LHS.Function << "\n").Write();
	LogMessageIndent indent(&log);

	int n1, n2, n3, n4, n5;//, n6;
	LPWSTR pCurr = command->LHS.FunctionName;

	if (_wcsicmp(pCurr, L"Exit") == 0)
	{
		if (command->LHS.ParameterCount > 0)
			return (log << "TVControl::ExecuteGlobalCommand - Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);
		return Exit();
	}
	else if (_wcsicmp(pCurr, L"Key") == 0)
	{
		if (command->LHS.ParameterCount < 1)
			return (log << "TVControl::ExecuteGlobalCommand - Keycode parameter expected: " << command->LHS.Function << "\n").Show(E_FAIL);
		if (command->LHS.ParameterCount > 4)
			return (log << "TVControl::ExecuteGlobalCommand - Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if ((command->LHS.Parameter[0][0] == '\'') && (command->LHS.Parameter[0][2] == '\''))
			n1 = command->LHS.Parameter[0][1];
		else
			n1 = StringToLong(command->LHS.Parameter[0]);

		n2 = 0;
		if (command->LHS.ParameterCount >= 2)
			n2 = StringToLong(command->LHS.Parameter[1]);

		n3 = 0;
		if (command->LHS.ParameterCount >= 3)
			n3 = StringToLong(command->LHS.Parameter[2]);

		n4 = 0;
		if (command->LHS.ParameterCount >= 4)
			n4 = StringToLong(command->LHS.Parameter[3]);

		return Key(n1, n2, n3, n4);
	}
	if (_wcsicmp(pCurr, L"AlwaysOnTop") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return AlwaysOnTop(n1);
	}
	else if (_wcsicmp(pCurr, L"Fullscreen") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return Fullscreen(n1);
	}
	else if (_wcsicmp(pCurr, L"SetSource") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		return SetSource(command->LHS.Parameter[0]);
	}
	else if (_wcsicmp(pCurr, L"VolumeUp") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return VolumeUp(n1);
	}
	else if (_wcsicmp(pCurr, L"VolumeDown") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return VolumeDown(n1);
	}
	else if (_wcsicmp(pCurr, L"SetVolume") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return SetVolume(n1);
	}
	else if (_wcsicmp(pCurr, L"Mute") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return Mute(n1);
	}
	else if (_wcsicmp(pCurr, L"SetColorControls") == 0)
	{
		if (command->LHS.ParameterCount != 5)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 5 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		n2 = StringToLong(command->LHS.Parameter[1]);
		n3 = StringToLong(command->LHS.Parameter[2]);
		n4 = StringToLong(command->LHS.Parameter[3]);
		n5 = StringToLong(command->LHS.Parameter[4]);
		return SetColorControls(n1, n2, n3, n4, n5);
	}
	else if (_wcsicmp(pCurr, L"Zoom") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return Zoom(n1);
	}
	else if (_wcsicmp(pCurr, L"ZoomIn") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return ZoomIn(n1);
	}
	else if (_wcsicmp(pCurr, L"ZoomOut") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return ZoomOut(n1);
	}
	else if (_wcsicmp(pCurr, L"ZoomMode") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		return ZoomMode(n1);
	}
	else if (_wcsicmp(pCurr, L"AspectRatio") == 0)
	{
		if ((command->LHS.ParameterCount != 1) &&
			(command->LHS.ParameterCount != 3))
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 or 3 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		if (command->LHS.ParameterCount == 1)
			return AspectRatio(n1, 0, 0);

		n2 = StringToLong(command->LHS.Parameter[1]);
		n3 = StringToLong(command->LHS.Parameter[2]);
		return AspectRatio(n1, n2, n3);
	}
	else if (_wcsicmp(pCurr, L"ShowMenu") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ShowMenu(command->LHS.Parameter[0]);
	}
	else if (_wcsicmp(pCurr, L"ExitMenu") == 0)
	{
		if ((command->LHS.ParameterCount != 0) && (command->LHS.ParameterCount != 1))
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 0 or 1 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount == 0)
		{
			return ExitMenu();
		}
		else
		{
			n1 = StringToLong(command->LHS.Parameter[0]);
			return ExitMenu(n1);
		}
	}
	else if (_wcsicmp(pCurr, L"ShowOSDItem") == 0)
	{
		if ((command->LHS.ParameterCount != 1) && (command->LHS.ParameterCount != 2))
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount == 1)
		{
			return ShowOSDItem(command->LHS.Parameter[0]);
		}
		else
		{
			n1 = StringToLong(command->LHS.Parameter[1]);
			return ShowOSDItem(command->LHS.Parameter[0], n1);
		}
	}
	else if (_wcsicmp(pCurr, L"HideOSDItem") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		return HideOSDItem(command->LHS.Parameter[0]);
	}
	else if (_wcsicmp(pCurr, L"ToggleOSDItem") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ToggleOSDItem(command->LHS.Parameter[0]);
	}
	else if (_wcsicmp(pCurr, L"SetMultiple") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.application.multiple = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetDisableScreenSaver") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.application.disableScreenSaver = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetPriority") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.application.priority = g_pData->GetPriority(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetAddToROT") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.application.addToROT = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetStartFullscreen") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.startFullscreen = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetStartAlwaysOnTop") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.startAlwaysOnTop = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetStartAtLastWindowPosition") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.startAtLastWindowPosition = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetStartWithLastWindowSize") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.startWithLastWindowSize = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetRememberFullscreenState") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.rememberFullscreenState = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetRememberAlwaysOnTopState") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.rememberAlwaysOnTopState = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetRememberWindowPosition") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.rememberWindowPosition = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetQuietOnMinimise") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.window.quietOnMinimise = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetStartWithAudioMuted") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.audio.bMute = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetAspectRatioOverride") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.video.aspectRatio.bOverride = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetCaptureFormat") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.capture.format = g_pData->GetFormat(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetCaptureFolder") == 0)
	{
		if FAILED(GetFolder(g_pData->hWnd, L"Sets The File Capture Path", &g_pData->settings.capture.folder) == FALSE )
			return S_OK;
		
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetCaptureName") == 0)
	{
		if FAILED(GetInputBox(g_pData->hWnd, L"Sets The Capture Default File Name", &g_pData->settings.capture.fileName) == FALSE )
			return S_OK;
		
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetTimeShiftFormat") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.timeshift.format = g_pData->GetFormat(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetTimeShiftFolder") == 0)
	{
		if FAILED(GetFolder(g_pData->hWnd, L"Sets The Time Shift Buffer File Path", &g_pData->settings.timeshift.folder) == FALSE )
			return S_OK;
		
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetTimeShiftChange") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->SetChange(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetTimeShiftBuffer") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (!command->LHS.Parameter[0])
			return S_OK;

		if (_wcsicmp(command->LHS.Parameter[0], L"Auto") == 0)
		{
			LPWSTR pValue = NULL;
			strCopy(pValue, g_pData->settings.timeshift.bufferMinutes);

			if FAILED(GetInputBox(g_pData->hWnd, L"Sets The Time Shift Buffer Size in Minutes", &pValue) == FALSE )
			{
				if (pValue)
					delete[] pValue;

				return S_OK;
			}

			if (pValue)
			{
				g_pData->settings.timeshift.bufferMinutes = _wtoi(pValue);
				g_pData->settings.timeshift.bufferMinutes = _wtoi(pValue);
				strCopy(g_pData->settings.timeshift.buffer, command->LHS.Parameter[0]);
				delete[] pValue;
			}

		}
		g_pData->SetBuffer(command->LHS.Parameter[0]);

		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetMultiCard") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.application.multicard = g_pData->GetBool(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetDSNetworkFormat") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pData->settings.dsnetwork.format = g_pData->GetFormat(command->LHS.Parameter[0]);
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetDSNetworkIP") == 0)
	{
		if FAILED(GetInputBox(g_pData->hWnd, L"Sets The DSNetwork IP Address", &g_pData->settings.dsnetwork.ipaddr) == FALSE )
			return S_OK;
		
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetDSNetworkPort") == 0)
	{
		LPWSTR pValue;
		strCopy(pValue, g_pData->settings.dsnetwork.port);

		if FAILED(GetInputBox(g_pData->hWnd, L"Sets The DSNetwork Port Number", &pValue) == FALSE )
			return S_OK;

		if (pValue)
			g_pData->settings.dsnetwork.port = _wtoi(pValue);
		
		if (g_pData->settings.dsnetwork.port < 0 || g_pData->settings.dsnetwork.port > 65535)
			return S_OK;

		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetDSNetworkNic") == 0)
	{
		if FAILED(GetInputBox(g_pData->hWnd, L"Sets The DSNetwork NIC Address", &g_pData->settings.dsnetwork.nicaddr) == FALSE )
			return S_OK;
		
		return g_pData->SaveSettings();
	}
	else if (_wcsicmp(pCurr, L"SetTempBool") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		n2 = StringToLong(command->LHS.Parameter[1]);
		if (n1 > 9 || n1 < 0)
			return (log << "TVControl::ExecuteGlobalCommand - First Parameter is out of Range 0->9 : " << command->LHS.Function << "\n").Show(E_FAIL);
	
		g_pData->temps.bools[n1] = n2;
		return S_OK;
	}
	else if (_wcsicmp(pCurr, L"SetTempInt") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		n2 = StringToLong(command->LHS.Parameter[1]);
		if (n1 > 9 || n1 < 0)
			return (log << "TVControl::ExecuteGlobalCommand - First Parameter is out of Range 0->9 : " << command->LHS.Function << "\n").Show(E_FAIL);
	
		g_pData->temps.ints[n1] = n2;
		return S_OK;
	}
	else if (_wcsicmp(pCurr, L"SetTempLong") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		n2 = StringToLong(command->LHS.Parameter[1]);
		if (n1 > 9 || n1 < 0)
			return (log << "TVControl::ExecuteGlobalCommand - First Parameter is out of Range 0->9 : " << command->LHS.Function << "\n").Show(E_FAIL);
	
		g_pData->temps.longs[n1] = n2;
		return S_OK;
	}
	else if (_wcsicmp(pCurr, L"SetTempString") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);
		if (n1 > 9 || n1 < 0)
			return (log << "TVControl::ExecuteGlobalCommand - First Parameter is out of Range 0->9 : " << command->LHS.Function << "\n").Show(E_FAIL);
	
		if (command->LHS.Parameter[1] == NULL)
		{
			if(g_pData->temps.lpstr[n1])
			{
				delete [] g_pData->temps.lpstr[n1];
				g_pData->temps.lpstr[n1] = NULL;
			}
		}
		else
			strCopy(g_pData->temps.lpstr[n1], command->LHS.Parameter[1]);

		return S_OK;
	}
	else if (_wcsicmp(pCurr, L"SetDataItem") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.Parameter[0] == NULL || command->LHS.Parameter[1] == NULL)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 valid parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pOSD->Data()->SetItem(command->LHS.Parameter[0], command->LHS.Parameter[1]);
		return S_OK;
	}
	else if (_wcsicmp(pCurr, L"SetDVBTDeviceStatus") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.Parameter[0] == NULL || command->LHS.Parameter[1] == NULL)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 valid parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (m_pActiveSource && _wcsicmp(m_pActiveSource->GetSourceType(), L"BDA") == NULL)
		{
			return S_FALSE;
		}

		std::vector<DWSource *>::iterator it = m_sources.begin();
		for ( ; it < m_sources.end() ; it++ )
		{
			DWSource *source = *it;
			LPWSTR pType = source->GetSourceType();

			if (_wcsicmp(pType, L"BDA") == 0)
				return source->ExecuteCommand(command);
		}
		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"SetDVBTDevicePosition") == 0)
	{
		if (command->LHS.ParameterCount != 2)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.Parameter[0] == NULL || command->LHS.Parameter[1] == NULL)
			return (log << "TVControl::ExecuteGlobalCommand - Expecting 2 valid parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (m_pActiveSource && _wcsicmp(m_pActiveSource->GetSourceType(), L"BDA") == NULL)
		{
			return S_FALSE;
		}

		std::vector<DWSource *>::iterator it = m_sources.begin();
		for ( ; it < m_sources.end() ; it++ )
		{
			DWSource *source = *it;
			LPWSTR pType = source->GetSourceType();

			if (_wcsicmp(pType, L"BDA") == 0)
				return source->ExecuteCommand(command);
		}
		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"RemoveDVBTDevice") == 0)
	{
		if (m_pActiveSource && _wcsicmp(m_pActiveSource->GetSourceType(), L"BDA") == NULL)
		{
			return S_FALSE;
		}

		std::vector<DWSource *>::iterator it = m_sources.begin();
		for ( ; it < m_sources.end() ; it++ )
		{
			DWSource *source = *it;
			LPWSTR pType = source->GetSourceType();

			if (_wcsicmp(pType, L"BDA") == 0)
				return source->ExecuteCommand(command);
		}
		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"ParseDVBTDevices") == 0)
	{
		if (m_pActiveSource && _wcsicmp(m_pActiveSource->GetSourceType(), L"BDA") == NULL)
		{
			return S_FALSE;
		}

		std::vector<DWSource *>::iterator it = m_sources.begin();
		for ( ; it < m_sources.end() ; it++ )
		{
			DWSource *source = *it;
			LPWSTR pType = source->GetSourceType();

			if (_wcsicmp(pType, L"BDA") == 0)
				return source->ExecuteCommand(command);
		}
		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"SetMediaTypeDecoder") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 2)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return m_pFilterGraph->SetMediaTypeDecoder(n1, command->LHS.Parameter[1]);
	}


	/*
	if (_wcsicmp(pCurr, "NetworkUp") == 0)
	{
		return NetworkUp();
	}
	if (_wcsicmp(pCurr, "NetworkDown") == 0)
	{
		return NetworkDown();
	}
	if (_wcsicmp(pCurr, "ProgramUp") == 0)
	{
		return ProgramUp();
	}
	if (_wcsicmp(pCurr, "ProgramDown") == 0)
	{
		return ProgramDown();
	}
	if (_wcsicmp(pCurr, "LastChannel") == 0)
	{
		return LastChannel();
	}

	if (_wcsicmp(pCurr, "TVPlaying") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return TVPlaying(n1);
		}
	}
	if (_wcsicmp(pCurr, "Recording") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
		}
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			char filename[256];
			strcpy((char*)&filename, pCurr);
			return Recording(n1, (char*)&filename);
		}
		else
		{
			return Recording(n1);
		}
	}
	if (_wcsicmp(pCurr, "RecordingTimer") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return RecordingTimer(n1);
		}
	}
	if (_wcsicmp(pCurr, "RecordingPause") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return RecordingPause(n1);
		}
	}


	if (_wcsicmp(pCurr, "VideoDecoderEntry") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return VideoDecoderEntry(n1);
		}
	}
	if (_wcsicmp(pCurr, "AudioDecoderEntry") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return AudioDecoderEntry(n1);
		}
	}
	if (_wcsicmp(pCurr, "ResolutionEntry") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return ResolutionEntry(n1);
		}
	}

	if (_wcsicmp(pCurr, "Resolution") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
			n1 = atoi(pCurr);
		if (parseFunction.GetNextParam(pCurr) == TRUE)
			n2 = atoi(pCurr);
		if (parseFunction.GetNextParam(pCurr) == TRUE)
			n3 = atoi(pCurr);
		if (parseFunction.GetNextParam(pCurr) == TRUE)
			n4 = atoi(pCurr);
		if (parseFunction.GetNextParam(pCurr) == TRUE)
			n5 = atoi(pCurr);
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n6 = atoi(pCurr);
			return Resolution(n1, n2, n3, n4, (n5==0), (n6!=0));
		}
	}
	*/
	/*

	if (_wcsicmp(pCurr, "ShowFilterProperties") == 0)
	{
		return ShowFilterProperties();
	}

	if (_wcsicmp(pCurr, "TimeShift") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return TimeShift(n1);
		}
	}

	if (_wcsicmp(pCurr, "TimeShiftJump") == 0)
	{
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
			return TimeShiftJump(n1);
		}
	}
	*/
	/*
	else
	{
		n1 = 1;
		if (parseFunction.GetNextParam(pCurr) == TRUE)
		{
			n1 = atoi(pCurr);
		}

		switch (n1)
		{
		case 1:
			//m_pOSD->ShowEntry(GROUPID_NAN, OSD_ON);
			m_pOSD->ShowItem(pBuff);
			break;
		case 2:
			//m_pOSD->ShowEntry(GROUPID_NAN, OSD_TOGGLE);
			m_pOSD->ToggleItem(pBuff);
			break;
		default:
			//m_pOSD->ShowEntry(GROUPID_NAN, OSD_OFF);
			m_pOSD->HideItem(pBuff);
			break;
		}
	}
	*/
	return S_FALSE;
}
/*
int TVControl::GetFunctionType(char* strFunction)
{
	ParseFunctionLine parseFunction;

	int nResult = FT_UNKNOWN;

	//char buff[256];
	char* pBuff;
	//pBuff = (char *)&buff;
	pBuff = strFunction;

	//strcpy(pBuff, strFunction);
	while (pBuff[0] == ' ') pBuff++;

	if (parseFunction.Parse(pBuff) == TRUE)
	{
		if (_wcsicmp(pBuff, "SetChannel") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "ManualChannel") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "NetworkUp") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "NetworkDown") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "ProgramUp") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "ProgramDown") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "LastChannel") == 0)
			nResult = FT_CHANNEL_CHANGE;

		else if (_wcsicmp(pBuff, "TVPlaying") == 0)
			nResult = FT_CHANNEL_CHANGE;
		else if (_wcsicmp(pBuff, "Recording") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "RecordingTimer") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "VolumeUp") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "VolumeDown") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "SetVolume") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "Mute") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "VideoDecoderEntry") == 0)
			nResult = FT_BEFORE_START;
		else if (_wcsicmp(pBuff, "AudioDecoderEntry") == 0)
			nResult = FT_BEFORE_START;
		else if (_wcsicmp(pBuff, "ResolutionEntry") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "Resolution") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "SetColorControls") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "AlwaysOnTop") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "Fullscreen") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "Zoom") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "ZoomIn") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "ZoomOut") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "ZoomMode") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "AspectRatio") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "ShowFilterProperties") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "TimeShift") == 0)
			nResult = FT_AFTER_START;
		else if (_wcsicmp(pBuff, "TimeShiftJump") == 0)
			nResult = FT_AFTER_START;

		else if (_wcsicmp(pBuff, "Key") == 0)
			nResult = FT_ANY;

		else
			nResult = FT_ANY;

		parseFunction.UndoParse();
	}
	return nResult;
}
*/

/*
BOOL TVControl::SetChannel(int nNetwork, int nProgram)
{
	if (m_pFilterGraph->IsRecording())
	{
		return_FALSE_SHOWERROR("Cannot change channel while recording");
	}
	if ((nNetwork <= 0) || (nNetwork >= m_pAppData->tvChannels.MaxNetworkCount))
		return FALSE;
	if (m_pAppData->tvChannels.Networks[nNetwork].bValid == FALSE)
		return FALSE;

	long freq, bwid;
	long pnum, vpid, apid;
	BOOL ac3;

	m_pOSD->HideItem("Channel");

	if (nNetwork == m_pAppData->values.currTVNetwork)
	{
		if ((nProgram == m_pAppData->values.currTVProgram) || (nProgram == -1))
		{
			//Show OSD if selecting same channel
			m_pOSD->ShowItem("Channel");

			return TRUE;
		}
		if (nProgram == -2)
		{
			//Select next program
			nProgram = m_pAppData->values.currTVProgram;
			do
			{
				nProgram++;
				if (nProgram >= m_pAppData->tvChannels.Networks[nNetwork].MaxProgramCount)
					nProgram = 0;
			}
			while (m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].bValid == FALSE);
		}
	}
	if ((nProgram < 0) || (nProgram >= m_pAppData->tvChannels.Networks[nNetwork].MaxProgramCount))
		nProgram = m_pAppData->tvChannels.Networks[nNetwork].DefaultProgram;
	if ((nProgram < 0) || (nProgram >= m_pAppData->tvChannels.Networks[nNetwork].MaxProgramCount))
		nProgram = 1;

	if (m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].bValid == FALSE)
		return FALSE;

	freq = m_pAppData->tvChannels.Networks[nNetwork].Frequency;
	bwid = m_pAppData->tvChannels.Networks[nNetwork].Bandwidth;
	pnum = m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].ProgramNumber;
	vpid = m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].VideoPid;
	apid = m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].AudioPid;
	ac3  = m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].AudioPidAC3;

	while (1)
	{
		m_pAppData->bChannelLocked = FALSE;
		m_pAppData->bBuilt = FALSE;
		if ((m_pAppData->values.currTVNetwork != nNetwork) || (m_pAppData->values.currTVProgram != nProgram))
		{
			m_pAppData->values.lastTVNetwork = m_pAppData->values.currTVNetwork;
			m_pAppData->values.lastTVProgram = m_pAppData->values.currTVProgram;
			m_pAppData->values.currTVNetwork = nNetwork;
			m_pAppData->values.currTVProgram = nProgram;
		}

		m_pOSD->ShowItem("Channel");

		m_pAppData->RestoreMarkedChanges();
		m_pAppData->StoreGlobalValues();

		if (m_pAppData->values.selectedVideoDecoder != m_pAppData->VideoDecoders.GetCurrent())
			VideoDecoderEntry(m_pAppData->values.selectedVideoDecoder);
		if (m_pAppData->values.selectedAudioDecoder != m_pAppData->AudioDecoders.GetCurrent())
			AudioDecoderEntry(m_pAppData->values.selectedAudioDecoder);

		//Do BEFORE functions
		for (int prefunc=0 ; prefunc<m_pAppData->tvChannels.Networks[nNetwork].FunctionsCount ; prefunc++)
		{
			int type = GetFunctionType(m_pAppData->tvChannels.Networks[nNetwork].Functions[prefunc]);
			if (type == FT_BEFORE_START)
				ExecuteCommand(m_pAppData->tvChannels.Networks[nNetwork].Functions[prefunc]);
		}
		for ( prefunc=0 ; prefunc<m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].FunctionsCount ; prefunc++)
		{
			int type = GetFunctionType(m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].Functions[prefunc]);
			if (type == FT_BEFORE_START)
				ExecuteCommand(m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].Functions[prefunc]);
		}

		if (!m_pFilterGraph->Stop())
			return_FALSE_SHOWERROR("Error Stopping");
		if (!m_pFilterGraph->Cleanup())
			return_FALSE_SHOWERROR("Error Unbuilding");
		if (!m_pDVBInput->LockChannel(freq, bwid))
			return_FALSE_SHOWERROR("Error Locking Channel");
		if (!m_pDVBInput->SetPid(pnum,vpid,apid,ac3))
			return_FALSE_SHOWERROR("Error Setting PID's");
		m_pAppData->bChannelLocked = TRUE;
		static int buildCount = 0;
		buildCount = 0;
		if (!m_pFilterGraph->Build(buildCount))
			return_FALSE_SHOWERROR("Error Building");
		if (buildCount >= 1)
			break;
		m_pAppData->bBuilt = TRUE;
		if (!m_pFilterGraph->Start())
			return_FALSE_SHOWERROR("Error Playing");
		if (!m_pFilterGraph->RefreshVideoPosition())
			return_FALSE_SHOWERROR("Error Refreshing Video");

		buildCount++;

		m_pOSD->GetStateInformation()->NaNCollClear();

		//Do AFTER functions
		for (int postfunc=0 ; postfunc<m_pAppData->tvChannels.Networks[nNetwork].FunctionsCount ; postfunc++)
		{
			int type = GetFunctionType(m_pAppData->tvChannels.Networks[nNetwork].Functions[postfunc]);
			if ((type == FT_AFTER_START) || (type == FT_ANY))
				ExecuteCommand(m_pAppData->tvChannels.Networks[nNetwork].Functions[postfunc]);
		}
		for ( postfunc=0 ; postfunc<m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].FunctionsCount ; postfunc++)
		{
			int type = GetFunctionType(m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].Functions[postfunc]);
			if ((type == FT_AFTER_START) || (type == FT_ANY))
				ExecuteCommand(m_pAppData->tvChannels.Networks[nNetwork].Programs[nProgram].Functions[postfunc]);
		}

		m_pFilterGraph->Mute(m_pAppData->values.bMute);

		m_pAppData->MarkChanges();

		return TRUE;
	}
	m_pFilterGraph->Stop();

	return FALSE;
}

BOOL TVControl::ManualChannel(int nFrequency, int nVPid, int nAPid, BOOL bAC3)
{
	int nPNum = 0;
	int Network = m_pAppData->tvChannels.FindNetwork(nFrequency);
	int Program = m_pAppData->tvChannels.FindProgram(nFrequency, nVPid, nAPid);
	if ((Network >= 0) && (Program >= 0))
		return SetChannel(Network, Program);

	if (m_pFilterGraph->IsRecording())
	{
		return_FALSE_SHOWERROR("Cannot change channel while recording");
	}

	while (1)
	{
		m_pAppData->bChannelLocked = FALSE;

		m_pAppData->values.lastTVNetwork = m_pAppData->values.currTVNetwork;
		m_pAppData->values.lastTVProgram = m_pAppData->values.currTVProgram;
		m_pAppData->values.currTVNetwork = 0;
		m_pAppData->values.currTVProgram = 0;

		m_pOSD->ShowItem("Channel");

		if (m_pAppData->values.selectedVideoDecoder != m_pAppData->VideoDecoders.GetCurrent())
			VideoDecoderEntry(m_pAppData->values.selectedVideoDecoder);
		if (m_pAppData->values.selectedAudioDecoder != m_pAppData->AudioDecoders.GetCurrent())
			AudioDecoderEntry(m_pAppData->values.selectedAudioDecoder);

		if (!m_pFilterGraph->Stop()						) return_FALSE_SHOWERROR("Error Stopping");
		if (!m_pDVBInput->LockChannel(nFrequency, 7)	) return_FALSE_SHOWERROR("Error Locking Channel");
		if (!m_pDVBInput->SetPid(nPNum,nVPid,nAPid,bAC3)) return_FALSE_SHOWERROR("Error Setting PID's");
		m_pAppData->bChannelLocked = TRUE;
		if (!m_pFilterGraph->Build()					) return FALSE;//SHOWERROR("Error Playing");
		//if (!m_pFilterGraph->Start()					) return FALSE;
		//if (!m_pFilterGraph->RefreshVideoPosition()	) return FALSE;//SHOWERROR("Error Refreshing Video");

		return TRUE;
	}
	m_pFilterGraph->Stop();

	return FALSE;
}

BOOL TVControl::NetworkUp()
{
	int value = m_pAppData->values.currTVNetwork;
	do
	{
		value++;
		if (value >= m_pAppData->tvChannels.MaxNetworkCount)
			value = 0;
	} while (m_pAppData->tvChannels.Networks[value].bValid == FALSE);
	return SetChannel(value, -1);
}

BOOL TVControl::NetworkDown()
{
	int value = m_pAppData->values.currTVNetwork;
	do
	{
		value--;
		if (value < 0)
			value = m_pAppData->tvChannels.MaxNetworkCount-1;
	} while (m_pAppData->tvChannels.Networks[value].bValid == FALSE);
	return SetChannel(value, -1);
}

BOOL TVControl::ProgramUp()
{
	int network = m_pAppData->values.currTVNetwork;
	while (m_pAppData->tvChannels.Networks[network].bValid == FALSE)
	{
		network++;
		if (network >= m_pAppData->tvChannels.MaxNetworkCount)
			network = 0;
	}

	int value = m_pAppData->values.currTVProgram;
	do
	{
		value++;
		if (value >= m_pAppData->tvChannels.Networks[network].MaxProgramCount)
			value = 0;
	} while (m_pAppData->tvChannels.Networks[network].Programs[value].bValid == FALSE);
	return SetChannel(network, value);
}

BOOL TVControl::ProgramDown()
{
	int network = m_pAppData->values.currTVNetwork;
	while (m_pAppData->tvChannels.Networks[network].bValid == FALSE)
	{
		network++;
		if (network >= m_pAppData->tvChannels.MaxNetworkCount)
			network = 0;
	}

	int value = m_pAppData->values.currTVProgram;
	do
	{
		value--;
		if (value <= 0)
			value = m_pAppData->tvChannels.Networks[network].MaxProgramCount-1;
	} while (m_pAppData->tvChannels.Networks[network].Programs[value].bValid == FALSE);
	return SetChannel(network, value);
}

BOOL TVControl::LastChannel()
{
	return SetChannel(m_pAppData->values.lastTVNetwork, m_pAppData->values.lastTVProgram);
}


BOOL TVControl::TVPlaying(int nPlaying)
{
	if ((nPlaying == 1) || ((nPlaying == 2) && !m_pFilterGraph->IsPlaying()))
	{
		if (!m_pFilterGraph->Start()				) return FALSE;
		if (!m_pFilterGraph->RefreshVideoPosition()	) return FALSE;
		return TRUE;
	}
	else
	{
		if (m_pFilterGraph->IsRecording())
		{
			return_FALSE_SHOWERROR("Cannot stop playing while recording");
		}

		return m_pFilterGraph->Stop();
	}
}

BOOL TVControl::Recording(int nRecording, char* filename)
{
	if ((nRecording == 1) || ((nRecording == 2) && !m_pFilterGraph->IsRecording()))
	{
		if (m_pAppData->bChannelLocked == FALSE)
		{
			return_FALSE_SHOWERROR("Can't Record. No channel selected");
		}

		BOOL bResult;
		if (filename != NULL)
		{
			bResult = m_pFilterGraph->StartRecording(filename);
		}
		else
		{
			char realFilename[512];
			m_pAppData->ReplaceTokens(m_pAppData->captureFileName, (char*)&realFilename, 512);
			bResult = m_pFilterGraph->StartRecording(realFilename);
		}

		if (bResult)
		{
			m_pAppData->bRecording = TRUE;
			m_pAppData->bRecordingPaused = FALSE;
			m_pOSD->ShowItem("Recording");
			return TRUE;
		}
	}
	else
	{
		KillTimer(m_pAppData->hWnd, 990);
		m_pAppData->recordingTimeLeft = 0;
		m_pAppData->bRecording = FALSE;
		m_pAppData->bRecordingPaused = FALSE;
		if (m_pFilterGraph->StopRecording())
		{
			m_pOSD->ShowItem("RecordingStopped");
			return TRUE;
		}
	}
	return FALSE;
}

BOOL TVControl::RecordingTimer(int nAddTime)
{
	if (m_pFilterGraph->IsRecording())
	{
		m_pAppData->recordingTimeLeft += nAddTime*60;
		m_pOSD->ShowItem("Recording");
		SetTimer(m_pAppData->hWnd, 990, 1000, NULL);
		return TRUE;
	}
	else
	{
		m_pAppData->recordingTimeLeft = 0;
		BOOL bResult = Recording(1);
		SetTimer(m_pAppData->hWnd, 990, 1000, NULL);
		return bResult;
	}
}

BOOL TVControl::RecordingPause(int nPause)
{
	if ((nPause == 1) || ((nPause == 2) && !m_pFilterGraph->IsRecordingPaused()))
	{
		if (m_pFilterGraph->IsRecording() == FALSE)
		{
			return_FALSE_SHOWERROR("Can't Pause Recording when not recording");
		}

		if (m_pFilterGraph->PauseRecording(TRUE))
		{
			m_pAppData->bRecordingPaused = TRUE;
			m_pOSD->ShowItem("RecordingPaused");
			return TRUE;
		}
	}
	else
	{
		if (m_pFilterGraph->IsRecording())
		{
			if (m_pFilterGraph->PauseRecording(FALSE))
			{
				m_pAppData->bRecordingPaused = FALSE;
				m_pOSD->ShowItem("RecordingUnpaused");
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL TVControl::VideoDecoderEntry(int nIndex)
{
	BOOL bResult;
	if (nIndex < 0)
		bResult = m_pAppData->VideoDecoders.Next();
	else
		bResult = m_pAppData->VideoDecoders.SetCurrent(nIndex);
	if (bResult)
		m_pOSD->ShowItem("VideoDecoder");
	m_pAppData->values.selectedVideoDecoder = m_pAppData->VideoDecoders.GetCurrent();
	return bResult;
}

BOOL TVControl::AudioDecoderEntry(int nIndex)
{
	BOOL bResult;
	if (nIndex < 0)
		bResult = m_pAppData->AudioDecoders.Next();
	else
		bResult = m_pAppData->AudioDecoders.SetCurrent(nIndex);
	if (bResult)
		m_pOSD->ShowItem("AudioDecoder");
	m_pAppData->values.selectedAudioDecoder = m_pAppData->AudioDecoders.GetCurrent();
	return bResult;
}

BOOL TVControl::ResolutionEntry(int nIndex)
{
	if (m_pAppData->values.bFullScreen)
		return FALSE;
	BOOL bResult;
	if (nIndex < 0)
		bResult = m_pAppData->resolutions.Next();
	else
		bResult = m_pAppData->resolutions.SetCurrent(nIndex);
	if (bResult)
		Resolution(m_pAppData->resolutions.Current()->left,
				   m_pAppData->resolutions.Current()->top,
				   m_pAppData->resolutions.Current()->width,
				   m_pAppData->resolutions.Current()->height,
				   m_pAppData->resolutions.Current()->move,
				   m_pAppData->resolutions.Current()->size);
	return TRUE;
}

BOOL TVControl::ShowFilterProperties()
{
	ShowCursor(FALSE);
	BOOL bResult = m_pFilterGraph->ShowFilterProperties();
	ShowCursor();
	return bResult;
}

BOOL TVControl::TimeShift(int nMode)
{
	BOOL bResult;
	if (((nMode == 1) || ((nMode == 2) && !m_pDVBInput->GetTimeShiftMode())))
		bResult = (m_pDVBInput->TimeShiftPlay() != 0);
	else
		bResult = (m_pDVBInput->TimeShiftPause() == 0);
	if (bResult)
		m_pOSD->ShowItem("TimeShift");
	return bResult;
}

BOOL TVControl::TimeShiftJump(int nSeconds)
{
	if (nSeconds == 0)
		return false;
	if (m_pDVBInput->TimeShiftJump(nSeconds))
	{
		m_pAppData->TimeShiftJump = nSeconds;
		m_pOSD->ShowItem("TimeShiftJump");
		return true;
	}
	else
	{
		return_FALSE_SHOWERROR("Seek Failed");
	}
}
*/

/*
BOOL TVControl::ControlBarMouseDown(int x, int y)
{
	if (m_pControlBar->MouseDown(x, y))
	{
		char function[256];
		if (m_pControlBar->GetFunction((char*)&function))
			return ExecuteCommand(function);
	}
	return FALSE;
}

BOOL TVControl::ControlBarMouseUp(int x, int y)
{
	m_pControlBar->MouseUp(x, y);
	return FALSE;
}

BOOL TVControl::ControlBarMouseMove(int x, int y)
{
	return m_pControlBar->MouseMove(x, y);
}

*/

BOOL TVControl::ShowCursor(BOOL bAllowHide)
{
	KillTimer(g_pData->hWnd, TIMER_AUTO_HIDE_CURSOR);
	if (g_pData->values.window.bFullScreen && bAllowHide)
	{
		SetTimer(g_pData->hWnd, TIMER_AUTO_HIDE_CURSOR, 3000, NULL);
	}
	if (!g_pData->application.bCursorVisible)
	{
		::ShowCursor(TRUE);
		g_pData->application.bCursorVisible = TRUE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL TVControl::HideCursor()
{
	KillTimer(g_pData->hWnd, TIMER_AUTO_HIDE_CURSOR);
	if (g_pData->application.bCursorVisible)
	{
		while (::ShowCursor(FALSE) >= 0);
		g_pData->application.bCursorVisible = FALSE;

		//m_pControlBar->Hide();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

HRESULT TVControl::OnPaint()
{
/*	if (m_pFilterGraph)
	{
		RECT gcr, gvr, rect;
		m_pFilterGraph->GetVideoRect(&gvr);

		HBRUSH br = CreateSolidBrush(0x00000000);

		HDC hdc;
		hdc = GetDC(g_pData->hWnd);

		GetClientRect(g_pData->hWnd, &gcr);

		if (gvr.left   < gcr.left  ) gvr.left   = gcr.left;
		if (gvr.right  > gcr.right ) gvr.right  = gcr.right;
		if (gvr.top    < gcr.top   ) gvr.top    = gcr.top;
		if (gvr.bottom > gcr.bottom) gvr.bottom = gcr.bottom;

		if (gvr.top > gcr.top)
		{
			CopyRect(&rect, &gcr);
			rect.bottom = gvr.top;
			FillRect(hdc, &rect, br);
		}
		if (gvr.bottom < gcr.bottom)
		{
			CopyRect(&rect, &gcr);
			rect.top = gvr.bottom;
			FillRect(hdc, &rect, br);
		}
		if (gvr.left > gcr.left)
		{
			CopyRect(&rect, &gcr);
			rect.right = gvr.left;
			rect.top = gvr.top;
			rect.bottom = gvr.bottom;
			FillRect(hdc, &rect, br);
		}
		if (gvr.right < gcr.right)
		{
			CopyRect(&rect, &gcr);
			rect.left = gvr.right;
			rect.top = gvr.top;
			rect.bottom = gvr.bottom;
			FillRect(hdc, &rect, br);
		}

		DeleteObject(br);
		ReleaseDC(g_pData->hWnd, hdc);
	}
*/
	return S_OK;
}

HRESULT TVControl::OnSizing(long fwSide, LPRECT rect)
{
	if ((GetKeyState(VK_SHIFT) & 0x80) != 0)
	{
		g_pData->values.window.aspectRatio.width = rect->right - rect->left;
		g_pData->values.window.aspectRatio.height = rect->bottom - rect->top;
	}
	else
	{
		int newSize;
		switch (fwSide)
		{
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
			newSize = (rect->right-rect->left);
			newSize = newSize * g_pData->values.window.aspectRatio.height / g_pData->values.window.aspectRatio.width;
			rect->bottom = rect->top + newSize;
			break;

		case WMSZ_TOP:
		case WMSZ_BOTTOM:
			newSize = (rect->bottom-rect->top);
			newSize = newSize * g_pData->values.window.aspectRatio.width / g_pData->values.window.aspectRatio.height;
			rect->right = rect->left + newSize;
			break;

		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			newSize = (rect->right-rect->left);
			newSize = newSize * g_pData->values.window.aspectRatio.height / g_pData->values.window.aspectRatio.width;
			rect->top = rect->bottom - newSize;
			break;
		};
	}

	return S_OK;
}

HRESULT TVControl::OnSize()
{
	HRESULT hr = OnMove();

	if (m_pActiveSource)
		hr = m_pActiveSource->GetFilterGraph()->RefreshVideoPosition(m_pActiveSource->GetGraphBuilder());
	else if (m_pFilterGraph)
		hr = m_pFilterGraph->RefreshVideoPosition();

	return hr;
}

HRESULT TVControl::OnMove()
{
	if ((m_windowInitialiseState == WIS_INITIALISED) && (g_pData->values.window.bFullScreen == FALSE))
	{
		//Update position and size values using new position and/or size
		RECT rect;
		POINT point;

		::GetClientRect(g_pData->hWnd, &rect);
		point.x = rect.left;
		point.y = rect.top;
		::ClientToScreen(g_pData->hWnd, &point);
		g_pData->values.window.position.x = point.x;
		g_pData->values.window.position.y = point.y;
		g_pData->values.window.size.width = rect.right - rect.left;
		g_pData->values.window.size.height = rect.bottom - rect.top;
	}
	return S_OK;
}

HRESULT TVControl::OnTimer(int wParam)
{
	switch (wParam)
	{
	case TIMER_RECORDING_TIMELEFT:	//Every second while recording
		//TODO: Create DWSource::RecordingTimeLeftUpdate()
		return S_OK;
/*
	case 996:	//every 100ms the whole time the program is running.
		m_pTv->m_pOSD->RepaintOSD(TRUE);
		return S_OK;
	case 997:	//Every second the whole time the program is running.
		m_pTv->m_pOSD->Update();
		return S_OK;
*/
	case TIMER_DISABLE_POWER_SAVING:	//Every 30 seconds to keep power saving coming on.
		SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		return S_OK;
	case TIMER_AUTO_HIDE_CURSOR:	//3 seconds after mouse movement
		KillTimer(g_pData->hWnd, TIMER_AUTO_HIDE_CURSOR);
		if (g_pData->values.window.bFullScreen)
			HideCursor();
		return S_OK;
	}
	return S_FALSE;
}

HRESULT TVControl::OnMinimize()
{
	if (!g_pData->settings.window.quietOnMinimise)
		return S_OK;

	HRESULT hr = S_OK;

	{
		ParseLine command;
		command.Parse(L"CloseBuffers");
		if (m_pActiveSource)
			hr = m_pActiveSource->ExecuteCommand(&command);
	}
	ParseLine command;
	command.Parse(L"CloseDisplay");
	if (m_pActiveSource)
		hr = m_pActiveSource->ExecuteCommand(&command);

	return hr;
}

HRESULT TVControl::OnRestore()
{
//	if (!g_pData->settings.window.quietOnMinimise)
//		return S_OK;

	HRESULT hr = S_OK;

	ParseLine command;
	command.Parse(L"OpenDisplay");
	if (m_pActiveSource)
		hr = m_pActiveSource->ExecuteCommand(&command);

	return hr;
}

HRESULT TVControl::MinimiseScreen()
{
	WINDOWPLACEMENT wPlace;
	GetWindowPlacement(g_pData->hWnd, &wPlace);
	wPlace.showCmd = SW_MINIMIZE;

	BOOL bResult = SetWindowPlacement(g_pData->hWnd, &wPlace);
	return (bResult ? S_OK : E_FAIL);
}



// We use this method to add commands to a queue to be processed by
// a separate thread so that our main message loop doesn't stall for
// long operations like changing channels. This should make the app
// seem more responsive and will mean that the OSD continues to
// update also.
void TVControl::ExecuteCommandsQueue(LPCWSTR command)
{
	(log << "Queue Command '" << command << "'\n").Write();

	LPWSTR newCommand = NULL;
	strCopy(newCommand, command);
	m_commandQueue.push_back(newCommand);
}

void TVControl::StartCommandQueueThread()
{
	if FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))
	{
		(log << "Failed to initialize COM in the command queue thread\n").Write();
		return;
	}

	DWORD result = 0;
	MSG msg;
	msg.wParam = 0;
	BOOL bGotMsg;

	do
	{
		bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

		if (bGotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			try
			{
				if (m_commandQueue.size() > 0)
				{
					LPWSTR command = m_commandQueue.front();
					ExecuteCommandsImmediate(command);
					m_commandQueue.pop_front();
					delete[] command;
				}
			}
			catch (LPWSTR str)
			{
				(log << "Error caught in command queue thread: " << str << "\n").Show();
			}
		}
		result = WaitForSingleObject(m_hCommandProcessingStopEvent, 10);
	} while (result == WAIT_TIMEOUT);
	if (result != WAIT_OBJECT_0)
	{
		DWORD err = GetLastError();
		(log << "Thread's WaitForSingleObject error: " << (int)err << "\n").Write();
	}
	
	SetEvent(m_hCommandProcessingDoneEvent);
}

HRESULT TVControl::GetFilterGraph(DWGraph **ppDWGraph)
{
	if (!ppDWGraph)
		return E_POINTER;

	if(m_pActiveSource)
		*ppDWGraph = m_pActiveSource->GetFilterGraph();
	else
		*ppDWGraph = m_pFilterGraph;

	return S_OK;
}

struct InputBox_Param
{
	LPWSTR lpwTitle;
	LPWSTR *lpwInput;
}INPUTBOX_PARAM;

BOOL CALLBACK GetInputBoxBack(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
static InputBox_Param *param;

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			param = (InputBox_Param*)lParam;
			SetDlgItemTextW(hDlg, IDC_INPUTBOX_PROMPT, param->lpwTitle);
			SetDlgItemTextW(hDlg, IDC_INPUTBOX_DLG_EDIT1, *param->lpwInput);
			return TRUE;
		}

		case WM_DESTROY:
		{
			DestroyWindow(hDlg);
			return TRUE;
		}
		case WM_CLOSE:
		{
			::EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}

		case WM_COMMAND:
		{
			BOOL checked = FALSE;
			switch (LOWORD (wParam))
			{
				case IDCANCEL :
				{
					::EndDialog(hDlg, IDCANCEL);
					break ;
				}

				case IDOK :
				{
					GetDlgItemTextW(hDlg, IDC_INPUTBOX_DLG_EDIT1, (*param->lpwInput), MAX_PATH);
					::EndDialog(hDlg, IDOK);
					break ;
				}
			};
			return TRUE;
		}
		default:
			return FALSE;
	}
	return TRUE;
}

HRESULT TVControl::GetInputBox(HWND hwnd, LPWSTR lpwTitle, LPWSTR *lpwName)
{
	InputBox_Param param;
	param.lpwInput = lpwName;
	param.lpwTitle = lpwTitle;

	HMODULE hModule = ::GetModuleHandle(0);
	HINSTANCE hInst = hModule;
	HRSRC hrsrc = ::FindResource(hModule, MAKEINTRESOURCE(IDD_INPUTBOX), RT_DIALOG);
	HGLOBAL hglobal = ::LoadResource(hModule, hrsrc);

	if (::DialogBoxIndirectParam(hInst, (LPCDLGTEMPLATE) hglobal, hwnd, (DLGPROC)GetInputBoxBack, (LPARAM)&param) != TRUE)
		return S_FALSE;

	return S_OK;
}

BOOL CALLBACK GetFolderCallBack(HWND hwnd, UINT msg, LPARAM lpType, LPARAM pData)
{
	char path[MAX_PATH];
	switch(msg)
	{
		case BFFM_INITIALIZED:
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
			break;

		case BFFM_SELCHANGED: 
			if (SHGetPathFromIDList((LPITEMIDLIST)lpType , path)) 
				SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)path);	

			break;
	}

	return 0;
}

HRESULT TVControl::GetFolder(HWND hwnd, LPWSTR lpwTitle, LPWSTR *lpwFolder)
{
	HRESULT hr = S_FALSE;

	TCHAR tmpTitle[MAX_PATH];
	LPTSTR ptTitle = (LPTSTR)&tmpTitle;
	sprintf(ptTitle, "%S", lpwTitle);

	TCHAR tmpFolder[MAX_PATH];
	LPTSTR ptFolder = (LPTSTR)&tmpFolder;
	sprintf(ptFolder, "%S", (*lpwFolder));

	LPMALLOC lpMalloc;
	hr = SHGetMalloc(&lpMalloc);
    if SUCCEEDED(hr) 
	{
		BROWSEINFO browseInfo;
		browseInfo.hwndOwner = hwnd;
		browseInfo.pidlRoot = NULL;
		browseInfo.pszDisplayName = NULL;
		browseInfo.lpszTitle = ptTitle;
		browseInfo.ulFlags = BIF_STATUSTEXT; //BIF_EDITBOX 
		browseInfo.lpfn = GetFolderCallBack;
		browseInfo.lParam = (LPARAM)ptFolder;

		char path[MAX_PATH + 1];
		LPITEMIDLIST pidList = SHBrowseForFolder(&browseInfo);
		if (pidList)
		{
			if (SHGetPathFromIDList(pidList, path)) 
			{
				USES_CONVERSION;
				strCopy((*lpwFolder), T2W(path));
				hr = S_OK;
			}
			else 
				hr = S_FALSE;

			lpMalloc->Free(pidList);
			lpMalloc->Release();
		}
	}
	return hr;
}

