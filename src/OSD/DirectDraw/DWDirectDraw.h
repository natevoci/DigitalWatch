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
#include <ddraw.h>

class DWDirectDraw  
{
public:
	DWDirectDraw();
	virtual ~DWDirectDraw();

	HRESULT Init(HWND hWnd);
	HRESULT Destroy();

	IDirectDraw7* get_Object();
	IDirectDrawSurface7* get_FrontSurface();
	IDirectDrawSurface7* get_BackSurface();
	IDDrawExclModeVideoCallback* get_OverlayCallbackInterface();

	HRESULT CheckSurfaces();

	HRESULT Clear();
	HRESULT Flip();

	long GetTickCount();
	void SetTickCount(long tickCount);
	double GetFPS();

	void SetOverlayEnabled(BOOL bEnabled);
	void SetOverlayPosition(const RECT* pRect);
	void SetOverlayColor(COLORREF color);

private:
	HWND m_hWnd;
	RECT m_rectBackBuffer;
	long m_lTickCount;
	double m_fFPS;

	CComPtr<IDirectDraw7>			m_piDDObject;
	CComPtr<IDirectDrawClipper>		m_piClipper;
	CComPtr<IDirectDrawSurface7>	m_piFrontSurface;
	CComPtr<IDirectDrawSurface7>	m_piBackSurface;

    IDDrawExclModeVideoCallback *m_pOverlayCallback;
	BOOL m_bOverlayEnabled;
	RECT m_OverlayPositionRect;
	COLORREF m_dwVideoKeyColor;
};


class DWOverlayCallback : public CUnknown, public IDDrawExclModeVideoCallback
{
public:
    DWOverlayCallback(HRESULT *phr) ;
    ~DWOverlayCallback() ;

    DECLARE_IUNKNOWN

    // IDDrawExclModeVideoCallback interface methods
    STDMETHODIMP OnUpdateOverlay(BOOL  bBefore, DWORD dwFlags, BOOL bOldVisible, const RECT *prcSrcOld, const RECT *prcDestOld, BOOL bNewVisible, const RECT *prcSrcNew, const RECT *prcDestNew);
    STDMETHODIMP OnUpdateColorKey(COLORKEY const *pKey, DWORD dwColor);
    STDMETHODIMP OnUpdateSize(DWORD dwWidth, DWORD dwHeight, DWORD dwARWidth, DWORD dwARHeight);
};

#endif

