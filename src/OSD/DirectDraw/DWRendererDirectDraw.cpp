/**
 *	DWRendererDirectDraw.cpp
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

#include "DWRendererDirectDraw.h"
#include "Globals.h"

DWRendererDirectDraw::DWRendererDirectDraw(DWSurface *pSurface) : DWRenderer(pSurface)
{
	m_pDirectDraw = new DWDirectDraw();
}

DWRendererDirectDraw::~DWRendererDirectDraw()
{
	if (m_pDirectDraw)
		delete m_pDirectDraw;
	m_pDirectDraw = NULL;
}

void DWRendererDirectDraw::SetLogCallback(LogMessageCallback *callback)
{
	DWRenderer::SetLogCallback(callback);

	m_pDirectDraw->SetLogCallback(callback);
}

HRESULT DWRendererDirectDraw::Initialise()
{
	HRESULT hr;

	hr = m_pDirectDraw->Init(g_pData->hWnd);
	if FAILED(hr)
		return (log << "Failed to initialise DWDirectDraw: " << hr << "\n").Write(hr);

	hr = m_pSurface->CreateMainSurface();
	if (FAILED(hr))
		return (log << "Failed to create DWSurface from directdraw back surface : " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWRendererDirectDraw::Destroy()
{
	HRESULT hr;

	hr = m_pSurface->Destroy();
	if FAILED(hr)
		(log << "Failed to destroy main directdraw surface: " << hr << "\n").Write();

	hr = m_pDirectDraw->Destroy();
	if FAILED(hr)
		(log << "Failed to destroy directdraw: " << hr << "\n").Write();

	return S_OK;
}

HRESULT DWRendererDirectDraw::Clear()
{
	HRESULT hr;

	m_pDirectDraw->SetTickCount(m_tickCount);
	hr = m_pDirectDraw->Clear();
	if FAILED(hr)
		return (log << "Failed to clear directdraw: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWRendererDirectDraw::Present()
{
	HRESULT hr;

#ifdef DEBUG
	//Display FPS
	DWSurfaceText text;
	text.crTextColor = RGB(255, 255, 255);

	wchar_t buffer[30];
	swprintf((LPWSTR)&buffer, L"FPS - %f", m_pDirectDraw->GetFPS());
	text.SetText(buffer);
	hr = m_pSurface->DrawText(&text, 0, 560);
	hr = m_pSurface->DrawText(&text, 700, 560);
#endif

	//Flip
	hr = m_pDirectDraw->Flip();
	if FAILED(hr)
		return (log << "Failed to flip directdraw: " << hr << "\n").Write(hr);

	return hr;
}

