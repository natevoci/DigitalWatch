/**
 *	DWDirectDraw.h
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

#ifndef DWDIRECTDRAW_H
#define DWDIRECTDRAW_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWSurface.h"
#include "DWSurfaceRendererDirectDraw.h"
#include "DWDirectDrawScreen.h"
#include <vector>

class DWDirectDraw : public LogMessageCaller
{
	friend DWSurfaceRendererDirectDraw;
public:
	DWDirectDraw();
	virtual ~DWDirectDraw();

	HRESULT Init(HWND hWnd);
	HRESULT Destroy();

	void Enum(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, HMONITOR hm);

	HRESULT GetOverlayCallbackInterface(IDDrawExclModeVideoCallback **ppOverlayCallback);

	HRESULT Clear();
	HRESULT Flip();

	//Methods to be called by DWOverlayCallback
	void SetOverlayEnabled(BOOL bEnabled);
	void SetOverlayPosition(const RECT* pRect);
	void SetOverlayColor(COLORREF color);

private:
	HRESULT CheckSurfaces();

	HWND m_hWnd;
	long m_nBackBufferWidth;
	long m_nBackBufferHeight;

	BOOL m_bAddEnumeratedDevices;

	std::vector<DWDirectDrawScreen*> m_screens;
	CCritSec m_screensLock;

	DWSurface *m_pDWSurface;

	CComPtr<IDDrawExclModeVideoCallback> m_pOverlayCallback;
	BOOL m_bOverlayEnabled;
	RECT m_OverlayPositionRect;
	COLORREF m_dwVideoKeyColor;

	CCritSec m_overlayLock;
};


#endif

