/**
 *	DWOSDControl.cpp
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

#include "DWOSDControl.h"
#include "Globals.h"

//////////////////////////////////////////////////////////////////////
// DWOSDControl
//////////////////////////////////////////////////////////////////////

DWOSDControl::DWOSDControl()
{
	m_piSurface = g_pOSD->get_DirectDraw()->get_BackSurface();

	m_bVisible = TRUE;
	m_lTimeToHide = 0;

	m_pName = new wchar_t[1];;
	m_pName[0] = 0;
}

DWOSDControl::~DWOSDControl()
{
	if (m_pName)
		delete[] m_pName;
}

LPWSTR DWOSDControl::Name()
{
	return m_pName;
}

HRESULT DWOSDControl::Render(long tickCount)
{
	if ((m_lTimeToHide != 0) && (m_lTimeToHide < tickCount))
	{
		m_bVisible = FALSE;
		m_lTimeToHide = 0;
	}

	if (m_bVisible)
		return Draw(tickCount);
	return S_FALSE;
}

void DWOSDControl::Show(long secondsToShowFor)
{
	if ((secondsToShowFor > 0) && (!m_bVisible || (m_lTimeToHide > 0)))
		m_lTimeToHide = GetTickCount() + (secondsToShowFor * 1000);
	m_bVisible = TRUE;
}

void DWOSDControl::Hide()
{
	m_lTimeToHide = 0;
	m_bVisible = FALSE;
}

void DWOSDControl::Toggle()
{
	m_lTimeToHide = 0;
	m_bVisible = !m_bVisible;
}


