/**
 *	FilterPropList.h
 *	Copyright (C) 2005 Nate
 *	Copyright (C) 2006 Bear
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

#ifndef FILTERPROPLIST_H
#define FILTERPROPLIST_H

#include "DWGraph.h"
#include "XMLDocument.h"
#include "IDWOSDDataList.h"
#include "LogMessage.h"
#include <vector>
#include "XMLDocument.h"
#include "GlobalFunctions.h"
#include "FilterGraphTools.h"

class FilterPropListItem
{
public:
	FilterPropListItem();
	virtual ~FilterPropListItem();

	LPWSTR index;
	LPWSTR flags;
	LPWSTR name;
};

class FilterPropList : public LogMessageCaller, public IDWOSDDataList
{
public:
	FilterPropList();
	virtual ~FilterPropList();

	virtual HRESULT Destroy();

	virtual HRESULT Initialise(IGraphBuilder *piGraphBuilder, LPWSTR ListName = NULL);
	virtual HRESULT LoadFilterList(BOOL bLogOutput = TRUE);

	//IDWOSDDataList Methods
	virtual LPWSTR GetListName();
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex = 0);
	virtual long GetListSize();

	virtual HRESULT FindFilterName(LPWSTR pFilterName, int *pIndex);
	virtual HRESULT ShowFilterProperties(HWND hWnd, LPWSTR filterName, int index);

private:
	std::vector<FilterPropListItem *> m_list;
	CCritSec m_listLock;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	FilterGraphTools graphTools;
	HRESULT GetFilterProperties(LPWSTR *pfilterName, int *pCount, UINT * pFlags);

	long m_offset;

	LPWSTR m_dataListName;
};

#endif
