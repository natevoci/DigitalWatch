/**
 *	DWSurface.cpp
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

#include "DWSurface.h"
#include "DirectDraw/DWSurfaceRendererDirectDraw.h"
#include "VMR9Bitmap/DWSurfaceRendererVMR9Bitmap.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWSurface
//////////////////////////////////////////////////////////////////////

DWSurface::DWSurface()
{
	m_pSurfaceRenderer = NULL;

	m_surfaceType = SCM_NONE;
	m_Width = 0;
	m_Height = 0;
	m_hInstance = 0;
	m_nResource = 0;
	m_szBitmap = NULL;

	m_bColorKey = FALSE;
	m_dwColorKey = 0x00000000;

	m_lastRenderMethodChangeCount = -1;
}

DWSurface::~DWSurface()
{
	if (m_szBitmap)
		delete[] m_szBitmap;
	Destroy();
}

HRESULT DWSurface::CreateMainSurface()
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);

	m_surfaceType = SCM_CREATEMAINSURFACE;

	hr = Destroy();
	hr = CreateSurfaceRenderer();

	if (m_pSurfaceRenderer)
	{
		hr = m_pSurfaceRenderer->CreateMainSurface();
		if FAILED(hr)
			return (log << "Failed to create main surface: " << hr << "\n").Write(hr);

		m_Width = m_pSurfaceRenderer->GetWidth();
		m_Height = m_pSurfaceRenderer->GetHeight();
	}
	return hr;
}

HRESULT DWSurface::Create(long width, long height)
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);

	hr = Destroy();

	m_surfaceType = SCM_CREATE;
	m_Width = width;
	m_Height = height;

	hr = CreateSurfaceRenderer();

	if (m_pSurfaceRenderer)
	{
		hr = m_pSurfaceRenderer->Create(width, height);
		if FAILED(hr)
			return (log << "Failed to create fixed size surface: " << hr << "\n").Write(hr);

		hr = m_pSurfaceRenderer->Clear();
		if FAILED(hr)
			return (log << "Failed to clear surface after creating it: " << hr << "\n").Write(hr);

		m_Width = m_pSurfaceRenderer->GetWidth();
		m_Height = m_pSurfaceRenderer->GetHeight();
	}
	return hr;
}

HRESULT DWSurface::LoadBitmap(HINSTANCE hInst, UINT nRes)
{
	if (m_szBitmap)
		delete[] m_szBitmap;
	m_szBitmap = NULL;
	m_hInstance = hInst;
	m_nResource = nRes;

	return LoadBitmap();
}

HRESULT DWSurface::LoadBitmap(LPCWSTR szBitmap, COLORREF dwColorKey)
{
	HRESULT hr = CheckSurface();
	if FAILED(hr)
		return hr;

	strCopy(m_szBitmap, szBitmap);
	m_hInstance = 0;
	m_nResource = 0;

	m_bColorKey = TRUE;
	m_dwColorKey = dwColorKey;

	return LoadBitmap();
}

HRESULT DWSurface::LoadBitmap(LPCWSTR szBitmap)
{
	strCopy(m_szBitmap, szBitmap);
	m_hInstance = 0;
	m_nResource = 0;

	m_bColorKey = FALSE;
	m_dwColorKey = 0x00000000;

	return LoadBitmap();
}

HRESULT DWSurface::Destroy()
{
	HRESULT hr = S_OK;

	CAutoLock surfacesLock(&m_surfacesLock);

	if (m_pSurfaceRenderer != NULL)
	{
		hr = m_pSurfaceRenderer->Destroy();
		if FAILED(hr)
			(log << "Error destroying surface renderer: " << hr << "\n").Write();

		delete m_pSurfaceRenderer;
		m_pSurfaceRenderer = NULL;
	}
	m_surfaceType = SCM_NONE;

	return hr;
}

HRESULT DWSurface::Clear()
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);

	hr = CheckSurface();
	if FAILED(hr)
		return hr;

	if (m_pSurfaceRenderer)
	{
		hr = m_pSurfaceRenderer->Clear();
		if FAILED(hr)
			return (log << "Failed to clear surface: " << hr << "\n").Write(hr);
	}

	return hr;
}

HRESULT DWSurface::Blt(DWSurface *targetSurface, RECT* lprcDest, RECT* lprcSrc)
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);
	CAutoLock targetSurfacesLock(&targetSurface->m_surfacesLock);

	hr = CheckSurface();
	if FAILED(hr)
		return hr;

	if (m_pSurfaceRenderer)
	{
		hr = m_pSurfaceRenderer->Blt(targetSurface->m_pSurfaceRenderer, lprcDest, lprcSrc);
		if FAILED(hr)
			return (log << "Failed to blit surface: " << hr << "\n").Write(hr);
	}
	return hr;
}

HRESULT DWSurface::DrawText(DWSurfaceText *text, int x, int y)
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);

	hr = CheckSurface();
	if FAILED(hr)
		return hr;

	if (m_pSurfaceRenderer)
	{
		hr = m_pSurfaceRenderer->DrawText(text, x, y);
		if FAILED(hr)
			return (log << "Failed to draw text on surface: " << hr << "\n").Write(hr);
	}

	return hr;
}

UINT DWSurface::GetWidth()
{
	return m_Width;
}

UINT DWSurface::GetHeight()
{
	return m_Height;
}

HRESULT DWSurface::CreateSurfaceRenderer()
{
	if (m_pSurfaceRenderer)
		return (log << "Error: Cannot create surface renderer. The old one still exists.\n").Write(E_FAIL);

	RENDER_METHOD renderMethod = g_pOSD->GetRenderMethod();
	m_lastRenderMethodChangeCount = g_pOSD->GetRenderMethodChangeCount();

	if ((renderMethod == RENDER_METHOD_OverlayMixer) || (renderMethod == RENDER_METHOD_DEFAULT))
	{
		m_pSurfaceRenderer = new DWSurfaceRendererDirectDraw();
	}
	else if (renderMethod == RENDER_METHOD_VMR7)
	{
		//m_pSurfaceRenderer = new DWSurfaceRendererVMR7Bitmap();
	}
	else if ((renderMethod == RENDER_METHOD_VMR9) || (renderMethod == RENDER_METHOD_VMR9Windowless))
	{
		m_pSurfaceRenderer = new DWSurfaceRendererVMR9Bitmap();
	}
	else if (renderMethod == RENDER_METHOD_VMR9Renderless)
	{
		//m_pSurfaceRenderer = new DWSurfaceRendererVMRRenderless();
	}

	if (m_pSurfaceRenderer)
	{
		m_pSurfaceRenderer->SetLogCallback(m_pLogCallback);
		if (m_bColorKey)
		{
			HRESULT hr = m_pSurfaceRenderer->SetColorKey(m_dwColorKey);
			if FAILED(hr)
				return (log << "Failed to set color key for surface: " << hr << "\n").Write(hr);
		}
	}

	return S_OK;
}

HRESULT DWSurface::CheckSurface()
{
	HRESULT hr = S_OK;

	// Check that the OSDRenderer hasnsn't changed.
	// If it has then destroy m_pSurfaceRenderer and create new one
	int changeCount = g_pOSD->GetRenderMethodChangeCount();
	if (m_lastRenderMethodChangeCount != changeCount)
	{
		m_lastRenderMethodChangeCount = changeCount;

		DWSurfaceCreateMethod currentSurfaceType = m_surfaceType;

		hr = Destroy();
		if FAILED(hr)
			return (log << "Failed to destroy surface renderer: " << hr << "\n").Write(hr);

		switch (currentSurfaceType)
		{
		case SCM_CREATEMAINSURFACE:
			hr = CreateMainSurface();
			break;
		case SCM_CREATE:
			hr = Create(m_Width, m_Height);
			break;
		case SCM_LOADBITMAP_RESOURCE:
			hr = LoadBitmap(m_hInstance, m_nResource);
			break;
		case SCM_LOADBITMAP_FILE:
			hr = LoadBitmap();
			break;
		};
		if FAILED(hr)
			return (log << "Failed to created surface renderer: " << hr << "\n").Write(hr);
	}
	return hr;
}

HRESULT DWSurface::LoadBitmap()
{
	HRESULT hr;

	CAutoLock surfacesLock(&m_surfacesLock);

	hr = Destroy();

	m_surfaceType = (m_szBitmap) ? SCM_LOADBITMAP_FILE : SCM_LOADBITMAP_RESOURCE;

	hr = CreateSurfaceRenderer();

	if (m_pSurfaceRenderer)
	{
		if (m_szBitmap)
		{
			hr = m_pSurfaceRenderer->LoadBitmap(m_szBitmap);
		}
		else
		{
			hr = m_pSurfaceRenderer->LoadBitmap(m_hInstance, m_nResource);
		}
		if FAILED(hr)
			return (log << "Failed to create surface from bitmap file: " << hr << "\n").Write(hr);

		m_Width = m_pSurfaceRenderer->GetWidth();
		m_Height = m_pSurfaceRenderer->GetHeight();
	}
	return hr;
}
