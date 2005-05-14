/**
 *	DWDirectDrawScreen.h
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

#ifndef DWDIRECTDRAWSCREEN_H
#define DWDIRECTDRAWSCREEN_H

#include "StdAfx.h"
#include "LogMessage.h"
#include <ddraw.h>
#include <vector>

class DWDirectDrawScreen : public LogMessageCaller
{
public:
	DWDirectDrawScreen(HWND hWnd, long nBackBufferWidth, long nBackBufferHeight);
	virtual ~DWDirectDrawScreen();

	HRESULT Create(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, HMONITOR hm);
	HRESULT Destroy();
	HRESULT CheckSurfaces();

	HRESULT Clear(BOOL bOverlayEnabled = FALSE, RECT *rectOverlayPosition = NULL, COLORREF dwVideoKeyColor = 0);
	HRESULT Flip();

	BOOL IsWindowOnScreen();

	IDirectDrawSurface7* get_BackSurface();

	HRESULT CreateSurface(long nWidth, long nHeight, IDirectDrawSurface7**	pSurface);

private:
	LPWSTR m_pstrDescription;
	LPWSTR m_pstrName;

	CComPtr<IDirectDraw7>			m_piDDObject;
	CComPtr<IDirectDrawClipper>		m_piClipper;
	CComPtr<IDirectDrawSurface7>	m_piFrontSurface;
	CComPtr<IDirectDrawSurface7>	m_piBackSurface;

	HWND m_hWnd;
	long m_nBackBufferWidth;
	long m_nBackBufferHeight;

	long m_nMonitorX;
	long m_nMonitorY;
	long m_nMonitorWidth;
	long m_nMonitorHeight;
};

#endif

