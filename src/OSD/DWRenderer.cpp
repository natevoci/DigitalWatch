/**
 *	DWRenderer.cpp
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

#include "DWRenderer.h"

DWRenderer::DWRenderer(DWSurface *pSurface)
{
	m_pSurface = pSurface;
	m_lTickCount = 0;
	m_bInitialised = FALSE;

	m_fFPS = 0;
	m_fpsTickCount = 0;
	m_fpsMultiplier = 1;

	m_backBufferWidth = 768;
	m_backBufferHeight = 576;
}

DWRenderer::~DWRenderer()
{
}

void DWRenderer::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	m_pSurface->SetLogCallback(callback);
}

HRESULT DWRenderer::GetSurface(DWSurface **ppDWSurface)
{
	if (!ppDWSurface)
		return E_POINTER;
	if (!m_pSurface)
		return E_FAIL;
	*ppDWSurface = m_pSurface;
	return S_OK;
}

long DWRenderer::GetTickCount()
{
	return m_lTickCount;
}

HRESULT DWRenderer::SetTickCount(long tickCount)
{
	m_lTickCount = tickCount;

	if (tickCount - m_fpsTickCount > 500)
	{
		m_fFPS = (long)(m_fpsMultiplier * 1000.0 / (double)(tickCount - m_fpsTickCount));
		m_fpsTickCount = tickCount;
		m_fpsMultiplier = 1;
	}
	else
	{
		m_fpsMultiplier++;
	}

	return S_OK;
}

double DWRenderer::GetFPS()
{
	return m_fFPS;
}

BOOL DWRenderer::Initialised()
{
	return m_bInitialised;
}

long DWRenderer::GetBackBufferWidth()
{
	return m_backBufferWidth;
}

long DWRenderer::GetBackBufferHeight()
{
	return m_backBufferHeight;
}
