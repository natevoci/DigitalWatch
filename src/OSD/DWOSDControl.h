/**
 *	DWOSDControl.h
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

#ifndef DWOSDCONTROL_H
#define DWOSDCONTROL_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include "DWSurface.h"

class DWOSDControl : public LogMessageCaller
{
public:
	DWOSDControl(DWSurface* pSurface);
	virtual ~DWOSDControl();

	virtual HRESULT LoadFromXML(XMLElement *pElement);

	LPWSTR Name();
	void SetName(LPWSTR pName);

	HRESULT Render(long tickCount);

	virtual void Show(long secondsToShowFor = -1);
	virtual void Hide();
	virtual void Toggle();

	virtual LPWSTR OnUp();
	virtual LPWSTR OnDown();
	virtual LPWSTR OnLeft();
	virtual LPWSTR OnRight();
	virtual LPWSTR OnSelect();

	void SetHighlight(BOOL bHighlighted);
	BOOL CanHighlight();
	BOOL IsHighlighted();

	void SetSelect(BOOL bSelected);
	BOOL CanSelect();
	BOOL IsSelected();

	void SetMask(BOOL bMasked);
	BOOL CanMask();
	BOOL IsMasked();

	DWSurface* m_pSurface;

protected:
	virtual HRESULT Draw(long tickCount) = 0;

	BOOL m_bVisible;
	long m_lTimeToHide;

	LPWSTR m_pName;

	BOOL m_bCanHighlight;
	BOOL m_bHighlighted;
	BOOL m_bCanSelect;
	BOOL m_bSelected;
	BOOL m_bCanMask;
	BOOL m_bMasked;
	LPWSTR m_pwcsOnUp;
	LPWSTR m_pwcsOnDown;
	LPWSTR m_pwcsOnLeft;
	LPWSTR m_pwcsOnRight;
	LPWSTR m_pwcsOnSelect;
};

#endif
