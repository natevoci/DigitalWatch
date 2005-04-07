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

//////////////////////////////////////////////////////////////////////
// DWDirectDraw
//////////////////////////////////////////////////////////////////////

DWDirectDraw::DWDirectDraw()
{
	m_hWnd = 0;
	SetRect(&m_rectBackBuffer, 0, 0, 768, 576);
	m_lTickCount = 0;
	m_fFPS = 0;

	m_piDDObject = NULL;
	m_piFrontSurface = NULL;
	m_piClipper = NULL;
	m_piBackSurface = NULL;

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
	HRESULT hr;

	m_hWnd = hWnd;
	
	// Create the main DirectDraw object.
	hr = DirectDrawCreateEx(NULL, (VOID**)&m_piDDObject, IID_IDirectDraw7, NULL);
	if( hr != DD_OK )
		return hr;

	// Set DDSCL_NORMAL to use windowed mode
	hr = m_piDDObject->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	if FAILED(hr)
		return hr;


	// Create Primary Surface
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	
	hr = m_piDDObject->CreateSurface(&ddsd, &m_piFrontSurface, NULL);
	if FAILED(hr)
		return hr;

	// Create Clipper
    hr = m_piDDObject->CreateClipper( 0, &m_piClipper, NULL );
    if FAILED(hr)
        return hr;
	
    hr = m_piClipper->SetHWnd( 0, m_hWnd );
    if FAILED(hr)
        return hr;

    hr = m_piFrontSurface->SetClipper(m_piClipper);
    if FAILED(hr)
        return hr;

	// Create back buffer
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = m_rectBackBuffer.right;
	ddsd.dwHeight = m_rectBackBuffer.bottom;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	
	hr = m_piDDObject->CreateSurface(&ddsd, &m_piBackSurface, NULL);
	if FAILED(hr)
		return hr;

	m_pOverlayCallback = new DWOverlayCallback(&hr);

	return S_OK;
}

HRESULT DWDirectDraw::Destroy()
{
	m_piBackSurface.Release();
	m_piFrontSurface.Release();
	m_piDDObject.Release();
	return S_OK;
}

IDirectDraw7* DWDirectDraw::get_Object()
{
	return m_piDDObject;
}

IDirectDrawSurface7* DWDirectDraw::get_FrontSurface()
{
	return m_piFrontSurface;
}

IDirectDrawSurface7* DWDirectDraw::get_BackSurface()
{
	return m_piBackSurface;
}

IDDrawExclModeVideoCallback* DWDirectDraw::get_OverlayCallbackInterface()
{
	return m_pOverlayCallback;
}

HRESULT DWDirectDraw::CheckSurfaces()
{
	HRESULT hr;
	if ((m_piFrontSurface) && (m_piFrontSurface->IsLost() == DDERR_SURFACELOST))
	{
		if FAILED(hr = m_piFrontSurface->Restore())
			return hr;
	}
	if ((m_piBackSurface) && (m_piBackSurface->IsLost() == DDERR_SURFACELOST))
	{
		if FAILED(hr = m_piBackSurface->Restore())
			return hr;
	}
	return S_OK;
}

HRESULT DWDirectDraw::Clear()
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
		return hr;

	if (m_bOverlayEnabled)
	{
		ddbfx.dwFillColor = m_dwVideoKeyColor;

		hr = m_piBackSurface->Blt(&m_OverlayPositionRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbfx);
		//hr = m_piBackSurface->Blt(&m_rectBackBuffer, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbfx);
		if FAILED(hr)
			return hr;
	}
	
	return S_OK;
}

HRESULT DWDirectDraw::Flip()
{
	HRESULT hr;
	RECT rcDest;
	POINT p;
	
	// find out where on the primary surface our window lives
	p.x = 0; p.y = 0;
	::ClientToScreen(m_hWnd, &p);
	::GetClientRect(m_hWnd, &rcDest);
	OffsetRect(&rcDest, p.x, p.y);
	hr = m_piFrontSurface->Blt(&rcDest, m_piBackSurface, NULL, DDBLT_WAIT, NULL);
	if FAILED(hr)
		return hr;

	return hr;
}

long DWDirectDraw::GetTickCount()
{
	return m_lTickCount;
}

void DWDirectDraw::SetTickCount(long tickCount)
{
	static int multiplier = 1;
	if (tickCount - m_lTickCount > 400)
	{
		m_fFPS = multiplier * 1000.0 / (double)(tickCount - m_lTickCount);
		m_lTickCount = tickCount;
		multiplier = 1;
	}
	else
	{
		multiplier++;
	}
}

