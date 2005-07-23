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
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWDirectDrawImage
//////////////////////////////////////////////////////////////////////

DWDirectDrawImage::DWDirectDrawImage()
{
	m_hInstance = 0;
	m_nResource = 0;
	m_szBitmap = NULL;
}

DWDirectDrawImage::~DWDirectDrawImage()
{
	if (m_szBitmap)
		delete m_szBitmap;
}

HRESULT DWDirectDrawImage::LoadBitmap(HINSTANCE hInst, UINT nRes)
{
	m_hInstance = hInst;
	m_nResource = nRes;
	return LoadBitmap();
}

HRESULT DWDirectDrawImage::LoadBitmap(LPCTSTR szBitmap)
{
	strCopy(m_szBitmap, szBitmap);
	return LoadBitmap();
}

HRESULT DWDirectDrawImage::LoadBitmap()
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
		return hr;

	CAutoLock surfacesLock(&m_surfacesLock);

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

