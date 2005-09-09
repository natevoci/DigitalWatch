/**
 *	DWSurfaceRenderer.h
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

#ifndef DWSURFACERENDERER_H
#define DWSURFACERENDERER_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWSurfaceText.h"
#include <vector>

class DWSurfaceRenderer : public LogMessageCaller
{
public:
	DWSurfaceRenderer();
	virtual ~DWSurfaceRenderer();

	virtual HRESULT CreateMainSurface() = 0;
	virtual HRESULT Create(long width, long height) = 0;
	virtual HRESULT LoadBitmap(HINSTANCE hInst, UINT nRes) = 0;
	virtual HRESULT LoadBitmap(LPCWSTR szBitmap) = 0;

	virtual HRESULT Destroy() = 0;

	virtual HRESULT Clear() = 0;
	virtual HRESULT SetColorKey(COLORREF dwColorKey) = 0;

	virtual HRESULT Blt(DWSurfaceRenderer *targetSurface, RECT* lprcDest = NULL, RECT* lprcSrc = NULL) = 0;

	virtual HRESULT DrawText(DWSurfaceText *text, int x, int y) = 0;

	UINT GetWidth();
	UINT GetHeight();

protected:
	long m_Width;
	long m_Height;
	BOOL m_bColorKey;
	COLORREF m_dwColorKey;

};

#endif
