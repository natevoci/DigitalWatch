/**
 *	DWSurfaceRendererDirectDraw.cpp
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
#include "DWRendererDirectDraw.h"
#include "DWSurfaceRendererDirectDraw.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWSurfaceRendererDirectDraw
//////////////////////////////////////////////////////////////////////

DWSurfaceRendererDirectDraw::DWSurfaceRendererDirectDraw()
{
	m_hInstance = 0;
	m_nResource = 0;
	m_szBitmap = NULL;
}

DWSurfaceRendererDirectDraw::~DWSurfaceRendererDirectDraw()
{
	if (m_szBitmap)
		delete[] m_szBitmap;
	m_szBitmap = NULL;
}

HRESULT DWSurfaceRendererDirectDraw::CreateMainSurface()
{
	HRESULT hr = S_OK;

	DWRenderer *pOSDRenderer;
	hr = g_pOSD->GetOSDRenderer(&pOSDRenderer);
	if FAILED(hr)
		return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);

	DWRendererDirectDraw *pOSDRendererDirectDraw = dynamic_cast<DWRendererDirectDraw *>(pOSDRenderer);
	if (!pOSDRendererDirectDraw)
		return (log << "Failed to cast OSD Renderer as DirectDraw OSD Renderer\n").Write(E_FAIL);

	DWDirectDraw *pDirectDraw = pOSDRendererDirectDraw->GetDirectDraw();
	if (pDirectDraw == NULL)
		return (log << "DWDirectDraw object does not exist\n").Write(E_FAIL);

	CAutoLock screenLock(&pDirectDraw->m_screensLock);

	if (pDirectDraw->m_screens.size() <= 0)
		return (log << "Could not create back surface because no directdraw screens exist\n").Write(E_FAIL);

	m_Width = pDirectDraw->m_nBackBufferWidth;
	m_Height = pDirectDraw->m_nBackBufferHeight;

	std::vector<DWDirectDrawScreen*>::iterator it = pDirectDraw->m_screens.begin();
	for ( ; it < pDirectDraw->m_screens.end() ; it++ )
	{
		DWScreenSurface* screen = new DWScreenSurface();
		screen->pDDScreen = *it;
		screen->piDDSurface = screen->pDDScreen->GetBackSurface();
		m_surfaces.push_back(screen);
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::Create(long width, long height)
{
	HRESULT hr = S_OK;

	DWRenderer *pOSDRenderer;
	hr = g_pOSD->GetOSDRenderer(&pOSDRenderer);
	if FAILED(hr)
		return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);

	DWRendererDirectDraw *pOSDRendererDirectDraw = dynamic_cast<DWRendererDirectDraw *>(pOSDRenderer);
	if (!pOSDRendererDirectDraw)
		return (log << "Failed to cast OSD Renderer as DirectDraw OSD Renderer\n").Write(E_FAIL);

	DWDirectDraw *pDirectDraw = pOSDRendererDirectDraw->GetDirectDraw();
	if (pDirectDraw == NULL)
		return (log << "DWDirectDraw object does not exist\n").Write(E_FAIL);

	CAutoLock screenLock(&pDirectDraw->m_screensLock);

	if (pDirectDraw->m_screens.size() <= 0)
		return (log << "Could not create surface because no directdraw screens exist\n").Write(E_FAIL);

	m_Width = width;
	m_Height = height;

	std::vector<DWDirectDrawScreen*>::iterator it = pDirectDraw->m_screens.begin();
	for ( ; it < pDirectDraw->m_screens.end() ; it++ )
	{
		DWScreenSurface* screen = new DWScreenSurface();
		screen->pDDScreen = *it;

		hr = screen->pDDScreen->CreateSurface(width, height, &screen->piDDSurface);
		if (FAILED(hr))
			return hr;

		m_surfaces.push_back(screen);
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::LoadBitmap(HINSTANCE hInst, UINT nRes)
{
	m_hInstance = hInst;
	m_nResource = nRes;
	return LoadBitmap();
}

HRESULT DWSurfaceRendererDirectDraw::LoadBitmap(LPCTSTR szBitmap)
{
	strCopy(m_szBitmap, szBitmap);
	return LoadBitmap();
}

HRESULT DWSurfaceRendererDirectDraw::Destroy()
{
	HRESULT hr = S_OK;

	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;
		screen->piDDSurface->Release();
		delete screen;
	}
	m_surfaces.clear();

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::Clear()
{
	HRESULT hr = S_OK;

	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;

		DDBLTFX ddbfx;
		ZeroMemory(&ddbfx, sizeof(ddbfx));
		ddbfx.dwSize = sizeof( ddbfx );
		ddbfx.dwFillColor = (DWORD)RGB(0, 0, 0);
		hr = screen->piDDSurface->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbfx);
		if FAILED(hr)
			return (log << "Failed to clear surface : " << hr << "\n").Write(hr);
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::SetColorKey(COLORREF dwColorKey)
{
	HRESULT hr = S_OK;

	m_bColorKey = TRUE;
	m_dwColorKey = dwColorKey;

	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;

	    DDCOLORKEY ddck;
		ddck.dwColorSpaceLowValue = m_dwColorKey;
		ddck.dwColorSpaceHighValue = 0;
		hr = screen->piDDSurface->SetColorKey(DDCKEY_SRCBLT, &ddck);
		if (FAILED(hr))
			return (log << "Failed to set color key : " << hr << "\n").Write(hr);
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::Blt(DWSurfaceRenderer *targetSurface, RECT* lprcDest /*= NULL*/, RECT* lprcSrc /*= NULL*/)
{
	HRESULT hr = S_OK;

	RECT rcDest;
	RECT rcSrc;

	DWSurfaceRendererDirectDraw* targetDDSurface = dynamic_cast<DWSurfaceRendererDirectDraw*>(targetSurface);
	if (!targetDDSurface)
		return (log << "Failed to cast Surface Renderer to DirectDraw Surface Renderer\n").Write(E_POINTER);

	std::vector<DWScreenSurface *>::iterator target_it = targetDDSurface->m_surfaces.begin();
	std::vector<DWScreenSurface *>::iterator source_it = m_surfaces.begin();
	while ((target_it < targetDDSurface->m_surfaces.end()) && (source_it < m_surfaces.end()))
	{
		DWScreenSurface* screenSrc = *source_it;
		DWScreenSurface* screenDest = *target_it;

		target_it++;
		source_it++;

		if (screenDest->pDDScreen->IsWindowOnScreen() == FALSE)
			continue;


		if (lprcDest)
			SetRect(&rcDest, lprcDest->left, lprcDest->top, lprcDest->right, lprcDest->bottom);
		else
			SetRect(&rcDest, 0, 0, 0, 0);

		if (lprcSrc)
			SetRect(&rcSrc, lprcSrc->left, lprcSrc->top, lprcSrc->right, lprcSrc->bottom);
		else
			SetRect(&rcSrc, 0, 0, 0, 0);

		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		screenDest->piDDSurface->GetSurfaceDesc(&ddsd);
		FixRects(rcSrc, rcDest, (long)ddsd.dwWidth, (long)ddsd.dwHeight);

		//If the coordinates are off screen then return ok
		if ((rcDest.left >= (long)ddsd.dwWidth) ||
			(rcDest.top >= (long)ddsd.dwHeight) ||
			(rcDest.right <= 0) ||
			(rcDest.bottom <= 0))
			return S_OK;

		if ((rcSrc.left >= m_Width) ||
			(rcSrc.top >= m_Height))
			return S_OK;

		while(1)
		{
			DDBLTFX ddbfx;
			ZeroMemory(&ddbfx, sizeof(ddbfx));
			ddbfx.dwSize = sizeof( ddbfx );

			if (m_bColorKey)
			{
				//hr = screenDest->piDDSurface->BltFast(iDestX, iDestY, m_pSurface, &rcSrc,  DDBLTFAST_SRCCOLORKEY);
				hr = screenDest->piDDSurface->Blt(&rcDest, screenSrc->piDDSurface, &rcSrc, DDBLT_KEYSRC | DDBLT_WAIT, &ddbfx);
			}
			else
			{
				//hr = screenDest->piDDSurface->BltFast(iDestX, iDestY, m_pSurface, &rcSrc,  DDBLTFAST_NOCOLORKEY);
				hr = screenDest->piDDSurface->Blt(&rcDest, screenSrc->piDDSurface, &rcSrc, DDBLT_WAIT, &ddbfx);
			}

			if(hr == DD_OK)
				break;

			if(hr == DDERR_SURFACELOST)
			{
				screenDest->piDDSurface->Restore();
			}
			else
			{
				if(hr != DDERR_WASSTILLDRAWING)
					return hr;
			}
		}
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::DrawText(DWSurfaceText *text, int x, int y)
{
	HRESULT hr = S_OK;
	HDC hDC;
	USES_CONVERSION;

	if (text->GetText() == NULL)
	{
		(log << "DWSurfaceRendererDirectDraw::DrawText : No text defined\n").Write(E_FAIL);
	}

	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;

		hr = screen->piDDSurface->GetDC(&hDC);
		if FAILED(hr)
			return hr;

		text->InitDC(hDC);
		TextOut(hDC, x, y, W2T(text->GetText()), wcslen(text->GetText()));
		text->UninitDC(hDC);

		hr = screen->piDDSurface->ReleaseDC(hDC);
		if FAILED(hr)
			return hr;
	}

	return hr;
}

HRESULT DWSurfaceRendererDirectDraw::LoadBitmap()
{
	HRESULT hr;
    HBITMAP hbm;
	HBITMAP hbmOld;
    BITMAP bm;
	HDC hdcImage;
	HDC hdc;

	Destroy();

	if (m_szBitmap)
	{
	    hbm = (HBITMAP)LoadImage(NULL, m_szBitmap, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if (hbm == NULL)
			return E_FAIL;
	}
	else
	{
		hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(m_nResource), IMAGE_BITMAP, 0, 0, 0L);
		if (hbm == NULL)
			return E_FAIL;
	}

	hdcImage = CreateCompatibleDC(NULL);
    if (!hdcImage)
        return E_FAIL;

	hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);

    GetObject(hbm, sizeof(bm), &bm);

	if FAILED(hr = Create(bm.bmWidth, bm.bmHeight))
		return (log << "Failed to create surface for image\n").Write(hr);

	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;

		if FAILED(hr = screen->piDDSurface->GetDC(&hdc))
			return hr;

		BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
		screen->piDDSurface->ReleaseDC(hdc);
	}

	SelectObject(hdcImage, hbmOld);
	DeleteDC(hdcImage);

    DeleteObject(hbm);

    return S_OK;
}

