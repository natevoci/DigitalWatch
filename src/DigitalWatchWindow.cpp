/**
 *	DigitalWatchWindow.cpp
 *	Copyright (C) 2003-2004 Nathan
 *  Copyright (C) 2004 Builty
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
#include "../resource.h"
#include <winuser.h>
#include "DigitalWatch.h"
#include "DigitalWatchWindow.h"
#include "Globals.h"

DigitalWatchWindow::DigitalWatchWindow()
{
	log.AddCallback(&g_DWLogWriter);
}

DigitalWatchWindow::~DigitalWatchWindow()
{
}

int DigitalWatchWindow::Create(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char szWindowClass[100];
	char szTitle[100];

	sprintf((char *)&szWindowClass, "DIGITALWATCH_BDA");
	sprintf((char *)&szTitle, "DigitalWatch");

	g_pData->hWnd = FindWindow(szWindowClass, NULL);
	//LATER: Option for multiple instances
	if (g_pData->hWnd != NULL)
	{
		(log << "An instance of Digital Watch is already running.\n  Bringing the existing instance to the foreground.\n").Write();
		SetForegroundWindow(g_pData->hWnd);
		return FALSE;
	}

	HBRUSH br = CreateSolidBrush(0x00000000);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_DBLCLKS;// | CS_HREDRAW | CS_VREDRAW;// | CS_NOCLOSE | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= 0;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_DIGITALWATCH);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= br;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	ATOM ar = RegisterClassEx(&wcex);

	g_pData->hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW/*WS_THICKFRAME*/, CW_USEDEFAULT, 0, 726, 435, NULL, NULL, NULL, NULL);
	if (!g_pData->hWnd)
	{
		__int64 err = GetLastError();
		//return_FALSE_SHOWMSG("Error creating window: " << err)
		return (log << "Error creating window: " << err << "\n").Show();
	}

	SetWindowLong(g_pData->hWnd, GWL_STYLE, GetWindowLong(g_pData->hWnd, GWL_STYLE) & (~(WS_CAPTION/* | WS_BORDER*/))); 
	
	SetWindowPos(g_pData->hWnd, NULL, 0, 0, 726, 414, SWP_NOMOVE | SWP_NOZORDER);

	if (g_pData->settings.window.startLastWindowPosition != 0)
	{
		(log << "Restoring last window position. x=" << g_pData->settings.window.lastWindowPositionX << " y=" << g_pData->settings.window.lastWindowPositionY << " width=" <<g_pData->settings.window.lastWindowWidth << " height=" << g_pData->settings.window.lastWindowHeight << "\n").Write();
		SetWindowPos(g_pData->hWnd, NULL, g_pData->settings.window.lastWindowPositionX, g_pData->settings.window.lastWindowPositionY, g_pData->settings.window.lastWindowWidth, g_pData->settings.window.lastWindowHeight, SWP_NOZORDER );
	}

	ShowWindow(g_pData->hWnd, nCmdShow);
	UpdateWindow(g_pData->hWnd);

	if (g_pData->settings.window.startFullscreen != 0)
	{
		(log << "Restoring fullscreen state\n").Write();
		g_pTv->Fullscreen();
	}
	if (g_pData->settings.window.startAlwaysOnTop != 0)
	{
		(log << "Restoring always on top\n").Write();
		g_pTv->AlwaysOnTop();
	}

	return TRUE;
}

