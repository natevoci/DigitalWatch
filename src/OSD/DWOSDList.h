/**
 *	DWOSDList.h
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

#ifndef DWOSDLIST_H
#define DWOSDLIST_H

#include "StdAfx.h"
#include "DWOSDControl.h"
#include "XMLDocument.h"
#include "DWOSDImage.h"
#include <vector>

class DWOSDList;
class DWOSDListItem : public DWOSDControl
{
	friend DWOSDList;
public:
	DWOSDListItem(DWSurface* pSurface);
	virtual ~DWOSDListItem();

	HRESULT LoadFromXML(XMLElement *pElement);

//	HRESULT Render(long tickCount, int x, int y, int width, int height);

	void CopyTo(DWOSDListItem* target);

protected:
	virtual HRESULT Draw(long tickCount);

	long m_nPosX;
	long m_nPosY;
	long m_nWidth;
	long m_nHeight;
	long m_nGap;

	unsigned int m_uAlignHorizontal;
	unsigned int m_uAlignVertical;

	LPWSTR m_wszText;
	LPWSTR m_wszFont;
	COLORREF m_dwTextColor;
	long m_nTextHeight;
	long m_nTextWeight;

	DWOSDImage* m_pBackgroundImage;
	DWOSDImage* m_pHighlightImage;

};

// DWOSDList
class DWOSDList : public DWOSDControl  
{
public:
	DWOSDList(DWSurface* pSurface);
	virtual ~DWOSDList();

	HRESULT LoadFromXML(XMLElement *pElement);

	virtual LPWSTR OnUp();
	virtual LPWSTR OnDown();
	virtual LPWSTR OnLeft();
	virtual LPWSTR OnRight();
	virtual LPWSTR OnSelect();

protected:
	virtual HRESULT Draw(long tickCount);

	virtual void UpdateScrolling();

	DWSurface* m_pListSurface;

	long m_nPosX;
	long m_nPosY;
	long m_nWidth;
	long m_nHeight;
	long m_nYOffset;
	long m_nLastTickCount;

	BOOL m_bMoving;
	long m_nMovingFinishesAtTickCount;
	long m_nMovingStartedAtYOffset;
	long m_nMovingToYOffset;

	std::vector <DWOSDListItem *> m_items;
};

#endif
