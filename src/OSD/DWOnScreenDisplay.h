/**
 *	DWOnScreenDisplay.h
 *	Copyright (C) 2005 Nate
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

#ifndef DWOSD
#define DWOSD

#include "StdAfx.h"
#include "DWOSDWindows.h"
#include "DWOSDData.h"
#include "LogMessage.h"
#include "ParseLine.h"
#include "DWRenderer.h"
#include <vector>

//TODO remove after GetDirectDraw() is removed from DWOnScreenDisplay
#include "DirectDraw/DWDirectDraw.h"

enum RENDER_METHOD
{
	RENDER_METHOD_NONE,
	RENDER_METHOD_DEFAULT,
	RENDER_METHOD_OverlayMixer,
	RENDER_METHOD_VMR7,
	RENDER_METHOD_VMR9,
	RENDER_METHOD_VMR9Windowless,
	RENDER_METHOD_VMR9Renderless
};

class DWOnScreenDisplay : public LogMessageCaller
{
public:
	DWOnScreenDisplay();
	virtual ~DWOnScreenDisplay();

	void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise();

	RENDER_METHOD GetRenderMethod();
	int GetRenderMethodChangeCount();
	void SetRenderMethod(RENDER_METHOD renderMethod);

	HRESULT Render(long tickCount);

	HRESULT ShowMenu(LPWSTR szMenuName);
	HRESULT ExitMenu(long nNumberOfMenusToExit = 1);

	DWOSDWindow* Overlay();

	DWDirectDraw* GetDirectDraw();
	DWSurface* GetBackSurface();

	HRESULT GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function);
	HRESULT ExecuteCommand(ParseLine* command);


	//Stuff for controls to use
	DWOSDImage* GetImage(LPWSTR pName);
	DWOSDData* Data();

private:

	DWOSDData* m_pData;

	DWOSDWindows windows;

	DWRenderer* m_pRenderer;

	DWOSDWindow* m_pOverlayWindow;
	DWOSDWindow* m_pCurrentWindow;
	std::vector <DWOSDWindow *> m_windowStack;
	CCritSec m_windowStackLock;

	RENDER_METHOD m_renderMethod;
	int m_renderMethodChangeCount;
};

#endif

