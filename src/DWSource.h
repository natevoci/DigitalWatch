/**
 *	DWSource.h
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

#ifndef DWSOURCE_H
#define DWSOURCE_H

#include "StdAfx.h"
#include "DWGraph.h"
#include "ParseLine.h"
#include "KeyMap.h"

class DWSource : public LogMessageCaller
{
public:
	/* General */
	virtual LPWSTR GetSourceType() = 0;

	virtual HRESULT Initialise(DWGraph* pFilterGraph) = 0;
	virtual HRESULT Destroy() = 0;

	/* Interface */
	virtual HRESULT GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function);
	virtual HRESULT ExecuteCommand(ParseLine* command) = 0;
	//Keys, ControlBar, OSD, Menu, etc...

	virtual HRESULT Play() = 0;

	/* Filtergraph */
/*	virtual HRESULT AddFilters() = 0;
	virtual HRESULT Connect() = 0;
	virtual HRESULT AfterGraphBuilt() = 0;
	virtual HRESULT Cleanup() = 0;
*/	

protected:
	KeyMap m_sourceKeyMap;

};

#endif