double DWDirectDraw::GetFPS()
{
	return m_fFPS;
}

void DWDirectDraw::SetOverlayEnabled(BOOL bEnabled)
{
	m_bOverlayEnabled = bEnabled;
}

void DWDirectDraw::SetOverlayPosition(const RECT* pRect)
{
	if (pRect)
	{
		m_OverlayPositionRect.left   = pRect->left;
		m_OverlayPositionRect.top    = pRect->top;
		m_OverlayPositionRect.right  = pRect->right;
		m_OverlayPositionRect.bottom = pRect->bottom;

		POINT p;
		p.x = 0; p.y = 0;
		::ScreenToClient(m_hWnd, &p);
		OffsetRect(&m_OverlayPositionRect, p.x, p.y);

		RECT rcDest;
		::GetClientRect(m_hWnd, &rcDest);

		m_OverlayPositionRect.left = (m_rectBackBuffer.right * m_OverlayPositionRect.left / rcDest.right);
		m_OverlayPositionRect.top = (m_rectBackBuffer.bottom * m_OverlayPositionRect.top / rcDest.bottom);
		m_OverlayPositionRect.right = (m_rectBackBuffer.right * m_OverlayPositionRect.right / rcDest.right);
		m_OverlayPositionRect.bottom = (m_rectBackBuffer.bottom * m_OverlayPositionRect.bottom / rcDest.bottom);
	}
}

void DWDirectDraw::SetOverlayColor(COLORREF color)
{
	m_dwVideoKeyColor = color;
}

//////////////////////////////////////////////////////////////////////
// DWOverlayCallback
//////////////////////////////////////////////////////////////////////
DWOverlayCallback::DWOverlayCallback(HRESULT *phr) :
    CUnknown("Overlay Callback Object", NULL, phr)
{
    AddRef();
}


DWOverlayCallback::~DWOverlayCallback()
{
}

HRESULT DWOverlayCallback::OnUpdateOverlay(BOOL bBefore, DWORD dwFlags, BOOL bOldVisible, const RECT *prcSrcOld, const RECT *prcDestOld, BOOL bNewVisible, const RECT *prcSrcNew, const RECT *prcDestNew)
{
    if (g_pOSD->get_DirectDraw() == NULL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: NULL DDraw object pointer was specified"))) ;
        return E_POINTER ;
    }

    if (bBefore)  // overlay is going to be updated
    {
        DbgLog((LOG_TRACE, 5, TEXT("Just turn off color keying and return"))) ;
        g_pOSD->get_DirectDraw()->SetOverlayEnabled(FALSE);
        //g_pOSD->Render(GetTickCount());  // render the surface so that video doesn't show anymore
        return S_OK ;
    }

    //
    // After overlay has been updated. Turn on/off overlay color keying based on 
    // flags and use new source/dest position etc.
    //
    if (dwFlags & (AM_OVERLAY_NOTIFY_VISIBLE_CHANGE |   // it's a valid flag
                   AM_OVERLAY_NOTIFY_SOURCE_CHANGE  |
                   AM_OVERLAY_NOTIFY_DEST_CHANGE))
    {               
        g_pOSD->get_DirectDraw()->SetOverlayEnabled(bNewVisible) ;  // paint/don't paint color key based on param
    }        

    if (dwFlags & AM_OVERLAY_NOTIFY_DEST_CHANGE)     // overlay destination rect change
    {
        ASSERT(prcDestOld);
        ASSERT(prcDestNew);
        g_pOSD->get_DirectDraw()->SetOverlayPosition(prcDestNew);
    }

    return S_OK ;
}


HRESULT DWOverlayCallback::OnUpdateColorKey(COLORKEY const *pKey,
                                           DWORD dwColor)
{
    DbgLog((LOG_TRACE, 5, TEXT("DWOverlayCallback::OnUpdateColorKey(.., 0x%lx) entered"), dwColor)) ;

	g_pOSD->get_DirectDraw()->SetOverlayColor(dwColor);
    return S_OK ;
}


HRESULT DWOverlayCallback::OnUpdateSize(DWORD dwWidth,   DWORD dwHeight, 
                                       DWORD dwARWidth, DWORD dwARHeight)
{
    DbgLog((LOG_TRACE, 5, TEXT("DWOverlayCallback::OnUpdateSize(%lu, %lu, %lu, %lu) entered"), 
            dwWidth, dwHeight, dwARWidth, dwARHeight)) ;

    //PostMessage(g_pData->hWnd, WM_SIZE_CHANGE, 0, 0) ;

    return S_OK ;
}





