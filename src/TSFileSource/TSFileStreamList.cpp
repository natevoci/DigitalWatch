/**
 *	TSFileStreamList.cpp
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

#include "TSFileStreamList.h"
#include "TSFileSourceGuids.h"

//////////////////////////////////////////////////////////////////////
// TSFileStreamListItem
//////////////////////////////////////////////////////////////////////

TSFileStreamListItem::TSFileStreamListItem()
{
	index = NULL;
	media = NULL;
	flags = NULL;
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
	if (flags)
		delete[] flags;
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

HRESULT TSFileStreamList::Initialise(IGraphBuilder* piGraphBuilder)
{
	(log << "Initialising the Stream List \n").Write();

	m_piGraphBuilder = piGraphBuilder;

	(log << "Finished Initialising the Stream List \n").Write();
	
	return S_OK;

}

HRESULT TSFileStreamList::LoadStreamList(BOOL bLogOutput)
{
	CAutoLock listLock(&m_listLock);

	if (bLogOutput)
		(log << "Loading IAMStreamSelect Info: " "\n").Write();

	HRESULT hr;

	if (!m_piGraphBuilder)
		return (log << "Graph Builder Interface not yet Initalised\n").Show(E_FAIL);

	IBaseFilter *pFilter;
	hr = graphTools.FindFilterByCLSID(m_piGraphBuilder, CLSID_TSFileSource, &pFilter);
	if (FAILED(hr))
		return (log << "Unable to find the TSFileSource Filter\n").Show(E_FAIL);;

	LogMessageIndent indent(&log);

	CComPtr<IAMStreamSelect>pIAMStreamSelect;
	hr = pFilter->QueryInterface(IID_IAMStreamSelect, (void**)&pIAMStreamSelect);
	if (SUCCEEDED(hr))
	{
		pFilter->Release();

		ULONG count = 0;
		pIAMStreamSelect->Count(&count);
		if (bLogOutput)
			(log << "Number of selections found : " << (int)count << "\n").Write();

		if (count)
		{
			LogMessageIndent indent(&log);
			Destroy();

			ULONG flags, group, lastgroup = -1, lcid;
				
			for(UINT i = 0; i < count; i++)
			{
				WCHAR* pStreamName = NULL;
				if(S_OK == pIAMStreamSelect->Info(i, 0, &flags, &lcid, &group, &pStreamName, 0, 0))
				{
					if(pStreamName)
					{
						if (bLogOutput)
							(log << "Stream found : " << pStreamName << "\n").Write();

						TSFileStreamListItem *item = new TSFileStreamListItem();
						strCopy(item->index, (long)i);
						strCopy(item->media, L"MPEG2");
						strCopy(item->flags, (long)flags);
						strCopy(item->lcid, (long)lcid);
						strCopy(item->group, (long)group);

						item->name = new WCHAR[wcslen((LPWSTR)pStreamName) + sizeof(WCHAR)*5];
						if (flags & AMSTREAMSELECTINFO_EXCLUSIVE)
							StringCchPrintfW(item->name, (wcslen((LPWSTR)pStreamName) + sizeof(WCHAR)*5), L"  * %s",(LPWSTR)pStreamName);  
						else if (flags & AMSTREAMSELECTINFO_ENABLED)
							StringCchPrintfW(item->name, (wcslen((LPWSTR)pStreamName) + sizeof(WCHAR)*5), L"->  %s",(LPWSTR)pStreamName);  
						else
							StringCchPrintfW(item->name, (wcslen((LPWSTR)pStreamName) + sizeof(WCHAR)*5), L"    %s",(LPWSTR)pStreamName);  

						strCopy(item->ltext, (long)(wcslen(pStreamName)+8));
						CoTaskMemFree(pStreamName);
						m_list.push_back(item);

						continue;
					}
				}
			}

			if (m_list.size() == 0)
			{
				indent.Release();
				if (bLogOutput)
					return (log << "No Streams found\n").Show(E_FAIL);
				else
					return E_FAIL;
			}
		}
	}
	else
		pFilter->Release();

	indent.Release();
	if (bLogOutput)
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

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		TSFileStreamListItem *item = m_list.at(nIndex);
		if (_wcsicmp(name, L".index") == 0)
			return item->index;
		else if (_wcsicmp(name, L".media") == 0)
			return item->media;
		else if (_wcsicmp(name, L".flags") == 0)
			return item->flags;
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

HRESULT TSFileStreamList::FindListItem(LPWSTR name, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);
	std::vector<TSFileStreamListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, name) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}

LPWSTR TSFileStreamList::GetServiceName()
{
	CAutoLock listLock(&m_listLock);

	if (!m_list.size())
		return NULL;

	for (int index = 0; index < (int)m_list.size(); index++)
	{
		long flag =	StringToLong(m_list[index]->flags);
		if (flag & AMSTREAMSELECTINFO_ENABLED)
			return m_list[index]->name;
	}

	return NULL;
}

HRESULT TSFileStreamList::FindServiceName(LPWSTR pServiceName, int *pIndex)
{
	CAutoLock listLock(&m_listLock);
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	if (!m_list.size())
		return E_FAIL;

	HRESULT hr;

	if FAILED(hr = LoadStreamList(FALSE))
		return (log << "Failed to get a Stream List: " << hr << "\n").Write(hr);

	//return if service is already selected
	if (GetServiceName() && pServiceName)
	{
		if (wcsstr(GetServiceName(), pServiceName) != NULL)
			return S_FALSE;
	}
	else
		return E_FAIL;


	for (int index = 1; index < (int)m_list.size(); index++)
	{
		if (wcsstr(m_list[index]->name, pServiceName) != NULL)
		{
			*pIndex = index;
			return S_OK;
		}
	}
	return E_FAIL;
}