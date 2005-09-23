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

HRESULT DWRenderer::SetTickCount(long tickCount)
{
	m_tickCount = tickCount;
	return S_OK;
}