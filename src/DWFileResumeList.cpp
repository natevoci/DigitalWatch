/**
 *	DWFileResumeList.cpp
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

#include "DWFileResumeList.h"

//////////////////////////////////////////////////////////////////////
// DWFileResumeListItem
//////////////////////////////////////////////////////////////////////

DWFileResumeListItem::DWFileResumeListItem()
{
	resume = NULL;
	name = NULL;
}

DWFileResumeListItem::~DWFileResumeListItem()
{
	if (resume)
		delete[] resume;
	if (name)
		delete[] name;
}

HRESULT DWFileResumeListItem::SaveToXML(XMLElement *pElement)
{
	pElement->Attributes.Add(new XMLAttribute(L"name", name));
	pElement->Attributes.Add(new XMLAttribute(L"resume", resume));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWFileResumeList
//////////////////////////////////////////////////////////////////////

DWFileResumeList::DWFileResumeList()
{
	m_filename = NULL;
	m_dataListName = NULL;
	m_ResumeSize = 50;
}

DWFileResumeList::~DWFileResumeList()
{
	Destroy();
	if (m_filename)
		delete[] m_filename;

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DWFileResumeList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<DWFileResumeListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (*it) delete *it;
	}
	m_list.clear();
	return S_OK;
}

void DWFileResumeList::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);
}

HRESULT DWFileResumeList::Initialise(int resumeSize)
{
	(log << "Initialising the Resume List \n").Write();

	m_ResumeSize = resumeSize;

	(log << "Finished Initialising the Resume List \n").Write();
	
	return S_OK;

}

LPWSTR DWFileResumeList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"ResumeInfo");
	return m_dataListName;
}

LPWSTR DWFileResumeList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DWFileResumeListItem *resumeItem = m_list.at(nIndex);
		if (_wcsicmp(name, L".resume") == 0)
			return resumeItem->resume;
		else if (_wcsicmp(name, L".name") == 0)
			return resumeItem->name;
	}
	return NULL;
}

long DWFileResumeList::GetListSize()
{
	CAutoLock listLock(&m_listLock);
	return m_list.size();
}

void DWFileResumeList::SetListItem(LPWSTR name, LPWSTR value)
{
	if (!name || !value)
		return;

	//do search for name and load resume time
	CAutoLock listLock(&m_listLock);
	std::vector<DWFileResumeListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, name) == 0)
		{
			strCopy((*it)->resume, value);
			return;
		}
	}

	// If not found then push it onto the list
	DWFileResumeListItem *resumeItem = new DWFileResumeListItem();
	strCopy(resumeItem->name, name);
	strCopy(resumeItem->resume, value);
	m_list.push_back(resumeItem);

	return;
}

HRESULT DWFileResumeList::Load(LPWSTR filename)
{

	(log << "Loading Resume Time file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		(log << "Could not load Resume Time file: " << m_filename << "\n").Show();
		if FAILED(MakeFile(filename))
			return (log << "Could not load or make the Resume Time File: " << m_filename << "\n").Show(hr);

		if FAILED(hr = file.Load(m_filename))
			return (log << "Could not load or make the Resume Time File: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;
	XMLAttribute *attr;

	DWFileResumeListItem *resumeItem;
	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"MediaFile") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if (!attr)
				continue;

			resumeItem = new DWFileResumeListItem();
			strCopy(resumeItem->name, attr->value);

			attr = element->Attributes.Item(L"resume");
			if (attr)
				strCopy(resumeItem->resume, attr->value);

			CAutoLock listLock(&m_listLock);
			m_list.push_back(resumeItem);
		}
	}

	(log << "Loaded " << (long)m_list.size() << " Resume Times\n").Write();

	indent.Release();
	(log << "Finished loading Resume Time file : " << hr << "\n").Write();
	return S_OK;
}

BOOL DWFileResumeList::Save(LPWSTR filename)
{
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	int count = 1;
	CAutoLock listLock(&m_listLock);
	if (!m_list.size())
		return TRUE;

	std::vector<DWFileResumeListItem *>::iterator it = m_list.end();
	it--;
	for ( ; it > m_list.begin() ; it-- )
	{
		// If it's a .tsbuffer file then skip
		long length = wcslen((*it)->name);
		if ((length >= 9) && (_wcsicmp((*it)->name+length-9, L".tsbuffer") == 0))
			continue;

		count++;
		// limit list
		if (count > m_ResumeSize)
			break;
	};

	for ( ; it < m_list.end() ; it++ )
	{
		// If it's a .tsbuffer file then skip
		long length = wcslen((*it)->name);
		if ((length >= 9) && (_wcsicmp((*it)->name+length-9, L".tsbuffer") == 0))
			continue;

		XMLElement *pElement = new XMLElement(L"MediaFile");
		DWFileResumeListItem *resumeItem = *it;
		resumeItem->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);
		
	return TRUE;
}

HRESULT DWFileResumeList::MakeFile(LPWSTR filename)
{
	(log << "Making the Resume Time file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);

	indent.Release();
	(log << "Finished Making the Resume Time File.\n").Write();
	return S_OK;
}

HRESULT DWFileResumeList::FindResumeName(LPWSTR pResumeName, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);
	std::vector<DWFileResumeListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, pResumeName) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}
