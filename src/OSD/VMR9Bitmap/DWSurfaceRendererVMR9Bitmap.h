/**
 *	DWSurfaceRendererVMR9Bitmap.h
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

#ifndef DWSURFACERENDERERVMR9BITMAP_H
#define DWSURFACERENDERERVMR9BITMAP_H

#include "StdAfx.h"
#include "DWSurfaceRenderer.h"
#include "DirectDraw/DWDirectDrawScreen.h"
#include <vector>
#include <D3DX9.h>
#include "DirectXCommon/d3dfont.h"

#define USE_CD3DFONT

class DWSurfaceRendererVMR9BitmapFont
{
public:
	LOGFONT logfont;
#ifndef USE_CD3DFONT
	CComPtr<ID3DXFont> pFont;
#else
	CD3DFont *pFont;
#endif
};

class DWSurfaceRendererVMR9Bitmap : public DWSurfaceRenderer
{
public:
	DWSurfaceRendererVMR9Bitmap();
	virtual ~DWSurfaceRendererVMR9Bitmap();

	virtual HRESULT CreateMainSurface();
	virtual HRESULT Create(long width, long height);
	virtual HRESULT LoadBitmap(HINSTANCE hInst, UINT nRes);
	virtual HRESULT LoadBitmap(LPCWSTR szBitmap);

	virtual HRESULT Destroy();

	virtual HRESULT Clear();
	virtual HRESULT SetColorKey(COLORREF dwColorKey);

	virtual HRESULT Blt(DWSurfaceRenderer *targetSurface, RECT* lprcDest = NULL, RECT* lprcSrc = NULL);

	virtual HRESULT DrawText(DWSurfaceText *text, int x, int y);

protected:
	HRESULT LoadBitmap();
	void Restore();

	void FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight);

	HINSTANCE	m_hInstance;
	UINT		m_nResource;
	LPWSTR		m_szBitmap;

	CComPtr<IDirect3DDevice9> m_pD3DDevice;
	CComPtr<IDirect3DSurface9> m_pD3DSurface;

	std::vector<DWSurfaceRendererVMR9BitmapFont *> m_fonts;
};

#endif
