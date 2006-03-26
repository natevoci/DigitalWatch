/**
 *	DWOSDData.cpp
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

#include "DWOSDData.h"
#include "GlobalFunctions.h"
#include "Globals.h"

//////////////////////////////////////////////////////////////////////
// DWOSDData
//////////////////////////////////////////////////////////////////////

DWOSDData::DWOSDData(DWOSDWindows *windows)
{
	m_pWindows = windows;
}

DWOSDData::~DWOSDData()
{
	std::vector<DWOSDDataItem *>::iterator itItems = m_items.begin();
	for ( ; itItems < m_items.end() ; itItems++ )
	{
		delete *itItems;
	}
	m_items.clear();

	std::vector<DWOSDDataList *>::iterator itLists = m_lists.begin();
	for ( ; itLists < m_lists.end() ; itLists++ )
	{
		delete *itLists;
	}
	m_lists.clear();
}

void DWOSDData::SetItem(LPWSTR name, LPWSTR value)
{
	CAutoLock lock(&m_itemsLock);

	DWOSDDataItem *item;
	std::vector<DWOSDDataItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		item = *it;
		LPWSTR pName = item->name;
		if (_wcsicmp(name, pName) == 0)
		{
			strCopy(item->value, value);
			return;
		}
	}
	item = new DWOSDDataItem();
	strCopy(item->name, name);
	strCopy(item->value, value);
	m_items.push_back(item);
	return;
}

LPWSTR DWOSDData::GetItem(LPWSTR name)
{
	CAutoLock lock(&m_itemsLock);

	DWOSDDataItem *item;
	std::vector<DWOSDDataItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		item = *it;
		LPWSTR pName = item->name;
		if (_wcsicmp(name, pName) == 0)
		{
			return item->value;
		}
	}
	return NULL;
}

void DWOSDData::AddList(IDWOSDDataList* list)
{
	CAutoLock lock(&m_listsLock);

	if (list)
	{
		LPWSTR pListName = list->GetListName();
		DWOSDDataList* newList = new DWOSDDataList();
		strCopy(newList->name, pListName);
		newList->list = list;
		m_lists.push_back(newList);
	}
}

void DWOSDData::RotateList(IDWOSDDataList* list)
{
	CAutoLock lock(&m_listsLock);

	//look for existing list of same name
	std::vector<DWOSDDataList *>::iterator it = m_lists.begin();
	for ( ; it < m_lists.end() ; it++ )
	{
		if (list == (*it)->list)
		{
			DWOSDDataList *newList = *it;
			m_lists.erase(it);
			m_lists.push_back(newList);
			return;
		}
	}
}

void DWOSDData::ClearAllListNames(LPWSTR pName)
{
	CAutoLock lock(&m_listsLock);

	//look for existing list of same name
	std::vector<DWOSDDataList *>::iterator it = m_lists.begin();
	for ( ; it < m_lists.end() ; it++ )
	{
		LPWSTR pListName = (*it)->name;
		if (_wcsnicmp(pListName, pName, wcslen(pName)) == 0)
		{
			m_lists.erase(it);
			it = m_lists.begin();
		}
	}
	return;
}

int DWOSDData::GetListCount(LPWSTR pName)
{
	if(!pName || (m_lists.end() == 0))
		return 0;

	int count = 0;
	//look for existing list of same name
	std::vector<DWOSDDataList *>::iterator it = m_lists.begin();
	for ( ; it < m_lists.end() ; it++ )
	{
		LPWSTR pListName = (*it)->name;
		if (_wcsicmp(pListName, pName) == 0)
		{
			count++;
		}
	}
	return count;
}

IDWOSDDataList* DWOSDData::GetListFromListName(LPWSTR pName)
{
	CAutoLock lock(&m_listsLock);

	//look for existing list of same name
	std::vector<DWOSDDataList *>::iterator it = m_lists.begin();
	for ( ; it < m_lists.end() ; it++ )
	{
		LPWSTR pListName = (*it)->name;
		if (_wcsicmp(pListName, pName) == 0)
		{
			return (*it)->list;
		}
	}
	return NULL;
}

IDWOSDDataList* DWOSDData::GetListFromItemName(LPWSTR pName)
{
	CAutoLock lock(&m_listsLock);

	IDWOSDDataList* piDataList = NULL;

	LPWSTR pLastFullStop = wcsrchr(pName, '.');
	if (pLastFullStop)
	{
		pLastFullStop[0] = '\0';
		piDataList = this->GetListFromListName(pName);
		pLastFullStop[0] = '.';
	}

	return piDataList;
}

HRESULT DWOSDData::ReplaceTokens(LPWSTR pSource, LPWSTR &pResult, long ixDataList)
{
	if (pSource == NULL)
	{
		strCopy(pResult, L"");
		return S_FALSE;
	}

	int dst = 0;
	LPWSTR pSrc;

	BOOL bMakeResultBigger = FALSE;
	long resultSize = 32;	//lets just start at 32.
	LPWSTR result = new wchar_t[resultSize];
	ZeroMemory(result, resultSize*sizeof(wchar_t));

	pSrc = pSource;

	//Note: If continue is called in this while loop then it will
	//		cause the result buffer to double in size
	while (pSrc[0] != '\0')
	{
		if (result[dst] != '\0')
			result[dst] = '\0';

		if (bMakeResultBigger)
		{
			LPWSTR newResult = new wchar_t[resultSize*2];
			ZeroMemory(newResult, resultSize*2*sizeof(wchar_t));
			wcscpy(newResult, result);
			delete[] result;
			result = newResult;
			resultSize = resultSize*2;
			bMakeResultBigger = FALSE;
			continue;
		}
		bMakeResultBigger = TRUE;

		LPWSTR pBackslash = wcschr(pSrc, '\\');
		if (pBackslash)
			pBackslash[0] = '\0';
		LPWSTR pFunction = wcsstr(pSrc, L"$(");
		if (pBackslash)
			pBackslash[0] = '\\';

		long srcLength = -1;
		if (pBackslash && (!pFunction || (pBackslash < pFunction)))
		{
			srcLength = pBackslash - pSrc;
			pFunction = NULL;
		}
		else if (pFunction && (!pBackslash || (pFunction < pBackslash)))
		{
			srcLength = pFunction - pSrc;
			pBackslash = NULL;
		}
		
		if (srcLength == -1)
			srcLength = wcslen(pSrc);

		if (dst + srcLength >= resultSize-1)
			continue;

		memcpy(result+dst, pSrc, srcLength * sizeof(wchar_t));
		result[dst+srcLength] = '\0';

		pSrc += srcLength;
		dst += srcLength;

		if (pBackslash)
		{
			if (dst+2 >= resultSize)
				continue;

			switch (pSrc[1])
			{
			case 'n':
				swprintf(result, L"%s\n", result); return E_FAIL;
			case 't':
				swprintf(result, L"%s\t", result); return E_FAIL;
			case 'v':
				swprintf(result, L"%s\v", result); return E_FAIL;
			case 'b':
				swprintf(result, L"%s\b", result); return E_FAIL;
			case 'r':
				swprintf(result, L"%s\r", result); return E_FAIL;
			case 'f':
				swprintf(result, L"%s\f", result); return E_FAIL;
			case 'a':
				swprintf(result, L"%s\a", result); return E_FAIL;
			case '\\':
				swprintf(result, L"%s\\", result); return E_FAIL;
			case '?':
				swprintf(result, L"%s\?", result); return E_FAIL;
			case '\'':
				swprintf(result, L"%s\'", result); return E_FAIL;
			case '"':
				swprintf(result, L"%s\"", result); return E_FAIL;
			case '0':
				swprintf(result, L"%s\0", result); return E_FAIL;
			}
			pSrc +=2;
		}
		else if (pFunction)
		{
			long srcUsed = 0;
			HRESULT hr = ReplaceVariable(pFunction+2, &srcUsed, result+dst, resultSize-dst, ixDataList);
			if FAILED(hr)
				break;
			if (hr == S_FALSE)
				continue;
			pSrc += srcUsed + 2;
		}

		bMakeResultBigger = FALSE;
		dst = wcslen(result);
	}

	if (pSrc[0] != '\0')
	{
		strCopy(pResult, pSrc);
	}
	else
	{
		if (pResult)
			delete[] pResult;
		pResult = result;
	}
	return (dst > 0) ? S_OK : S_FALSE;
}

HRESULT DWOSDData::ReplaceVariable(LPWSTR pSrc, long *pSrcUsed, LPWSTR pResult, long resultSize, long ixDataList)
{
	if (!pSrcUsed)
		return E_POINTER;
	*pSrcUsed = 0;

	LPWSTR pCurr = NULL;
	AutoDeletingString localString;
	int var;

	long lTimeRetrieved = 0;
	SYSTEMTIME systime;

	LPWSTR pCloseBracket = wcschr(pSrc, ')');
	if (pCloseBracket)
		pCloseBracket[0] = '\0';
	LPWSTR pFunction = wcsstr(pSrc, L"$(");
	if (pCloseBracket)
		pCloseBracket[0] = ')';

	if (pCloseBracket && (!pFunction || (pCloseBracket < pFunction)))
	{
		pFunction = NULL;
	}
	else if (pFunction && (!pCloseBracket || (pFunction < pCloseBracket)))
	{
		pCloseBracket = NULL;
	}

	if (pFunction)
	{
		long srcUsed;
		long dstSize = 16;
		LPWSTR pTmpString = NULL;
		HRESULT hr = S_FALSE;
		while (hr == S_FALSE)
		{
			dstSize = dstSize * 2;

			if (pTmpString)
				delete[] pTmpString;
			pTmpString = new wchar_t[dstSize];
			ZeroMemory(pTmpString, dstSize*sizeof(wchar_t));

			hr = ReplaceVariable(pFunction+2, &srcUsed, pTmpString, dstSize, ixDataList);
			srcUsed += 2;

			dstSize *= 2;
		}

		long length1 = pFunction - pSrc;
		long length2 = wcslen(pTmpString);
		long length3 = wcslen(pFunction) - srcUsed;
		long newLength = length1 + length2 + length3;

		localString.pStr = new wchar_t[newLength+1];
		memcpy(localString.pStr, pSrc, length1*sizeof(wchar_t));
		memcpy(localString.pStr + length1, pTmpString, length2*sizeof(wchar_t));
		memcpy(localString.pStr + length1 + length2, pFunction + srcUsed, length3*sizeof(wchar_t));
		localString.pStr[newLength] = '\0';

		delete[] pTmpString;

		*pSrcUsed = srcUsed - length2;

		hr = ReplaceVariable(localString.pStr, &srcUsed, pResult, resultSize, ixDataList);
		if (hr != S_OK)
			return hr;

		*pSrcUsed += srcUsed;
		return S_OK;
	}
	else if (pCloseBracket)
	{
		strCopy(localString.pStr, pSrc, pCloseBracket-pSrc);
		pCurr = localString.pStr;
		*pSrcUsed = pCloseBracket - pSrc + 1;
	}
	else
	{
		return (log << "Missing closing bracket\n").Write(S_OK);
	}


	if (_wcsicmp(pCurr, L"LongYear") == 0)
	{
		if (4 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wYear;
		pResult[0] = int(var/1000) + 48;
		var -= int(var/1000)*1000;

		pResult[1] = int(var/100) + 48;
		var -= int(var/100)*100;

		pResult[2] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[3] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"ShortYear") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wYear;
		var -= int(var/100)*100;

		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"Month") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wMonth;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"Day") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wDay;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"Hour12") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wHour;
		if (var > 12)
			var -= 12;
		if (var == 0)
			var = 12;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"Hour24") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wHour;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"Minute") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wMinute;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;

	}
	else if (_wcsicmp(pCurr, L"Second") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wSecond;
		pResult[0] = int(var/10) + 48;
		var -= int(var/10)*10;

		pResult[1] = int(var) + 48;
	}
	else if (_wcsicmp(pCurr, L"AMPM") == 0)
	{
		if (2 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wHour;
		if (var < 12)
			swprintf(pResult, L"%sAM\0", pResult);
		else
			swprintf(pResult, L"%sPM\0", pResult);
	}
	else if (_wcsicmp(pCurr, L"ap") == 0)
	{
		if (1 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		var = systime.wHour;
		if (var < 12)
			swprintf(pResult, L"%sa\0", pResult);
		else
			swprintf(pResult, L"%sp\0", pResult);
	}
	else if (_wcsicmp(pCurr, L"LongDayOfWeek") == 0)
	{
		if (9 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		switch (systime.wDayOfWeek)
		{
		case 0:
			swprintf(pResult, L"%sSunday\0", pResult); return E_FAIL;
		case 1:
			swprintf(pResult, L"%sMonday\0", pResult); return E_FAIL;
		case 2:
			swprintf(pResult, L"%sTuesday\0", pResult); return E_FAIL;
		case 3:
			swprintf(pResult, L"%sWednesday\0", pResult); return E_FAIL;
		case 4:
			swprintf(pResult, L"%sThursday\0", pResult); return E_FAIL;
		case 5:
			swprintf(pResult, L"%sFriday\0", pResult); return E_FAIL;
		case 6:
			swprintf(pResult, L"%sSaturday\0", pResult);
			return E_FAIL;
		}
	}
	else if (_wcsicmp(pCurr, L"ShortDayOfWeek") == 0)
	{
		if (3 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		switch (systime.wDayOfWeek)
		{
		case 0:
			swprintf(pResult, L"%sSun\0", pResult); return E_FAIL;
		case 1:
			swprintf(pResult, L"%sMon\0", pResult); return E_FAIL;
		case 2:
			swprintf(pResult, L"%sTue\0", pResult); return E_FAIL;
		case 3:
			swprintf(pResult, L"%sWed\0", pResult); return E_FAIL;
		case 4:
			swprintf(pResult, L"%sThu\0", pResult); return E_FAIL;
		case 5:
			swprintf(pResult, L"%sFri\0", pResult); return E_FAIL;
		case 6:
			swprintf(pResult, L"%sSat\0", pResult); return E_FAIL;
		}
	}
	else if (_wcsicmp(pCurr, L"LongMonth") == 0)
	{
		if (9 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		switch (systime.wMonth)
		{
		case 1:
			swprintf(pResult, L"%sJanuary\0", pResult); return E_FAIL;
		case 2:
			swprintf(pResult, L"%sFebruary\0", pResult); return E_FAIL;
		case 3:
			swprintf(pResult, L"%sMarch\0", pResult); return E_FAIL;
		case 4:
			swprintf(pResult, L"%sApril\0", pResult); return E_FAIL;
		case 5:
			swprintf(pResult, L"%sMay\0", pResult); return E_FAIL;
		case 6:
			swprintf(pResult, L"%sJune\0", pResult); return E_FAIL;
		case 7:
			swprintf(pResult, L"%sJuly\0", pResult); return E_FAIL;
		case 8:
			swprintf(pResult, L"%sAugust\0", pResult); return E_FAIL;
		case 9:
			swprintf(pResult, L"%sSeptember\0", pResult); return E_FAIL;
		case 10:
			swprintf(pResult, L"%sOctober\0", pResult); return E_FAIL;
		case 11:
			swprintf(pResult, L"%sNovember\0", pResult); return E_FAIL;
		case 12:
			swprintf(pResult, L"%sDecember\0", pResult); return E_FAIL;
		}
	}
	else if (_wcsicmp(pCurr, L"ShortMonth") == 0)
	{
		if (3 >= resultSize)
			return S_FALSE;

		if (!lTimeRetrieved++)
			GetLocalTime(&systime);

		switch (systime.wMonth)
		{
		case 1:
			swprintf(pResult, L"%sJan\0", pResult); return E_FAIL;
		case 2:
			swprintf(pResult, L"%sFeb\0", pResult); return E_FAIL;
		case 3:
			swprintf(pResult, L"%sMar\0", pResult); return E_FAIL;
		case 4:
			swprintf(pResult, L"%sApr\0", pResult); return E_FAIL;
		case 5:
			swprintf(pResult, L"%sMay\0", pResult); return E_FAIL;
		case 6:
			swprintf(pResult, L"%sJun\0", pResult); return E_FAIL;
		case 7:
			swprintf(pResult, L"%sJul\0", pResult); return E_FAIL;
		case 8:
			swprintf(pResult, L"%sAug\0", pResult); return E_FAIL;
		case 9:
			swprintf(pResult, L"%sSep\0", pResult); return E_FAIL;
		case 10:
			swprintf(pResult, L"%sOct\0", pResult); return E_FAIL;
		case 11:
			swprintf(pResult, L"%sNov\0", pResult); return E_FAIL;
		case 12:
			swprintf(pResult, L"%sDec\0", pResult); return E_FAIL;
		}
	}

	else if (_wcsicmp(pCurr, L"Zoom") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.zoom) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"ZoomMode") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.zoomMode) < 0)
			return S_FALSE;
	}

	else if (_wcsicmp(pCurr, L"AlwaysOnTop") == 0)
	{
		if (g_pData->values.window.bAlwaysOnTop)
		{
			if (_snwprintf(pResult, resultSize, L"%sAlways On Top", pResult) < 0)
				return S_FALSE;
		}
	}
	else if (_wcsicmp(pCurr, L"Fullscreen") == 0)
	{
		if (g_pData->values.window.bFullScreen)
		{
			if (_snwprintf(pResult, resultSize, L"%sFullscreen", pResult) < 0)
				return S_FALSE;
		}
	}

	else if (_wcsicmp(pCurr, L"Volume") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.audio.volume) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"Mute") == 0)
	{
		if (g_pData->values.audio.bMute)
		{
			if (_snwprintf(pResult, resultSize, L"%sMute", pResult) < 0)
				return S_FALSE;
		}
	}

	else if (_wcsicmp(pCurr, L"KeyCode") == 0)
	{
		//if (_snwprintf(pResult, resultSize, "%s%i", pResult, g_pData->KeyPress.nKeycode) < 0)
		//	return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"KeyShift") == 0)
	{
		//if (g_pData->KeyPress.bShift)
		//	if (_snwprintf(pResult, resultSize, "%sShift", pResult) < 0)
		//		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"KeyCtrl") == 0)
	{
		//if (g_pData->KeyPress.bCtrl)
		//	if (_snwprintf(pResult, resultSize, "%sCtrl", pResult) < 0)
		//		return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"KeyAlt") == 0)
	{
		//if (g_pData->KeyPress.bAlt)
		//	if (_snwprintf(pResult, resultSize, "%sAlt", pResult) < 0)
				return S_FALSE;
	}

	else if (_wcsicmp(pCurr, L"Brightness") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.overlay.brightness) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"Contrast") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.overlay.contrast) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"Hue") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.overlay.hue) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"Saturation") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.overlay.saturation) < 0)
			return S_FALSE;
	}
	else if (_wcsicmp(pCurr, L"Gamma") == 0)
	{
		if (_snwprintf(pResult, resultSize, L"%s%i", pResult, g_pData->values.video.overlay.gamma) < 0)
			return S_FALSE;
	}

	else if (_wcsnicmp(pCurr, L"window.", 7) == 0)
	{
		pCurr += 7;
		LPWSTR pParameter = wcschr(pCurr, '[');
		if (pParameter)
		{
			LPWSTR pEnd = wcschr(pParameter, ']');
			if (pEnd)
			{
				pParameter[0] = '\0';
				pParameter++;
				pEnd[0] = '\0';
				long id = _wtoi(pParameter);

				DWOSDWindow *window = m_pWindows->GetWindow(pCurr);
				if (window)
				{
					LPWSTR data = window->GetParameter(id);
					if (data)
					{
						if (_snwprintf(pResult, resultSize, L"%s%s", pResult, data) < 0)
							return S_FALSE;
					}
				}
			}
		}
		
	}

	else if (_wcsicmp(pCurr, L"ErrorMessage") == 0)
	{
		//if (_snwprintf(pResult, resultSize, L"%s%s", pResult, g_pData->ErrorMessage.GetMessage()) < 0)
		//	return S_FALSE;
	}

	else if (_wcsicmp(pCurr, L"(") == 0)
	{
		if (1 >= resultSize)
			return S_FALSE;

		pResult[0] = '(';
	}

	else
	{
		IDWOSDDataList* piDataList = this->GetListFromItemName(pCurr);
		if (piDataList)
		{
			LPWSTR data = piDataList->GetListItem(pCurr, ixDataList);
			if (data)
			{
				if (_snwprintf(pResult, resultSize-1, L"%s%s", pResult, data) < 0)
					return S_FALSE;
			}
			else
			{
#ifdef DEBUG
				// Output the variable so we can see that it doesn't exist
				if (_snwprintf(pResult, resultSize, L"%s$(%s)", pResult, pCurr) < 0)
					return S_FALSE;
#endif
			}
		}
		else
		{
			LPWSTR data = GetItem(pCurr);
			if (data)
			{
				if (_snwprintf(pResult, resultSize-1, L"%s%s", pResult, data) < 0)
					return S_FALSE;
			}
		}
	}

	return S_OK;
}

