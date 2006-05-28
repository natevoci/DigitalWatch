/**
 *	AppData.h
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

#ifndef APPDATA_H
#define APPDATA_H
//#pragma warning (disable : 4995)

#include "StdAfx.h"
#include "LogMessage.h"

class AppData : LogMessageCaller
{
public:
	AppData();
	~AppData();

	HRESULT LoadSettings();
	HRESULT SaveSettings();

	HWND hWnd;

	//These values are used as temp values passed within the OSD menu.
	struct TEMP_VALUES
	{
		BOOL bools[9];
		int	ints[9];
		long longs[9];
		LPWSTR lpstr[9];
	} temps;

	struct APPLICATION
	{
		LPWSTR appPath;
		long bCursorVisible;
	} application;

	//These values reflect what is stored in the settings file.
	struct SETTINGS
	{
		struct SETTINGS_APPLICATION
		{
			BOOL multiple;
			BOOL disableScreenSaver;
			int priority;
			BOOL addToROT;
			BOOL multicard;
			BOOL cyclecards;
			BOOL rememberLastService;
			LPWSTR lastServiceCmd;
			LPWSTR currentServiceCmd;
			BOOL longNetworkName;
			BOOL decoderTest;
			//LPWSTR logFilename;
		} application;

		struct SETTINGS_WINDOW
		{
			BOOL startFullscreen;
			BOOL startAlwaysOnTop;

			BOOL startAtLastWindowPosition;
			BOOL startWithLastWindowSize;

			struct SETTINGS_WINDOW_POSITION
			{
				long x;
				long y;
			} position;
			struct SETTINGS_WINDOW_SIZE
			{
				long width;
				long height;
			} size;

			//int lastWindowPositionX;
			//int lastWindowPositionY;
			//int lastWindowWidth;
			//int lastWindowHeight;

			BOOL rememberFullscreenState;
			BOOL rememberAlwaysOnTopState;
			BOOL rememberWindowPosition;
			BOOL quietOnMinimise;
			BOOL closeBuffersOnMinimise;
		} window;

		struct VALUES_AUDIO
		{
			long volume;
			long bMute;
		} audio;

		struct SETTINGS_VIDEO
		{
			struct SETTINGS_ASPECT_RATIO
			{
				long bOverride;
				long width;
				long height;
			} aspectRatio;

			int zoom;
			int zoomMode;

			struct OVERLAY_DISPLAY_SETTINGS
			{
				int brightness;
				int contrast;
				int hue;
				int saturation;
				int gamma;
			} overlay;
		} video;

		struct SETTINGS_CAPTURE
		{
			LPWSTR fileName;
			LPWSTR folder;
			int format;
		} capture;
		
		struct SETTINGS_TIMESHIFT
		{
			LPWSTR folder;
			LPWSTR change;
			LPWSTR buffer;
			int dlimit;
			int flimit;
			int fdelay;
			int bufferMinutes;
			int maxnumbfiles;
			int numbfilesrecycled;
			int bufferfilesize;
			int format;
		} timeshift;

		struct SETTINGS_DSNETWORK
		{
			int format;
			LPWSTR ipaddr;
			long port;
			LPWSTR nicaddr;
		} dsnetwork;

		struct SETTINGS_FILTERGUID
		{
			GUID filesourceclsid;
			GUID filewriterclsid;
			GUID timeshiftclsid;
			GUID mpgmuxclsid;
			GUID dsnetclsid;
			GUID demuxclsid;
			GUID infteeclsid;
			GUID quantizerclsid;
		} filterguids;

		int loadedFromFile;
	} settings;

	//These values reflect runtime values that are not stored in the settings file
	//all values in this structure have to be 32bit to make marking work.
	struct VALUES
	{
		struct SETTINGS_APPLICATION
		{
			BOOL multiple;
			long multicard;
		} application;

		struct VALUES_WINDOW
		{
			long bFullScreen;
			long bAlwaysOnTop;
			struct VALUES_WINDOW_POSITION
			{
				long x;
				long y;
			} position;
			struct VALUES_WINDOW_SIZE
			{
				long width;
				long height;
			} size;
			struct VALUES_WINDOW_ASPECTRATIO
			{
				long width;
				long height;
			} aspectRatio;
		} window;

		struct VALUES_AUDIO
		{
			long volume;
			long bMute;
		} audio;

		struct VALUES_VIDEO
		{
			struct VALUES_ASPECT_RATIO
			{
				long bOverride;
				long width;
				long height;
			} aspectRatio;

			long zoom;
			long zoomMode;

			struct VALUES_OVERLAY_SETTINGS
			{
				long brightness;
				long contrast;
				long hue;
				long saturation;
				long gamma;
			} overlay;
		} video;

		struct SETTINGS_CAPTURE
		{
			long format;
		} capture;

		struct SETTINGS_TIMESHIFT
		{
			long format;
			long dlimit;
			long flimit;
			long fdelay;
			long bufferMinutes;
			long maxnumbfiles;
			long numbfilesrecycled;
			long bufferfilesize;
		} timeshift;

		struct SETTINGS_DSNETWORK
		{
			long format;
		} dsnetwork;

	} values;

	void RestoreMarkedChanges();
	void StoreGlobalValues();
	void MarkValuesChanges();
	LPWSTR GetSelectionItem(LPWSTR selection);
	LPWSTR GetTempItem(LPWSTR selection);
	void SetBuffer(LPWSTR lpwstr);
	void SetChange(LPWSTR lpwstr);
	LPWSTR GetFormat(long value);
	LPWSTR GetBool(long value);
	LPWSTR GetStatus(long value);
	LPWSTR GetStatusString(LPWSTR lpwstr);
	LPWSTR GetPriority(long value);
	long GetFormat(LPWSTR lpwstr);
	BOOL GetBool(LPWSTR lpwstr);
	BOOL GetStatus(LPWSTR lpwstr);
	long GetPriority(LPWSTR lpwstr);


private:
	struct VALUES globalValues;
	struct VALUES markedValues;
};

#endif
