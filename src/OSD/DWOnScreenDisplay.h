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
#include "DirectDraw/DWDirectDraw.h"
#include "DWOSDWindows.h"
#include "DWOSDData.h"
#include "LogMessage.h"
#include "ParseLine.h"
#include <vector>

class DWOnScreenDisplay : public LogMessageCaller
{
public:
	DWOnScreenDisplay();
	virtual ~DWOnScreenDisplay();

	void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise();
	HRESULT Render(long tickCount);

	HRESULT ShowMenu(LPWSTR szMenuName);
	HRESULT ExitMenu(long nNumberOfMenusToExit = 1);

	DWDirectDraw* get_DirectDraw();
	DWOSDWindow* Overlay();

	DWSurface* get_BackSurface();

	HRESULT GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function);
	HRESULT ExecuteCommand(ParseLine* command);

	//Stuff for controls to use
	DWOSDImage* GetImage(LPWSTR pName);
	DWOSDData* Data();

private:
	DWOSDData* m_pData;

	DWOSDWindows windows;

	DWSurface* m_pBackSurface;

	DWDirectDraw* m_pDirectDraw;
	DWOSDWindow* m_pOverlayWindow;
	DWOSDWindow* m_pCurrentWindow;
	std::vector <DWOSDWindow *> m_windowStack;
};

#endif

