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
#if (_MSC_VER == 1200)
	#include <fstream.h>
#else
	#include <fstream>
	using namespace std;
#endif

AppData::AppData()
{
	hWnd = 0;

	//APPLICATION
	application.appPath = new wchar_t[MAX_PATH];
	GetCommandPath(application.appPath);
	application.bCursorVisible = TRUE;
	application.recordingTimeLeft = 0;

	//SETTINGS
	settings.application.disableScreenSaver = 0;
	settings.application.priority = 0;
	settings.application.addToROT = 1;
//	settings.application.logFilename = new wchar_t[MAX_PATH];
//	swprintf(settings.application.logFilename, L"%s%s", application.appPath, L"DigitalWatch.log");
	
	settings.window.startFullscreen = 0;
	settings.window.startAlwaysOnTop = 0;

	settings.window.startLastWindowPosition = 0;
	settings.window.lastWindowPositionX = 0;
	settings.window.lastWindowPositionY = 0;
	settings.window.lastWindowWidth = 720;
	settings.window.lastWindowHeight = 408;

	settings.window.storeFullscreenState = 0;
	settings.window.storeAlwaysOnTopState = 0;
	settings.window.storeLastWindowPosition = 0;

	settings.display.aspectRatio.width = 16;
	settings.display.aspectRatio.height = 9;

	settings.display.zoom = 100;
	settings.display.zoomMode = 0;

	settings.display.OSDTimeFormat = NULL;

	settings.display.defaultVideoDecoder;
	settings.display.defaultAudioDecoder;

	settings.display.overlay.brightness = 750;
	settings.display.overlay.contrast = 10000;
	settings.display.overlay.hue = 0;
	settings.display.overlay.saturation = 10000;
	settings.display.overlay.gamma = 1;

	settings.capture.fileName = NULL;
	settings.capture.format = 0;

	settings.timeshift.folder = NULL;
	settings.timeshift.bufferMinutes = 30;

	settings.lastChannel.startLastChannel = 0;
	settings.lastChannel.network = 0;
	settings.lastChannel.program = 0;

	//VALUES
	values.window.bFullScreen = FALSE;
	values.window.bAlwaysOnTop = FALSE;

	values.currTVNetwork = -1;
	values.currTVProgram = -1;

	values.lastTVNetwork = -1;
	values.lastTVProgram = -1;

	values.selectedVideoDecoder = 0;
	values.selectedAudioDecoder = 0;

	values.currVolume = 100;
	values.bMute = FALSE;

	values.display.aspectRatio.width = 16;
	values.display.aspectRatio.height = 9;

	values.display.zoom = 100;
	values.display.zoomMode = 0;

	values.display.overlay.brightness = 750;
	values.display.overlay.contrast = 10000;
	values.display.overlay.hue = 0;
	values.display.overlay.saturation = 10000;
	values.display.overlay.gamma = 1;

	//Load VideoDecoders.ini
	//Load AudioDecoders.ini
	//Load Channels.ini
	//Load Resolutions.ini
	//Load Keys.ini

	ZeroMemory(&markedValues, sizeof(VALUES));
	ZeroMemory(&globalValues, sizeof(VALUES));
}

