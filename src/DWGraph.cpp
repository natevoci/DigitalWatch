/**
 *	DWGraph.cpp
 *	Copyright (C) 2004 Nate
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

#include "DWGraph.h"
#include "Globals.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWGraph::DWGraph()
{
	m_piGraphBuilder = NULL;
	m_piMediaControl = NULL;
	m_bInitialised = FALSE;
	m_rotEntry = 0;
}

DWGraph::~DWGraph()
{

}

BOOL DWGraph::Initialise()
{
	HRESULT hr;
	if (m_bInitialised)
		return (g_log << "DigitalWatch graph tried to initialise a second time").Write();

	//--- COM should already be initialized ---

	//--- Create Graph ---
	if (FAILED(hr = m_piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER)))
		return (g_log << "Failed Creating Graph Builder").Write();

	//--- Add To Running Object Table --- (for graphmgr.exe)
	if (g_pData->settings.application.addToROT)
	{
		if (FAILED(hr = AddToRot(m_piGraphBuilder, &m_rotEntry)))
		{
			//TODO: release graphbuilder
			return (g_log << "Failed adding graph to ROT").Write();
		}
	}

	//--- Get InterfacesInFilters ---
	if (FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_piMediaControl.p)))
		return (g_log << "Failed to get Media Control interface").Write();

//	m_pOverlayCallback = new OverlayCallback(g_pData->hWnd, &hr);
	m_bInitialised = TRUE;

	return TRUE;
}

BOOL DWGraph::Destroy()
{
	if (m_piMediaControl)
		m_piMediaControl.Release();

	if (m_rotEntry)
	{
		RemoveFromRot(m_rotEntry);
		m_rotEntry = 0;
	}

	if (m_piGraphBuilder)
		m_piGraphBuilder.Release();

	m_bInitialised = FALSE;

	return TRUE;
}

HRESULT DWGraph::GetGraphBuilder(CComPtr<IGraphBuilder> &piGraphBuilder)
{
	if (!m_piGraphBuilder)
		return S_FALSE;
	piGraphBuilder = m_piGraphBuilder;
	return S_OK;
}
	
HRESULT DWGraph::GetMediaControl(CComPtr<IMediaControl> &piMediaControl)
{
	if (!m_piMediaControl)
		return S_FALSE;
	piMediaControl = m_piMediaControl;
	return S_OK;
}
	