LRESULT DigitalWatchWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;

	static BOOL bLeftMouseDown = FALSE;
	//static BOOL controlBarCapture = 0;
	static short wheelMovement = 0;
	static int xPos = 0;
	static int yPos = 0;
	static RECT moveRect;
	static COPYDATASTRUCT *cds;
	static BOOL bShiftDown = FALSE;
	static BOOL bCtrlDown = FALSE;
	static BOOL bAltDown = FALSE;

	switch (message) 
	{
/*	case WM_COPYDATA:
		cds = (COPYDATASTRUCT*) lParam;
		switch (cds->dwData)
		{
		case 1:
			return m_pTv->ExecuteFunction((char*)cds->lpData);
		}
		break;*/
	case WM_PAINT:
		//if (m_pTv == NULL)
			lResult = DefWindowProc(hWnd, message, wParam, lParam);
		/*else if (!m_pTv->m_pFilterGraph->IsPlaying())
			lResult = DefWindowProc(hWnd, message, wParam, lParam);
		else
		{
			lResult = DefWindowProc(hWnd, message, wParam, lParam);
			m_pTv->m_pOSD->RefreshOSD();
		}*/
		break;
	case WM_ERASEBKGND:
		//if (m_pTv == NULL)
			lResult = DefWindowProc(hWnd, message, wParam, lParam);
		/*else
		{
			m_pTv->m_pOSD->EraseBackground();
			m_pTv->m_pOSD->RefreshOSD();
		}*/

		break;
/*	case WM_USER:
		if (wParam == 1)
		{
			//Sleep(100);
			m_pTv->m_pFilterGraph->ApplyColorControls();
			m_pTv->m_pOSD->RefreshOSD();
			break;
		}*/
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		if (wParam == 255)
			break;
		if (wParam == 16)
		{
			bShiftDown = TRUE;
			break;
		}
		if (wParam == 17)
		{
			bCtrlDown = TRUE;
			break;
		}
		if (wParam == 18)
		{
			bAltDown = TRUE;
			break;
		}
		//hack to stop alt-space (system menu) locking alt key down.
		if ((wParam == 32) && !bShiftDown && !bCtrlDown && bAltDown)
		{
			bAltDown = FALSE;
		}
		else
		{
			g_pTv->Key(wParam, bShiftDown, bCtrlDown, bAltDown);
		}
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if (wParam == 16)
			bShiftDown = FALSE;
		if (wParam == 17)
			bCtrlDown = FALSE;
		if (wParam == 18)
			bAltDown = FALSE;
		break;
	case WM_SIZE:
		lResult = DefWindowProc(hWnd, message, wParam, lParam);
		/*switch (wParam)
		{
		case SIZE_MAXIMIZED:
			m_pAppData->values.bFullScreen = TRUE;
			break;
		case SIZE_RESTORED:
			m_pAppData->values.bFullScreen = FALSE;
			break;
		}
		if ((m_pTv != NULL) && (m_pTv->m_pFilterGraph->IsPlaying()))
		{
			m_pTv->m_pFilterGraph->RefreshVideoPosition();
			m_pTv->m_pOSD->RefreshOSD();
		}*/
		break;

	case WM_LBUTTONDBLCLK:
		/*if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
			m_pTv->Key(MOUSE_LDBLCLICK, bShiftDown, bCtrlDown, bAltDown);*/
		break;
	case WM_RBUTTONDOWN:
		/*if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
			m_pTv->Key(MOUSE_RCLICK, bShiftDown, bCtrlDown, bAltDown);*/
		break;
	case WM_MBUTTONDOWN:
		/*if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
			m_pTv->Key(MOUSE_MCLICK, bShiftDown, bCtrlDown, bAltDown);*/
		break;
	case WM_MOUSEWHEEL:
		/*if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
		{
			wheelMovement = (short)HIWORD(wParam);
			if (wheelMovement > 0)
				m_pTv->Key(MOUSE_SCROLL_UP  , bShiftDown, bCtrlDown, bAltDown);
			if (wheelMovement < 0)
				m_pTv->Key(MOUSE_SCROLL_DOWN, bShiftDown, bCtrlDown, bAltDown);
		}*/
		break;

	case WM_LBUTTONDOWN:
		if ((wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON) && !(wParam & MK_RBUTTON))
		{
			SetCapture(hWnd);
			/*if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
			{
				m_pTv->ControlBarMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			else*/
			{
				bLeftMouseDown = TRUE;
				GetWindowRect(hWnd, &moveRect);
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
			}
		}
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		//m_pTv->ControlBarMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		bLeftMouseDown = FALSE;
		break;
	case WM_MOUSEMOVE:
		//m_pTv->ShowCursor();

		if (bLeftMouseDown /*&& ((m_pTv == NULL) || !m_pTv->m_pAppData->values.bFullScreen)*/)
		{
			RECT currRect;
			GetWindowRect(hWnd, &currRect);
			if ((currRect.left == moveRect.left) && (currRect.top == moveRect.top))
			{
				Sleep(10);
				int xOff = GET_X_LPARAM(lParam) - xPos;
				int yOff = GET_Y_LPARAM(lParam) - yPos;
				if ((xOff != 0) || (yOff != 0))
				{
					moveRect.left += xOff;
					moveRect.top  += yOff;
					SetWindowPos(hWnd, HWND_TOP, moveRect.left, moveRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
			}
		}
		/*else if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
		{
		}*/
		break;
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE)
		{
			ReleaseCapture();
			bLeftMouseDown = FALSE;
			bShiftDown = FALSE;
			bCtrlDown = FALSE;
			bAltDown = FALSE;
		}
		break;
	case WM_TIMER:
/*		switch (wParam)
		{
		case 990:	//Every second while recording
			if (m_pAppData->recordingTimeLeft == 1)
			{
				m_pTv->Recording(0);
			}
			if (m_pAppData->recordingTimeLeft > 1)
			{
				m_pAppData->recordingTimeLeft--;
			}
			break;
		case 996:	//every 100ms the whole time the program is running.
			m_pTv->m_pOSD->RepaintOSD(TRUE);
			break;
		case 997:	//Every second the whole time the program is running.
			m_pTv->m_pOSD->Update();
			break;
		case 998:	//Every 30 seconds to keep power saving coming on.
			SetThreadExecutionState(ES_DISPLAY_REQUIRED);
			break;
		case 999:	//3 seconds after mouse movement
			KillTimer(hWnd, 999);
			if (m_pTv->m_pAppData->values.bFullScreen)
				m_pTv->HideCursor();
			break;
		default:	//values from 1000 up can be used by the osd.*/
			g_pTv->Timer((int)wParam);
			/*break;
		}*/
		break;
	case WM_NCHITTEST:
	case WM_SETCURSOR:
		lResult = DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_DESTROY:
		/*if (g_pAppSettings != NULL)
		{
			m_pAppSettings->lastNetwork = m_pAppData->values.currTVNetwork;
			m_pAppSettings->lastProgram = m_pAppData->values.currTVProgram;

			if (m_pAppSettings->storeFullscreenState != 0)
			{
				//m_pAppData->appSettings.startFullscreen = (wPlace.showCmd == SW_MAXIMIZE) ? 1 : 0;
				m_pAppSettings->startFullscreen = m_pAppData->values.bFullScreen;
			}
			if (m_pAppSettings->storeAlwaysOnTopState != 0)
			{
				m_pAppSettings->startAlwaysOnTop = m_pAppData->values.bAlwaysOnTop;
			}
			if (m_pAppSettings->storeLastWindowPosition != 0)
			{
				WINDOWPLACEMENT wPlace;
				GetWindowPlacement(hWnd, &wPlace);
				wPlace.showCmd = SW_RESTORE;
				SetWindowPlacement(hWnd, &wPlace);

				RECT gwr;
				GetWindowRect(hWnd, &gwr);
				m_pAppSettings->lastWindowPositionX = gwr.left;
				m_pAppSettings->lastWindowPositionY = gwr.top;
				m_pAppSettings->lastWindowWidth  = gwr.right - gwr.left;
				m_pAppSettings->lastWindowHeight = gwr.bottom - gwr.top;
			}
			m_pAppSettings->SaveSettings();
		}*/
		/*if (m_pTv != NULL)
		{
			m_pTv->m_pDVBInput->StopRecording();
			m_pTv->m_pFilterGraph->Stop();
		}*/
		break;
	default:
		lResult = DefWindowProc(hWnd, message, wParam, lParam);
//		Sleep(10);
//		PaintNumber(hWnd, message);
		break;
	}

	return lResult;
}