AppData::~AppData()
{
	if (application.appPath)
		delete[] application.appPath;
	if (settings.display.OSDTimeFormat)
		delete[] settings.display.OSDTimeFormat;
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

void AppData::LoadSettings()
{/*
	char filename[MAX_PATH];
	sprintf((char*)&filename, "%s%s", m_pTv->m_pAppData->appPath, "Settings.ini");
	
	ifstream file;
	file.open(filename);
	if (file.is_open() != 1)
	{
		//TODO: OSD Error
		char* error = (char*) alloca(MAX_PATH+12);
		sprintf(error, "Error: Cannot load %s", filename);
		MessageBox(NULL, error, "DigitalWatch", MB_OK);
		return;
	}

	char* buff = (char*)alloca(256);
	char* value;
	while (!file.eof())
	{
		file.getline(buff, 256);

		if (buff[0] == '\0')
			continue;
		if (buff[0] == '#')
			continue;
		if ((buff[0] == '/') && (buff[1] == '/'))
			continue;
		value = strpbrk(buff, "= ");
		value[0] = '\0';
		value++;
		while ((value[0] == ' ') || (value[0] == '=')) value++;

		if (_stricmp(buff, "CardNumber") == 0)
			cardNumber = atoi(value);
		else if (_stricmp(buff, "DVBInput") == 0)
			dvbInput = atoi(value);
		else if (_stricmp(buff, "CaptureFileName") == 0)
			strcpy(captureFileName, value);
		else if (_stricmp(buff, "CaptureFormat") == 0)
			captureFormat = atoi(value);
		else if (_stricmp(buff, "OSDTimeFormat") == 0)
			strcpy(OSDTimeFormat, value);
		else if (_stricmp(buff, "TimeShiftFolder") == 0)
			strcpy(timeshiftFolder, value);
		else if (_stricmp(buff, "TimeShiftBufferMinutes") == 0)
			timeshiftBufferMinutes = atoi(value);
		else if (_stricmp(buff, "AspectRatio") == 0)
		{
			char* colon;
			colon = strchr(value, ':');
			colon[0] = '\0';
			colon++;
			aspectRatioWidth = atoi(value);
			aspectRatioHeight = atoi(colon);
		}
		else if (_stricmp(buff, "Zoom") == 0)
			zoom = atoi(value);
		else if (_stricmp(buff, "ZoomMode") == 0)
			zoomMode = atoi(value);

		else if (_stricmp(buff, "StartFullscreen") == 0)
			startFullscreen = atoi(value);
		else if (_stricmp(buff, "StartAlwaysOnTop") == 0)
			startAlwaysOnTop = atoi(value);

		else if (_stricmp(buff, "StartLastWindowPosition") == 0)
			startLastWindowPosition = atoi(value);
		else if (_stricmp(buff, "LastWindowPositionX") == 0)
			lastWindowPositionX = atoi(value);
		else if (_stricmp(buff, "LastWindowPositionY") == 0)
			lastWindowPositionY = atoi(value);
		else if (_stricmp(buff, "LastWindowWidth") == 0)
			lastWindowWidth = atoi(value);
		else if (_stricmp(buff, "LastWindowHeight") == 0)
			lastWindowHeight = atoi(value);

		else if (_stricmp(buff, "StoreFullscreenState") == 0)
			storeFullscreenState = atoi(value);
		else if (_stricmp(buff, "StoreAlwaysOnTopState") == 0)
			storeAlwaysOnTopState = atoi(value);
		else if (_stricmp(buff, "StoreLastWindowPosition") == 0)
			storeLastWindowPosition = atoi(value);
		
		else if (_stricmp(buff, "StartLastChannel") == 0)
			startLastChannel = atoi(value);
		else if (_stricmp(buff, "LastNetwork") == 0)
			lastNetwork = atoi(value);
		else if (_stricmp(buff, "LastProgram") == 0)
			lastProgram = atoi(value);
		else if (_stricmp(buff, "DefaultVideoDecoder") == 0)
			defaultVideoDecoder = atoi(value);
		else if (_stricmp(buff, "DefaultAudioDecoder") == 0)
			defaultAudioDecoder = atoi(value);

		else if (_stricmp(buff, "DefaultBrightness") == 0)
			brightness = atoi(value);
		else if (_stricmp(buff, "DefaultContrast") == 0)
			contrast = atoi(value);
		else if (_stricmp(buff, "DefaultHue") == 0)
			hue = atoi(value);
		else if (_stricmp(buff, "DefaultSaturation") == 0)
			saturation = atoi(value);
		else if (_stricmp(buff, "DefaultGamma") == 0)
			gamma = atoi(value);

		else if (_stricmp(buff, "DisableScreenSaver") == 0)
			disableScreenSaver = atoi(value);

		else if (_stricmp(buff, "BasePriority") == 0)
			priority = atoi(value);

		else if (_stricmp(buff, "AddToROT") == 0)
			addToROT = atoi(value);
	}
	file.close();

	m_pTv->m_pAppData->cardNumber = cardNumber;
	m_pTv->m_pAppData->dvbInput = dvbInput;
	strcpy((char*)m_pTv->m_pAppData->captureFileName, captureFileName);
	m_pTv->m_pAppData->captureFormat = captureFormat;
	strcpy((char*)m_pTv->m_pAppData->OSDTimeFormat, OSDTimeFormat);
	strcpy((char*)m_pTv->m_pAppData->timeshiftFolder, timeshiftFolder);
	m_pTv->m_pAppData->timeshiftBufferMinutes = timeshiftBufferMinutes;
	m_pTv->m_pAppData->values.aspectRatioWidth = aspectRatioWidth;
	m_pTv->m_pAppData->values.aspectRatioHeight = aspectRatioHeight;
	m_pTv->m_pAppData->values.zoom = zoom;
	m_pTv->m_pAppData->values.zoomMode = zoomMode;
	m_pTv->m_pAppData->values.brightness = brightness;
	m_pTv->m_pAppData->values.contrast = contrast;
	m_pTv->m_pAppData->values.hue = hue;
	m_pTv->m_pAppData->values.saturation = saturation;
	m_pTv->m_pAppData->values.gamma = gamma;
	m_pTv->m_pAppData->addToROT = addToROT;
	*/
}

void AppData::SaveSettings()
{/*
	char filename[MAX_PATH];
	sprintf((char*)&filename, "%s%s", m_pTv->m_pAppData->appPath, "Settings.ini");

	ofstream file;
	file.open(filename);
	if (file.is_open() != 1)
	{
		MessageBox(NULL, "Cannot save settings.ini", "DigitalWatch", MB_OK);
		return;
	}

	file << "# DigitalWatch - Settings.ini" << endl;
	file << "#" << endl;
	file << endl;
	file << "CardNumber = " << cardNumber << endl;
	file << "DVBInput = " << dvbInput << endl;
	file << endl;
	file << "CaptureFileName = " << captureFileName << endl;
	file << "CaptureFormat = " << captureFormat << endl;
	file << endl;
	file << "OSDTimeFormat = " << OSDTimeFormat << endl;
	file << endl;
	file << "TimeShiftFolder = " << timeshiftFolder << endl;
	file << "TimeShiftBufferMinutes = " << timeshiftBufferMinutes << endl;
	file << endl;
	file << "AspectRatio = " << aspectRatioWidth << ":" << aspectRatioHeight << endl;
	file << "Zoom = " << zoom << endl;
	file << "ZoomMode = " << zoomMode << endl;
	file << endl;
	file << "StartFullscreen = " << startFullscreen << endl;
	file << "StartAlwaysOnTop = " << startAlwaysOnTop << endl;
	file << endl;
	file << "StartLastWindowPosition = " << startLastWindowPosition << endl;
	file << "LastWindowPositionX = " << lastWindowPositionX << endl;
	file << "LastWindowPositionY = " << lastWindowPositionY << endl;
	file << "LastWindowWidth = " << lastWindowWidth << endl;
	file << "LastWindowHeight = " << lastWindowHeight << endl;
	file << endl;
	file << "StoreFullscreenState = " << storeFullscreenState << endl;
	file << "StoreAlwaysOnTopState = " << storeAlwaysOnTopState << endl;
	file << "StoreLastWindowPosition = " << storeLastWindowPosition << endl;
	file << endl;
	file << "StartLastChannel = " << startLastChannel << endl;
	file << "LastNetwork = " << lastNetwork << endl;
	file << "LastProgram = " << lastProgram << endl;
	file << endl;
	file << "DefaultVideoDecoder = " << defaultVideoDecoder << endl;
	file << "DefaultAudioDecoder = " << defaultAudioDecoder << endl;
	file << endl;
	file << "DefaultBrightness = "  << brightness << endl;
	file << "DefaultContrast = "  << contrast << endl;
	file << "DefaultHue = "  << hue << endl;
	file << "DefaultSaturation = "  << saturation << endl;
	file << "DefaultGamma = "  << gamma << endl;
	file << endl;
	file << "DisableScreenSaver = " << disableScreenSaver << endl;
	file << endl;
	file << "BasePriority = " << priority << endl;
	file << endl;
	file << "AddToROT = " << addToROT << endl;
	file << endl;

	file.close();
	*/
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