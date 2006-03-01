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

AppData::AppData()
{
	hWnd = 0;

	this->SetLogCallback(&g_DWLogWriter);

	//APPLICATION
	application.appPath = new wchar_t[MAX_PATH];
	GetCommandPath(application.appPath);
	application.bCursorVisible = TRUE;

	//SETTINGS
	settings.application.disableScreenSaver = TRUE;
	settings.application.priority = ABOVE_NORMAL_PRIORITY_CLASS;
	settings.application.addToROT = TRUE;
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
	settings.capture.format = 0;

	settings.timeshift.folder = new wchar_t[MAX_PATH];
	wcscpy(settings.timeshift.folder, L"");
	settings.timeshift.dlimit = 5000;
	settings.timeshift.flimit = 4000000;
	settings.timeshift.fdelay = 1000;
	settings.timeshift.bufferMinutes = 30;
	settings.timeshift.format = 0;

	settings.dsnetwork.format = 0;
	settings.dsnetwork.ipaddr = new wchar_t[MAX_PATH];
	wcscpy(settings.dsnetwork.ipaddr, L"224.0.0.1");
	settings.dsnetwork.port = 0;
	settings.dsnetwork.nicaddr = new wchar_t[MAX_PATH];
	wcscpy(settings.dsnetwork.nicaddr, L"127.0.0.1");

	CComBSTR bstrCLSID(L"{4F8BF30C-3BEB-43a3-8BF2-10096FD28CF2}");
	CLSIDFromString(bstrCLSID, &settings.filterguids.filesourceguid);
	bstrCLSID = GUID_NULL; 
	CLSIDFromString(bstrCLSID, &settings.filterguids.filewriterguid);
	bstrCLSID = L"{5cdd5c68-80dc-43e1-9e44-c849ca8026e7}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.timeshiftguid);
	bstrCLSID = L"{4DF35815-79C5-44C8-8753-847D5C9C3CF5}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.mpgmuxguid);
	bstrCLSID = L"{A07E6137-6C07-45D9-A00C-7DE7A7E6319B}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.dsnetguid);
	bstrCLSID = L"{afb6c280-2c41-11d3-8a60-0000f81e0e4a}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.demuxguid);
	bstrCLSID = L"{F8388A40-D5BB-11d0-BE5A-0080C706568E}";
	CLSIDFromString(bstrCLSID, &settings.filterguids.infteeguid);

	HRESULT hr = LoadSettings();
	settings.loadedFromFile = SUCCEEDED(hr);

	//VALUES
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

	values.dsnetwork.format = settings.dsnetwork.format;

	ZeroMemory(&markedValues, sizeof(VALUES));
	ZeroMemory(&globalValues, sizeof(VALUES));
}

AppData::~AppData()
{
	SaveSettings();

	if (application.appPath)
		delete[] application.appPath;

	if (settings.capture.fileName)
		delete[] settings.capture.fileName;

	if (settings.capture.folder)
		delete[] settings.capture.folder;

	if (settings.timeshift.folder)
		delete[] settings.timeshift.folder;

	if (settings.dsnetwork.ipaddr)
		delete[] settings.dsnetwork.ipaddr;

	if (settings.dsnetwork.nicaddr)
		delete[] settings.dsnetwork.nicaddr;
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
				if (_wcsicmp(pSubElement->name, L"disableScreenSaver") == 0)
				{
					settings.application.disableScreenSaver = (_wcsicmp(pSubElement->value, L"true") == 0);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"priority") == 0)
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
				if (_wcsicmp(pSubElement->name, L"Format") == 0)
				{
					settings.capture.format = _wtoi(pSubElement->value);
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
				if (_wcsicmp(pSubElement->name, L"Format") == 0)
				{
					settings.timeshift.format = _wtoi(pSubElement->value);
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
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"LoadPauseDelay") == 0)
				{
					settings.timeshift.fdelay = _wtoi(pSubElement->value);
					continue;
				}
				if (_wcsicmp(pSubElement->name, L"BufferMinutes") == 0)
				{
					settings.timeshift.bufferMinutes = _wtoi(pSubElement->value);
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
				if (_wcsicmp(pSubElement->name, L"Format") == 0)
				{
					settings.dsnetwork.format = _wtoi(pSubElement->value);
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

		if (_wcsicmp(element->name, L"FilterGUID") == 0)
		{
			int subCount = element->Elements.Count();
			for ( int subItem=0 ; subItem<subCount ; subItem++ )
			{
				XMLElement *pSubElement = element->Elements.Item(subItem);
				if (_wcsicmp(pSubElement->name, L"FileSourceGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.filesourceguid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"FileWriterGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.filewriterguid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"TimeShiftGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.timeshiftguid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"MPGMuxGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.mpgmuxguid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"DSNetGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.dsnetguid);
					}
					continue;
				}
				
				if (_wcsicmp(pSubElement->name, L"DemuxGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.demuxguid);
					}
					continue;
				}

				if (_wcsicmp(pSubElement->name, L"InfTeeGuid") == 0)
				{
					if (pSubElement->value)
					{
						CComBSTR bstrCLSID(pSubElement->value);
						CLSIDFromString(bstrCLSID, &settings.filterguids.infteeguid);
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

	XMLElement *pCapture = new XMLElement(L"Capture");
	file.Elements.Add(pCapture);
	{
		pCapture->Elements.Add(new XMLElement(L"Filename", settings.capture.fileName));
		pCapture->Elements.Add(new XMLElement(L"Folder", settings.capture.folder));
		strCopy(pValue, settings.capture.format);
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
		strCopy(pValue, settings.timeshift.format);
		pTimeshift->Elements.Add(new XMLElement(L"Format", pValue));
	}

	XMLElement *pDSNetwork = new XMLElement(L"DSNetwork");
	file.Elements.Add(pDSNetwork);
	{
		strCopy(pValue, settings.dsnetwork.format);
		pDSNetwork->Elements.Add(new XMLElement(L"Format", pValue));
		pDSNetwork->Elements.Add(new XMLElement(L"IP-Addr", settings.dsnetwork.ipaddr));
		strCopy(pValue, settings.dsnetwork.port);
		pDSNetwork->Elements.Add(new XMLElement(L"Port", pValue));
		pDSNetwork->Elements.Add(new XMLElement(L"Nic-Addr", settings.dsnetwork.nicaddr));
	}

	XMLElement *pFilterGUID = new XMLElement(L"FilterGUID");
	file.Elements.Add(pFilterGUID);
	{
		LPOLESTR clsid = new WCHAR[sizeof(CLSID)];
		StringFromCLSID(settings.filterguids.filesourceguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"FileSourceGuid", clsid));
		StringFromCLSID(settings.filterguids.filewriterguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"FileWriterGuid", clsid));
		StringFromCLSID(settings.filterguids.timeshiftguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"TimeShiftGuid", clsid));
		StringFromCLSID(settings.filterguids.mpgmuxguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"MPGMuxGuid", clsid));
		StringFromCLSID(settings.filterguids.dsnetguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"DSNetGuid", clsid));
		StringFromCLSID(settings.filterguids.demuxguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"DemuxGuid", clsid));
		StringFromCLSID(settings.filterguids.infteeguid, &clsid);
		pFilterGUID->Elements.Add(new XMLElement(L"InfTeeGuid", clsid));
		delete[] clsid;
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

*/