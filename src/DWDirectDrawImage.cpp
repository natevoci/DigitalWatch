/**
 *	DWDirectDrawImage.cpp
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

#include "DWDirectDrawImage.h"

//////////////////////////////////////////////////////////////////////
// DWDirectDrawImage
//////////////////////////////////////////////////////////////////////

DWDirectDrawImage::DWDirectDrawImage(DWDirectDraw* pDirectDraw)
{
	m_pDirectDraw = pDirectDraw;
	m_pSurface = NULL;
	m_dwColorKey = -1;
}

DWDirectDrawImage::~DWDirectDrawImage()
{
	if(m_pSurface != NULL)
	{
		OutputDebugString("Surface Destroyed\n");
		m_pSurface->Release();
		m_pSurface = NULL;
	}
}

HRESULT DWDirectDrawImage::LoadBitmap(HINSTANCE hInst, UINT nRes)
{
    HRESULT hr;
	HBITMAP	hbm;
	HBITMAP hbmOld;
    BITMAP bm;
    HDC hdcImage;
    HDC hdc;

	Destroy();

	hbm = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(nRes), IMAGE_BITMAP, 0, 0, 0L);
    if (hbm == NULL)
        return E_FAIL;

    hdcImage = CreateCompatibleDC(NULL);
    if (!hdcImage)
        return E_FAIL;
    hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);

    GetObject(hbm, sizeof(bm), &bm);

	if FAILED(hr = Create(bm.bmWidth, bm.bmHeight))
		return hr;

    if FAILED(hr = m_pSurface->GetDC(&hdc))
		return hr;

	BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
	m_pSurface->ReleaseDC(hdc);

	SelectObject(hdcImage, hbmOld);
	DeleteDC(hdcImage);
	DeleteObject(hbm);

	return S_OK;
}

HRESULT DWDirectDrawImage::LoadBitmap(LPCTSTR szBitmap)
{
	HRESULT hr;
    HBITMAP hbm;
	HBITMAP hbmOld;
    BITMAP bm;
	HDC hdcImage;
	HDC hdc;

	Destroy();

    hbm = (HBITMAP)LoadImage(NULL, szBitmap, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
    if (hbm == NULL)
        return E_FAIL;

	hdcImage = CreateCompatibleDC(NULL);
    if (!hdcImage)
        return E_FAIL;

	hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);

    GetObject(hbm, sizeof(bm), &bm);

	if FAILED(hr = Create(bm.bmWidth, bm.bmHeight))
		return hr;

	if FAILED(hr = m_pSurface->GetDC(&hdc))
		return hr;

	BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcImage, 0, 0, SRCCOPY);
	m_pSurface->ReleaseDC(hdc);

	SelectObject(hdcImage, hbmOld);
	DeleteDC(hdcImage);
    DeleteObject(hbm);

    return S_OK;
}

void DWDirectDrawImage::SetColorKey(COLORREF dwColorKey)
{
	m_dwColorKey = dwColorKey;

	if (((int)m_dwColorKey != -1) && m_pSurface)
	{
	    DDCOLORKEY ddck;
		ddck.dwColorSpaceLowValue = dwColorKey;
		ddck.dwColorSpaceHighValue = 0;
		m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ddck);
	}
}

HRESULT DWDirectDrawImage::Draw(RECT* lprcDest, RECT* lprcSrc)
{
	HRESULT	hr;
	RECT rcDest;
	RECT rcSrc;

	if (lprcDest)
		SetRect(&rcDest, lprcDest->left, lprcDest->top, lprcDest->right, lprcDest->bottom);
	else
		SetRect(&rcDest, 0, 0, 0, 0);

	if (lprcSrc)
		SetRect(&rcSrc, lprcSrc->left, lprcSrc->top, lprcSrc->right, lprcSrc->bottom);
	else
		SetRect(&rcSrc, 0, 0, 0, 0);


	IDirectDrawSurface7* lpDest = m_pDirectDraw->get_BackSurface();
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	lpDest->GetSurfaceDesc(&ddsd);
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
		if((int)m_dwColorKey < 0)
		{
//			hr = lpDest->BltFast(iDestX, iDestY, m_pSurface, &rcSrc,  DDBLTFAST_NOCOLORKEY);
			DDBLTFX ddbfx;
			ZeroMemory(&ddbfx, sizeof(ddbfx));
			ddbfx.dwSize = sizeof( ddbfx );
			hr = lpDest->Blt(&rcDest, m_pSurface, &rcSrc, DDBLT_WAIT, &ddbfx);
		}
		else
		{
			//hr = lpDest->BltFast(iDestX, iDestY, m_pSurface, &rcSrc,  DDBLTFAST_SRCCOLORKEY);
			DDBLTFX ddbfx;
			ZeroMemory(&ddbfx, sizeof(ddbfx));
			ddbfx.dwSize = sizeof( ddbfx );
			hr = lpDest->Blt(&rcDest, m_pSurface, &rcSrc, DDBLT_KEYSRC | DDBLT_WAIT, &ddbfx);
		}

		if(hr == DD_OK)
			break;

		if(hr == DDERR_SURFACELOST)
		{
			Restore();
		}
		else
		{
			if(hr != DDERR_WASSTILLDRAWING)
				return hr;
		}
	}

	return S_OK;
}

UINT DWDirectDrawImage::Width()
{
	return m_Width;
}

UINT DWDirectDrawImage::Height()
{
	return m_Height;
}

LPDIRECTDRAWSURFACE7 DWDirectDrawImage::GetSurface()
{
	return m_pSurface;
}

HRESULT DWDirectDrawImage::Create(long nWidth, long nHeight)
{
	HRESULT				hr;
    DDSURFACEDESC2		ddsd;

    ZeroMemory( &ddsd, sizeof( ddsd ) );

    ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	ddsd.dwWidth  = nWidth;
    ddsd.dwHeight = nHeight;

	
    hr = m_pDirectDraw->get_Object()->CreateSurface(&ddsd, &m_pSurface, NULL );
    if FAILED(hr)
	{
		if (hr == DDERR_OUTOFVIDEOMEMORY)
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			hr = m_pDirectDraw->get_Object()->CreateSurface(&ddsd, &m_pSurface, NULL );
		}

		if FAILED(hr)
		{
			return hr;
		}
	}

	if((int)m_dwColorKey != -1)
	{
	    DDCOLORKEY ddck;
		ddck.dwColorSpaceLowValue = m_dwColorKey;
		ddck.dwColorSpaceHighValue = 0;
		m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ddck);
	}

	m_Width  = nWidth;
	m_Height = nHeight;

	return S_OK;
}

void DWDirectDrawImage::Destroy()
{
	if(m_pSurface != NULL)
	{
		m_pSurface->Release();
		m_pSurface = NULL;
	}
}

void DWDirectDrawImage::Restore()
{
	m_pSurface->Restore();
}

void DWDirectDrawImage::FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight)
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

