/**
 *	DWOSDData.h
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

#ifndef DWOSDDATA_H
#define DWOSDDATA_H

#include "DWOSDDataItem.h"
#include "IDWOSDDataList.h"
#include "DWOSDWindows.h"
#include "LogMessage.h"

class DWOSDData : public LogMessageCaller
{
private:
	class DWOSDDataList
	{
	public:
		DWOSDDataList()
		{
			name = NULL;
			list = NULL;
		}
		virtual ~DWOSDDataList()
		{
			if (name)
				delete[] name;
		}

		LPWSTR name;
		IDWOSDDataList *list;
	};

public:
	DWOSDData(DWOSDWindows *windows);
	virtual ~DWOSDData();

	void SetItem(LPWSTR name, LPWSTR value);
	LPWSTR GetItem(LPWSTR name);

	void AddList(IDWOSDDataList* list);
	void RotateList(IDWOSDDataList* list);
	void ClearAllListNames(LPWSTR pName);

	int GetListCount(LPWSTR pName);
	IDWOSDDataList* GetListFromListName(LPWSTR pName);
	IDWOSDDataList* GetListFromItemName(LPWSTR pName);

	HRESULT ReplaceTokens(LPWSTR pSource, LPWSTR &pResult, long ixDataList = 0);

private:
	HRESULT ReplaceVariable(LPWSTR pSrc, long *pSrcUsed, LPWSTR pResult, long resultSize, long ixDataList);

	std::vector<DWOSDDataItem *> m_items;
	CCritSec m_itemsLock;

	std::vector<DWOSDDataList *> m_lists;
	CCritSec  m_listsLock;

	DWOSDWindows *m_pWindows;
};

#endif
