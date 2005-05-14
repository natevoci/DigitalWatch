/**
 *	DWOnScreenDisplay.cpp
 *	Copyright (C) 2005 Nate
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

#include "DWOnScreenDisplay.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSD
//////////////////////////////////////////////////////////////////////

DWOnScreenDisplay::DWOnScreenDisplay()
{
	m_pBackSurface = new DWSurface();
	m_pDirectDraw = new DWDirectDraw();
	m_pOverlayWindow = NULL;
	m_pCurrentWindow = NULL;
}

DWOnScreenDisplay::~DWOnScreenDisplay()
{
	delete m_pDirectDraw;
	m_pDirectDraw = NULL;
}

void DWOnScreenDisplay::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	windows.SetLogCallback(callback);
	m_pBackSurface->SetLogCallback(callback);
	m_pDirectDraw->SetLogCallback(callback);
}

HRESULT DWOnScreenDisplay::Initialise()
{
	HRESULT hr;
	m_pBackSurface->Destroy();

	hr = m_pDirectDraw->Init(g_pData->hWnd);
	if FAILED(hr)
		return (log << "Failed to initialise DWDirectDraw: " << hr << "\n").Write(hr);

	hr = m_pBackSurface->CreateFromDirectDrawBackSurface();
	if (FAILED(hr))
		return (log << "Failed to create DWSurface from directdraw back surface : " << hr << "\n").Write(hr);

	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%s%s", g_pData->application.appPath, L"OSD.xml");
	if FAILED(hr = windows.Load((LPWSTR)&file))
		return hr;

	m_pOverlayWindow = windows.GetWindow(L"Overlay");
	m_pCurrentWindow = m_pOverlayWindow;

	return S_OK;
}

HRESULT DWOnScreenDisplay::Render(long tickCount)
{
	HRESULT hr;

	m_pDirectDraw->SetTickCount(tickCount);

	hr = m_pDirectDraw->Clear();
	if FAILED(hr)
		return (log << "Failed to clear directdraw: " << hr << "\n").Write(hr);

	if (m_pCurrentWindow != m_pOverlayWindow)
	{
		if (!m_pCurrentWindow->HideOverlay())
			m_pOverlayWindow->Render(tickCount);
	}
	m_pCurrentWindow->Render(tickCount);

	//Display FPS
	DWSurfaceText text;
	text.crTextColor = RGB(255, 255, 255);

	wchar_t buffer[30];
	swprintf((LPWSTR)&buffer, L"FPS - %f", m_pDirectDraw->GetFPS());
	strCopy(text.text, buffer);
	hr = m_pBackSurface->DrawText(&text, 0, 560);
	hr = m_pBackSurface->DrawText(&text, 700, 560);
	
	//Flip
	hr = m_pDirectDraw->Flip();
	if FAILED(hr)
		return (log << "Failed to flip directdraw: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWOnScreenDisplay::ShowMenu(LPWSTR szMenuName)
{
	DWOSDWindow *window = windows.GetWindow(szMenuName);
	if (window)
	{
		if (m_pCurrentWindow != m_pOverlayWindow)
			m_windowStack.push_back(m_pCurrentWindow);
		m_pCurrentWindow = window;
		return S_OK;
	}
	return S_FALSE;
}

HRESULT DWOnScreenDisplay::ExitMenu(long nNumberOfMenusToExit)
{
	if (nNumberOfMenusToExit < 0)
	{
		if (m_pCurrentWindow == m_pOverlayWindow)
			return S_FALSE;
		m_windowStack.clear();
		m_pCurrentWindow = m_pOverlayWindow;
		return S_OK;
	}
	for (int i=0 ; i<nNumberOfMenusToExit ; i++ )
	{
		if (m_windowStack.size() > 0)
		{
			m_pCurrentWindow = m_windowStack.back();
			m_windowStack.pop_back();
		}
		else
		{
			m_pCurrentWindow = m_pOverlayWindow;
			break;
		}
	}
	return (i<nNumberOfMenusToExit) ? S_FALSE : S_OK;
}

DWDirectDraw* DWOnScreenDisplay::get_DirectDraw()
{
	return m_pDirectDraw;
}

DWOSDWindow* DWOnScreenDisplay::Overlay()
{
	return m_pOverlayWindow;
}

DWSurface* DWOnScreenDisplay::get_BackSurface()
{
	return m_pBackSurface;
}

HRESULT DWOnScreenDisplay::GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function)
{
	if (m_pCurrentWindow == NULL)
		return (log << "DWOnScreenDisplay::GetKeyFunction, current window is not set\n").Write(E_FAIL);

	return m_pCurrentWindow->GetKeyFunction(keycode, shift, ctrl, alt, function);
}

HRESULT DWOnScreenDisplay::ExecuteCommand(ParseLine* command)
{
	(log << "DWOnScreenDisplay::ExecuteCommand - " << command->LHS.Function << "\n").Write();
	LogMessageIndent indent(&log);

	if (m_pCurrentWindow == NULL)
		return (log << "DWOnScreenDisplay::ExecuteCommand, current window is not set\n").Write(S_FALSE);

	LPWSTR pCurr = command->LHS.FunctionName;

	if (_wcsicmp(pCurr, L"Up") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return m_pCurrentWindow->OnUp();
	}
	else if (_wcsicmp(pCurr, L"Down") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return m_pCurrentWindow->OnDown();
	}
	else if (_wcsicmp(pCurr, L"Left") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return m_pCurrentWindow->OnLeft();
	}
	else if (_wcsicmp(pCurr, L"Right") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return m_pCurrentWindow->OnRight();
	}
	else if (_wcsicmp(pCurr, L"Select") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return m_pCurrentWindow->OnSelect();
	}
	return S_FALSE;
}

///////////////////////////////
// Stuff for controls to use //
///////////////////////////////
DWOSDImage* DWOnScreenDisplay::GetImage(LPWSTR pName)
{
	return windows.GetImage(pName);
}