void DWSurfaceRendererDirectDraw::Restore()
{
	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;
		screen->piDDSurface->Restore();
	}
}

void DWSurfaceRendererDirectDraw::FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight)
{
	if (rcSrc.right == 0)
		rcSrc.right = m_Width;
	if (rcSrc.bottom == 0)
		rcSrc.bottom = m_Height;

	if (rcSrc.left + rcSrc.right > m_Width)
		rcSrc.right = m_Width - rcSrc.left;
	if (rcSrc.top + rcSrc.bottom > m_Height)
		rcSrc.bottom = m_Height - rcSrc.top;


	if (rcDest.right == 0)
		rcDest.right = rcSrc.right;
	if (rcDest.bottom == 0)
		rcDest.bottom = rcSrc.bottom;

	rcDest.right += rcDest.left;
	rcDest.bottom += rcDest.top;

	if (rcDest.right > destSurfaceWidth)
	{
		double part = (destSurfaceWidth - rcDest.left) / (double)(rcDest.right - rcDest.left);
		rcSrc.right = (long)(rcSrc.right * part);
		rcDest.right = destSurfaceWidth;
	}
	if (rcDest.left < 0)
	{
		double part = (0 - rcDest.left) / (double)(rcDest.right - rcDest.left);
		rcSrc.left  += (long)(rcSrc.right * part);
		rcSrc.right -= (long)(rcSrc.right * part);
		rcDest.left = 0;
	}
	if (rcDest.bottom > destSurfaceHeight)
	{
		double part = (destSurfaceHeight - rcDest.top) / (double)(rcDest.bottom - rcDest.top);
		rcSrc.bottom = (long)(rcSrc.bottom * part);
		rcDest.bottom = destSurfaceHeight;
	}
	if (rcDest.top < 0)
	{
		double part = (0 - rcDest.top) / (double)(rcDest.bottom - rcDest.top);
		rcSrc.top += (long)(rcSrc.bottom * part);
		rcSrc.bottom -= (long)(rcSrc.bottom * part);
		rcDest.top = 0;
	}


	
	rcSrc.right += rcSrc.left;
	rcSrc.bottom += rcSrc.top;

}

