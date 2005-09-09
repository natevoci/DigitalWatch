/**
 *	DWSurface.h
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

#ifndef DWSURFACE_H
#define DWSURFACE_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWSurfaceText.h"
#include <vector>

class DWSurface;

#include "DWSurfaceRenderer.h"

class DWSurface : public LogMessageCaller
{
public:
	DWSurface();
	virtual ~DWSurface();

	HRESULT CreateMainSurface();
	HRESULT Create(long width, long height);
	HRESULT LoadBitmap(HINSTANCE hInst, UINT nRes);
	HRESULT LoadBitmap(LPCTSTR szBitmap);

	HRESULT Destroy();

	HRESULT Clear();
	HRESULT SetColorKey(COLORREF dwColorKey);

	HRESULT Blt(DWSurface *targetSurface, RECT* lprcDest = NULL, RECT* lprcSrc = NULL);

	HRESULT DrawText(DWSurfaceText *text, int x, int y);

	UINT GetWidth();
	UINT GetHeight();

protected:
	HRESULT CreateSurfaceRenderer();
	HRESULT CheckSurface();

	CCritSec m_surfacesLock;

	DWSurfaceRenderer *m_pSurfaceRenderer;

	enum DWSurfaceCreateMethod
	{
		CM_NONE,
		CM_CREATEMAINSURFACE,
		CM_CREATE,
		CM_LOADBITMAP_RESOURCE,
		CM_LOADBITMAP_FILE
	} m_surfaceType;
	long		m_Width;
	long		m_Height;
	HINSTANCE	m_hInstance;
	UINT		m_nResource;
	LPTSTR		m_szBitmap;

	int m_lastRenderMethodChangeCount;
};

#endif
