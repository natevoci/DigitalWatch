/**
 *	DWOSDList.cpp
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


#include "DWOSDList.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSDListItem
//////////////////////////////////////////////////////////////////////

DWOSDListItem::DWOSDListItem(DWSurface* pSurface) : DWOSDControl(pSurface)
{
	m_nPosX = 0;
	m_nPosY = 0;
	m_nWidth = 100;
	m_nHeight = 60;
	m_nGap = 20;

	m_uAlignHorizontal = TA_CENTER;
	m_uAlignVertical = TA_CENTER;
	
	m_wszText = NULL;
	m_wszFont = NULL;
	m_dwTextColor = 0;
	m_nTextHeight = 40;
	m_nTextWeight = 400;

	m_bCanHighlight = TRUE;
}

DWOSDListItem::~DWOSDListItem()
{
	if (m_wszText)
		delete[] m_wszText;
}

HRESULT DWOSDListItem::LoadFromXML(XMLElement *pElement)
{
	DWOSDControl::LoadFromXML(pElement);

	XMLAttribute *attr;
	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = pElement->Elements.Item(item);
		if (_wcsicmp(element->name, L"text") == 0)
		{
			if (element->value)
				strCopy(m_wszText, element->value);
		}
		else if (_wcsicmp(element->name, L"onSelect") == 0)
		{
			if (element->value)
				strCopy(m_pwcsOnSelect, element->value);
		}
		else if (_wcsicmp(element->name, L"height") == 0)
		{
			if (element->value)
				m_nHeight = _wtoi(element->value);
		}
		else if (_wcsicmp(element->name, L"gap") == 0)
		{
			if (element->value)
				m_nGap = _wtoi(element->value);
		}
		else if (_wcsicmp(element->name, L"font") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if (attr)
				strCopy(m_wszFont, attr->value);

			attr = element->Attributes.Item(L"height");
			if (attr)
				m_nTextHeight = _wtoi(attr->value);

			attr = element->Attributes.Item(L"weight");
			if (attr)
				m_nTextWeight = _wtoi(attr->value);

			attr = element->Attributes.Item(L"color");
			if (attr)
				m_dwTextColor = wcsToColor(attr->value);
		}
		else if (_wcsicmp(element->name, L"align") == 0)
		{
			attr = element->Attributes.Item(L"horizontal");
			if (attr)
			{
				if (_wcsicmp(attr->value, L"left") == 0)
					m_uAlignHorizontal = TA_LEFT;
				else if (_wcsicmp(attr->value, L"center") == 0)
					m_uAlignHorizontal = TA_CENTER;
				else if (_wcsicmp(attr->value, L"centre") == 0)
					m_uAlignHorizontal = TA_CENTER;
				else if (_wcsicmp(attr->value, L"right") == 0)
					m_uAlignHorizontal = TA_RIGHT;
			}
			attr = element->Attributes.Item(L"vertical");
			if (attr)
			{
				if (_wcsicmp(attr->value, L"top") == 0)
					m_uAlignVertical = TA_TOP;
				else if (_wcsicmp(attr->value, L"center") == 0)
					m_uAlignVertical = TA_CENTER;
				else if (_wcsicmp(attr->value, L"centre") == 0)
					m_uAlignVertical = TA_CENTER;
				else if (_wcsicmp(attr->value, L"bottom") == 0)
					m_uAlignVertical = TA_BOTTOM;
			}
		}
		else if (_wcsicmp(element->name, L"background") == 0)
		{
			int subElementCount = element->Elements.Count();
			for ( int subitem=0 ; subitem<subElementCount ; subitem++ )
			{
				subelement = element->Elements.Item(subitem);
				if (_wcsicmp(subelement->name, L"image") == 0)
				{
					if (subelement->value)
						m_pBackgroundImage = g_pOSD->GetImage(subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"highlightImage") == 0)
				{
					if (subelement->value)
						m_pHighlightImage = g_pOSD->GetImage(subelement->value);
				}
			}
		}
	}

	return S_OK;
}

void DWOSDListItem::CopyTo(DWOSDListItem* target)
{
	target->m_nWidth = m_nWidth;
	target->m_nHeight = m_nHeight;
	target->m_nGap = m_nGap;

	target->m_uAlignHorizontal = m_uAlignHorizontal;
	target->m_uAlignVertical = m_uAlignVertical;

	if (m_wszText)
		strCopy(target->m_wszText, m_wszText);
	if (m_wszFont)
		strCopy(target->m_wszFont, m_wszFont);
	target->m_dwTextColor = m_dwTextColor;
	target->m_nTextHeight = m_nTextHeight;
	target->m_nTextWeight = m_nTextWeight;

	target->m_pBackgroundImage = m_pBackgroundImage;
	target->m_pHighlightImage = m_pHighlightImage;
}

HRESULT DWOSDListItem::Draw(long tickCount)
{
	USES_CONVERSION;

	HRESULT hr;

	LPWSTR pStr = NULL;
	//Replace Tokens
	g_pOSD->Data()->ReplaceTokens(m_wszText, pStr);

	DWSurfaceText text;
	strCopy(text.text, pStr);

	//Set Font
	ZeroMemory(&text.font, sizeof(LOGFONT));
	text.font.lfHeight = m_nTextHeight;
	text.font.lfWeight = m_nTextWeight;
	text.font.lfOutPrecision = OUT_OUTLINE_PRECIS; //OUT_DEVICE_PRECIS;
	text.font.lfQuality = ANTIALIASED_QUALITY;
	lstrcpy(text.font.lfFaceName, (m_wszFont) ? W2A(m_wszFont) : TEXT("Arial"));
	
	text.crTextColor = m_dwTextColor;
	
	if (m_bHighlighted)
	{
		if (m_pHighlightImage)
			m_pHighlightImage->Draw(m_pSurface, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
		else
		{
			//TODO: draw something since no image was supplied
		}
	}
	else
	{
		if (m_pBackgroundImage)
			m_pBackgroundImage->Draw(m_pSurface, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
		else
		{
			//TODO: draw something since no image was supplied
		}
	}

	long nPosX = m_nPosX;
	long nPosY = m_nPosY;

	if ((m_uAlignHorizontal != TA_LEFT) || (m_uAlignVertical != TA_TOP))
	{
		SIZE extent;
		hr = text.GetTextExtent(&extent);

		if (m_uAlignHorizontal == TA_CENTER)
			nPosX += (m_nWidth / 2) - (extent.cx / 2);
		else if (m_uAlignHorizontal == TA_RIGHT)
			nPosX += m_nWidth - extent.cx;
		
		if (m_uAlignVertical == TA_CENTER)
			nPosY += (m_nHeight / 2) - (extent.cy / 2);
		else if (m_uAlignVertical == TA_BOTTOM)
			nPosY += m_nHeight - extent.cy;
	}

	m_pSurface->DrawText(&text, nPosX, nPosY);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWOSDList
//////////////////////////////////////////////////////////////////////

DWOSDList::DWOSDList(DWSurface* pSurface) : DWOSDControl(pSurface)
{
	m_pListSurface = new DWSurface();
	m_pListItemTemplate = new DWOSDListItem(m_pListSurface);

	m_nPosX = 0;
	m_nPosY = 0;
	m_nWidth = 0;
	m_nHeight = 0;

	m_nHighlighedItem = 0;

	m_pItemsSource = NULL;
	m_pItemsText = NULL;
	m_pItemsOnSelect = NULL;
	m_pItemsOnLeft = NULL;
	m_pItemsOnRight = NULL;

	m_nYOffset = 0;
	m_nLastTickCount = 0;

	m_bMoving = FALSE;
	m_nMovingFinishesAtTickCount = 0;
	m_nMovingStartedAtYOffset = 0;
	m_nMovingToYOffset = 0;

	m_bCanHighlight = TRUE;
}

DWOSDList::~DWOSDList()
{
	if (m_pItemsSource)
		delete[] m_pItemsSource;
	if (m_pItemsText)
		delete[] m_pItemsText;
	if (m_pItemsOnSelect)
		delete[] m_pItemsOnSelect;

	ClearItems();
}



HRESULT DWOSDList::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;
	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

	ClearItems();


	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = pElement->Elements.Item(item);
		if (_wcsicmp(element->name, L"pos") == 0)
		{
			attr = element->Attributes.Item(L"x");
			if (attr)
				m_nPosX = _wtoi(attr->value);

			attr = element->Attributes.Item(L"y");
			if (attr)
				m_nPosY = _wtoi(attr->value);
		}
		else if (_wcsicmp(element->name, L"size") == 0)
		{
			attr = element->Attributes.Item(L"width");
			if (attr)
				m_nWidth = _wtoi(attr->value);

			attr = element->Attributes.Item(L"height");
			if (attr)
				m_nHeight = _wtoi(attr->value);

			m_pListSurface->Create(m_nWidth, m_nHeight);
			m_pListSurface->SetColorKey(RGB(0, 0, 0));
		}
		else if (_wcsicmp(element->name, L"itemTemplate") == 0)
		{
			if FAILED(m_pListItemTemplate->LoadFromXML(element))
			{
				(log << "Loading item template failed. Resetting to defaults\n").Write();
				delete m_pListItemTemplate;
				m_pListItemTemplate = new DWOSDListItem(m_pListSurface);
			}
		}
		else if (_wcsicmp(element->name, L"item") == 0)
		{
			DWOSDListItem* listItem = new DWOSDListItem(m_pListSurface);
			listItem->SetLogCallback(m_pLogCallback);
			m_pListItemTemplate->CopyTo(listItem);

			if FAILED(listItem->LoadFromXML(element))
			{
				delete listItem;
			}
			else
			{
				if (m_items.size() == m_nHighlighedItem)
					listItem->SetHighlight(TRUE);
				m_items.push_back(listItem);
			}
		}
		else if (_wcsicmp(element->name, L"items") == 0)
		{
			int subElementCount = element->Elements.Count();
			for ( int subitem=0 ; subitem<subElementCount ; subitem++ )
			{
				subelement = element->Elements.Item(subitem);
				if (_wcsicmp(subelement->name, L"source") == 0)
				{
					if (subelement->value)
						strCopy(m_pItemsSource, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"text") == 0)
				{
					if (subelement->value)
						strCopy(m_pItemsText, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onSelect") == 0)
				{
					if (subelement->value)
						strCopy(m_pItemsOnSelect, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onLeft") == 0)
				{
					if (subelement->value)
						strCopy(m_pItemsOnLeft, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onRight") == 0)
				{
					if (subelement->value)
						strCopy(m_pItemsOnRight, subelement->value);
				}
			}
		}
	}

	return S_OK;
}

LPWSTR DWOSDList::OnUp()
{
	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;
		if (item->IsHighlighted())
		{
			it--;
			if (it < m_items.begin())
				return DWOSDControl::OnUp();

			DWOSDListItem* lastItem = *it;

			item->SetHighlight(FALSE);
			lastItem->SetHighlight(TRUE);
			m_nHighlighedItem--;

			UpdateScrolling();
			return L"";
		}
	}

	return NULL;
}

LPWSTR DWOSDList::OnDown()
{
	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;
		if (item->IsHighlighted())
		{
			it++;
			if (it >= m_items.end())
				return DWOSDControl::OnDown();

			DWOSDListItem* nextItem = *it;

			item->SetHighlight(FALSE);
			nextItem->SetHighlight(TRUE);
			m_nHighlighedItem++;

			UpdateScrolling();
			return L"";
		}
	}

	return NULL;
}

LPWSTR DWOSDList::OnLeft()
{
	if (m_items.size() <= 0)
		return NULL;
	DWOSDListItem* item = m_items.at(m_nHighlighedItem);
	return item->OnLeft();
}

LPWSTR DWOSDList::OnRight()
{
	if (m_items.size() <= 0)
		return NULL;
	DWOSDListItem* item = m_items.at(m_nHighlighedItem);
	return item->OnRight();
}

LPWSTR DWOSDList::OnSelect()
{
	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;
		if (item->IsHighlighted())
		{
			return item->OnSelect();
		}
	}

	return NULL;
}

void DWOSDList::ClearItems()
{
	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;
		delete item;
	}
	m_items.clear();
}

HRESULT DWOSDList::Draw(long tickCount)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;

	RefreshListItems();

	long nLastTickCount = m_nLastTickCount;
	m_nLastTickCount = tickCount;

	static double overflow = 0;

	if (m_nMovingFinishesAtTickCount == 0)
	{
		m_nMovingFinishesAtTickCount = tickCount + 200;
	}
	if (m_nMovingToYOffset != m_nYOffset)
	{
		double position = 1;
		if ((m_nMovingFinishesAtTickCount - nLastTickCount) > 0)
			position = (tickCount - nLastTickCount) / (double)(m_nMovingFinishesAtTickCount - nLastTickCount);
		if (position > 1)
			position = 1;
		double dNewYOffset = (m_nMovingToYOffset - m_nYOffset) * position + overflow;
		overflow = dNewYOffset - (long)dNewYOffset;
		m_nYOffset += (long)dNewYOffset;
	}

	long nYOffset = -1 * m_nYOffset;

	m_pListSurface->Clear();

	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;
		item->m_nPosY = nYOffset;
		item->m_nWidth = m_nWidth;
		item->Render(tickCount);
		nYOffset += item->m_nHeight + item->m_nGap;
	}
	
	RECT rcDest, rcSrc;
	SetRect(&rcDest, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
	SetRect(&rcSrc, 0, 0, m_nWidth, m_nHeight);

	hr = m_pListSurface->Blt(m_pSurface, &rcDest, NULL);
	if FAILED(hr)
		return hr;

	return S_OK;
}

HRESULT DWOSDList::RefreshListItems()
{
	if (m_pItemsSource)
	{
		LPWSTR pStr = NULL;
		g_pOSD->Data()->ReplaceTokens(m_pItemsSource, pStr);
		IDWOSDDataList* list = g_pOSD->Data()->GetList(pStr);
		if (list)
		{
			ClearItems();

			long listSize = list->GetListSize();

			for (int i=0 ; i<listSize ; i++ )
			{
				DWOSDListItem* listItem = new DWOSDListItem(m_pListSurface);
				listItem->SetLogCallback(m_pLogCallback);

				m_pListItemTemplate->CopyTo(listItem);

				g_pOSD->Data()->ReplaceTokens(m_pItemsText, pStr, list, i);
				strCopy(listItem->m_wszText, pStr);

				g_pOSD->Data()->ReplaceTokens(m_pItemsOnSelect, pStr, list, i);
				strCopy(listItem->m_pwcsOnSelect, pStr);

				g_pOSD->Data()->ReplaceTokens(m_pItemsOnLeft, pStr, list, i);
				strCopy(listItem->m_pwcsOnLeft, pStr);

				g_pOSD->Data()->ReplaceTokens(m_pItemsOnRight, pStr, list, i);
				strCopy(listItem->m_pwcsOnRight, pStr);

				if (m_items.size() == m_nHighlighedItem)
					listItem->SetHighlight(TRUE);
				m_items.push_back(listItem);
			}
			if (m_nHighlighedItem >= listSize)
				m_nHighlighedItem = listSize-1;
		}
	}
	return S_OK;
}

void DWOSDList::UpdateScrolling()
{
	long nOffsetLast = 0;
	long nOffsetHighlighted = 0;
	long nOffsetNext = 0;

	long nOffsetLastCentre = 0;
	long nOffsetHighlightedCentre = 0;

	long nMinimumOffset = 0;
	long nMaximumOffset = 0;

	long nLastItemHeight = 0;

	std::vector<DWOSDListItem *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		DWOSDListItem* item = *it;

		nOffsetLast = nOffsetHighlighted;
		nOffsetHighlighted = nOffsetNext;
		nOffsetNext += item->m_nHeight + item->m_nGap;

		nOffsetLastCentre = nOffsetHighlightedCentre;
		nOffsetHighlightedCentre += (item->m_nHeight / 2);

		if (item->IsHighlighted())
		{
			if ((nOffsetNext - nOffsetLast) < m_nHeight)
			{
				nMinimumOffset = nOffsetLast;
				nMaximumOffset = nOffsetNext - m_nHeight;
			}
			else
			{
				nMinimumOffset = nOffsetHighlightedCentre - (m_nHeight / 2);
				nMaximumOffset = nOffsetNext - m_nHeight;
			}

			it++;
			if (it < m_items.end())
			{
				item = *it;
				nMaximumOffset += item->m_nHeight;
			}
			else
			{
				nMaximumOffset -= item->m_nGap;
			}
			break;
		}
		nOffsetHighlightedCentre += (item->m_nHeight / 2) + item->m_nGap;
	}

	if (m_nMovingToYOffset < nMaximumOffset)
	{
		m_nMovingToYOffset = nMaximumOffset;
		m_nMovingFinishesAtTickCount = 0;
	}
	if (m_nMovingToYOffset > nMinimumOffset)
	{
		m_nMovingToYOffset = nMinimumOffset;
		m_nMovingFinishesAtTickCount = 0;
	}
}

