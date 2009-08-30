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

	g_pData->hWnd = FindWindowW(CA2W(szWindowClass), NULL);

	//LATER: Option for multiple instances
	if (g_pData->hWnd != NULL && !g_pData->settings.application.multiple)
	{
		(log << "An instance of Digital Watch is already running.\n  Bringing the existing instance to the foreground.\n").Write();
		SetForegroundWindow(g_pData->hWnd);
		return FALSE;
	}

//	if (g_pData->hWnd != NULL && g_pData->settings.application.multiple)
//		g_pData->values.application.multiple = g_pData->settings.application.multiple;

	HBRUSH br = CreateSolidBrush(0x00000000);

	WNDCLASSEXA wcex;
	wcex.cbSize = sizeof(WNDCLASSEXA);

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

	ATOM ar = RegisterClassExA(&wcex);

	USES_CONVERSION;
	g_pData->hWnd = CreateWindowA(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW/*WS_THICKFRAME*/, CW_USEDEFAULT, 0, 726, 435, NULL, NULL, NULL, NULL);
	if (!g_pData->hWnd)
	{
		__int64 err = GetLastError();
		//return_FALSE_SHOWMSG("Error creating window: " << err)
		return (log << "Error creating window: " << err << "\n").Show();
	}

	SetWindowLong(g_pData->hWnd, GWL_STYLE, GetWindowLong(g_pData->hWnd, GWL_STYLE) & (~(WS_CAPTION/* | WS_BORDER*/))); 

	if (g_pData->settings.window.startAtLastWindowPosition && g_pData->settings.loadedFromFile)
	{
		(log << "Restoring last window position. x=" << g_pData->values.window.position.x << " y=" << g_pData->values.window.position.y << " width=" <<g_pData->values.window.size.width << " height=" << g_pData->values.window.size.height << "\n").Write();
	}
	
	BOOL bFullscreen = g_pData->values.window.bFullScreen;
	g_pData->values.window.bFullScreen = FALSE;
	g_pTv->SetWindowPos(g_pData->values.window.position.x, g_pData->values.window.position.y, g_pData->values.window.size.width, g_pData->values.window.size.height, g_pData->settings.window.startAtLastWindowPosition && g_pData->settings.loadedFromFile, TRUE);
	g_pData->values.window.bFullScreen = bFullscreen;

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

	switch (message) 
	{
	case WM_COPYDATA:
		/* This is an example of how you call it from another application

			char command[256];
			sprintf((char*)&command, "SetChannel(%i)", m_serviceId);
			COPYDATASTRUCT cds;
			cds.dwData = 1;
			cds.lpData = (void *) &command;
			cds.cbData = strlen((char *) cds.lpData)+1; // include space for null char
			SendMessage(m_hWndDW, WM_COPYDATA, 0, (LPARAM)&cds);

		*/
		cds = (COPYDATASTRUCT*) lParam;
		switch (cds->dwData)
		{
		case 1:
			{
				USES_CONVERSION;
				g_pTv->ExecuteCommandsQueue(A2W((char*)cds->lpData));
				return 0;
			}
		}
		break;
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
		if (wParam == VK_SHIFT)
			break;
		if (wParam == VK_CONTROL)
			break;
		if (wParam == VK_MENU)
			break;

		{
			BOOL bShiftDown = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
			BOOL bCtrlDown = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
			BOOL bAltDown = ((GetKeyState(VK_MENU) & 0x80) != 0);

			g_pTv->OnKey(wParam, bShiftDown, bCtrlDown, bAltDown);
		}
		break;
	case WM_SIZING:
		g_pTv->OnSizing(wParam, (LPRECT)lParam);
		break;
	case WM_SIZE:
		lResult = DefWindowProc(hWnd, message, wParam, lParam);
//		g_pTv->OnSize();
		if (wParam == SIZE_MINIMIZED)
			g_pTv->OnMinimize();
		else if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
			g_pTv->OnRestore();

		g_pTv->OnSize();
		break;
	case WM_MOVE:
		lResult = DefWindowProc(hWnd, message, wParam, lParam);
		g_pTv->OnMove();
		break;

	case WM_LBUTTONDBLCLK:
		//if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
		{
			BOOL bShiftDown = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
			BOOL bCtrlDown = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
			BOOL bAltDown = ((GetKeyState(VK_MENU) & 0x80) != 0);
			g_pTv->OnKey(MOUSE_LDBLCLICK, bShiftDown, bCtrlDown, bAltDown);
		}
		break;
	case WM_RBUTTONDOWN:
		//if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
		{
			BOOL bShiftDown = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
			BOOL bCtrlDown = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
			BOOL bAltDown = ((GetKeyState(VK_MENU) & 0x80) != 0);
			g_pTv->OnKey(MOUSE_RCLICK, bShiftDown, bCtrlDown, bAltDown);
		}
		break;
	case WM_MBUTTONDOWN:
		//if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
		{
			BOOL bShiftDown = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
			BOOL bCtrlDown = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
			BOOL bAltDown = ((GetKeyState(VK_MENU) & 0x80) != 0);
			g_pTv->OnKey(MOUSE_MCLICK, bShiftDown, bCtrlDown, bAltDown);
		}
		break;
	case WM_MOUSEWHEEL:
		//if (m_pTv->ControlBarMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) == FALSE)
		{
			BOOL bShiftDown = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
			BOOL bCtrlDown = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
			BOOL bAltDown = ((GetKeyState(VK_MENU) & 0x80) != 0);
			wheelMovement = (short)HIWORD(wParam);
			if (wheelMovement > 0)
				g_pTv->OnKey(MOUSE_SCROLL_UP  , bShiftDown, bCtrlDown, bAltDown);
			if (wheelMovement < 0)
				g_pTv->OnKey(MOUSE_SCROLL_DOWN, bShiftDown, bCtrlDown, bAltDown);
		}
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
		g_pTv->ShowCursor();

		if (bLeftMouseDown && !g_pData->values.window.bFullScreen)
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
		}
		break;
	case WM_TIMER:
		g_pTv->OnTimer((int)wParam);
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

