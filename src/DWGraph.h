/**
 *	DWGraph.h
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

#ifndef DWGRAPH_H
#define DWGRAPH_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWMediaTypes.h"

class DWGraph  
{
public:
	DWGraph();
	virtual ~DWGraph();

	BOOL Initialise();
	BOOL Destroy();

	HRESULT QueryGraphBuilder(IGraphBuilder** piGraphBuilder);
	HRESULT QueryMediaControl(IMediaControl** piMediaControl);
	
	HRESULT Start();
	HRESULT Stop();

	HRESULT Cleanup();

	HRESULT RenderPin(IPin *piPin);

	HRESULT RefreshVideoPosition();

protected:
	void GetVideoRect(RECT *rect);

private:
	CComPtr<IGraphBuilder> m_piGraphBuilder;
	CComPtr<IMediaControl> m_piMediaControl;

	BOOL m_bInitialised;

	DWORD m_rotEntry;

	DWMediaTypes m_mediaTypes;

	LogMessage log;
};

#endif
