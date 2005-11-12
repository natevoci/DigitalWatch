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
#include "DirectDraw/DWRendererDirectDraw.h"
#include "VMR9Bitmap/DWRendererVMR9Bitmap.h"

//////////////////////////////////////////////////////////////////////
// DWOSD
//////////////////////////////////////////////////////////////////////

DWOnScreenDisplay::DWOnScreenDisplay()
{
	m_pOverlayWindow = NULL;
	m_pCurrentWindow = NULL;
	
	m_pData = new DWOSDData(&windows);

	m_pRenderer = NULL;
	m_pMainSurface = new DWSurface();

	m_renderMethod = RENDER_METHOD_NONE;
	m_renderMethodChangeCount = 0;
}

DWOnScreenDisplay::~DWOnScreenDisplay()
{
	if (m_pMainSurface)
		delete m_pMainSurface;

	if (m_pData)
		delete m_pData;
	m_pData = NULL;
}

void DWOnScreenDisplay::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	windows.SetLogCallback(callback);
}

HRESULT DWOnScreenDisplay::Initialise()
{
	HRESULT hr;

	SetRenderMethod(RENDER_METHOD_DEFAULT);

	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%s%s", g_pData->application.appPath, L"OSD.xml");
	if FAILED(hr = windows.Load((LPWSTR)&file))
		return hr;

	m_pOverlayWindow = windows.GetWindow(L"Overlay");
	m_pCurrentWindow = m_pOverlayWindow;

	return S_OK;
}

RENDER_METHOD DWOnScreenDisplay::GetRenderMethod()
{
	return m_renderMethod;
}

int DWOnScreenDisplay::GetRenderMethodChangeCount()
{
	return m_renderMethodChangeCount;
}

void DWOnScreenDisplay::SetRenderMethod(RENDER_METHOD renderMethod)
{
	if (renderMethod == RENDER_METHOD_DEFAULT)
		renderMethod = RENDER_METHOD_OverlayMixer;

	if ((m_renderMethod != renderMethod) /*|| (m_renderMethod == RENDER_METHOD_NONE)*/)
	{
		m_renderMethod = renderMethod;
		m_renderMethodChangeCount++;

		CAutoLock lock(&m_rendererLock);

		HRESULT hr;
		if (m_pRenderer)
		{
			hr = m_pRenderer->Destroy();
			delete m_pRenderer;
			m_pRenderer = NULL;
		}

		if (renderMethod == RENDER_METHOD_OverlayMixer)
		{
			m_pRenderer = new DWRendererDirectDraw(m_pMainSurface);
		}
		else if (renderMethod == RENDER_METHOD_VMR9)
		{
			m_pRenderer = new DWRendererVMR9Bitmap(m_pMainSurface);
		}
		else if (renderMethod == RENDER_METHOD_VMR9Windowless)
		{
		}

		if (m_pRenderer)
		{
			m_pRenderer->SetLogCallback(m_pLogCallback);
			m_pRenderer->Initialise();
		}
	}

}

HRESULT DWOnScreenDisplay::Render(long tickCount)
{
	HRESULT hr;

	CAutoLock lock(&m_rendererLock);

	if (!m_pRenderer)
		return S_FALSE;
	if (!m_pRenderer->Initialised())
		return S_FALSE;

	m_pRenderer->SetTickCount(tickCount);

	hr = m_pRenderer->Clear();
	if FAILED(hr)
		return (log << "Renderer failed to clear: " << hr << "\n").Write(hr);

	if (m_pCurrentWindow != m_pOverlayWindow)
	{
		CAutoLock windowStackLock(&m_windowStackLock);

		std::vector <DWOSDWindow *>::iterator it_begin = m_windowStack.begin();
		std::vector <DWOSDWindow *>::iterator it_end = m_windowStack.end();
		long it_count = m_windowStack.size();
		std::vector <DWOSDWindow *>::iterator it = m_windowStack.end();
		while (it && (it > m_windowStack.begin()))
		{
			it--;
			if ((*it)->HideWindowsBehindThisOne())
				break;
		}

		if ((it == NULL) || ((*it)->HideWindowsBehindThisOne() == FALSE))
			m_pOverlayWindow->Render(tickCount);

		for ( ; it < m_windowStack.end() ; it++ )
		{
			(*it)->Render(tickCount);
		}
		m_pCurrentWindow->Render(tickCount);
	}
	else
	{
		m_pCurrentWindow->Render(tickCount);
	}

	//Present
	hr = m_pRenderer->Present();
	if FAILED(hr)
		return (log << "Renderer failed to present: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWOnScreenDisplay::ShowMenu(LPWSTR szMenuName)
{
	LPWSTR pCurr = NULL;
	ParseLine parseLine;
	parseLine.IgnoreRHS();

	if (parseLine.Parse(szMenuName) == FALSE)
	{
		return (log << "Parse error in string: " << szMenuName << "\n").Write(E_FAIL);
	}

	pCurr = parseLine.LHS.FunctionName;

	DWOSDWindow *window = windows.GetWindow(pCurr);
	if (window)
	{
		window->ClearParameters();
		for (int arg = 0 ; arg < parseLine.LHS.ParameterCount ; arg++ )
		{
			window->AddParameter(parseLine.LHS.Parameter[arg]);
		}

		if (m_pCurrentWindow != m_pOverlayWindow)
		{
			CAutoLock windowStackLock(&m_windowStackLock);
			m_windowStack.push_back(m_pCurrentWindow);
		}
		m_pCurrentWindow = window;
		return S_OK;
	}
	return S_FALSE;
}

HRESULT DWOnScreenDisplay::ExitMenu(long nNumberOfMenusToExit)
{
	CAutoLock windowStackLock(&m_windowStackLock);

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


DWOSDWindow* DWOnScreenDisplay::Overlay()
{
	return m_pOverlayWindow;
}

HRESULT DWOnScreenDisplay::GetOSDRenderer(DWRenderer **ppDWRenderer)
{
	if (!ppDWRenderer)
		return E_POINTER;
	if (!m_pRenderer)
		return E_FAIL;
	*ppDWRenderer = m_pRenderer;
	return S_OK;
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

DWOSDData* DWOnScreenDisplay::Data()
{
	return m_pData;
}

