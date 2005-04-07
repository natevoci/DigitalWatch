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
#include "ParseLine.h"

//////////////////////////////////////////////////////////////////////
// DWOSDData
//////////////////////////////////////////////////////////////////////

DWOSDData::DWOSDData()
{

}

DWOSDData::~DWOSDData()
{

}

void DWOSDData::SetItem(LPWSTR name, LPWSTR value)
{
	DWOSDDataItem *item;
	std::vector<DWOSDDataItem *>::iterator it = m_Items.begin();
	for ( ; it < m_Items.end() ; it++ )
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
	m_Items.push_back(item);
	return;
}

LPWSTR DWOSDData::GetItem(LPWSTR name)
{
	DWOSDDataItem *item;
	std::vector<DWOSDDataItem *>::iterator it = m_Items.begin();
	for ( ; it < m_Items.end() ; it++ )
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

DWOSDDataList* DWOSDData::GetList(LPWSTR pListName)
{
	//look for existing list of same name
	std::vector<DWOSDDataList *>::iterator it = m_Lists.begin();
	for ( ; it < m_Lists.end() ; it++ )
	{
		LPWSTR pName = (*it)->name;
		if (_wcsicmp(pListName, pName) == 0)
		{
			return (*it);
		}
	}
	DWOSDDataList* newList = new DWOSDDataList();
	strCopy(newList->name, pListName);
	m_Lists.push_back(newList);
	return newList;
}

HRESULT DWOSDData::ReplaceTokens(LPWSTR pSource, LPWSTR &pResult)
{
	int dst = 0;
	LPWSTR pBuff = NULL;
	LPWSTR pCurr;
	LPWSTR pSrc;
	int var;

	BOOL bMakeResultBigger = FALSE;
	long resultSize = 32;	//lets just start at 32.
	LPWSTR result = new wchar_t[resultSize];
	ZeroMemory(result, resultSize*sizeof(wchar_t));

	long lTimeRetrieved = 0;
	SYSTEMTIME systime;
	
	strCopy(pBuff, pSource);
	pSrc = pBuff;

	//Note: If continue is called in this while loop then it will
	//		cause the result buffer to double in size
	while (pSrc[0] != '\0')
	{
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

		if (pSrc[0] == '\\')
		{
			if (dst+2 >= resultSize)
				continue;

			switch (pSrc[1])
			{
			case 'n':
				swprintf(result, L"%s\n", result); break;
			case 't':
				swprintf(result, L"%s\t", result); break;
			case 'v':
				swprintf(result, L"%s\v", result); break;
			case 'b':
				swprintf(result, L"%s\b", result); break;
			case 'r':
				swprintf(result, L"%s\r", result); break;
			case 'f':
				swprintf(result, L"%s\f", result); break;
			case 'a':
				swprintf(result, L"%s\a", result); break;
			case '\\':
				swprintf(result, L"%s\\", result); break;
			case '?':
				swprintf(result, L"%s\?", result); break;
			case '\'':
				swprintf(result, L"%s\'", result); break;
			case '"':
				swprintf(result, L"%s\"", result); break;
			case '0':
				swprintf(result, L"%s\0", result); break;
			}
			pSrc +=2;
		}
		else if (_wcsnicmp(pSrc, L"$(", 2) == 0)
		{
			//pSrc[0] = '';
			ParseLine parseLine;
			parseLine.IgnoreRHS();
			if (parseLine.Parse(pSrc) == FALSE)
			{
				(log << "Parse error in string: " << pSrc << "\n").Write();
				break;
			}

			if (parseLine.LHS.ParameterCount < 1)
			{
				(log << "Parameter missing in string: " << pSrc << "\n").Write();
				break;
			}

			pCurr = parseLine.LHS.Parameter[0];

			if (_wcsicmp(pCurr, L"LongYear") == 0)
			{
				if (dst+4 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wYear;
				result[dst++] = int(var/1000) + 48;
				var -= int(var/1000)*1000;

				result[dst++] = int(var/100) + 48;
				var -= int(var/100)*100;

				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"ShortYear") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wYear;
				var -= int(var/100)*100;

				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"Month") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wMonth;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"Day") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wDay;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"Hour12") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wHour;
				if (var > 12)
					var -= 12;
				if (var == 0)
					var = 12;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"Hour24") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wHour;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"Minute") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wMinute;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;

			}
			else if (_wcsicmp(pCurr, L"Second") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wSecond;
				result[dst++] = int(var/10) + 48;
				var -= int(var/10)*10;

				result[dst++] = int(var) + 48;
			}
			else if (_wcsicmp(pCurr, L"AMPM") == 0)
			{
				if (dst+2 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wHour;
				if (var < 12)
					swprintf(result, L"%sAM\0", result);
				else
					swprintf(result, L"%sPM\0", result);
			}
			else if (_wcsicmp(pCurr, L"ap") == 0)
			{
				if (dst+1 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				var = systime.wHour;
				if (var < 12)
					swprintf(result, L"%sa\0", result);
				else
					swprintf(result, L"%sp\0", result);
			}
			else if (_wcsicmp(pCurr, L"LongDayOfWeek") == 0)
			{
				if (dst+9 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				switch (systime.wDayOfWeek)
				{
				case 0:
					swprintf(result, L"%sSunday\0", result); break;
				case 1:
					swprintf(result, L"%sMonday\0", result); break;
				case 2:
					swprintf(result, L"%sTuesday\0", result); break;
				case 3:
					swprintf(result, L"%sWednesday\0", result); break;
				case 4:
					swprintf(result, L"%sThursday\0", result); break;
				case 5:
					swprintf(result, L"%sFriday\0", result); break;
				case 6:
					swprintf(result, L"%sSaturday\0", result);
					break;
				}
			}
			else if (_wcsicmp(pCurr, L"ShortDayOfWeek") == 0)
			{
				if (dst+3 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				switch (systime.wDayOfWeek)
				{
				case 0:
					swprintf(result, L"%sSun\0", result); break;
				case 1:
					swprintf(result, L"%sMon\0", result); break;
				case 2:
					swprintf(result, L"%sTue\0", result); break;
				case 3:
					swprintf(result, L"%sWed\0", result); break;
				case 4:
					swprintf(result, L"%sThu\0", result); break;
				case 5:
					swprintf(result, L"%sFri\0", result); break;
				case 6:
					swprintf(result, L"%sSat\0", result); break;
				}
			}
			else if (_wcsicmp(pCurr, L"LongMonth") == 0)
			{
				if (dst+9 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				switch (systime.wMonth)
				{
				case 1:
					swprintf(result, L"%sJanuary\0", result); break;
				case 2:
					swprintf(result, L"%sFebruary\0", result); break;
				case 3:
					swprintf(result, L"%sMarch\0", result); break;
				case 4:
					swprintf(result, L"%sApril\0", result); break;
				case 5:
					swprintf(result, L"%sMay\0", result); break;
				case 6:
					swprintf(result, L"%sJune\0", result); break;
				case 7:
					swprintf(result, L"%sJuly\0", result); break;
				case 8:
					swprintf(result, L"%sAugust\0", result); break;
				case 9:
					swprintf(result, L"%sSeptember\0", result); break;
				case 10:
					swprintf(result, L"%sOctober\0", result); break;
				case 11:
					swprintf(result, L"%sNovember\0", result); break;
				case 12:
					swprintf(result, L"%sDecember\0", result); break;
				}
			}
			else if (_wcsicmp(pCurr, L"ShortMonth") == 0)
			{
				if (dst+3 >= resultSize)
					continue;

				if (!lTimeRetrieved++)
					GetLocalTime(&systime);

				switch (systime.wMonth)
				{
				case 1:
					swprintf(result, L"%sJan\0", result); break;
				case 2:
					swprintf(result, L"%sFeb\0", result); break;
				case 3:
					swprintf(result, L"%sMar\0", result); break;
				case 4:
					swprintf(result, L"%sApr\0", result); break;
				case 5:
					swprintf(result, L"%sMay\0", result); break;
				case 6:
					swprintf(result, L"%sJun\0", result); break;
				case 7:
					swprintf(result, L"%sJul\0", result); break;
				case 8:
					swprintf(result, L"%sAug\0", result); break;
				case 9:
					swprintf(result, L"%sSep\0", result); break;
				case 10:
					swprintf(result, L"%sOct\0", result); break;
				case 11:
					swprintf(result, L"%sNov\0", result); break;
				case 12:
					swprintf(result, L"%sDec\0", result); break;
				}
			}
			else if (_wcsicmp(pCurr, L"NetworkName") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%s", result, g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Name) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"ProgramName") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%s", result, g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].Name) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"NetworkNumber") == 0)
			{
				/*var = g_pData->values.currTVNetwork;
				if (var >= 10)
				{
					result[dst++] = int(var/10) + 48;
					var -= int(var/10)*10;

					result[dst++] = int(var) + 48;
				}
				else
					result[dst] = int(var) + 48;
					*/
			}
			else if (_wcsicmp(pCurr, L"ProgramNumber") == 0)
			{
				/*var = g_pData->values.currTVProgram;
				if (var >= 10)
				{
					result[dst++] = int(var/10) + 48;
					var -= int(var/10)*10;

					result[dst++] = int(var) + 48;
				}
				else
					result[dst] = int(var) + 48;*/
			}
			else if (_wcsicmp(pCurr, L"Frequency") == 0)
			{
				//int freq = g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Frequency;
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Frequency) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"VideoPid") == 0)
			{
				//int vpid = g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].VideoPid;
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].VideoPid) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"AudioPid") == 0)
			{
				//int apid = g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].AudioPid;
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].AudioPid) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"AC3") == 0)
			{
				/*if (g_pData->tvChannels.Networks[g_pData->values.currTVNetwork].Programs[g_pData->values.currTVProgram].AudioPidAC3)
				{
					if (_snwprintf(result, resultSize-dst, "%sAC3", result) < 0)
						continue;
				}*/
			}
			else if (_wcsicmp(pCurr, L"Recording") == 0)
			{
				/*if (g_pData->bRecording)
				{
					if (_snwprintf(result, resultSize-dst, "%sREC", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"RecordingStopped") == 0)
			{
				/*if (!g_pData->bRecording)
				{
					if (_snwprintf(result, resultSize-dst, "%sStopped Recording", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"RecordingPaused") == 0)
			{
				/*if (g_pData->bRecordingPaused)
				{
					if (_snwprintf(result, resultSize-dst, "%sREC - Paused", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"RecordingUnpaused") == 0)
			{
				/*if (!g_pData->bRecordingPaused)
				{
					if (_snwprintf(result, resultSize-dst, "%sREC", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"RecordingTimeLeft") == 0)
			{
				/*
				if (dst+8 >= resultSize)
					continue;
				if (g_pData->recordingTimeLeft > 0)
				{
					int hours = g_pData->recordingTimeLeft/3600;
					int minutes = g_pData->recordingTimeLeft/60 - (hours*60);
					int seconds = g_pData->recordingTimeLeft - (hours*3600) - (minutes*60);
				
					if (hours < 10)		swprintf(result, L"%s 0%i:", result, hours);
					else				swprintf(result, L"%s %i:" , result, hours);

					if (minutes < 10)	swprintf(result, L"%s0%i:", result, minutes);
					else				swprintf(result, L"%s%i:" , result, minutes);

					if (seconds < 10)	swprintf(result, L"%s0%i", result, seconds);
					else				swprintf(result, L"%s%i" , result, seconds);
				}*/
			}
			else if (_wcsicmp(pCurr, L"VideoDecoder") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%s", result, g_pData->VideoDecoders.Current()->strName) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"VideoDecoderId") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->VideoDecoders.GetCurrent()) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"AudioDecoder") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%s", result, g_pData->AudioDecoders.Current()->strName) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"AudioDecoderId") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->AudioDecoders.GetCurrent()) < 0)
				//	continue;
			}

			else if (_wcsicmp(pCurr, L"Zoom") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.zoom) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"ZoomMode") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.zoomMode) < 0)
					continue;
			}

			else if (_wcsicmp(pCurr, L"AlwaysOnTop") == 0)
			{
				if (g_pData->values.window.bAlwaysOnTop)
				{
					if (_snwprintf(result, resultSize-dst, L"%sAlways On Top", result) < 0)
						continue;
				}
			}
			else if (_wcsicmp(pCurr, L"Fullscreen") == 0)
			{
				if (g_pData->values.window.bFullScreen)
				{
					if (_snwprintf(result, resultSize-dst, L"%sFullscreen", result) < 0)
						continue;
				}
			}

			else if (_wcsicmp(pCurr, L"Volume") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.audio.currVolume) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"Mute") == 0)
			{
				if (g_pData->values.audio.bMute)
				{
					if (_snwprintf(result, resultSize-dst, L"%sMute", result) < 0)
						continue;
				}
			}

			else if (_wcsicmp(pCurr, L"KeyCode") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->KeyPress.nKeycode) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"KeyShift") == 0)
			{
				//if (g_pData->KeyPress.bShift)
				//	if (_snwprintf(result, resultSize-dst, "%sShift", result) < 0)
				//		continue;
			}
			else if (_wcsicmp(pCurr, L"KeyCtrl") == 0)
			{
				//if (g_pData->KeyPress.bCtrl)
				//	if (_snwprintf(result, resultSize-dst, "%sCtrl", result) < 0)
				//		continue;
			}
			else if (_wcsicmp(pCurr, L"KeyAlt") == 0)
			{
				//if (g_pData->KeyPress.bAlt)
				//	if (_snwprintf(result, resultSize-dst, "%sAlt", result) < 0)
						continue;
			}

			else if (_wcsicmp(pCurr, L"SignalQuality") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%, resultSize-dsts%i", result, signalQuality) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"SignalStrength") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, "%s%i", result, signalStrength) < 0)
				//	continue;
			}
			else if (_wcsicmp(pCurr, L"SignalLock") == 0)
			{
				/*if (signalLock)
				{
					if (_snwprintf(result, resultSize-dst, "%sTrue", result) < 0)
						continue;
				}
				else
				{
					if (_snwprintf(result, resultSize-dst, "%sFalse", result) < 0)
						continue;
				}
				*/
			}

			else if (_wcsicmp(pCurr, L"TimeShift") == 0)
			{
				/*if (m_pFilterGraph->GetDVBInput()->GetTimeShiftMode())
				{
					if (_snwprintf(result, resultSize-dst, "%sPlay", result) < 0)
						continue;
				}
				else
				{
					if (_snwprintf(result, resultSize-dst, "%sPause", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"TimeShiftJumpDirection") == 0)
			{
				/*if (g_pData->TimeShiftJump > 0)
				{
					if (_snwprintf(result, resultSize-dst, "%sForwards", result) < 0)
						continue;
				}
				else
				{
					if (_snwprintf(result, resultSize-dst, "%sBackwards", result) < 0)
						continue;
				}
				*/
			}
			else if (_wcsicmp(pCurr, L"TimeShiftJumpSeconds") == 0)
			{
				/*if (g_pData->TimeShiftJump > 0)
				{
					if (_snwprintf(result, resultSize-dst, "%s%i", result, g_pData->TimeShiftJump) < 0)
						continue;
				}
				else
				{
					if (_snwprintf(result, resultSize-dst, "%s%i", result, -g_pData->TimeShiftJump) < 0)
						continue;
				}
				*/
			}

			else if (_wcsicmp(pCurr, L"Brightness") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.overlay.brightness) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"Contrast") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.overlay.contrast) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"Hue") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.overlay.hue) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"Saturation") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.overlay.saturation) < 0)
					continue;
			}
			else if (_wcsicmp(pCurr, L"Gamma") == 0)
			{
				if (_snwprintf(result, resultSize-dst, L"%s%i", result, g_pData->values.display.overlay.gamma) < 0)
					continue;
			}

			else if (_wcsicmp(pCurr, L"NowAndNext") == 0)
			{
				/*if (nanColl.itemCount <= 0)
					swprintf(result, "%sNo now and next information available.", result);
				for (int i=0 ; i<nanColl.itemCount ; i++ )
				{
					int len = strlen(result);
					len += strlen(nanColl.items[i].starttime);
					len += strlen(nanColl.items[i].eventName);
					len += strlen(nanColl.items[i].description);
					len += 10;
					if (len < resultLength)
					{
						if (i == 0)
						{
							if (_snwprintf(result, resultSize-dst, L"%s%s : %s\n%s", result, nanColl.items[i].starttime, nanColl.items[i].eventName, nanColl.items[i].description) < 0)
								continue;
						}
						else
						{
							if (_snwprintf(result, resultSize-dst, L"%s\n\n%s : %s\n%s", result, nanColl.items[i].starttime, nanColl.items[i].eventName, nanColl.items[i].description) < 0)
								continue;
						}
					}
				}*/
			}
			else if (_wcsnicmp(pCurr, L"NaNTime[", 8) == 0)
			{
				/*pCurr += 8;
				char* pEnd = strchr(pCurr, ']');
				if (pEnd)
				{
					pEnd[0] = '\0';
					int id = atoi(pCurr);
					if (id < nanColl.itemCount)
						if (_snwprintf(result, resultSize-dst, L"%s%s", result, nanColl.items[id].starttime) < 0)
							continue;
					pEnd[0] = ']';
				}*/
			}
			else if (_wcsnicmp(pCurr, L"NaNLength[", 10) == 0)
			{
				/*pCurr += 10;
				char* pEnd = strchr(pCurr, ']');
				if (pEnd)
				{
					pEnd[0] = '\0';
					int id = atoi(pCurr);
					if (id < nanColl.itemCount)
						if (_snwprintf(result, resultSize-dst, L"%s%s", result, nanColl.items[id].duration) < 0)
							continue;
					pEnd[0] = ']';
				}*/
			}
			else if (_wcsnicmp(pCurr, L"NaNProgram[", 11) == 0)
			{
				/*pCurr += 11;
				char* pEnd = strchr(pCurr, ']');
				if (pEnd)
				{
					pEnd[0] = '\0';
					int id = atoi(pCurr);
					if (id < nanColl.itemCount)
						if (_snwprintf(result, resultSize-dst, L"%s%s", result, nanColl.items[id].eventName) < 0)
							continue;
					pEnd[0] = ']';
				}*/
			}
			else if (_wcsnicmp(pCurr, L"NaNDescription[", 15) == 0)
			{
				/*pCurr += 15;
				char* pEnd = strchr(pCurr, ']');
				if (pEnd)
				{
					pEnd[0] = '\0';
					int id = atoi(pCurr);
					if (id < nanColl.itemCount)
						if (_snwprintf(result, resultSize-dst, L"%s%s", result, nanColl.items[id].description) < 0)
							continue;
					pEnd[0] = ']';
				}*/
			}

			else if (_wcsicmp(pCurr, L"ErrorMessage") == 0)
			{
				//if (_snwprintf(result, resultSize-dst, L"%s%s", result, g_pData->ErrorMessage.GetMessage()) < 0)
				//	continue;
			}

			else if (_wcsicmp(pCurr, L"(") == 0)
			{
				result[dst] = '(';
			}
			else
			{
				LPWSTR data = GetItem(pCurr);
				if (data)
				{
					if (_snwprintf(result, resultSize-dst, L"%s%s", result, data) < 0)
						continue;
				}
				else
				{
					//if (_snwprintf(result, resultSize-dst, L"%s$(%s)", result, pCurr) < 0)
					//	continue;
				}
			}
			pSrc += wcslen(parseLine.LHS.Function);
		}
		else
		{
			if (dst+1 >= resultSize)
				continue;

			result[dst] = pSrc[0];
			result[dst+1] = '\0';
			pSrc++;
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
		strCopy(pResult, result);
	}
	delete[] pBuff;
	delete[] result;
	return (dst > 0) ? S_OK : S_FALSE;
}

