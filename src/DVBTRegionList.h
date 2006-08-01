/**
 *	DVBTRegionList.h
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

#ifndef DVBTREGIONLIST_H
#define DVBTREGIONLIST_H

#include "XMLDocument.h"
#include "IDWOSDDataList.h"
#include "LogMessage.h"
#include <vector>

class DVBTRegionListItem
{
public:
	DVBTRegionListItem();
	virtual ~DVBTRegionListItem();
	HRESULT SaveToXML(XMLElement *pElement);

	LPWSTR name;
	LPWSTR regionPath;
};

class DVBTRegionList : public LogMessageCaller, public IDWOSDDataList
{
public:
	DVBTRegionList();
	virtual ~DVBTRegionList();

	virtual HRESULT Destroy();

	virtual	HRESULT ParseDirectoryList(LPWSTR path);
	virtual HRESULT LoadRegionList(LPWSTR filename);

	//IDWOSDDataList Methods
	virtual LPWSTR GetListName();
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex = 0);
	virtual long GetListSize();
	virtual HRESULT FindListItem(LPWSTR name, int *pIndex);

private:
	std::vector<DVBTRegionListItem *> m_list;
	CCritSec m_listLock;

	HRESULT MakeFile(LPWSTR filename);
	long m_offset;

	LPWSTR m_filename;
	LPWSTR m_dataListName;
};

#endif
