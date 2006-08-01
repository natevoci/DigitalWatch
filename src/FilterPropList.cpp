/**
 *	FilterPropList.cpp
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

#include "FilterPropList.h"

//////////////////////////////////////////////////////////////////////
// FilterPropListItem
//////////////////////////////////////////////////////////////////////

FilterPropListItem::FilterPropListItem()
{
	index = NULL;
	flags = NULL;
	name = NULL;
}

FilterPropListItem::~FilterPropListItem()
{
	if (index)
		delete[] index;
	if (flags)
		delete[] flags;
	if (name)
		delete[] name;
}

//////////////////////////////////////////////////////////////////////
// DVBTFrequencyList
//////////////////////////////////////////////////////////////////////

FilterPropList::FilterPropList()
{
	m_offset = 0;
	m_dataListName = NULL;
}

FilterPropList::~FilterPropList()
{
	Destroy();
	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT FilterPropList::Destroy()
{
	CAutoLock listLock(&m_listLock);

	std::vector<FilterPropListItem *>::iterator it = m_list.begin();
	for ( ; it < m_list.end() ; it++ )
	{
		if (*it) delete *it;
	}
	m_list.clear();
	return S_OK;
}

HRESULT FilterPropList::Initialise(IGraphBuilder *piGraphBuilder, LPWSTR listName)
{
	(log << "Initialising the Filter Property List \n").Write();

	m_piGraphBuilder = piGraphBuilder;
	
	if (listName)
		strCopy(m_dataListName, listName);

	(log << "Finished Initialising the Filter Property List \n").Write();
	
	return S_OK;

}

HRESULT FilterPropList::LoadFilterList(BOOL bLogOutput)
{
	CAutoLock listLock(&m_listLock);

	if (bLogOutput)
		(log << "Loading Filter Property Info: " "\n").Write();

	if (!m_piGraphBuilder)
		return (log << "Graph Builder Interface not yet initalised\n").Show(E_FAIL);

	LogMessageIndent indent(&log);

	int count = 0;
	GetFilterProperties(NULL, &count, NULL);

	if(count)
	{
		Destroy();
		for(int i = 1; i < count+1; i++)
		{
			int index = i;
			LPOLESTR filterName = NULL;
			UINT uFlags = FALSE; 
			if(S_OK == GetFilterProperties(&filterName, &index, &uFlags))
			{
				if(filterName)
				{
					FilterPropListItem *item = new FilterPropListItem();
					strCopy(item->index, (long)i);
					strCopy(item->flags, (long)uFlags);
					strCopy(item->name, (LPWSTR)filterName);
					delete [] filterName;
					m_list.push_back(item);

					continue;
				}
			}	
		}
	}

	if (m_list.size() == 0)
	{
		indent.Release();
		if (bLogOutput)
			return (log << "No Filter Properties found.\n").Show(E_FAIL);
		else
			return E_FAIL;
	}

	indent.Release();
	if (bLogOutput)
		(log << "Finished Loading the Filter Properties List: " "\n").Write();

	return S_OK;
}

LPWSTR FilterPropList::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"FilterInfo");
	return m_dataListName;
}

LPWSTR FilterPropList::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock listLock(&m_listLock);

	if (nIndex >= (long)m_list.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		FilterPropListItem *item = m_list.at(nIndex);
		if (_wcsicmp(name, L".index") == 0)
			return item->index;
		else if (_wcsicmp(name, L".flags") == 0)
			return item->flags;
		else if (_wcsicmp(name, L".name") == 0)
			return item->name;
	}
	return NULL;
}

long FilterPropList::GetListSize()
{
	CAutoLock listLock(&m_listLock);
	return m_list.size();
}

HRESULT FilterPropList::FindListItem(LPWSTR name, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock listLock(&m_listLock);

	if (!m_list.size())
		return E_FAIL;

	HRESULT hr;

	if FAILED(hr = LoadFilterList(FALSE))
		return (log << "Failed to get a Filter Property List: " << hr << "\n").Write(hr);

	for (int index = 1; index < m_list.size(); index++)
	{
		if (wcsstr(m_list[index]->name, name) != NULL)
		{
			*pIndex = index;
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT FilterPropList::GetFilterProperties(LPWSTR *pfilterName, int *pCount, UINT * pFlags)
{
	if (!pCount)
		return E_INVALIDARG;

	if (!m_piGraphBuilder)
		return (log << "Graph Builder Interface not yet Initalised\n").Show(E_FAIL);

	int search = 0;

	if (*pCount > 0)
		search = *pCount;

	*pCount = 0;

	
	ULONG refCount;
	IEnumFilters * piEnumFilters = NULL;
	if SUCCEEDED(m_piGraphBuilder->EnumFilters(&piEnumFilters))
	{
		IBaseFilter * pFilter;
		while (piEnumFilters->Next(1, &pFilter, 0) == NOERROR )
		{
			*pCount = *pCount + 1;
			FILTER_INFO filterInfo;
			pFilter->QueryFilterInfo(&filterInfo);
			if (search && search == *pCount)
			{
				strCopy(*pfilterName, filterInfo.achName);
				if (pFlags)
					*pFlags = FALSE; 

				ISpecifyPropertyPages* piProp = NULL;
				if ((pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&piProp) == S_OK) && (piProp != NULL))
				{
					CAUUID caGUID;
					piProp->GetPages(&caGUID);
					piProp->Release();

					if(caGUID.cElems)
					{
						if (pFlags)
							*pFlags = caGUID.cElems; 

					}

					if (caGUID.pElems)
						CoTaskMemFree(caGUID.pElems);
				}
			}
			filterInfo.pGraph->Release(); 
			refCount = pFilter->Release();
			pFilter = NULL;
		}
		refCount = piEnumFilters->Release();
	}
	return NOERROR;
}

HRESULT FilterPropList::ShowFilterProperties(HWND hWnd, LPWSTR filterName, int index)
{
	if (!m_piGraphBuilder)
		return (log << "Graph Builder Interface not yet Initalised\n").Show(E_FAIL);

	int count = index;
	ULONG refCount;
	IEnumFilters * piEnumFilters = NULL;
	if SUCCEEDED(m_piGraphBuilder->EnumFilters(&piEnumFilters))
	{
		IBaseFilter * pFilter = NULL;
		while (piEnumFilters->Next(1, &pFilter, 0) == NOERROR )
		{
			FILTER_INFO filterInfo;
			if ((filterName && SUCCEEDED(pFilter->QueryFilterInfo(&filterInfo)) &&
				(wcsstr(filterInfo.achName, filterName) != NULL))
				|| (!filterName && count == index)) 
			{
				ISpecifyPropertyPages* piProp = NULL;
				if ((pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&piProp) == S_OK) && (piProp != NULL))
				{
					CAUUID caGUID;
					piProp->GetPages(&caGUID);
					piProp->Release();

					if(caGUID.cElems)
					{
						IUnknown *piFilterUnk = NULL;
						pFilter->QueryInterface(IID_IUnknown, (void **)&piFilterUnk);
						if (piFilterUnk)
						{
							OleCreatePropertyFrame(hWnd, 0, 0, filterInfo.achName, 1, &piFilterUnk, caGUID.cElems, caGUID.pElems, 0, 0, NULL);
							piFilterUnk->Release();
						}
						CoTaskMemFree(caGUID.pElems);
					}
				}
				filterInfo.pGraph->Release(); 
			}
			refCount = pFilter->Release();
			pFilter = NULL;
			count++;
		}
		refCount = piEnumFilters->Release();
	}
	return NOERROR;
}
