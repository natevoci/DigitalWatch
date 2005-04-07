/**
 *	KeyMap.h
 *	Copyright (C) 2003-2004 Nate
 *	Copyright (C) 2004 Builty
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

#ifndef KEYMAP_H
#define KEYMAP_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include <vector>

typedef enum MouseFunction
{
	MOUSE_LDBLCLICK		= -2,
	MOUSE_RCLICK		= -3,
	MOUSE_MCLICK		= -4,
	MOUSE_SCROLL_UP		= -5,
	MOUSE_SCROLL_DOWN	= -6
};

struct KeyMapEntry
{
	long Keycode;
	BOOL Shift;
	BOOL Ctrl;
	BOOL Alt;
	LPWSTR Function;
};

class KeyMap : public LogMessageCaller
{
public:
	KeyMap();
	~KeyMap();

	HRESULT GetFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function);

	HRESULT LoadFromFile(LPWSTR filename);
	HRESULT LoadFromXML(XMLElementList* elementList);

private:
	std::vector<KeyMapEntry> keyMaps;
};

#endif
