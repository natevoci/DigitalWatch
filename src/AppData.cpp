/**
 *	AppData.cpp
 *	Copyright (C) 2003-2004 Nate
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

#include "StdAfx.h"
#include "AppData.h"
#include "GlobalFunctions.h"
#include "Globals.h"

#include "XMLDocument.h"

#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif

#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif
static 	LPWSTR MUX_FORMAT[6] = {L"None", L"FullMux", L"TSMux", L"MpgMux", L"SepMux", L"DVR-MS"};
static 	LPWSTR PRIORITY[6] = {L"Realtime", L"High", L"AboveNormal", L"Normal.", L"BelowNormal", L"Low"};
static 	LPWSTR BOOLVALUE[2] = {L"False", L"True"};
static 	LPWSTR STATUSVALUE[2] = {L"Disabled", L"Enabled"};

AppData::AppData()
{
	hWnd = 0;

	this->SetLogCallback(&g_DWLogWriter);

	//TEMPS
	for (int i = 0; i < 9; i++)
	{
		temps.bools[i] = 0;
		temps.ints[i] = 0;
		temps.longs[i] = 0;
		temps.lpstr[i] = NULL;
	}

	//APPLICATION
	application.appPath = new wchar_t[MAX_PATH];
	GetCommandPath(application.appPath);
	application.bCursorVisible = TRUE;

	//SETTINGS
	settings.application.multiple = FALSE;
	settings.application.disableScreenSaver = TRUE;
	settings.application.priority = ABOVE_NORMAL_PRIORITY_CLASS;
	settings.application.addToROT = TRUE;
	settings.application.multicard = FALSE;
	settings.application.rememberLastService = TRUE;
	settings.application.lastServiceCmd = new wchar_t[MAX_PATH];
	wcscpy(settings.application.lastServiceCmd, L"");
	settings.application.currentServiceCmd = new wchar_t[MAX_PATH];
	wcscpy(settings.application.currentServiceCmd, L"");
	settings.application.longNetworkName = FALSE;
//	settings.application.logFilename = new wchar_t[MAX_PATH];
//	swprintf(settings.application.logFilename, L"%s%s", application.appPath, L"DigitalWatch.log");
	
	settings.window.startFullscreen = FALSE;
	settings.window.startAlwaysOnTop = FALSE;
	settings.window.startAtLastWindowPosition = TRUE;
	settings.window.startWithLastWindowSize = TRUE;

	settings.window.position.x = 0;
	settings.window.position.y = 0;
	settings.window.size.width = 1024;
	settings.window.size.height = 576;

	settings.window.rememberFullscreenState = TRUE;
	settings.window.rememberAlwaysOnTopState = TRUE;
	settings.window.rememberWindowPosition = TRUE;
	settings.window.quietOnMinimise = FALSE;
	settings.window.closeBuffersOnMinimise = FALSE;

	settings.audio.volume = 100;
	settings.audio.bMute = FALSE;

	settings.video.aspectRatio.bOverride = FALSE;
	settings.video.aspectRatio.width = 16;
	settings.video.aspectRatio.height = 9;

	settings.video.zoom = 100;
	settings.video.zoomMode = 0;

	settings.video.overlay.brightness = 750;
	settings.video.overlay.contrast = 10000;
	settings.video.overlay.hue = 0;
	settings.video.overlay.saturation = 10000;
	settings.video.overlay.gamma = 1;

	settings.capture.fileName = new wchar_t[MAX_PATH];
	wcscpy(settings.capture.fileName, L"");
	settings.capture.folder = new wchar_t[MAX_PATH];
	wcscpy(settings.capture.folder, L"");
	settings.capture.format = 2;

	settings.timeshift.folder = new wchar_t[MAX_PATH];
	wcscpy(settings.timeshift.folder, L"");
	settings.timeshift.dlimit = 0;
	settings.timeshift.flimit = 0;
	settings.timeshift.fdelay = 0;
	settings.timeshift.buffer = new wchar_t[MAX_PATH];
	wcscpy(settings.timeshift.buffer, L"Medium");
	settings.timeshift.change = new wchar_t[MAX_PATH];
	wcscpy(settings.timeshift.change, L"Fast");
	settings.timeshift.bufferMinutes = 0;
	settings.timeshift.format = 1;
	settings.timeshift.maxnumbfiles = 40;
	settings.timeshift.numbfilesrecycled = 6;
	settings.timeshift.bufferfilesize = 250;

	settings.dsnetwork.format = 0;
	settings.dsnetwork.ipaddr = new wchar_t[MAX_PATH];
	wcscpy(settings.dsnetwork.ipaddr, L"224.0.0.1");
	settings.dsnetwork.port = 0;
	settings.dsnetwork.nicaddr = new wchar_t[MAX_PATH];
	wcscpy(settings.dsnetwork.nicaddr, L"127.0.0.1");

	CComBSTR bstrCLSID(L"{4F8BF30C-3BEB-43a3-8BF2-10096FD28CF2}");
	CLSIDFromString(bstrCLSID, &settings.filterguids.filesourceclsid);
	bstrCLSID = GUID_NULL; 
	CLSIDFromString(bstrCLSID, &settings.filterguids.filewriterclsid);
	bstrCLSID = L"{5cdd5c68-80dc-43e1-9e44-c849ca8026e7}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.timeshiftclsid);
	bstrCLSID = L"{4DF35815-79C5-44C8-8753-847D5C9C3CF5}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.mpgmuxclsid);
	bstrCLSID = L"{A07E6137-6C07-45D9-A00C-7DE7A7E6319B}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.dsnetclsid);
	bstrCLSID = L"{afb6c280-2c41-11d3-8a60-0000f81e0e4a}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.demuxclsid);
	bstrCLSID = L"{F8388A40-D5BB-11d0-BE5A-0080C706568E}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.infteeclsid);
	bstrCLSID = GUID_NULL; 
	CLSIDFromString(bstrCLSID, &settings.filterguids.quantizerclsid);

	HRESULT hr = LoadSettings();
	settings.loadedFromFile = SUCCEEDED(hr);

	//VALUES
	values.application.multiple = FALSE; // This gets set if you have one or more instance running.
	values.application.multicard = settings.application.multicard;
	values.window.bFullScreen = settings.window.startFullscreen;
	values.window.bAlwaysOnTop = settings.window.startAlwaysOnTop;

	if (settings.window.startAtLastWindowPosition)
	{
		values.window.position.x = settings.window.position.x;
		values.window.position.y = settings.window.position.y;
	}
	else
	{
		values.window.position.x = 0;
		values.window.position.y = 0;
	}
	if (settings.window.startWithLastWindowSize)
	{
		values.window.size.width = settings.window.size.width;
		values.window.size.height = settings.window.size.height;
	}
	else
	{
		values.window.size.width = 1024;
		values.window.size.height = 576;
	}
	values.window.aspectRatio.width = values.window.size.width;
	values.window.aspectRatio.height = values.window.size.height;

	values.audio.volume = settings.audio.volume;
	values.audio.bMute = settings.audio.bMute;

	values.video.aspectRatio.bOverride = settings.video.aspectRatio.bOverride;
	values.video.aspectRatio.width = settings.video.aspectRatio.width;
	values.video.aspectRatio.height = settings.video.aspectRatio.height;

	values.video.zoom = settings.video.zoom;
	values.video.zoomMode = settings.video.zoomMode;

	values.video.overlay.brightness = settings.video.overlay.brightness;
	values.video.overlay.contrast = settings.video.overlay.contrast;
	values.video.overlay.hue = settings.video.overlay.hue;
	values.video.overlay.saturation = settings.video.overlay.saturation;
	values.video.overlay.gamma = settings.video.overlay.gamma;

	values.capture.format =	settings.capture.format;

	values.timeshift.format = settings.timeshift.format;
	values.timeshift.dlimit = settings.timeshift.dlimit;;
	values.timeshift.flimit = settings.timeshift.flimit;
	values.timeshift.fdelay = settings.timeshift.fdelay;
	values.timeshift.bufferMinutes = settings.timeshift.bufferMinutes;
	values.timeshift.maxnumbfiles = settings.timeshift.maxnumbfiles;
	values.timeshift.numbfilesrecycled = settings.timeshift.numbfilesrecycled;
	values.timeshift.bufferfilesize = settings.timeshift.bufferfilesize;

	values.dsnetwork.format = settings.dsnetwork.format;

	ZeroMemory(&markedValues, sizeof(VALUES));
	ZeroMemory(&globalValues, sizeof(VALUES));
}

AppData::~AppData()
{
	SaveSettings();

	for (int i = 0; i < 9; i++)
	{
		if (temps.lpstr[i])
			delete[] temps.lpstr[i];
	}

	if (application.appPath)
		delete[] application.appPath;

	if (settings.application.lastServiceCmd)
		delete[] settings.application.lastServiceCmd;

	if (settings.application.currentServiceCmd)
		delete[] settings.application.currentServiceCmd;

	if (settings.capture.fileName)
		delete[] settings.capture.fileName;

	if (settings.capture.folder)
		delete[] settings.capture.folder;

	if (settings.timeshift.folder)
		delete[] settings.timeshift.folder;

	if (settings.timeshift.change)
		delete[] settings.timeshift.change;

	if (settings.timeshift.buffer)
		delete[] settings.timeshift.buffer;

	if (settings.dsnetwork.ipaddr)
		delete[] settings.dsnetwork.ipaddr;

	if (settings.dsnetwork.nicaddr)
		delete[] settings.dsnetwork.nicaddr;
}

LPWSTR AppData::GetSelectionItem(LPWSTR selection)
{
	if (!selection)
		return NULL;

	WCHAR StrTemp[MAX_PATH];
	LPWSTR pStrTemp = NULL;
	//Replace Tokens
	g_pOSD->Data()->ReplaceTokens(selection, pStrTemp);
	if (&pStrTemp)
	{
		wcscpy((LPWSTR)&StrTemp[0], pStrTemp);
		selection = &StrTemp[0];
		delete[] pStrTemp;
		pStrTemp = NULL;
	}

	long startsWithLength = strStartsWith(selection, L"settings.");
	if (startsWithLength > 0)
	{
		selection += startsWithLength;

		startsWithLength = strStartsWith(selection, L"capture.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"format") == 0)
				return GetFormat(settings.capture.format);

			if (_wcsicmp(selection, L"folder") == 0)
				return settings.capture.folder;

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"timeshift.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"format") == 0)
				return GetFormat(settings.timeshift.format);

			if (_wcsicmp(selection, L"folder") == 0)
				return settings.timeshift.folder;

			if (_wcsicmp(selection, L"change") == 0)
				return settings.timeshift.change;

			if (_wcsicmp(selection, L"buffer") == 0)
				return settings.timeshift.buffer;

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"dsnetwork.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"format") == 0)
				return GetFormat(settings.dsnetwork.format);

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"application.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"multiple") == 0)
				return GetBool(settings.application.multiple);

			if (_wcsicmp(selection, L"disableScreenSaver") == 0)
				return GetBool(settings.application.disableScreenSaver);

			if (_wcsicmp(selection, L"priority") == 0)
				return GetPriority(settings.application.priority);

			if (_wcsicmp(selection, L"addToROT") == 0)
				return GetBool(settings.application.addToROT);

			if (_wcsicmp(selection, L"multicard") == 0)
				return GetBool(settings.application.multicard);

			if (_wcsicmp(selection, L"rememberLastService") == 0)
				return GetBool(settings.application.rememberLastService);

			if (_wcsicmp(selection, L"longNetworkName") == 0)
				return GetBool(settings.application.longNetworkName);

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"window.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"startFullscreen") == 0)
				return GetBool(settings.window.startFullscreen);

			if (_wcsicmp(selection, L"startAlwaysOnTop") == 0)
				return GetBool(settings.window.startAlwaysOnTop);

			if (_wcsicmp(selection, L"startAtLastWindowPosition") == 0)
				return GetBool(settings.window.startAtLastWindowPosition);

			if (_wcsicmp(selection, L"startWithLastWindowSize") == 0)
				return GetBool(settings.window.startWithLastWindowSize);

			if (_wcsicmp(selection, L"rememberFullscreenState") == 0)
				return GetBool(settings.window.rememberFullscreenState);

			if (_wcsicmp(selection, L"rememberAlwaysOnTopState") == 0)
				return GetBool(settings.window.rememberAlwaysOnTopState);

			if (_wcsicmp(selection, L"rememberWindowPosition") == 0)
				return GetBool(settings.window.rememberWindowPosition);

			if (_wcsicmp(selection, L"quietOnMinimise") == 0)
				return GetBool(settings.window.quietOnMinimise);

			if (_wcsicmp(selection, L"closeBuffersOnMinimise") == 0)
				return GetBool(settings.window.closeBuffersOnMinimise);

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"audio.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"bMute") == 0)
				return GetBool(settings.audio.bMute);

			return NULL;
		}

		startsWithLength = strStartsWith(selection, L"video.aspectRatio.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			if (_wcsicmp(selection, L"bOverride") == 0)
				return GetBool(settings.video.aspectRatio.bOverride);

			return NULL;
		}
	}

	startsWithLength = strStartsWith(selection, L"bool.");
	if (startsWithLength > 0)
	{
		selection += startsWithLength;

		startsWithLength = strStartsWith(selection, L"1");
		if (startsWithLength > 0)
			return GetBool((long)1);

		startsWithLength = strStartsWith(selection, L"0");
		if (startsWithLength > 0)
			return GetBool((long)0);

		startsWithLength = strStartsWith(selection, L"true");
		if (startsWithLength > 0)
			return GetBool((long)1);

		startsWithLength = strStartsWith(selection, L"false");
		if (startsWithLength > 0)
			return GetBool((long)0);

		return NULL;
	}

	startsWithLength = strStartsWith(selection, L"status.");
	if (startsWithLength > 0)
	{
		selection += startsWithLength;

		startsWithLength = strStartsWith(selection, L"1");
		if (startsWithLength > 0)
			return GetStatus((long)1);

		startsWithLength = strStartsWith(selection, L"0");
		if (startsWithLength > 0)
			return GetStatus((long)0);

		startsWithLength = strStartsWith(selection, L"true");
		if (startsWithLength > 0)
			return GetStatus((long)1);

		startsWithLength = strStartsWith(selection, L"false");
		if (startsWithLength > 0)
			return GetStatus((long)0);

		LPWSTR pValue = GetTempItem(selection);
		if (pValue)
		{
			pValue = GetStatusString(pValue);
			return pValue;
		}
		return NULL;
	}

	startsWithLength = strStartsWith(selection, L"string.");
	if (startsWithLength > 0)
	{
		selection += startsWithLength;

		LPWSTR pValue = NULL;
		strCopy(pValue, selection);
		return pValue;
	}

	return GetTempItem(selection);
}

LPWSTR AppData::GetTempItem(LPWSTR selection)
{
	if (!selection)
		return NULL;

	long startsWithLength = strStartsWith(selection, L"temps.");
	if (startsWithLength > 0)
	{
		selection += startsWithLength;

		startsWithLength = strStartsWith(selection, L"bools.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			int pos = StringToLong(selection);
			if (pos > 9 || pos < 0)
				return NULL;

			return GetBool(temps.bools[pos]);
		}

		startsWithLength = strStartsWith(selection, L"ints.");
		if (startsWithLength > 0)
		{
			LPWSTR pValue = NULL;
			selection += startsWithLength;
			int pos = StringToLong(selection);
			if (pos > 9 || pos < 0)
				return NULL;

			strCopy(pValue, temps.ints[pos]);
			return pValue;
		}

		startsWithLength = strStartsWith(selection, L"longs.");
		if (startsWithLength > 0)
		{
			LPWSTR pValue = NULL;
			selection += startsWithLength;
			int pos = StringToLong(selection);
			if (pos > 9 || pos < 0)
				return NULL;

			strCopy(pValue, temps.longs[pos]);
			return pValue;
		}

		startsWithLength = strStartsWith(selection, L"lpstr.");
		if (startsWithLength > 0)
		{
			selection += startsWithLength;
			int pos = StringToLong(selection);
			if (pos > 9 || pos < 0)
				return NULL;

			return temps.lpstr[pos];
		}
	}
	return NULL;
}

void AppData::SetBuffer(LPWSTR lpwstr)
{
	if (!lpwstr)
		return;
		
	strCopy(settings.timeshift.buffer, lpwstr);	

	if (_wcsicmp(lpwstr, L"Auto") == 0)
		return;

	if (_wcsicmp(lpwstr, L"Small") == 0)
		settings.timeshift.numbfilesrecycled = 2;	
	else if (_wcsicmp(lpwstr, L"Medium") == 0)
		settings.timeshift.numbfilesrecycled = 6;	
	else if (_wcsicmp(lpwstr, L"Large") == 0)
		settings.timeshift.numbfilesrecycled = 20;	

	settings.timeshift.bufferMinutes = 0;	

	return;
}

void AppData::SetChange(LPWSTR lpwstr)
{
	if (!lpwstr)
		return;

	strCopy(settings.timeshift.change, lpwstr);	

	if ((_wcsicmp(lpwstr, L"Fast") == 0))
		settings.timeshift.flimit = 0;	
	else if ((_wcsicmp(lpwstr, L"Normal") == 0))
		settings.timeshift.flimit = 2000000;	
	else if ((_wcsicmp(lpwstr, L"Slow") == 0))
		settings.timeshift.flimit = 4000000;	

	return;
}

LPWSTR AppData::GetFormat(long value)
{
	if (value >= 6 || value < 0)
		return MUX_FORMAT[0];

	return MUX_FORMAT[value];
}

long AppData::GetFormat(LPWSTR lpwstr)
{
	if (!lpwstr)
		return 0;

	if ((_wcsicmp(lpwstr, L"none") == 0))
		return 0;
	if ((_wcsicmp(lpwstr, L"fullmux") == 0))
		return 1;
	if ((_wcsicmp(lpwstr, L"tsmux") == 0))
		return 2;
	if ((_wcsicmp(lpwstr, L"mpgmux") == 0))
		return 3;
	if ((_wcsicmp(lpwstr, L"sepmux") == 0))
		return 4;
	if ((_wcsicmp(lpwstr, L"dvr-ms") == 0))
		return 5;

	return 0;
}

LPWSTR AppData::GetBool(long value)
{
	if (value >= 2 || value < 0)
		return BOOLVALUE[0];

	return BOOLVALUE[value];
}

BOOL AppData::GetBool(LPWSTR lpwstr)
{
	if (!lpwstr)
		return FALSE;

	return (_wcsicmp(lpwstr, L"true") == 0);
}

LPWSTR AppData::GetStatus(long value)
{
	if (value >= 2 || value < 0)
		return STATUSVALUE[0];

	return STATUSVALUE[value];
}

LPWSTR AppData::GetStatusString(LPWSTR lpwstr)
{
	if (!lpwstr)
		return NULL;

	if (_wcsicmp(lpwstr, L"true") == 0 || _wcsicmp(lpwstr, L"1") == 0)
		return STATUSVALUE[1];
	
	return STATUSVALUE[0];
}

BOOL AppData::GetStatus(LPWSTR lpwstr)
{
	if (!lpwstr)
		return FALSE;

	return (_wcsicmp(lpwstr, L"enabled") == 0);
}


LPWSTR AppData::GetPriority(long value)
{
	switch (value)
	{
		case REALTIME_PRIORITY_CLASS:
			return PRIORITY[0];
			break;
		case HIGH_PRIORITY_CLASS:
			return PRIORITY[1];
			break;
		case ABOVE_NORMAL_PRIORITY_CLASS:
			return PRIORITY[2];
			break;
		case NORMAL_PRIORITY_CLASS:
			return PRIORITY[3];
			break;
		case BELOW_NORMAL_PRIORITY_CLASS:
			return PRIORITY[4];
			break;
		case IDLE_PRIORITY_CLASS:
			return PRIORITY[5];
			break;
		default:
			return PRIORITY[3];
	}

	return PRIORITY[3];
	
}

long AppData::GetPriority(LPWSTR lpwstr)
{
	if (!lpwstr)
		return NORMAL_PRIORITY_CLASS;

	if ((_wcsicmp(lpwstr, L"realtime") == 0))
		return REALTIME_PRIORITY_CLASS;
	if ((_wcsicmp(lpwstr, L"high") == 0))
		return HIGH_PRIORITY_CLASS;
	if ((_wcsicmp(lpwstr, L"abovenormal") == 0))
		return ABOVE_NORMAL_PRIORITY_CLASS;
	if ((_wcsicmp(lpwstr, L"normal.") == 0))
		return NORMAL_PRIORITY_CLASS;
	if ((_wcsicmp(lpwstr, L"belownormal") == 0))
		return BELOW_NORMAL_PRIORITY_CLASS;
	if ((_wcsicmp(lpwstr, L"low") == 0))
		return IDLE_PRIORITY_CLASS;

	return NORMAL_PRIORITY_CLASS;
}

void AppData::RestoreMarkedChanges()
{
	long* val    = (long *)&values;
	long* global = (long *)&globalValues;
	long* marked = (long *)&markedValues;

	int size = sizeof(VALUES);
	size = size / 4;

	for (int i=0 ; i<size ; i++)
	{
		if (marked[i] != 0)
			val[i] = global[i];
	}
}

void AppData::StoreGlobalValues()
{
	long* val    = (long *)&values;
	long* global = (long *)&globalValues;

	int size = sizeof(VALUES);
	size = size / 4;

	for (int i=0 ; i<size ; i++)
	{
		global[i] = val[i];
	}
}

void AppData::MarkValuesChanges()
{
	long* val    = (long *)&values;
	long* global = (long *)&globalValues;
	long* marked = (long *)&markedValues;

	int size = sizeof(VALUES);
	size = size / 4;

	for (int i=0 ; i<size ; i++)
	{
		long diff = val[i] ^ global[i];
		marked[i] = (diff == 0) ? 0 : 0xFFFFFFFF;
	}
}

HRESULT AppData::LoadSettings()
{
	wchar_t filename[MAX_PATH];
	swprintf((LPWSTR)&filename, L"%s%s", application.appPath, L"Settings.xml");

	//(log << "Loading DVBT Channels file: " << filename << "\n").Write();
	//LogMessageIndent indent(&log);

	HRESULT hr;

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	if FAILED(hr = file.Load(filename))
	{
		//return (log << "Could not load channels file: " << m_filename << "\n").Show(hr);
		return hr;
	}

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"Application") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"MultipleInstances") == 0)
				{
					settings.application.multiple = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"disableScreenSaver") == 0)
				{
					settings.application.disableScreenSaver = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"priority") == 0 && pSubElement->value)
				{
					if (_wcsicmp(pSubElement->value, L"RealTime") == 0)
					{
						settings.application.priority = REALTIME_PRIORITY_CLASS;
					}
					else if (_wcsicmp(pSubElement->value, L"High") == 0)
					{
						settings.application.priority = HIGH_PRIORITY_CLASS;
					}
					else if (_wcsicmp(pSubElement->value, L"AboveNormal") == 0)
					{
						settings.application.priority = ABOVE_NORMAL_PRIORITY_CLASS;
					}
					else if (_wcsicmp(pSubElement->value, L"BelowNormal") == 0)
					{
						settings.application.priority = BELOW_NORMAL_PRIORITY_CLASS;
					}
					else if (_wcsicmp(pSubElement->value, L"Low") == 0)
					{
						settings.application.priority = IDLE_PRIORITY_CLASS;
					}
					else //if (_wcsicmp(pSubElement->value, L"Normal") == 0)
					{
						settings.application.priority = NORMAL_PRIORITY_CLASS;
					}
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"addToROT") == 0)
				{
					settings.application.addToROT = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"MultiCard") == 0)
				{
					settings.application.multicard = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"RememberLastService") == 0)
				{
					settings.application.rememberLastService = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LastServiceCmd") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.application.lastServiceCmd, pSubElement->value);

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LongNetworkName") == 0)
				{
					settings.application.longNetworkName = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
			}
			continue;
		}
		if (_wcsicmp(element->name, L"Window") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"startFullscreen") == 0)
				{
					settings.window.startFullscreen = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"startAlwaysOnTop") == 0)
				{
					settings.window.startAlwaysOnTop = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"startAtLastWindowPosition") == 0)
				{
					settings.window.startAtLastWindowPosition = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"startWithLastWindowSize") == 0)
				{
					settings.window.startWithLastWindowSize = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"Position") == 0)
				{
					XMLAttribute *attrX = pSubElement->Attributes.Item(L"x");
					XMLAttribute *attrY = pSubElement->Attributes.Item(L"y");
					if (attrX && attrY)
					{
						settings.window.position.x = StringToLong(attrX->value);
						settings.window.position.y = StringToLong(attrY->value);
					}
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Size") == 0)
				{
					XMLAttribute *attrX = pSubElement->Attributes.Item(L"width");
					XMLAttribute *attrY = pSubElement->Attributes.Item(L"height");
					if (attrX && attrY)
					{
						settings.window.size.width  = StringToLong(attrX->value);
						settings.window.size.height = StringToLong(attrY->value);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"rememberFullscreenState") == 0)
				{
					settings.window.rememberFullscreenState = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"rememberAlwaysOnTopState") == 0)
				{
					settings.window.rememberAlwaysOnTopState = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"rememberWindowPosition") == 0)
				{
					settings.window.rememberWindowPosition = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"quietOnMinimise") == 0)
				{
					settings.window.quietOnMinimise = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"closeBuffersOnMinimise") == 0)
				{
					settings.window.closeBuffersOnMinimise = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}

			}
			continue;
		}
		if (_wcsicmp(element->name, L"Audio") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"volume") == 0)
				{
					settings.audio.volume = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"mute") == 0)
				{
					settings.audio.bMute = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
			}
			continue;
		}
		if (_wcsicmp(element->name, L"Video") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"AspectRatio") == 0)
				{
					XMLAttribute *attrOverride = pSubElement->Attributes.Item(L"Override");
					if (attrOverride)
						settings.video.aspectRatio.bOverride = (_wcsicmp(attrOverride->value, L"true") == 0);

					XMLAttribute *attrX = pSubElement->Attributes.Item(L"width");
					XMLAttribute *attrY = pSubElement->Attributes.Item(L"height");
					if (attrX && attrY)
					{
						settings.video.aspectRatio.width  = StringToLong(attrX->value);
						settings.video.aspectRatio.height = StringToLong(attrY->value);
					}
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"zoom") == 0)
				{
					settings.video.zoom = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"zoomMode") == 0)
				{
					settings.video.zoomMode = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Overlay") == 0)
				{
					int sub2Count = pSubElement->Elements.Count();
					for ( int sub2Item=0 ; sub2Item<sub2Count ; sub2Item++ )
					{
						XMLElement *pSub2Element = pSubElement->Elements.Item(sub2Item);
						if (_wcsicmp(pSub2Element->name, L"Brightness") == 0)
						{
							settings.video.overlay.brightness = _wtoi(pSub2Element->value);
							continue;
						}
						if (_wcsicmp(pSub2Element->name, L"Contrast") == 0)
						{
							settings.video.overlay.contrast = _wtoi(pSub2Element->value);
							continue;
						}
						if (_wcsicmp(pSub2Element->name, L"Hue") == 0)
						{
							settings.video.overlay.hue = _wtoi(pSub2Element->value);
							continue;
						}
						if (_wcsicmp(pSub2Element->name, L"Saturation") == 0)
						{
							settings.video.overlay.saturation = _wtoi(pSub2Element->value);
							continue;
						}
						if (_wcsicmp(pSub2Element->name, L"Gamma") == 0)
						{
							settings.video.overlay.gamma = _wtoi(pSub2Element->value);
							continue;
						}
					} // for ( int sub2Item=0 ; sub2Item<sub2Count ; sub2Item++ )
					continue;
				}
			} // for ( int subItem=0 ; subItem<subCount ; subItem++ )
			continue;
		}

		if (_wcsicmp(element->name, L"Capture") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"Format") == 0 && pSubElement->value)
				{
					if (_wcsicmp(pSubElement->value, L"FullMux") == 0)
					{
						settings.capture.format = 1;
					}
					else if (_wcsicmp(pSubElement->value, L"TSMux") == 0)
					{
						settings.capture.format = 2;
					}
					else if (_wcsicmp(pSubElement->value, L"MPGMux") == 0)
					{
						settings.capture.format = 3;
					}
					else if (_wcsicmp(pSubElement->value, L"SepMux") == 0)
					{
						settings.capture.format = 4;
					}
					else if (_wcsicmp(pSubElement->value, L"DVR-MS") == 0)
					{
						settings.capture.format = 5;
					}
					else
					{
						settings.capture.format = 0;
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"Filename") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.capture.fileName, pSubElement->value);

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Folder") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.capture.folder, pSubElement->value);

					continue;
				}
			}
			continue;
		}

		if (_wcsicmp(element->name, L"TimeShift") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"MaxNumbFiles") == 0)
				{
					settings.timeshift.maxnumbfiles = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"BufferMinutes") == 0)
				{
					settings.timeshift.bufferMinutes = _wtoi(pSubElement->value);
					if (settings.timeshift.bufferMinutes)
						strCopy(settings.timeshift.buffer, L"Auto");

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"NumbFilesRecycled") == 0)
				{
					settings.timeshift.numbfilesrecycled = _wtoi(pSubElement->value);

					if (settings.timeshift.bufferMinutes)
						strCopy(settings.timeshift.buffer, L"Auto");
					else  if (settings.timeshift.numbfilesrecycled <= 2)
						strCopy(settings.timeshift.buffer, L"Small");
					else if(settings.timeshift.numbfilesrecycled <= 6)
						strCopy(settings.timeshift.buffer, L"Medium");
					else if(settings.timeshift.numbfilesrecycled > 6)
						strCopy(settings.timeshift.buffer, L"Large");

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"BufferFileSize") == 0)
				{
					settings.timeshift.bufferfilesize = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Format") == 0 && pSubElement->value)
				{
					if (_wcsicmp(pSubElement->value, L"FullMux") == 0)
						settings.timeshift.format = 1;
					else if (_wcsicmp(pSubElement->value, L"TSMux") == 0)
						settings.timeshift.format = 2;
					else if (_wcsicmp(pSubElement->value, L"MPGMux") == 0)
						settings.timeshift.format = 3;
					else
						settings.timeshift.format = 0;

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LoadDelayLimit") == 0)
				{
					settings.timeshift.dlimit = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LoadFileSize") == 0)
				{
					settings.timeshift.flimit = _wtoi(pSubElement->value);

					if (!settings.timeshift.flimit)
						strCopy(settings.timeshift.change, L"Fast");
					else  if (settings.timeshift.flimit >= 2000000)
						strCopy(settings.timeshift.change, L"Normal");
					else if(settings.timeshift.flimit > 2000000)
						strCopy(settings.timeshift.change, L"Slow");

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LoadPauseDelay") == 0)
				{
					settings.timeshift.fdelay = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Folder") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.timeshift.folder, pSubElement->value);

					continue;
				}
			}
			continue;
		}

		if (_wcsicmp(element->name, L"DSNetwork") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"Format") == 0 && pSubElement->value)
				{
					if (_wcsicmp(pSubElement->value, L"FullMux") == 0)
						settings.dsnetwork.format = 1;
					else if (_wcsicmp(pSubElement->value, L"TSMux") == 0)
						settings.dsnetwork.format = 2;
					else if (_wcsicmp(pSubElement->value, L"MPGMux") == 0)
						settings.dsnetwork.format = 3;
					else
						settings.dsnetwork.format = 0;

					continue;
				}
				if (_wcsicmp(pSubElement->name, L"Port") == 0)
				{
					settings.dsnetwork.port = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"IP-Addr") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.dsnetwork.ipaddr, pSubElement->value);

					continue;
				}

				if (_wcsicmp(pSubElement->name, L"Nic-Addr") == 0)
				{
					if (pSubElement->value)
						wcscpy(settings.dsnetwork.nicaddr, pSubElement->value);

					continue;
				}
			}
			continue;
		}

		if (_wcsicmp(element->name, L"Filter_CLSID") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"CLSID_FileSource") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.filesourceclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_FileWriter") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.filewriterclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_TimeShiftWriter") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.timeshiftclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_MPGMuxer") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.mpgmuxclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_DSNetSender") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.dsnetclsid);
					}
					continue;
				}
				
				if (_wcsicmp(pSubElement->name, L"CLSID_DeMultiplexer") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.demuxclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_InfiniteTee") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.infteeclsid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"CLSID_Quantizer") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.quantizerclsid);
					}
					continue;
				}

			}
			continue;
		}

	} // for ( int item=0 ; item<elementCount ; item++ )

	return S_OK;
}

HRESULT AppData::SaveSettings()
{
	wchar_t filename[MAX_PATH];
	swprintf((LPWSTR)&filename, L"%s%s", application.appPath, L"Settings.xml");

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	LPWSTR pValue = NULL;

	XMLElement *pApplication = new XMLElement(L"Application");
	file.Elements.Add(pApplication);
	{
		pApplication->Elements.Add(new XMLElement(L"MultipleInstances", (settings.application.multiple ? L"True" : L"False")));
		pApplication->Elements.Add(new XMLElement(L"DisableScreenSaver", (settings.application.disableScreenSaver ? L"True" : L"False")));
		switch (settings.application.priority)
		{
			case REALTIME_PRIORITY_CLASS:
				strCopy(pValue, L"Realtime");
				break;
			case HIGH_PRIORITY_CLASS:
				strCopy(pValue, L"High");
				break;
			case ABOVE_NORMAL_PRIORITY_CLASS:
				strCopy(pValue, L"AboveNormal");
				break;
			case BELOW_NORMAL_PRIORITY_CLASS:
				strCopy(pValue, L"BelowNormal");
				break;
			case IDLE_PRIORITY_CLASS:
				strCopy(pValue, L"Low");
				break;
			default:
				strCopy(pValue, L"Normal");
				break;
		};
		pApplication->Elements.Add(new XMLElement(L"Priority", pValue));
		pApplication->Elements.Add(new XMLElement(L"AddToROT", (settings.application.addToROT ? L"True" : L"False")));
		pApplication->Elements.Add(new XMLElement(L"MultiCard", (settings.application.multicard ? L"True" : L"False")));
		pApplication->Elements.Add(new XMLElement(L"RememberLastService", (settings.application.rememberLastService ? L"True" : L"False")));
		pApplication->Elements.Add(new XMLElement(L"LastServiceCmd", settings.application.lastServiceCmd));
		pApplication->Elements.Add(new XMLElement(L"LongNetworkName", (settings.application.longNetworkName ? L"True" : L"False")));
	}

	XMLElement *pWindow = new XMLElement(L"Window");
	file.Elements.Add(pWindow);
	{
		if (settings.window.rememberFullscreenState)
			settings.window.startFullscreen = values.window.bFullScreen;
		if (settings.window.rememberAlwaysOnTopState)
			settings.window.startAlwaysOnTop = values.window.bAlwaysOnTop;
		if (settings.window.rememberWindowPosition)
		{
			settings.window.position.x = values.window.position.x;
			settings.window.position.y = values.window.position.y;
			settings.window.size.width = values.window.size.width;
			settings.window.size.height = values.window.size.height;
		}

		pWindow->Elements.Add(new XMLElement(L"StartFullscreen", (settings.window.startFullscreen ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"StartAlwaysOnTop", (settings.window.startAlwaysOnTop ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"StartAtLastWindowPosition", (settings.window.startAtLastWindowPosition ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"StartWithLastWindowSize", (settings.window.startWithLastWindowSize ? L"True" : L"False")));

		XMLElement *pPosition = new XMLElement(L"Position");
		pWindow->Elements.Add(pPosition);
		{
			strCopy(pValue, settings.window.position.x);
			pPosition->Attributes.Add(new XMLAttribute(L"x", pValue));
			strCopy(pValue, settings.window.position.y);
			pPosition->Attributes.Add(new XMLAttribute(L"y", pValue));
		}

		XMLElement *pSize = new XMLElement(L"Size");
		pWindow->Elements.Add(pSize);
		{
			strCopy(pValue, settings.window.size.width);
			pSize->Attributes.Add(new XMLAttribute(L"width", pValue));
			strCopy(pValue, settings.window.size.height);
			pSize->Attributes.Add(new XMLAttribute(L"height", pValue));
		}

		pWindow->Elements.Add(new XMLElement(L"RememberFullscreenState", (settings.window.rememberFullscreenState ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"RememberAlwaysOnTopState", (settings.window.rememberAlwaysOnTopState ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"RememberWindowPosition", (settings.window.rememberWindowPosition ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"QuietOnMinimise", (settings.window.quietOnMinimise ? L"True" : L"False")));
		pWindow->Elements.Add(new XMLElement(L"CloseBuffersOnMinimise", (settings.window.closeBuffersOnMinimise ? L"True" : L"False")));
	}

	XMLElement *pAudio = new XMLElement(L"Audio");
	file.Elements.Add(pAudio);
	{
		strCopy(pValue, settings.audio.volume);
		pAudio->Elements.Add(new XMLElement(L"Volume", pValue));
		pAudio->Elements.Add(new XMLElement(L"Mute", (settings.audio.bMute ? L"True" : L"False")));
	}

	XMLElement *pVideo = new XMLElement(L"Video");
	file.Elements.Add(pVideo);
	{
		XMLElement *pAspectRatio = new XMLElement(L"AspectRatio");
		pVideo->Elements.Add(pAspectRatio);
		{
			pAspectRatio->Attributes.Add(new XMLAttribute(L"override", (settings.video.aspectRatio.bOverride ? L"True" : L"False")));
			strCopy(pValue, settings.video.aspectRatio.width);
			pAspectRatio->Attributes.Add(new XMLAttribute(L"width", pValue));
			strCopy(pValue, settings.video.aspectRatio.height);
			pAspectRatio->Attributes.Add(new XMLAttribute(L"height", pValue));
		}

		strCopy(pValue, settings.video.zoom);
		pVideo->Elements.Add(new XMLElement(L"Zoom", pValue));
		strCopy(pValue, settings.video.zoomMode);
		pVideo->Elements.Add(new XMLElement(L"ZoomMode", pValue));

		XMLElement *pOverlay = new XMLElement(L"Overlay");
		pVideo->Elements.Add(pOverlay);
		{
			strCopy(pValue, settings.video.overlay.brightness);
			pOverlay->Elements.Add(new XMLElement(L"Brightness", pValue));
			strCopy(pValue, settings.video.overlay.contrast);
			pOverlay->Elements.Add(new XMLElement(L"Contrast", pValue));
			strCopy(pValue, settings.video.overlay.hue);
			pOverlay->Elements.Add(new XMLElement(L"Hue", pValue));
			strCopy(pValue, settings.video.overlay.saturation);
			pOverlay->Elements.Add(new XMLElement(L"Saturation", pValue));
			strCopy(pValue, settings.video.overlay.gamma);
			pOverlay->Elements.Add(new XMLElement(L"Gamma", pValue));
		}

	}





/*

	LPWSTR pValue = NULL;

	strCopy(pValue, frequency);
	pElement->Attributes.Add(new XMLAttribute(L"Frequency", pValue));
	delete pValue;
	pValue = NULL;

	strCopy(pValue, bandwidth);
	pElement->Attributes.Add(new XMLAttribute(L"Bandwidth", pValue));
	delete pValue;
	pValue = NULL;

	strCopyHex(pValue, originalNetworkId);
	pElement->Attributes.Add(new XMLAttribute(L"OriginalNetworkId", pValue));
	delete pValue;
	pValue = NULL;

	strCopyHex(pValue, transportStreamId);
	pElement->Attributes.Add(new XMLAttribute(L"TransportStreamId", pValue));
	delete pValue;
	pValue = NULL;

	strCopyHex(pValue, networkId);
	pElement->Attributes.Add(new XMLAttribute(L"NetworkId", pValue));
	delete pValue;
	pValue = NULL;
*/


	XMLElement *pCapture = new XMLElement(L"Capture");
	file.Elements.Add(pCapture);
	{
		pCapture->Elements.Add(new XMLElement(L"Filename", settings.capture.fileName));
		pCapture->Elements.Add(new XMLElement(L"Folder", settings.capture.folder));
		switch (settings.capture.format)
		{
			case 1:
				strCopy(pValue, L"FullMux");
				break;
			case 2:
				strCopy(pValue, L"TSMux");
				break;
			case 3:
				strCopy(pValue, L"MPGMux");
				break;
			case 4:
				strCopy(pValue, L"SepMux");
				break;
//			case 5:
//				strCopy(pValue, L"DVR-MS");
//				break;
			default:
				strCopy(pValue, L"None");
				break;
		};
//		strCopy(pValue, settings.capture.format);
		pCapture->Elements.Add(new XMLElement(L"Format", pValue));
	}

	XMLElement *pTimeshift = new XMLElement(L"Timeshift");
	file.Elements.Add(pTimeshift);
	{
		pTimeshift->Elements.Add(new XMLElement(L"Folder", settings.timeshift.folder));
		strCopy(pValue, settings.timeshift.dlimit);
		pTimeshift->Elements.Add(new XMLElement(L"LoadDelayLimit", pValue));
		strCopy(pValue, settings.timeshift.flimit);
		pTimeshift->Elements.Add(new XMLElement(L"LoadFileSize", pValue));
		strCopy(pValue, settings.timeshift.fdelay);
		pTimeshift->Elements.Add(new XMLElement(L"LoadPauseDelay", pValue));
		strCopy(pValue, settings.timeshift.bufferMinutes);
		pTimeshift->Elements.Add(new XMLElement(L"BufferMinutes", pValue));
		strCopy(pValue, settings.timeshift.maxnumbfiles);
		pTimeshift->Elements.Add(new XMLElement(L"MaxNumbFiles", pValue));
		strCopy(pValue, settings.timeshift.numbfilesrecycled);
		pTimeshift->Elements.Add(new XMLElement(L"NumbFilesRecycled", pValue));
		strCopy(pValue, settings.timeshift.bufferfilesize);
		pTimeshift->Elements.Add(new XMLElement(L"BufferFileSize", pValue));
		switch (settings.timeshift.format)
		{
			case 1:
				strCopy(pValue, L"FullMux");
				break;
			case 2:
				strCopy(pValue, L"TSMux");
				break;
			case 3:
				strCopy(pValue, L"MPGMux");
				break;
			default:
				strCopy(pValue, L"None");
				break;
		};
//		strCopy(pValue, settings.timeshift.format);
		pTimeshift->Elements.Add(new XMLElement(L"Format", pValue));
	}

	XMLElement *pDSNetwork = new XMLElement(L"DSNetwork");
	file.Elements.Add(pDSNetwork);
	{
		switch (settings.dsnetwork.format)
		{
			case 1:
				strCopy(pValue, L"FullMux");
				break;
			case 2:
				strCopy(pValue, L"TSMux");
				break;
			case 3:
				strCopy(pValue, L"MPGMux");
				break;
			default:
				strCopy(pValue, L"None");
				break;
		};
//		strCopy(pValue, settings.dsnetwork.format);
		pDSNetwork->Elements.Add(new XMLElement(L"Format", pValue));
		pDSNetwork->Elements.Add(new XMLElement(L"IP-Addr", settings.dsnetwork.ipaddr));
		strCopy(pValue, settings.dsnetwork.port);
		pDSNetwork->Elements.Add(new XMLElement(L"Port", pValue));
		pDSNetwork->Elements.Add(new XMLElement(L"Nic-Addr", settings.dsnetwork.nicaddr));
	}

	XMLElement *pFilterGUID = new XMLElement(L"Filter_CLSID");
	file.Elements.Add(pFilterGUID);
	{
		LPOLESTR clsid = NULL;
		StringFromCLSID(settings.filterguids.filesourceclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_FileSource", clsid));
		StringFromCLSID(settings.filterguids.filewriterclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_FileWriter", clsid));
		StringFromCLSID(settings.filterguids.timeshiftclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_TimeShiftWriter", clsid));
		StringFromCLSID(settings.filterguids.mpgmuxclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_MPGMuxer", clsid));
		StringFromCLSID(settings.filterguids.dsnetclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_DSNetSender", clsid));
		StringFromCLSID(settings.filterguids.demuxclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_DeMuxer", clsid));
		StringFromCLSID(settings.filterguids.infteeclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_InfiniteTee", clsid));
		StringFromCLSID(settings.filterguids.quantizerclsid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"CLSID_Quantizer", clsid));
	}

	if (pValue)
	{
		delete pValue;
		pValue = NULL;
	}

	return file.Save(filename);
}

/*double AppData::GetAspectRatio()
{
	return aspectRatioWidth / (double)aspectRatioHeight;
}

void AppData::SetAspectRatio(double AR)
{
	aspectRatioWidth = (AR > 0) ? AR : -1;
	aspectRatioHeight = 1;
}

void AppData::SetAspectRatio(int width, int height)
{
	if ((height > 0) && (width > 0))
	{
		aspectRatioWidth = width;
		aspectRatioHeight = height;
	}
	else
	{
		aspectRatioWidth = -1;
		aspectRatioHeight = 1;
	}
}

int AppData::GetZoom()
{
	return (zoom);
}

void AppData::SetZoom(int newZoom)
{
	zoom = (newZoom < 1) ? 1 : newZoom;
}
USES_CONVERSION;
::OutputDebugString(W2T(selection));
::OutputDebugString(W2T(selection));


*/