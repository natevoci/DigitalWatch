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
			BOOL disableScreenSaver;
			int priority;
			BOOL addToROT;
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
			int dlimit;
			int flimit;
			int fdelay;
			int bufferMinutes;
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
		} filterguids;

		int loadedFromFile;
	} settings;

	//These values reflect runtime values that are not stored in the settings file
	//all values in this structure have to be 32bit to make marking work.
	struct VALUES
	{
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
		} timeshift;

		struct SETTINGS_DSNETWORK
		{
			long format;
		} dsnetwork;

	} values;

	void RestoreMarkedChanges();
	void StoreGlobalValues();
	void MarkValuesChanges();

private:
	struct VALUES globalValues;
	struct VALUES markedValues;

};

#endif
