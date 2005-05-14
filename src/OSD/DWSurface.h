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
#include "DirectDraw/DWDirectDrawScreen.h"
#include <vector>

class DWScreenSurface
{
public:
	IDirectDrawSurface7 *piDDSurface;
	DWDirectDrawScreen *pDDScreen;
};


class DWSurface : public LogMessageCaller
{
public:
	DWSurface();
	virtual ~DWSurface();

	HRESULT CreateFromDirectDrawBackSurface();
	HRESULT Create(long width, long height);
	HRESULT Destroy();

	HRESULT Clear();
	HRESULT SetColorKey(COLORREF dwColorKey);

	HRESULT Blt(DWSurface *targetSurface, RECT* lprcDest = NULL, RECT* lprcSrc = NULL);

	//HRESULT DrawImage()

	HRESULT DrawText(DWSurfaceText *text, int x, int y);

	UINT Width();
	UINT Height();

protected:
	void Restore();

	void FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight);

	std::vector<DWScreenSurface *> m_Surfaces;

	long m_Width;
	long m_Height;
	BOOL m_bColorKey;
	COLORREF m_dwColorKey;

};

#endif
