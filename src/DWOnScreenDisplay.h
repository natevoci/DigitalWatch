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
#include "DWDirectDraw.h"
//#include "DWDirectDrawImage.h"

#include "DWOSDWindows.h"

#include "DWOSDData.h"

#include "LogMessage.h"
#include <vector>

class DWOnScreenDisplay : public LogMessageCaller
{
public:
	DWOnScreenDisplay();
	virtual ~DWOnScreenDisplay();

	void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise();

	HRESULT Render(long tickCount);

	DWDirectDraw* get_DirectDraw();

	DWOSDWindows windows;
	DWOSDData data;

private:
	long GetPanningPos(long tickCount, long span, double speed);

	DWDirectDraw* m_pDirectDraw;
};

#endif

