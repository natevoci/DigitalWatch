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
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSDControl
//////////////////////////////////////////////////////////////////////

DWOSDControl::DWOSDControl(DWSurface* pSurface)
{
	m_pSurface = pSurface;

	m_bVisible = TRUE;
	m_lTimeToHide = 0;

	m_pName = new wchar_t[1];;
	m_pName[0] = 0;

	m_bCanHighlight = FALSE;
	m_bHighlighted = FALSE;
	m_bCanSelect = FALSE;
	m_bSelected = FALSE;
	m_bCanMask = FALSE;
	m_bMasked = FALSE;
	m_pwcsOnUp = NULL;
	m_pwcsOnDown = NULL;
	m_pwcsOnLeft = NULL;
	m_pwcsOnRight = NULL;
	m_pwcsOnSelect = NULL;
}

DWOSDControl::~DWOSDControl()
{
	if (m_pName)
		delete[] m_pName;
	if (m_pwcsOnUp)
		delete[] m_pwcsOnUp;
	if (m_pwcsOnDown)
		delete[] m_pwcsOnDown;
	if (m_pwcsOnLeft)
		delete[] m_pwcsOnLeft;
	if (m_pwcsOnRight)
		delete[] m_pwcsOnRight;
	if (m_pwcsOnSelect)
		delete[] m_pwcsOnSelect;
}

HRESULT DWOSDControl::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;
	attr = pElement->Attributes.Item(L"name");
	if (attr)
		strCopy(m_pName, attr->value);
	return S_OK;
}

LPWSTR DWOSDControl::Name()
{
	return m_pName;
}

void DWOSDControl::SetName(LPWSTR pName)
{
	strCopy(m_pName, pName);
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

LPWSTR DWOSDControl::OnUp()
{
	return m_pwcsOnUp;
}

LPWSTR DWOSDControl::OnDown()
{
	return m_pwcsOnDown;
}

LPWSTR DWOSDControl::OnLeft()
{
	return m_pwcsOnLeft;
}

LPWSTR DWOSDControl::OnRight()
{
	return m_pwcsOnRight;
}

LPWSTR DWOSDControl::OnSelect()
{
	return m_pwcsOnSelect;
}

void DWOSDControl::SetHighlight(BOOL bHighlighted)
{
	m_bHighlighted = bHighlighted;
}

BOOL DWOSDControl::CanHighlight()
{
	return m_bCanHighlight;
}

BOOL DWOSDControl::IsHighlighted()
{
	return m_bHighlighted;
}

void DWOSDControl::SetSelect(BOOL bSelected)
{
	m_bSelected = bSelected;
}

BOOL DWOSDControl::CanSelect()
{
	return m_bCanSelect;
}

BOOL DWOSDControl::IsSelected()
{
	return m_bSelected;
}

void DWOSDControl::SetMask(BOOL bMasked)
{
	m_bMasked = bMasked;
}

BOOL DWOSDControl::CanMask()
{
	return m_bCanMask;
}

BOOL DWOSDControl::IsMasked()
{
	return m_bMasked;
}
