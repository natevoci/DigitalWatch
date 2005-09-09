/**
 *	DWDirectDrawScreen.cpp
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

#include "DWDirectDrawScreen.h"
#include "Globals.h"
#include "GlobalFunctions.h"

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

//////////////////////////////////////////////////////////////////////
// DWDirectDrawScreen
//////////////////////////////////////////////////////////////////////

DWDirectDrawScreen::DWDirectDrawScreen(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, HMONITOR hm, HWND hWnd, long nBackBufferWidth, long nBackBufferHeight)
{
	USES_CONVERSION;

	m_lpGUID = NULL;
	if (lpGUID)
	{
		m_lpGUID = new GUID();
		memcpy(m_lpGUID, lpGUID, sizeof(GUID));
	}

	m_pstrDescription = NULL;
	strCopy(m_pstrDescription, A2W(lpDriverDescription));

	m_pstrName = NULL;
	strCopy(m_pstrName, A2W(lpDriverName));

	m_hm = hm;
	m_hWnd = hWnd;
	m_nBackBufferWidth = nBackBufferWidth;
	m_nBackBufferHeight = nBackBufferHeight;

	m_nMonitorX = 0;
	m_nMonitorY = 0;
	m_nMonitorWidth = 0;
	m_nMonitorHeight = 0;
}

DWDirectDrawScreen::~DWDirectDrawScreen()
{
	Destroy();

	if (m_lpGUID)
		delete m_lpGUID;
	if (m_pstrDescription)
		delete[] m_pstrDescription;
	if (m_pstrName)
		delete[] m_pstrName;
}

HRESULT DWDirectDrawScreen::Create()
{
	HRESULT hr;

	if (m_hm != 0)
	{
		MONITORINFO monInfo;
		ZeroMemory(&monInfo, sizeof(MONITORINFO));
		monInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(m_hm, &monInfo);

		m_nMonitorX = monInfo.rcMonitor.left;
		m_nMonitorY = monInfo.rcMonitor.top;
		m_nMonitorWidth = monInfo.rcMonitor.right - monInfo.rcMonitor.left;
		m_nMonitorHeight = monInfo.rcMonitor.bottom - monInfo.rcMonitor.top;
	}
	else
	{
		m_nMonitorWidth = GetSystemMetrics(SM_CXSCREEN);
		m_nMonitorHeight = GetSystemMetrics(SM_CYSCREEN);
	}

	(log << "Creating DirectDrawScreen: " << (long)m_hm << " \"" << m_pstrName << "\" \"" << m_pstrDescription << "\"  ").Write();
	(log << "monitor(" 
		 << m_nMonitorX << ", "
		 << m_nMonitorY << ", "
		 << m_nMonitorWidth << ", "
		 << m_nMonitorHeight << ")\n").Write();

	LogMessageIndent indent(&log);

	hr = DirectDrawCreateEx(m_lpGUID, (void**)&m_piDDObject, IID_IDirectDraw7, NULL);
	if (hr != DD_OK)
		return (log << "DirectDrawCreateEx Failed : " << hr << "\n").Write(hr);

	// Set DDSCL_NORMAL to use windowed mode
	hr = m_piDDObject->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	if FAILED(hr)
		return (log << "SetCooperativeLevel Failed : " << hr << "\n").Write(hr);


	// Create Primary Surface
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	
	hr = m_piDDObject->CreateSurface(&ddsd, &m_piFrontSurface, NULL);
	if FAILED(hr)
		return (log << "Failed to create front surface : " << hr << "\n").Write(hr);

	// Create Clipper
    hr = m_piDDObject->CreateClipper( 0, &m_piClipper, NULL );
    if FAILED(hr)
        return (log << "Failed to create clipper : " << hr << "\n").Write(hr);
	
    hr = m_piClipper->SetHWnd( 0, m_hWnd );
    if FAILED(hr)
        return (log << "Failed to set window for clipper : " << hr << "\n").Write(hr);

    hr = m_piFrontSurface->SetClipper(m_piClipper);
    if FAILED(hr)
        return (log << "Failed to set clipper for front surface : " << hr << "\n").Write(hr);

	// Create back buffer
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = m_nBackBufferWidth;
	ddsd.dwHeight = m_nBackBufferHeight;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	
	hr = m_piDDObject->CreateSurface(&ddsd, &m_piBackSurface, NULL);

    if FAILED(hr)
	{
		(log << "Failed to create back surface in video memory. Trying system memory\n").Write();
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		hr = m_piDDObject->CreateSurface(&ddsd, &m_piBackSurface, NULL);

		if FAILED(hr)
			return (log << "Failed to create back surface : " << hr << "\n").Write(hr);
	}

	return S_OK;
}

HRESULT DWDirectDrawScreen::Destroy()
{
	m_piClipper.Release();
	m_piBackSurface.Release();
	m_piFrontSurface.Release();
	m_piDDObject.Release();
	return S_OK;
}

HRESULT DWDirectDrawScreen::CheckSurfaces()
{
	HRESULT hr;
	if ((m_piFrontSurface) && (m_piFrontSurface->IsLost() == DDERR_SURFACELOST))
	{
		if FAILED(hr = m_piFrontSurface->Restore())
			return (log << "Failed to restore front surface : " << hr << "\n").Write(hr);
	}
	if ((m_piBackSurface) && (m_piBackSurface->IsLost() == DDERR_SURFACELOST))
	{
		if FAILED(hr = m_piBackSurface->Restore())
			return (log << "Failed to restore back surface : " << hr << "\n").Write(hr);
	}
	return S_OK;
}

HRESULT DWDirectDrawScreen::Clear(BOOL bOverlayEnabled, RECT *rectOverlayPosition, COLORREF dwVideoKeyColor)
{
	HRESULT hr;

	if FAILED(hr = CheckSurfaces())
		return hr;

	DDBLTFX ddbfx;
	ZeroMemory(&ddbfx, sizeof(ddbfx));
	ddbfx.dwSize = sizeof( ddbfx );
	ddbfx.dwFillColor = (DWORD)RGB(0, 0, 0);
	hr = m_piBackSurface->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbfx);
	if FAILED(hr)
		return (log << "Failed to blt to back surface : " << hr << "\n").Write(hr);

	if (bOverlayEnabled)
	{
		ddbfx.dwFillColor = dwVideoKeyColor;

		hr = m_piBackSurface->Blt(rectOverlayPosition, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbfx);
		if FAILED(hr)
			return (log << "Failed to blt video key to back surface : " << hr << "\n").Write(hr);
	}

	return S_OK;
}

HRESULT DWDirectDrawScreen::Flip()
{
	HRESULT hr;
	RECT rcDest;
	RECT rcSrc;
	POINT p;

	if FAILED(hr = CheckSurfaces())
		return hr;
	
	SetRect(&rcSrc, 0, 0, m_nBackBufferWidth, m_nBackBufferHeight);

	// find out where on the surface our window lives
	p.x = 0; p.y = 0;
	::ClientToScreen(m_hWnd, &p);
	::GetClientRect(m_hWnd, &rcDest);
	OffsetRect(&rcDest, p.x, p.y);

	OffsetRect(&rcDest, -m_nMonitorX, -m_nMonitorY);

	if ((rcDest.left >= m_nMonitorWidth) ||
		(rcDest.top >= m_nMonitorHeight) ||
		(rcDest.right <= 0) ||
		(rcDest.bottom <= 0))
		return S_OK;

	if (rcDest.left < 0)
	{
		rcSrc.left = m_nBackBufferWidth * (0-rcDest.left) / (rcDest.right-rcDest.left);
		rcDest.left = 0;
	}
	if (rcDest.top < 0)
	{
		rcSrc.top = m_nBackBufferHeight * (0-rcDest.top) / (rcDest.bottom-rcDest.top);
		rcDest.top = 0;
	}
	if (rcDest.right >= m_nMonitorWidth)
	{
		rcSrc.right = m_nBackBufferWidth * (m_nMonitorWidth-rcDest.left) / (rcDest.right-rcDest.left);
		rcDest.right = m_nMonitorWidth - 1;
	}
	if (rcDest.bottom >= m_nMonitorHeight)
	{
		rcSrc.bottom = m_nBackBufferHeight * (m_nMonitorHeight-rcDest.top) / (rcDest.bottom-rcDest.top);
		rcDest.bottom = m_nMonitorHeight - 1;
	}

	if ((rcSrc.right - rcSrc.left > 0) && (rcSrc.bottom - rcSrc.top > 0))
	{
		hr = m_piFrontSurface->Blt(&rcDest, m_piBackSurface, &rcSrc, DDBLT_WAIT, NULL);
		if FAILED(hr)
			return (log << "Failed to blt back surface to front surface : " << hr << "\n").Write(hr);
	}

	return hr;
}

BOOL DWDirectDrawScreen::IsWindowOnScreen()
{
	RECT rcDest;
	POINT p;

	// find out where on the surface our window lives
	p.x = 0; p.y = 0;
	::ClientToScreen(m_hWnd, &p);
	::GetClientRect(m_hWnd, &rcDest);
	OffsetRect(&rcDest, p.x, p.y);

	OffsetRect(&rcDest, -m_nMonitorX, -m_nMonitorY);

	if ((rcDest.left >= m_nMonitorWidth) ||
		(rcDest.top >= m_nMonitorHeight) ||
		(rcDest.right <= 0) ||
		(rcDest.bottom <= 0))
		return FALSE;
	return TRUE;
}

IDirectDrawSurface7* DWDirectDrawScreen::GetBackSurface()
{
	m_piBackSurface.p->AddRef();
	return m_piBackSurface;
}

HRESULT DWDirectDrawScreen::CreateSurface(long nWidth, long nHeight, IDirectDrawSurface7** pSurface)
{
	if (pSurface == NULL)
		return E_POINTER;

	DDSURFACEDESC2 ddsd;
    ZeroMemory( &ddsd, sizeof( ddsd ) );

    ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	ddsd.dwWidth  = nWidth;
    ddsd.dwHeight = nHeight;

	HRESULT hr;
	hr = m_piDDObject->CreateSurface(&ddsd, pSurface, NULL );
    if FAILED(hr)
	{
		(log << "Failed to create surface in video memory. Trying system memory\n").Write();
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		hr = m_piDDObject->CreateSurface(&ddsd, pSurface, NULL );

		if FAILED(hr)
			return (log << "Failed to create surface : " << hr << "\n").Write(hr);
	}

	return S_OK;
}

