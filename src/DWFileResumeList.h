/**
 *	DWFileResumeList.h
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

#ifndef DWFILERESUMELIST_H
#define DWFILERESUMELIST_H

#include "XMLDocument.h"
#include "IDWOSDDataList.h"
#include "LogMessage.h"
#include <vector>
#include "XMLDocument.h"
#include "GlobalFunctions.h"

class DWFileResumeListItem
{
public:
	DWFileResumeListItem();
	virtual ~DWFileResumeListItem();

	LPWSTR resume;
	LPWSTR name;

	HRESULT SaveToXML(XMLElement *pElement);
};

class DWFileResumeList : public LogMessageCaller, public IDWOSDDataList
{
public:
	DWFileResumeList();
	virtual ~DWFileResumeList();

	HRESULT Destroy();

	HRESULT Initialise(int resumeSize);
	HRESULT Load(LPWSTR filename);
	HRESULT MakeFile(LPWSTR filename);
	HRESULT FindResumeName(LPWSTR pResumeName, int *pIndex);
	BOOL Save(LPWSTR filename);

	//IDWOSDDataList Methods
	virtual LPWSTR GetListName();
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex = 0);
	virtual long GetListSize();
	virtual void SetLogCallback(LogMessageCallback *callback);

	void SetListItem(LPWSTR name, LPWSTR value);

private:
	std::vector<DWFileResumeListItem *> m_list;
	CCritSec m_listLock;

	LPWSTR m_dataListName;
	LPWSTR m_filename;
	int m_ResumeSize;
};

#endif
