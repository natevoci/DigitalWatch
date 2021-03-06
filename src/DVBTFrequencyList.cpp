/**
 *	DVBTFrequencyList.cpp
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

#include "DVBTFrequencyList.h"
#include "XMLDocument.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DVBTFrequencyListItem
//////////////////////////////////////////////////////////////////////

DVBTFrequencyListItem::DVBTFrequencyListItem()
{
	frequencyLow = NULL;
	frequencyCentre = NULL;
	frequencyHigh = NULL;
	bandwidth = NULL;
}

DVBTFrequencyListItem::~DVBTFrequencyListItem()
{
	if (frequencyLow)
		delete[] frequencyLow;
	if (frequencyCentre)
		delete[] frequencyCentre;
	if (frequencyHigh)
		delete[] frequencyHigh;
	if (bandwidth)
		delete[] bandwidth;
}

//////////////////////////////////////////////////////////////////////
// DVBTFrequencyList
//////////////////////////////////////////////////////////////////////

DVBTFrequencyList::DVBTFrequencyList()
{
	m_offset = 0;
	m_dataListName = NULL;
}

DVBTFrequencyList::~DVBTFrequencyList()
{
	Destroy();
	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DVBTFrequencyList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<DVBTFrequencyListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		DVBTFrequencyListItem *item = *it;
		if (item)
			delete item;

		item = NULL;
	}
	m_list.clear();
	return S_OK;
}

HRESULT DVBTFrequencyList::LoadFrequencyList(LPWSTR filename)
{
	CAutoLock listLock(&m_listLock);

	(log << "Loading DVBT Frequency List file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(filename))
	{
		return (log << "Could not load frequency list file: " << filename << "\n").Show(hr);
	}

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *pElement = file.Elements.Item(item);
		if (_wcsicmp(pElement->name, L"Network") == 0)
		{
			XMLAttribute *attr;
			DVBTFrequencyListItem *item = new DVBTFrequencyListItem();

			attr = pElement->Attributes.Item(L"Frequency");
			if (attr == NULL)
			{
				delete item;
				return (log << "Frequency must be supplied in a network definition\n").Write(E_FAIL);
			}

			long frequency = _wtoi(attr->value);
			strCopy(item->frequencyLow, frequency-125);
			strCopy(item->frequencyCentre, frequency);
			strCopy(item->frequencyHigh, frequency+125);

			attr = pElement->Attributes.Item(L"Bandwidth");
			if (attr == NULL)
			{
				delete item;
				return (log << "Bandwidth must be supplied in a network definition\n").Write(E_FAIL);
			}
			strCopy(item->bandwidth, attr->value);

			m_list.push_back(item);

			continue;
		}

	}

	if (m_list.size() == 0)
		return (log << "You need to specify at least one network in your channels file\n").Show(E_FAIL);

	indent.Release();
	(log << "Finished Loading Frequency List file: " << filename << "\n").Write();

	return S_OK;
}

HRESULT DVBTFrequencyList::ChangeOffset(long change)
{
	if (change >= 0)
		m_offset++;
	if (change < 0)
		m_offset--;

	if (change && m_offset > 1)
		m_offset = 1;
	if (change && m_offset < -1)
		m_offset = -1;
	else if (m_offset > 1)
		m_offset = -1;
	return S_OK;
}

LPWSTR DVBTFrequencyList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"FrequencyList");
	return m_dataListName;
}

LPWSTR DVBTFrequencyList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DVBTFrequencyListItem *item = m_list.at(nIndex);
		if (_wcsicmp(name, L".frequency") == 0)
		{
			if (m_offset < 0)
				return item->frequencyLow;
			if (m_offset == 0)
				return item->frequencyCentre;
			else
				return item->frequencyHigh;
		}
		else if (_wcsicmp(name, L".bandwidth") == 0)
		{
			return item->bandwidth;
		}
	}
	return NULL;
}

long DVBTFrequencyList::GetListSize()
{
	CAutoLock listLock(&m_listLock);
	return m_list.size();
}

HRESULT DVBTFrequencyList::FindListItem(LPWSTR name, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);
	std::vector<DVBTFrequencyListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->bandwidth, name) == 0)
			return S_OK;
		if (_wcsicmp((*it)->frequencyLow, name) == 0)
			return S_OK;
		if (_wcsicmp((*it)->frequencyCentre, name) == 0)
			return S_OK;
		if (_wcsicmp((*it)->frequencyHigh, name) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}



