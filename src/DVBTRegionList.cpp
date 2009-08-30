/**
 *	DVBTRegionList.cpp
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

#include "DVBTRegionList.h"
#include "XMLDocument.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DVBTRegionListItem
//////////////////////////////////////////////////////////////////////

DVBTRegionListItem::DVBTRegionListItem()
{
	name = NULL;
	regionPath = NULL;
}

DVBTRegionListItem::~DVBTRegionListItem()
{
	if (name)
		delete[] name;

	if (regionPath)
		delete[] regionPath;
}

HRESULT DVBTRegionListItem::SaveToXML(XMLElement *pElement)
{
	pElement->Attributes.Add(new XMLAttribute(L"name", name));
	pElement->Attributes.Add(new XMLAttribute(L"regionPath", regionPath));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DVBTRegionList
//////////////////////////////////////////////////////////////////////

DVBTRegionList::DVBTRegionList()
{
	m_offset = 0;
	m_dataListName = NULL;
	m_filename = NULL;
}

DVBTRegionList::~DVBTRegionList()
{
	Destroy();

	if (m_filename)
		delete[] m_filename;

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DVBTRegionList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<DVBTRegionListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		DVBTRegionListItem *item = *it;
		if (item)
			delete item;

		item = NULL;
	}
	m_list.clear();
	return S_OK;
}

HRESULT DVBTRegionList::LoadRegionList(LPWSTR filename)
{
	CAutoLock listLock(&m_listLock);

	(log << "Loading DVBT Region List file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		(log << "Could not load media types file: " << m_filename <<"\n").Show();
		if FAILED(hr = MakeFile(filename))
			return (log << "Could not load or make the Region list File: " << m_filename << "\n").Show(hr);

		Destroy();

		if FAILED(hr = file.Load(m_filename))
			return (log << "Could not load or make the Region list File: " << m_filename << "\n").Show(hr);
	}

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *pElement = file.Elements.Item(item);
		if (_wcsicmp(pElement->name, L"Region") == 0)
		{
			XMLAttribute *attr;
			DVBTRegionListItem *item = new DVBTRegionListItem();

			attr = pElement->Attributes.Item(L"name");
			if (attr == NULL)
			{
				delete item;
				return (log << "Region must be supplied in a name definition\n").Write(E_FAIL);
			}
			strCopy(item->name, attr->value);

			attr = pElement->Attributes.Item(L"regionPath");
			if (attr == NULL)
			{
				delete item;
				return (log << "Region must be supplied in a path definition\n").Write(E_FAIL);
			}
			strCopy(item->regionPath, attr->value);
			m_list.push_back(item);

			continue;
		}

	}

	if (m_list.size() == 0)
		return (log << "You need to specify at least one path in your Regions file\n").Show(E_FAIL);

	indent.Release();
	(log << "Finished Loading Region List file: " << filename << "\n").Write();

	return S_OK;
}

LPWSTR DVBTRegionList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"RegionList");
	return m_dataListName;
}

LPWSTR DVBTRegionList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DVBTRegionListItem *item = m_list.at(nIndex);
		if (_wcsicmp(name, L".regionPath") == 0)
			return item->regionPath;
		else if (_wcsicmp(name, L".name") == 0)
			return item->name;
	}
	return NULL;
}

long DVBTRegionList::GetListSize()
{
	CAutoLock listLock(&m_listLock);
	return m_list.size();
}

HRESULT DVBTRegionList::FindListItem(LPWSTR name, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);
	std::vector<DVBTRegionListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, name) == 0)
			return S_OK;

		if (_wcsicmp((*it)->regionPath, name) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}

HRESULT DVBTRegionList::ParseDirectoryList(LPWSTR path)
{
	LPWSTR newPath = new WCHAR[MAX_PATH];
	LPWSTR pathname = NULL;
	strCopy(pathname, path);
	
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;

	BOOL nextDir = TRUE;

	StringCchPrintfW(newPath, MAX_PATH, L"%s%S", pathname, L"\\*");

	hFind = FindFirstFileExW(newPath, FindExInfoStandard, &FindFileData,
	FindExSearchLimitToDirectories, NULL, 0 );

	if (hFind == INVALID_HANDLE_VALUE) 
	{
		delete[] pathname;
		int error = GetLastError();
		(log << "Invalid File Handle. GetLastError reports: " << error << "\n").Write();
		return E_FAIL;
	} 
	else 
	{
		BOOL bFound = TRUE;
		while (bFound)
		{
			if (FindFileData.cFileName && _wcsicmp(FindFileData.cFileName, L"frequencylist.xml") == 0)
			{
				DVBTRegionListItem *item = new DVBTRegionListItem();
				strCopy(item->regionPath, pathname);
				LPWSTR pos = wcsrchr(pathname, '\\');
				if (pos > pathname)
					strCopy(item->name, pos+1);

				CAutoLock listLock(&m_listLock);
				m_list.push_back(item);
				(log << "Found Frequency List File for : " << item->name << "\n").Write();
			}
			else if (FindFileData.cFileName && (FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) && wcschr(FindFileData.cFileName, '.') == NULL)
			{
				LPWSTR tempPath = new WCHAR[MAX_PATH];
				StringCchPrintfW(tempPath, MAX_PATH, L"%S\\%S", pathname, FindFileData.cFileName);

				(log << "Searching Directory: " << tempPath << "\n").Write();
				ParseDirectoryList(tempPath);
			}
			bFound = FindNextFileW(hFind, &FindFileData);
		};

		FindClose(hFind);
	}
	delete[] pathname;
	return S_OK;
}

HRESULT DVBTRegionList::MakeFile(LPWSTR filename)
{
	(log << "Making the Region List file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	strCopy(m_filename, filename);

	Destroy();

	LPWSTR path = new WCHAR[MAX_PATH];
	LPWSTR pathname = NULL;
	LPWSTR pos = wcsrchr(m_filename, '\\');
	if (pos > m_filename)
		strCopy(pathname, m_filename, pos - m_filename);
	else
		strCopy(pathname, m_filename);
		
	StringCchPrintfW(path, MAX_PATH, L"%S%S", pathname, L"\\Regions");
	delete[] pathname;
	
	ParseDirectoryList(path);

	CAutoLock listLock(&m_listLock);
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	std::vector<DVBTRegionListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		XMLElement *pElement = new XMLElement(L"Region");
		DVBTRegionListItem *regionItem = *it;
		regionItem->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);


	indent.Release();
	(log << "Finished Making the Media Types File.\n").Write();
	return S_OK;
}

