/**
 *	DWMulticastingList.cpp
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
#include "Globals.h"
#include "DWMulticastingList.h"

//////////////////////////////////////////////////////////////////////
// DWMulticastingListItem
//////////////////////////////////////////////////////////////////////

DWMulticastingListItem::DWMulticastingListItem()
{
	address = NULL;
	name = NULL;
}

DWMulticastingListItem::~DWMulticastingListItem()
{
	if (address)
		delete[] address;
	if (name)
		delete[] name;
}

HRESULT DWMulticastingListItem::SaveToXML(XMLElement *pElement)
{
	pElement->Attributes.Add(new XMLAttribute(L"name", name));
	pElement->Attributes.Add(new XMLAttribute(L"address", address));
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWMulticastingList
//////////////////////////////////////////////////////////////////////

DWMulticastingList::DWMulticastingList()
{
	m_filename = NULL;
	m_dataListName = NULL;
	m_MulticastSize = 50;
}

DWMulticastingList::~DWMulticastingList()
{
	Destroy();
	if (m_filename)
		delete[] m_filename;

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DWMulticastingList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<DWMulticastingListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (*it) delete *it;
	}
	m_list.clear();
	return S_OK;
}

void DWMulticastingList::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);
}

HRESULT DWMulticastingList::Initialise(int multicastSize)
{
	(log << "Initialising the Multicast List \n").Write();

	m_MulticastSize = multicastSize;

	(log << "Finished Initialising the Multicast List \n").Write();
	
	return S_OK;

}

LPWSTR DWMulticastingList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"MulticastInfo");
	return m_dataListName;
}

LPWSTR DWMulticastingList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DWMulticastingListItem *multicastItem = m_list.at(nIndex);
		if (_wcsicmp(name, L".address") == 0)
			return multicastItem->address;
		else if (_wcsicmp(name, L".name") == 0)
			return multicastItem->name;
	}
	return NULL;
}

long DWMulticastingList::GetListSize()
{
	CAutoLock listLock(&m_listLock);
	return m_list.size();
}

void DWMulticastingList::SetListItem(LPWSTR name, LPWSTR value)
{
	if (!name || !value)
		return;

	//do search for name and load multicast address
	CAutoLock listLock(&m_listLock);
	std::vector<DWMulticastingListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, name) == 0)
		{
			strCopy((*it)->address, value);
			return;
		}
	}

	// If not found then push it onto the list
	DWMulticastingListItem *multicastItem = new DWMulticastingListItem();
	strCopy(multicastItem->name, name);
	strCopy(multicastItem->address, value);
	m_list.push_back(multicastItem);

	return;
}

HRESULT DWMulticastingList::Load(LPWSTR filename)
{

	(log << "Loading Multicast List file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		(log << "Could not load Multicast list file: " << m_filename << "\n").Show();
		if FAILED(MakeFile(filename))
			return (log << "Could not load or make the Multicast list File: " << m_filename << "\n").Show(hr);

		Destroy();

		if FAILED(hr = file.Load(m_filename))
			return (log << "Could not load or make the Multicast list File: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;
	XMLAttribute *attr;

	DWMulticastingListItem *multicastItem;
	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"Multicast") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if (!attr)
				continue;

			multicastItem = new DWMulticastingListItem();
			strCopy(multicastItem->name, attr->value);

			attr = element->Attributes.Item(L"address");
			if (attr)
				strCopy(multicastItem->address, attr->value);

			CAutoLock listLock(&m_listLock);
			m_list.push_back(multicastItem);
		}
	}

	(log << "Loaded " << (long)m_list.size() << " Multicast List Items\n").Write();

	indent.Release();
	(log << "Finished loading Multicast List file : " << hr << "\n").Write();
	return S_OK;
}

BOOL DWMulticastingList::Save(LPWSTR filename)
{
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	int count = 1;
	CAutoLock listLock(&m_listLock);
	if (!m_list.size())
		return TRUE;

	std::vector<DWMulticastingListItem *>::iterator it = m_list.end();
	it--;
	for ( ; it > m_list.begin() ; it-- )
	{
		// If it's a .tsbuffer file then skip
		long length = wcslen((*it)->name);
		if ((length >= 9) && (_wcsicmp((*it)->name+length-9, L".tsbuffer") == 0))
			continue;

		count++;
		// limit list
		if (count > m_MulticastSize)
			break;
	};

	for ( ; it < m_list.end() ; it++ )
	{
		// If it's a .tsbuffer file then skip
		long length = wcslen((*it)->name);
		if ((length >= 9) && (_wcsicmp((*it)->name+length-9, L".tsbuffer") == 0))
			continue;

		XMLElement *pElement = new XMLElement(L"Multicast");
		DWMulticastingListItem *multicastItem = *it;
		multicastItem->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);
		
	return TRUE;
}

HRESULT DWMulticastingList::MakeFile(LPWSTR filename)
{
	(log << "Making the Multicast List file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	SetListItem(L"Multicast Channel 1", L"udp://224.0.0.1:1234@127.0.0.1");
	SetListItem(L"Multicast Channel 2", L"udp://224.0.0.2:1234@127.0.0.1");
	SetListItem(L"Multicast Channel 3", L"udp://224.0.0.3:1234@127.0.0.1");
	SetListItem(L"Multicast Channel 4", L"udp://224.0.0.4:1234@127.0.0.1");
	SetListItem(L"Multicast Channel 5", L"udp://224.0.0.5:1234@127.0.0.1");

	if (filename)
		Save(filename);
	else
		Save(m_filename);

	indent.Release();
	(log << "Finished Making the Multicast List File.\n").Write();
	return S_OK;
}

HRESULT DWMulticastingList::FindMulticastName(LPWSTR pMulticastName, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);
	std::vector<DWMulticastingListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, pMulticastName) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}
