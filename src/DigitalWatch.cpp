/**
 *	DigitalWatch.cpp
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

#include "stdafx.h"
#include "DigitalWatch.h"
#include "AppData.h"
#include "TVControl.h"
#include "DigitalWatchWindow.h"
#include "LogMessage.h"
#include "GlobalFunctions.h"

AppData* g_pData;
TVControl* g_pTv;
DigitalWatchWindow* g_pDWWindow;

LogMessage g_log;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	msg.wParam = 0;

	//This is needed before any logging gets done so that g_pData->settings.application.logFilename gets set
	g_pData = new AppData();

	g_log.ClearFile();
	(g_log << "---------------------\nDigitalWatch starting").Write();

	LogVersionNumber();

	//--- Initialize COM ---
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		(g_log << "Failed to initialize COM").Write();
		return -1;
	}

	g_pTv = new TVControl();
	g_pDWWindow = new DigitalWatchWindow();

	switch (g_pData->settings.application.priority)
	{
	case 2:
		(g_log << "Warning: Setting priority to Realtime").Write();
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		break;
	case 1:
		(g_log << "Setting priority to high").Write();
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		break;
	case -1:
		(g_log << "Setting priority to idle").Write();
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
		break;
	default:
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		break;
	}

	if (g_pDWWindow->Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
	{
		g_pTv->Initialise();
		//tv->VideoDecoderEntry(appSettings->defaultVideoDecoder);
		//tv->AudioDecoderEntry(appSettings->defaultAudioDecoder);

		//appData->StoreGlobalValues();
		/*if (appSettings->startLastChannel != 0)
		{
			tv->SetChannel(appSettings->lastNetwork, appSettings->lastProgram);
		}*/
		/*if (appSettings->disableScreenSaver != 0)
		{
			SetTimer(appData->hWnd, 998, 30000, NULL);
		}*/
		//g_pTv->StartTimer();
		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//KillTimer(appData->hWnd, 998);
	}

	delete g_pDWWindow;
	delete g_pTv;
	delete g_pData;

	CoUninitialize();

	return (int) msg.wParam;
}

LRESULT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndDW;

	switch (message) 
	{
	case WM_DESTROY:
		hWndDW = g_pData->hWnd;
		if (hWnd == hWndDW)
			g_pDWWindow->WndProc(hWnd, message, wParam, lParam);

		PostQuitMessage(0);
		break;
	default:
		hWndDW = g_pData->hWnd;
		if (hWnd == hWndDW)
			return g_pDWWindow->WndProc(hWnd, message, wParam, lParam);

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void LogVersionNumber()
{
	USES_CONVERSION;
	wchar_t filename[MAX_PATH];
	GetCommandExe((LPWSTR)&filename);

	DWORD zeroHandle, size = 0;
	while (TRUE)
	{
		size = GetFileVersionInfoSize(W2T((LPWSTR)&filename), &zeroHandle);
		if (size == 0)
			break;

		LPVOID pBlock = (LPVOID) new char[size];
		if (!GetFileVersionInfo(W2T((LPWSTR)&filename), zeroHandle, size, pBlock))
		{
			delete[] pBlock;
			break;
		}

		struct LANGANDCODEPAGE
		{
			WORD wLanguage;
			WORD wCodePage;
		} *lpTranslate;
		UINT uLen = 0;

		if (!VerQueryValue(pBlock, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &uLen) || (uLen == 0))
		{
			delete[] pBlock;
			break;
		}

		LPWSTR SubBlock = new wchar_t[256];
		swprintf(SubBlock, L"\\StringFileInfo\\%04x%04x\\FileVersion", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

		LPSTR lpBuffer;
		UINT dwBytes = 0;
		if (!VerQueryValue(pBlock, W2T(SubBlock), (LPVOID*)&lpBuffer, &dwBytes))
		{
			delete[] pBlock;
			delete[] SubBlock;
			break;
		}
		(g_log << "Build - v" << lpBuffer << "\n").Write();

		delete[] SubBlock;
		delete[] pBlock;
		break;
	}
	if (size == 0)
		(g_log << "Error reading version number\nDigitalWatch built at - " __TIME__ " " __DATE__ "\n").Write();

}