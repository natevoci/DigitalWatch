/**
 *	DWDirectDrawSurface.h
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

#ifndef DWDIRECTDRAWSURFACE_H
#define DWDIRECTDRAWSURFACE_H

#include <ddraw.h>
#include "DWDirectDraw.h"

struct SURFACE_SOURCE_INFO
{
	HINSTANCE	m_hInstance;
	UINT		m_nResource;
	int			m_iX;
	int			m_iY;
	int			m_iWidth;
	int			m_iHeight;

};

class DWDirectDrawImage
{
public:
	DWDirectDrawImage(DWDirectDraw* pDirectDraw);
	virtual ~DWDirectDrawImage();

	HRESULT LoadBitmap(HINSTANCE hInst, UINT nRes);
	HRESULT LoadBitmap(LPCTSTR szBitmap);
	void SetColorKey(COLORREF dwColorKey);

	HRESULT Draw(RECT* lprcDest = NULL, RECT* lprcSrc = NULL);

	UINT Width();
	UINT Height();
	LPDIRECTDRAWSURFACE7 GetSurface();

	SURFACE_SOURCE_INFO m_srcInfo;

protected:
	HRESULT Create(long nWidth, long nHeight);
	void Destroy();
	void Restore();
	void FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight);


	DWDirectDraw* m_pDirectDraw;

	COLORREF m_dwColorKey;
	long m_Height;
	long m_Width;
	LPDIRECTDRAWSURFACE7 m_pSurface;
};

#endif
