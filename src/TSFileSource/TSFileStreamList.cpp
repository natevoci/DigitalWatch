/**
 *	TSFileStreamList.cpp
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

#include "TSFileStreamList.h"
#include "TSFileSourceGuids.h"

//////////////////////////////////////////////////////////////////////
// TSFileStreamListItem
//////////////////////////////////////////////////////////////////////

TSFileStreamListItem::TSFileStreamListItem()
{
	index = NULL;
	media = NULL;
	lcid = NULL;
	group = NULL;
	name = NULL;
	ltext = NULL;
}

TSFileStreamListItem::~TSFileStreamListItem()
{
	if (index)
		delete[] index;
	if (media)
		delete[] media;
	if (lcid)
		delete[] lcid;
	if (group)
		delete[] group;
	if (name)
		delete[] name;
	if (ltext)
		delete[] ltext;
}

//////////////////////////////////////////////////////////////////////
// DVBTFrequencyList
//////////////////////////////////////////////////////////////////////

TSFileStreamList::TSFileStreamList()
{
	m_offset = 0;
	m_dataListName = NULL;
}

TSFileStreamList::~TSFileStreamList()
{
	Destroy();
	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT TSFileStreamList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<TSFileStreamListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		delete (*it);
	}
	m_list.clear();
	return S_OK;
}

HRESULT TSFileStreamList::Initialise(DWGraph* pFilterGraph)
{
	(log << "Initialising the Stream List \n").Write();

	m_pDWGraph = pFilterGraph;
	
	return S_OK;

}

HRESULT TSFileStreamList::LoadStreamList(LPWSTR filename)
{
	CAutoLock listLock(&m_listLock);

	(log << "Loading IAMStreamSelect Info: " "\n").Write();

	HRESULT hr;

	if (!m_piGraphBuilder)
	{
		hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder);
		if (FAILED(hr))
			return (log << "Unable to Query for the Graph Builder Interface\n").Show(E_FAIL);
	}

	IBaseFilter *pFilter;
	hr = graphTools.FindFilterByCLSID(m_piGraphBuilder, CLSID_TSFileSource, &pFilter);
	if (FAILED(hr))
		return (log << "Unable to find the TSFileSource Filter\n").Show(E_FAIL);;

	LogMessageIndent indent(&log);

	IAMStreamSelect *pIAMStreamSelect;
	hr = pFilter->QueryInterface(IID_IAMStreamSelect, (void**)&pIAMStreamSelect);
	if (SUCCEEDED(hr))
	{

		ULONG count;
		pIAMStreamSelect->Count(&count);
		(log << "Number of selections found : " << (int)count << "\n").Write();

		if (count)
		{
			LogMessageIndent indent(&log);
			Destroy();

			ULONG flags, group, lastgroup = -1, lcid;
				
			for(UINT i = 0; i < count; i++)
			{
				WCHAR* pStreamName = NULL;
//				AMMediaType media = NULL

				if(S_OK == pIAMStreamSelect->Info(i, 0, &flags, &lcid, &group, &pStreamName, 0, 0))
				{
//					if(lastgroup != group && i) 
//						::AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);

						lastgroup = group;

					if(pStreamName)
					{
						TSFileStreamListItem *item = new TSFileStreamListItem();
						(log << "Name of Stream found : " << pStreamName << "\n").Write();
						strCopy(item->index, (long)i);
						strCopy(item->media, L"MPEG2");
						strCopy(item->lcid, (long)lcid);
						strCopy(item->group, (long)group);
						strCopy(item->name, (LPWSTR)pStreamName);
						strCopy(item->ltext, (long)wcslen(pStreamName));
						CoTaskMemFree(pStreamName);
						m_list.push_back(item);

						continue;
					}
				}
			}
			indent.Release();

			if (m_list.size() == 0)
			{
				indent.Release();
				return (log << "No Streams found\n").Show(E_FAIL);
			}
			else
			{
				(log << "Saving the XML stream List file : " "\n").Write();
				LogMessageIndent indent(&log);

				CAutoLock lock(&m_listLock);

				XMLDocument file;
				file.SetLogCallback(m_pLogCallback);
//				if FAILED(hr = file.Load(filename))
//				{
//					return (log << "Could not load Stream list file: " << filename << "\n").Show(hr);
//				}

				LPWSTR pValue = NULL;

				std::vector<TSFileStreamListItem *>::iterator it = m_list.begin();
				for ( ; it != m_list.end() ; it++ )
				{
					TSFileStreamListItem *pStreams = *it;
					XMLElement *pStreamElement = new XMLElement(L"Stream");
					pStreamElement->Attributes.Add(new XMLAttribute(L"Index", (pStreams->index ? (LPWSTR)pStreams->index : L"")));
					pStreamElement->Attributes.Add(new XMLAttribute(L"Media", (pStreams->media ? (LPWSTR)pStreams->media : L"")));
					pStreamElement->Attributes.Add(new XMLAttribute(L"Lcid", (pStreams->lcid ? (LPWSTR)pStreams->lcid : L"")));
					pStreamElement->Attributes.Add(new XMLAttribute(L"Group", (pStreams->group ? (LPWSTR)pStreams->group : L"")));
					pStreamElement->Attributes.Add(new XMLAttribute(L"Name", (pStreams->name ? (LPWSTR)pStreams->name : L"")));
					pStreamElement->Attributes.Add(new XMLAttribute(L"Ltext", (pStreams->ltext ? (LPWSTR)pStreams->ltext : L"")));

					file.Elements.Add(pStreamElement);
				}
				if (filename)
					file.Save(filename);

				indent.Release();
			}
		}
	}

	indent.Release();
	(log << "Finished Loading the Streams List: " "\n").Write();

	return S_OK;
}

LPWSTR TSFileStreamList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"StreamInfo");
	return m_dataListName;
}

LPWSTR TSFileStreamList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		TSFileStreamListItem *item = m_list.at(nIndex);
		if (_wcsicmp(name, L".index") == 0)
			return item->index;
		if (_wcsicmp(name, L".media") == 0)
			return item->media;
		else if (_wcsicmp(name, L".lcid") == 0)
			return item->lcid;
		else if (_wcsicmp(name, L".group") == 0)
			return item->group;
		else if (_wcsicmp(name, L".name") == 0)
			return item->name;
		else if (_wcsicmp(name, L".ltext") == 0)
			return item->ltext;
	}
	return NULL;
}

long TSFileStreamList::GetListSize()
{
	CAutoLock listLock(&m_listLock);

	return m_list.size();
}

