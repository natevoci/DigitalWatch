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

class AppData
{
public:
	AppData();
	~AppData();

	void LoadSettings();
	void SaveSettings();

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

			BOOL startLastWindowPosition;
			int lastWindowPositionX;
			int lastWindowPositionY;
			int lastWindowWidth;
			int lastWindowHeight;

			BOOL storeFullscreenState;
			BOOL storeAlwaysOnTopState;
			BOOL storeLastWindowPosition;
		} window;

		struct SETTINGS_DISPLAY
		{
			struct SETTINGS_ASPECT_RATIO
			{
				double width;
				double height;
			} aspectRatio;

			int zoom;
			int zoomMode;

			LPWSTR OSDTimeFormat;

			int defaultVideoDecoder;
			int defaultAudioDecoder;

			struct OVERLAY_DISPLAY_SETTINGS
			{
				int brightness;
				int contrast;
				int hue;
				int saturation;
				int gamma;
			} overlay;
		} display;

		struct SETTINGS_CAPTURE
		{
			LPWSTR fileName;
			int format;
		} capture;

		struct SETTINGS_TIMESHIFT
		{
			LPWSTR folder;
			int bufferMinutes;
		} timeshift;

		struct SETTINGS_LASTCHANNEL
		{
			BOOL startLastChannel;
			int network;
			int program;
		} lastChannel;
	} settings;

	//These values reflect runtime values that are not stored in the settings file
	//all values in this structure have to be 32bit to make marking work.
	struct VALUES
	{
		struct VALUES_WINDOW
		{
			long bFullScreen;
			long bAlwaysOnTop;
			long bLockAspect;
			struct VALUES_ASPECT_RATIO
			{
				float width;
				float height;
			} aspectRatio;
		} window;

		long selectedVideoDecoder;
		long selectedAudioDecoder;

		struct VALUES_AUDIO
		{
			long currVolume;
			long bMute;
		} audio;

		struct VALUES_DISPLAY
		{
			struct VALUES_ASPECT_RATIO
			{
				float width;
				float height;
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
		} display;
	} values;

	void RestoreMarkedChanges();
	void StoreGlobalValues();
	void MarkValuesChanges();

private:
	struct VALUES globalValues;
	struct VALUES markedValues;

};

#endif
