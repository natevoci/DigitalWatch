/**
 *	DWDirectDraw.cpp
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

#include "DWDirectDraw.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "DWOverlayCallback.h"

//////////////////////////////////////////////////////////////////////
// DWDirectDraw
//////////////////////////////////////////////////////////////////////

static BOOL CALLBACK DirectDrawEnumCB(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm);

DWDirectDraw::DWDirectDraw()
{
	m_hWnd = 0;
	m_nBackBufferWidth = 1024;
	m_nBackBufferHeight = 576;

	m_bAddEnumeratedDevices = FALSE;

	m_pOverlayCallback = NULL;
	m_bOverlayEnabled = FALSE;
	SetRect(&m_OverlayPositionRect, 0, 0, 1, 1);
	m_dwVideoKeyColor = 0x00000000;
}

DWDirectDraw::~DWDirectDraw()
{
	Destroy();
}


HRESULT DWDirectDraw::Init(HWND hWnd)
{
	CAutoLock screensLock(&m_screensLock);

	HRESULT hr = S_OK;

	(log << "Initialising DirectDraw\n").Write();
	LogMessageIndent indent(&log);

	m_hWnd = hWnd;

	if (!m_pOverlayCallback)
	{
		IDDrawExclModeVideoCallback *pNewCallback = new DWOverlayCallback(&hr, this);
		m_pOverlayCallback.Attach(pNewCallback);
		if (FAILED(hr))
		{
			return (log << "Failed to create DWOverlayCallback : " << hr << "\n").Write(hr);
		}
	}

	m_bAddEnumeratedDevices = FALSE;
	{
		(log << "Enumerating Primary DirectDraw devices\n").Write();
		LogMessageIndent indent2(&log);
		hr = DirectDrawEnumerateEx(&DirectDrawEnumCB, this, NULL);
		if (FAILED(hr))
			return (log << "Failed to enumerate devices : " << hr << "\n").Write(hr);
	}
	{
		(log << "Enumerating Attached DirectDraw devices\n").Write();
		LogMessageIndent indent2(&log);
		hr = DirectDrawEnumerateEx(&DirectDrawEnumCB, this, DDENUM_ATTACHEDSECONDARYDEVICES);
		if (FAILED(hr))
			return (log << "Failed to enumerate devices : " << hr << "\n").Write(hr);
	}
	{
		(log << "Enumerating Detached DirectDraw devices\n").Write();
		LogMessageIndent indent2(&log);
		hr = DirectDrawEnumerateEx(&DirectDrawEnumCB, this, DDENUM_DETACHEDSECONDARYDEVICES);
		if (FAILED(hr))
			return (log << "Failed to enumerate devices : " << hr << "\n").Write(hr);
	}
	
	m_bAddEnumeratedDevices = TRUE;
	{	
		(log << "Enumerating DirectDraw devices\n").Write();
		LogMessageIndent indent2(&log);
		hr = DirectDrawEnumerateEx(&DirectDrawEnumCB, this, DDENUM_ATTACHEDSECONDARYDEVICES);
		if (FAILED(hr))
			return (log << "Failed to enumerate devices : " << hr << "\n").Write(hr);
	}

	if (m_screens.size() > 1)
	{
		DWDirectDrawScreen* ddScreen = m_screens.at(0);
		m_screens.erase(m_screens.begin());
		delete ddScreen;
	}

	{
		(log << "Creating DirectDraw screens\n").Write();
		LogMessageIndent indent2(&log);
		std::vector<DWDirectDrawScreen*>::iterator it = m_screens.begin();
		for ( ; it < m_screens.end() ; it++ )
		{
			DWDirectDrawScreen* ddScreen = *it;
			hr = ddScreen->Create();
			if (FAILED(hr))
				return (log << "Failed to create DWDirectDrawScreen : " << hr << "\n").Write(hr);
		}
	}

	return hr;
}

static BOOL CALLBACK DirectDrawEnumCB(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
	DWDirectDraw *pDWDirectDraw = (DWDirectDraw *)lpContext;
	pDWDirectDraw->Enum(lpGUID, lpDriverDescription, lpDriverName, hm);
	return TRUE;
}

void DWDirectDraw::Enum(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, HMONITOR hm)
{
	(log << "Found device: " << (long)hm << " \"" << lpDriverName << "\" \"" << lpDriverDescription << "\"\n").Write();
	LogMessageIndent indent(&log);

	if (m_bAddEnumeratedDevices)
	{
		CAutoLock screensLock(&m_screensLock);

		DWDirectDrawScreen* ddScreen = new DWDirectDrawScreen(lpGUID, lpDriverDescription, lpDriverName, hm, m_hWnd, m_nBackBufferWidth, m_nBackBufferHeight);
		ddScreen->SetLogCallback(m_pLogCallback);
		m_screens.push_back(ddScreen);
	}
}

HRESULT DWDirectDraw::Destroy()
{
	CAutoLock screensLock(&m_screensLock);

	std::vector<DWDirectDrawScreen*>::iterator it = m_screens.begin();
	for ( ; it < m_screens.end() ; it++ )
	{
		DWDirectDrawScreen* ddScreen = *it;
		delete ddScreen;
	}
	m_screens.clear();

	return S_OK;
}

HRESULT DWDirectDraw::GetOverlayCallbackInterface(IDDrawExclModeVideoCallback **ppOverlayCallback)
{
	if (!ppOverlayCallback)
		return E_POINTER;
	if (!m_pOverlayCallback.p)
		return E_FAIL;
	*ppOverlayCallback = m_pOverlayCallback.p;
	return S_OK;
}

HRESULT DWDirectDraw::Clear()
{
	CAutoLock screensLock(&m_screensLock);
	CAutoLock overlayLock(&m_overlayLock);

	HRESULT hr;

	std::vector<DWDirectDrawScreen*>::iterator it = m_screens.begin();
	for ( ; it < m_screens.end() ; it++ )
	{
		DWDirectDrawScreen* ddScreen = *it;
		hr = ddScreen->Clear(m_bOverlayEnabled, &m_OverlayPositionRect, m_dwVideoKeyColor);
		if FAILED(hr)
			return (log << "Failed clearing surfaces\n").Write(hr);
	}

	return S_OK;
}

HRESULT DWDirectDraw::Flip()
{
	CAutoLock screensLock(&m_screensLock);

	HRESULT hr;

	std::vector<DWDirectDrawScreen*>::iterator it = m_screens.begin();
	for ( ; it < m_screens.end() ; it++ )
	{
		DWDirectDrawScreen* ddScreen = *it;
		hr = ddScreen->Flip();
		if FAILED(hr)
			return (log << "Failed flipping surfaces\n").Write(hr);
	}

	return S_OK;
}

void DWDirectDraw::SetOverlayEnabled(BOOL bEnabled)
{
	CAutoLock overlayLock(&m_overlayLock);
	m_bOverlayEnabled = bEnabled;
}

void DWDirectDraw::SetOverlayPosition(const RECT* pRect)
{
	if (pRect)
	{
		CAutoLock overlayLock(&m_overlayLock);

		CopyRect(&m_OverlayPositionRect, pRect);

		POINT p;
		p.x = 0; p.y = 0;
		::ScreenToClient(m_hWnd, &p);
		OffsetRect(&m_OverlayPositionRect, p.x, p.y);

		RECT rcDest;
		::GetClientRect(m_hWnd, &rcDest);

		RECT rcIntersect;
		IntersectRect(&rcIntersect, &m_OverlayPositionRect, &rcDest);

		m_OverlayPositionRect.left = (m_nBackBufferWidth * rcIntersect.left / rcDest.right);
		m_OverlayPositionRect.top = (m_nBackBufferHeight * rcIntersect.top / rcDest.bottom);
		m_OverlayPositionRect.right = (m_nBackBufferWidth * rcIntersect.right / rcDest.right);
		m_OverlayPositionRect.bottom = (m_nBackBufferHeight * rcIntersect.bottom / rcDest.bottom);
	}
}

void DWDirectDraw::SetOverlayColor(COLORREF color)
{
	CAutoLock overlayLock(&m_overlayLock);
	m_dwVideoKeyColor = color;
}

HRESULT DWDirectDraw::CheckSurfaces()
{
	CAutoLock screensLock(&m_screensLock);

	HRESULT hr;

	std::vector<DWDirectDrawScreen*>::iterator it = m_screens.begin();
	for ( ; it < m_screens.end() ; it++ )
	{
		DWDirectDrawScreen* ddScreen = *it;
		hr = ddScreen->CheckSurfaces();
		if FAILED(hr)
			return (log << "Failed checking surface\n").Write(hr);
	}

	return S_OK;
}

