/**
 *	DWOnScreenDisplay.cpp
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

#include "DWOnScreenDisplay.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSD
//////////////////////////////////////////////////////////////////////

DWOnScreenDisplay::DWOnScreenDisplay()
{
	m_pDirectDraw = new DWDirectDraw();
//	m_pImage = new DWDirectDrawImage(m_pDirectDraw);
}

DWOnScreenDisplay::~DWOnScreenDisplay()
{
//	if (m_pImage)
//	{
//		delete m_pImage;
//		m_pImage = NULL;
//	}
	delete m_pDirectDraw;
	m_pDirectDraw = NULL;
}

void DWOnScreenDisplay::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	m_windows.SetLogCallback(callback);

}

HRESULT DWOnScreenDisplay::Initialise()
{
	HRESULT hr;
	hr = m_pDirectDraw->Init(g_pData->hWnd);
	if FAILED(hr)
		return (log << "Failed to initialise DWDirectDraw: " << hr << "\n").Write(hr);

	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%s%s", g_pData->application.appPath, L"OSD.xml");
	if FAILED(hr = m_windows.Load((LPWSTR)&file))
		return hr;

//	m_pImage->LoadBitmap(TEXT("image.bmp"));
//	m_pImage->SetColorKey(RGB(0, 4, 0));

	return S_OK;
}

long DWOnScreenDisplay::GetPanningPos(long tickCount, long span, double speed)
{
	double distPerTick = 2*span/speed/1000.0;

	long ticksPerSpan = 1000*speed;	

	long result = (tickCount%ticksPerSpan);
	if (result >= ticksPerSpan/2)
		result = ticksPerSpan - result;

	return result * distPerTick;
}

HRESULT DWOnScreenDisplay::Render(long tickCount)
{
	HRESULT hr;

	m_pDirectDraw->SetTickCount(tickCount);

	hr = m_pDirectDraw->Clear();
	if FAILED(hr)
		return (log << "Failed to clear directdraw: " << hr << "\n").Write(hr);

	//TODO: Stuff
	RECT rcDest, rcSrc;
	SetRect(&rcDest,
		GetPanningPos(tickCount, 500, 5.0),
		GetPanningPos(tickCount, 200, 22.0),
		100,
		100);
	SetRect(&rcSrc, 0, 0, 0, 0);

//	hr = m_pImage->Draw(&rcDest, &rcSrc);
//	if FAILED(hr)
//		return hr;


	//TODO: End Stuff

	//Display FPS
	IDirectDrawSurface7* piSurface = m_pDirectDraw->get_BackSurface();
	HDC hdc;
	hr = piSurface->GetDC(&hdc);

	SetBkColor(hdc, RGB(0, 0, 255));
	SetTextColor(hdc, RGB(255, 255, 0));
	TCHAR buffer[30];
	sprintf(buffer, TEXT("FPS - %f"), m_pDirectDraw->GetFPS());
	TextOut(hdc, 0, 560, buffer, lstrlen(buffer));

	hr = piSurface->ReleaseDC(hdc);

	//Flip
	hr = m_pDirectDraw->Flip();
	if FAILED(hr)
		return (log << "Failed to flip directdraw: " << hr << "\n").Write(hr);

	return S_OK;
}

DWDirectDraw* DWOnScreenDisplay::get_DirectDraw()
{
	return m_pDirectDraw;
}

DWOnScreenDisplayDataList* DWOnScreenDisplay::GetList(LPWSTR pListName)
{
	//look for existing list of same name
	std::vector<DWOnScreenDisplayDataList *>::iterator it = m_Lists.begin();
	for ( ; it < m_Lists.end() ; it++ )
	{
		LPWSTR pName = (*it)->name;
		if (_wcsicmp(pListName, pName) == 0)
		{
			return (*it);
		}
	}
	DWOnScreenDisplayDataList* newList = new DWOnScreenDisplayDataList();
	strCopy(newList->name, pListName);
	m_Lists.push_back(newList);
	return newList;
}

