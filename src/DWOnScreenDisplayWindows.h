/**
 *	DWOnScreenDisplayWindows.h
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

#ifndef DWONSCREENDISPLAYWINDOWS_H
#define DWONSCREENDISPLAYWINDOWS_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include <vector>

class DWOnScreenDisplayWindows;
class DWOnScreenDisplayWindow : public LogMessageCaller
{
	friend DWOnScreenDisplayWindows;
public:
	DWOnScreenDisplayWindow();
	virtual ~DWOnScreenDisplayWindow();

	LPWSTR Name();

private:
	XMLElement *m_pElement;
};


class DWOnScreenDisplayWindows : public LogMessageCaller
{
public:
	DWOnScreenDisplayWindows();
	virtual ~DWOnScreenDisplayWindows();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Load(LPWSTR filename);

	DWOnScreenDisplayWindow *Item(LPWSTR pName);

private:
	std::vector<DWOnScreenDisplayWindow *> m_windows;

	LPWSTR m_filename;
};

#endif
