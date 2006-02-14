//Here's the places in the code you need to look to add recording.
//In BDADVBTSourceTuner::AddSourceFilters add code to insert the extra demux and dump filter.
//Implement recording methods on BDADVBTSourceTuner. A list of the one's i thought would be needed are commented out in BDADVBTSourceTuner.h.
//Add code to the BDADVBTSource::ExecuteCommand method to handle the key functions defined in bin/BDA_DVB-T/keys.xml and call the methods you implemented on m_pCurrentTuner.
/**
 *	TSFileStreamList.h
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

#ifndef TSFILESTREAMLIST_H
#define TSFILESTREAMLIST_H

#include "DWGraph.h"
#include "XMLDocument.h"
#include "IDWOSDDataList.h"
#include "LogMessage.h"
#include <vector>
#include "XMLDocument.h"
#include "GlobalFunctions.h"
#include "FilterGraphTools.h"

class TSFileStreamListItem
{
public:
	TSFileStreamListItem();
	virtual ~TSFileStreamListItem();

	LPWSTR index;
	LPWSTR media;
	LPWSTR lcid;
	LPWSTR group;
	LPWSTR name;
	LPWSTR ltext;
};

class TSFileStreamList : public LogMessageCaller, public IDWOSDDataList
{
public:
	TSFileStreamList();
	virtual ~TSFileStreamList();

	virtual HRESULT Destroy();

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT LoadStreamList(LPWSTR filename);

	//IDWOSDDataList Methods
	virtual LPWSTR GetListName();
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex = 0);
	virtual long GetListSize();
private:
	std::vector<TSFileStreamListItem *> m_list;
	CCritSec m_listLock;

	DWGraph *m_pDWGraph;
	CComPtr <IGraphBuilder> m_piGraphBuilder;
	FilterGraphTools graphTools;

	long m_offset;

	LPWSTR m_dataListName;
};

#endif
