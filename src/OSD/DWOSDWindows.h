/**
 *	DWOSDWindows.h
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

#ifndef DWOSDWINDOWS_H
#define DWOSDWINDOWS_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include "DWOSDImage.h"
#include "DWOSDControl.h"
#include "KeyMap.h"
#include <vector>

class DWOSDWindows;
class DWOSDWindow : public LogMessageCaller
{
	friend DWOSDWindows;
public:
	DWOSDWindow();
	virtual ~DWOSDWindow();

	LPWSTR Name();
	HRESULT Render(long tickCount);

	DWOSDControl *GetControl(LPWSTR pName);

	HRESULT GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function);

	HRESULT OnUp();
	HRESULT OnDown();
	HRESULT OnLeft();
	HRESULT OnRight();
	HRESULT OnSelect();

	BOOL HideWindowsBehindThisOne();

	void ClearParameters();
	void AddParameter(LPWSTR pwcsParameter);
	LPWSTR GetParameter(long nIndex);

private:
	HRESULT LoadFromXML(XMLElement *pElement);

	HRESULT OnKeyCommand(LPWSTR command);
	HRESULT SetHighlightedControl(LPWSTR wszNextControl);

	LPWSTR m_pName;
	std::vector<DWOSDControl *> m_controls;

	DWOSDControl *m_pHighlightedControl;
	BOOL m_bHideWindowsBehindThisOne;

	KeyMap m_keyMap;

	std::vector<LPWSTR> m_parameters;
};


class DWOSDWindows : public LogMessageCaller
{
public:
	DWOSDWindows();
	virtual ~DWOSDWindows();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Load(LPWSTR filename);
	
	void Show(LPWSTR pWindowName);

	DWOSDWindow *GetWindow(LPWSTR pName);
	DWOSDImage *GetImage(LPWSTR pName);

private:
	std::vector<DWOSDWindow *> m_windows;
	std::vector<DWOSDImage *> m_images;

	LPWSTR m_filename;
};

#endif
