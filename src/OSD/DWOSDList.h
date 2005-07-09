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

// DWOSDListEntry
class DWOSDListEntry
{
public:
	DWOSDListEntry();
	virtual ~DWOSDListEntry();
};

// DWOSDListItemList
class DWOSDListItemList : public DWOSDListEntry
{
public:
	DWOSDListItemList();
	virtual ~DWOSDListItemList();

	LPWSTR m_pSource;
	LPWSTR m_pText;
	LPWSTR m_pOnSelect;
	LPWSTR m_pOnLeft;
	LPWSTR m_pOnRight;
};

// DWOSDListItem
class DWOSDListItem : public DWOSDListEntry, public DWOSDControl
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
	virtual void ClearItems();
	virtual void ClearItemsToRender();

	virtual HRESULT Draw(long tickCount);
	virtual HRESULT RefreshListItems();

	virtual void UpdateScrolling();

	DWSurface* m_pListSurface;
	DWOSDListItem* m_pListItemTemplate;

	long m_nPosX;
	long m_nPosY;
	long m_nWidth;
	long m_nHeight;

	long m_nHighlighedItem;



	long m_nYOffset;
	long m_nLastTickCount;

	BOOL m_bMoving;
	long m_nMovingFinishesAtTickCount;
	long m_nMovingStartedAtYOffset;
	long m_nMovingToYOffset;

	std::vector <DWOSDListEntry *> m_items;
	std::vector <DWOSDListItem *> m_itemsToRender;
};

#endif
