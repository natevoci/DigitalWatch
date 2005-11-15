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
#include "DWOnScreenDisplay.h"
#include "LogMessage.h"
#include "LogMessageWriter.h"
#include "GlobalFunctions.h"

//  Start Memory Leak Detect. Uncomment this to do memory leak detection
//#include "MemLeakDetect.h"
//CMemLeakDetect memLeakDetect;
//  Ene Memory Leak Detect

AppData* g_pData;
TVControl* g_pTv;
DigitalWatchWindow* g_pDWWindow;
DWOnScreenDisplay* g_pOSD;
LogMessageWriter g_DWLogWriter;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	msg.wParam = 0;

	LogMessage log;
	g_DWLogWriter.SetFilename(L"DigitalWatch.log");
	log.AddCallback(&g_DWLogWriter);

	log.ClearFile();
	(log << "---------------------\nDigitalWatch starting\n").Write();

	log.LogVersionNumber();

	g_pData = new AppData();

	//--- Initialize COM ---
	if FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))
	{
		(log << "Failed to initialize COM\n").Write();
		return -1;
	}

	g_pTv = new TVControl();
	g_pDWWindow = new DigitalWatchWindow();
	g_pOSD = new DWOnScreenDisplay();

	g_pTv->SetLogCallback(&g_DWLogWriter);
	g_pDWWindow->SetLogCallback(&g_DWLogWriter);
	g_pOSD->SetLogCallback(&g_DWLogWriter);

	BOOL bSucceeded = SetPriorityClass(GetCurrentProcess(), g_pData->settings.application.priority);
	
	if (g_pDWWindow->Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
	{
		if FAILED(g_pOSD->Initialise())
			return -1;
		if FAILED(g_pTv->Initialise())
			return -1;

		//appData->StoreGlobalValues();
		//if (appSettings->startLastChannel != 0)
		/*{
			tv->SetChannel(appSettings->lastNetwork, appSettings->lastProgram);
		}*/

		BOOL bGotMsg;
		msg.message = WM_NULL;
		PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

		long lastTickCount = 0;

		while (WM_QUIT != msg.message)
		{
			// Use PeekMessage() so we can use idle time to render the scene.
			bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

			if (bGotMsg)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				// Give some CPU time to other processes
				long tickCount = GetTickCount();
				if ((tickCount - lastTickCount) > 10)
				{
					lastTickCount = tickCount;
					// Render a frame during idle time (no messages are waiting)
					if FAILED(g_pOSD->Render(tickCount))
						SendMessage(g_pData->hWnd, WM_CLOSE, 0, 0);
				}
				Sleep(1);
			}
		}

		//KillTimer(appData->hWnd, 998);
	}

	g_pTv->UnloadSource();
	delete g_pOSD;
	g_pOSD = NULL;
	delete g_pDWWindow;
	g_pDWWindow = NULL;
	delete g_pTv;
	g_pTv = NULL;
	delete g_pData;
	g_pData = NULL;

	CoUninitialize();

	return (int) msg.wParam;
}

LRESULT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndDW = (g_pData ? g_pData->hWnd : 0);

	switch (message) 
	{
	case WM_DESTROY:
		if (hWnd == hWndDW)
			g_pDWWindow->WndProc(hWnd, message, wParam, lParam);

		PostQuitMessage(0);
		break;
	default:
		if (hWnd == hWndDW)
			return g_pDWWindow->WndProc(hWnd, message, wParam, lParam);

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

