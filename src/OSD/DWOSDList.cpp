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
// DWOSDListEntry
//////////////////////////////////////////////////////////////////////

DWOSDListEntry::DWOSDListEntry()
{
}

DWOSDListEntry::~DWOSDListEntry()
{
}

//////////////////////////////////////////////////////////////////////
// DWOSDListItemList
//////////////////////////////////////////////////////////////////////

DWOSDListItemList::DWOSDListItemList()
{
	m_pSource = NULL;
	m_pText = NULL;
	m_pOnSelect = NULL;
	m_pOnLeft = NULL;
	m_pOnRight = NULL;
	m_pSelectedName = NULL;
	m_pMaskedName = NULL;
	m_pMaskText = NULL;
}

DWOSDListItemList::~DWOSDListItemList()
{
	if (m_pSource)
		delete[] m_pSource;
	if (m_pText)
		delete[] m_pText;
	if (m_pOnSelect)
		delete[] m_pOnSelect;
	if (m_pOnLeft)
		delete[] m_pOnLeft;
	if (m_pOnRight)
		delete[] m_pOnRight;
	if (m_pSelectedName)
		delete[] m_pSelectedName;
	if (m_pMaskedName)
		delete[] m_pMaskedName;
	if (m_pMaskText)
		delete[] m_pMaskText;
}

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

	m_bCanSelect = TRUE;
	m_bCanHighlight = TRUE;
	m_pSelectedName = NULL;
	m_pSelectedImage = NULL;
	m_bCanMask = TRUE;
	m_pMaskedName = NULL;
	m_pMaskedImage = NULL;
	m_wszMask = NULL;
}

DWOSDListItem::~DWOSDListItem()
{
	if (m_wszText)
		delete[] m_wszText;
	if (m_wszFont)
		delete[] m_wszFont;
	if (m_pSelectedName)
		delete[] m_pSelectedName;
	if (m_pMaskedName)
		delete[] m_pMaskedName;
	if (m_wszMask)
		delete[] m_wszMask;
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
		else if (_wcsicmp(element->name, L"masktext") == 0)
		{
			if (element->value)
				strCopy(m_wszMask, element->value);
		}
		else if (_wcsicmp(element->name, L"onSelect") == 0)
		{
			if (element->value)
				strCopy(m_pwcsOnSelect, element->value);
		}
		else if (_wcsicmp(element->name, L"onLeft") == 0)
		{
			if (element->value)
				strCopy(m_pwcsOnLeft, element->value);
		}
		else if (_wcsicmp(element->name, L"onRight") == 0)
		{
			if (element->value)
				strCopy(m_pwcsOnRight, element->value);
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
				else if (_wcsicmp(subelement->name, L"selectedImage") == 0)
				{
					if (subelement->value)
						m_pSelectedImage = g_pOSD->GetImage(subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"maskedImage") == 0)
				{
					if (subelement->value)
						m_pMaskedImage = g_pOSD->GetImage(subelement->value);
				}
			}
		}
		else if (_wcsicmp(element->name, L"selection") == 0 && element->value)
		{
			strCopy(m_pSelectedName, element->value);

			if (m_pSelectedName)
				m_bCanSelect = TRUE;
		}
		else if (_wcsicmp(element->name, L"maskname") == 0 && element->value)
		{
			strCopy(m_pMaskedName, element->value);

			if (m_pMaskedName)
				m_bCanMask = TRUE;
		}
	}

	return S_OK;
}

BOOL DWOSDListItem::Equals(DWOSDListItem* target)
{
	if (target->m_nWidth != m_nWidth)
		return FALSE;
	if (target->m_nHeight != m_nHeight)
		return FALSE;
	if (target->m_nGap != m_nGap)
		return FALSE;

	if (target->m_uAlignHorizontal != m_uAlignHorizontal)
		return FALSE;
	if (target->m_uAlignVertical != m_uAlignVertical)
		return FALSE;

	if ((m_wszText && (!target->m_wszText || _wcsicmp(target->m_wszText, m_wszText) != 0)) || (!m_wszText && target->m_wszText))
		return FALSE;
	if ((m_pwcsOnSelect && (!target->m_pwcsOnSelect || _wcsicmp(target->m_pwcsOnSelect, m_pwcsOnSelect) != 0)) || (!m_pwcsOnSelect && target->m_pwcsOnSelect))
		return FALSE;
	if ((m_wszFont && (!target->m_wszFont || _wcsicmp(target->m_wszFont, m_wszFont) != 0)) || (!m_wszFont && target->m_wszFont))
		return FALSE;
	if ((m_pwcsOnSelect && (!target->m_pwcsOnSelect || _wcsicmp(target->m_pwcsOnSelect, m_pwcsOnSelect) != 0)) || (!m_pwcsOnSelect && target->m_pwcsOnSelect))
		return FALSE;
	if ((m_pwcsOnUp && (!target->m_pwcsOnUp || _wcsicmp(target->m_pwcsOnUp, m_pwcsOnUp) != 0)) || (!m_pwcsOnUp && target->m_pwcsOnUp))
		return FALSE;
	if ((m_pwcsOnDown && (!target->m_pwcsOnDown || _wcsicmp(target->m_pwcsOnDown, m_pwcsOnDown) != 0)) || (!m_pwcsOnDown && target->m_pwcsOnDown))
		return FALSE;
	if ((m_pwcsOnLeft && (!target->m_pwcsOnLeft || _wcsicmp(target->m_pwcsOnLeft, m_pwcsOnLeft) != 0)) || (!m_pwcsOnLeft && target->m_pwcsOnLeft))
		return FALSE;
	if ((m_pwcsOnRight && (!target->m_pwcsOnRight || _wcsicmp(target->m_pwcsOnRight, m_pwcsOnRight) != 0)) || (!m_pwcsOnRight && target->m_pwcsOnRight))
		return FALSE;


	if (target->m_dwTextColor != m_dwTextColor)
		return FALSE;
	if (target->m_nTextHeight != m_nTextHeight)
		return FALSE;
	if (target->m_nTextWeight != m_nTextWeight)
		return FALSE;

	if (target->m_pBackgroundImage != m_pBackgroundImage)
		return FALSE;
	if (target->m_pHighlightImage != m_pHighlightImage)
		return FALSE;
	if (target->m_pSelectedImage != m_pSelectedImage)
		return FALSE;
	if ((m_pSelectedName && (!target->m_pSelectedName || _wcsicmp(target->m_pSelectedName, m_pSelectedName) != 0)) || (!m_pSelectedName && target->m_pSelectedName))
		return FALSE;
	if ((m_pMaskedName && (!target->m_pMaskedName || _wcsicmp(target->m_pMaskedName, m_pMaskedName) != 0)) || (!m_pMaskedName && target->m_pMaskedName))
		return FALSE;
	if ((m_wszMask && (!target->m_wszMask || _wcsicmp(target->m_wszMask, m_wszMask) != 0)) || (!m_wszMask && target->m_wszMask))
		return FALSE;
	if (target->m_pMaskedImage != m_pMaskedImage)
		return FALSE;

	return TRUE;
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
	if (m_pwcsOnSelect)
		strCopy(target->m_pwcsOnSelect, m_pwcsOnSelect);
	if (m_wszFont)
		strCopy(target->m_wszFont, m_wszFont);
	if (m_pwcsOnSelect)
		strCopy(target->m_pwcsOnSelect, m_pwcsOnSelect);
	if (m_pwcsOnUp)
		strCopy(target->m_pwcsOnUp, m_pwcsOnUp);
	if (m_pwcsOnDown)
		strCopy(target->m_pwcsOnDown, m_pwcsOnDown);
	if (m_pwcsOnLeft)
		strCopy(target->m_pwcsOnLeft, m_pwcsOnLeft);
	if (m_pwcsOnRight)
		strCopy(target->m_pwcsOnRight, m_pwcsOnRight);
	if (m_pSelectedName)
		strCopy(target->m_pSelectedName, m_pSelectedName);
	if (m_pMaskedName)
		strCopy(target->m_pMaskedName, m_pMaskedName);
	if (m_wszMask)
		strCopy(target->m_wszMask, m_wszMask);


	target->m_dwTextColor = m_dwTextColor;
	target->m_nTextHeight = m_nTextHeight;
	target->m_nTextWeight = m_nTextWeight;

	target->m_pBackgroundImage = m_pBackgroundImage;
	target->m_pHighlightImage = m_pHighlightImage;
	target->m_pSelectedImage = m_pSelectedImage;
	target->m_pMaskedImage = m_pMaskedImage;
}

HRESULT DWOSDListItem::Draw(long tickCount)
{
	USES_CONVERSION;

	HRESULT hr;

	LPWSTR pStr = NULL;
	if (m_pwcsOnSelect)
	{
		//Replace Tokens
		g_pOSD->Data()->ReplaceTokens(m_pwcsOnSelect, pStr);
		if (pStr)
		{
			strCopy(m_pwcsOnSelect, pStr);
			delete[] pStr;
			pStr = NULL;
		}
	}

	if (m_pSelectedName)
	{
		//Replace Tokens
		g_pOSD->Data()->ReplaceTokens(m_pSelectedName, pStr);
		if (pStr)
		{
			strCopy(m_pSelectedName, pStr);
			delete[] pStr;
			pStr = NULL;
		}
	}

	if (m_pMaskedName)
	{
		//Replace Tokens
		g_pOSD->Data()->ReplaceTokens(m_pMaskedName, pStr);
		if (pStr)
		{
			strCopy(m_pMaskedName, pStr);
			delete[] pStr;
			pStr = NULL;
		}
	}

	if (m_wszMask)
	{
		//Replace Tokens
		g_pOSD->Data()->ReplaceTokens(m_wszMask, pStr);
		if (pStr)
		{
			strCopy(m_wszMask, pStr);
			delete[] pStr;
			pStr = NULL;
		}
	}

//	LPWSTR pStr = NULL;
	//Replace Tokens
	g_pOSD->Data()->ReplaceTokens(m_wszText, pStr);
	if (pStr[0] == '\0')
	{
		delete[] pStr;
		return S_OK;
	}

	DWSurfaceText text;
	text.SetText(pStr);
	delete[] pStr;
	pStr = NULL;

	//Set Font
	ZeroMemory(&text.font, sizeof(LOGFONT));
	text.font.lfHeight = m_nTextHeight;
	text.font.lfWeight = m_nTextWeight;
	text.font.lfOutPrecision = OUT_OUTLINE_PRECIS; //OUT_DEVICE_PRECIS;
	text.font.lfQuality = ANTIALIASED_QUALITY;
	lstrcpy(text.font.lfFaceName, (m_wszFont) ? W2A(m_wszFont) : TEXT("Arial"));
	
	text.crTextColor = m_dwTextColor;
	
	if (m_bMasked)
	{
		if (m_pMaskedImage)
			m_pMaskedImage->Draw(m_pSurface, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
		else
		{
			//TODO: draw something since no image was supplied
		}
	}

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
		if (!m_bMasked && m_pBackgroundImage)
			m_pBackgroundImage->Draw(m_pSurface, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
		else
		{
			//TODO: draw something since no image was supplied
		}
	}

	if (m_bSelected)
	{
		if (m_pSelectedImage)
			m_pSelectedImage->Draw(m_pSurface, m_nPosX, m_nPosY, m_nWidth, m_nHeight);
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

	m_nHighlightedItem = 0;

	m_nYOffset = 0;
	m_nLastTickCount = 0;

	m_bMoving = FALSE;
	m_nMovingFinishesAtTickCount = 0;
	m_nMovingStartedAtYOffset = 0;
	m_nMovingToYOffset = 0;

	m_bCanSelect = FALSE;
	m_bCanHighlight = TRUE;
	m_pSelectedName = NULL;
	m_bCanMask = FALSE;
	m_pMaskedName = NULL;
}

DWOSDList::~DWOSDList()
{
	ClearItems();
	ClearItemsToRender();

	if (m_pSelectedName)
		delete[] m_pSelectedName;

	if (m_pMaskedName)
		delete[] m_pMaskedName;

	delete m_pListItemTemplate;
	delete m_pListSurface;
}

HRESULT DWOSDList::LoadFromXML(XMLElement *pElement)
{
	CAutoLock itemsLock(&m_itemsLock);

	DWOSDControl::LoadFromXML(pElement);

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

			if FAILED(m_pListSurface->Create(m_nWidth, m_nHeight))
				(log << "Failed to create surface for OSD List\n").Write();
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
				m_items.push_back(listItem);
			}
		}
		else if (_wcsicmp(element->name, L"itemList") == 0)
		{
			DWOSDListItemList* listItemList = new DWOSDListItemList();

			int subElementCount = element->Elements.Count();
			for ( int subitem=0 ; subitem<subElementCount ; subitem++ )
			{
				subelement = element->Elements.Item(subitem);
				if (_wcsicmp(subelement->name, L"source") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pSource, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"text") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pText, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"masktext") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pMaskText, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onSelect") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pOnSelect, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onLeft") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pOnLeft, subelement->value);
				}
				else if (_wcsicmp(subelement->name, L"onRight") == 0)
				{
					if (subelement->value)
						strCopy(listItemList->m_pOnRight, subelement->value);
				}
			}
			m_items.push_back(listItemList);
		}
		else if (_wcsicmp(element->name, L"selection") == 0 && element->value)
		{
			strCopy(m_pSelectedName, element->value);

			if (m_pSelectedName)
				m_bCanSelect = TRUE;
		}
		else if (_wcsicmp(element->name, L"maskname") == 0 && element->value)
		{
			strCopy(m_pMaskedName, element->value);

			if (m_pMaskedName)
				m_bCanMask = TRUE;
		}
	}

	return S_OK;
}

LPWSTR DWOSDList::OnUp()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	if (m_nHighlightedItem == 0)
		return DWOSDControl::OnUp();

	m_nHighlightedItem--;
	UpdateScrolling();
	return L"";
}

LPWSTR DWOSDList::OnDown()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	if (m_nHighlightedItem >= (long)m_itemsToRender.size()-1)
		return DWOSDControl::OnDown();

	m_nHighlightedItem++;
	UpdateScrolling();
	return L"";
}

LPWSTR DWOSDList::OnLeft()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	if (m_itemsToRender.size() <= 0)
		return NULL;
	DWOSDListItem* item = m_itemsToRender.at(m_nHighlightedItem);
	return item->OnLeft();
}

LPWSTR DWOSDList::OnRight()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	if (m_itemsToRender.size() <= 0)
		return NULL;
	DWOSDListItem* item = m_itemsToRender.at(m_nHighlightedItem);
	return item->OnRight();
}

LPWSTR DWOSDList::OnSelect()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	std::vector<DWOSDListItem *>::iterator it = m_itemsToRender.begin();
	for ( ; it < m_itemsToRender.end() ; it++ )
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
	CAutoLock itemsLock(&m_itemsLock);

	std::vector<DWOSDListEntry *>::iterator it = m_items.begin();
	for ( ; it < m_items.end() ; it++ )
	{
		delete (*it);
	}
	m_items.clear();
}

void DWOSDList::ClearItemsToRender()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	std::vector<DWOSDListItem *>::iterator it = m_itemsToRender.begin();
	for ( ; it < m_itemsToRender.end() ; it++ )
	{
		delete (*it);
	}
	m_itemsToRender.clear();
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

	{
		CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

		long itemID = 0;
		std::vector<DWOSDListItem *>::iterator it = m_itemsToRender.begin();
		for ( ; it < m_itemsToRender.end() ; it++ )
		{
			DWOSDListItem* item = *it;
			if ((nYOffset < m_nHeight) && (nYOffset > -item->m_nHeight))
			{
				item->m_nPosY = nYOffset;
				item->m_nWidth = m_nWidth;

				LPWSTR selection = g_pData->GetSelectionItem(m_pSelectedName);
				if (selection && wcsstr(selection, L".") && selection < wcsstr(selection, L"."))
				{
					LPWSTR wsztemp = NULL;
					strCopy(wsztemp, selection, wcsstr(selection, L".") - selection);
					if (item->m_wszText && wsztemp)
						item->SetSelect((wcsstr(item->m_wszText, wsztemp) != NULL) && 
										(wcslen(item->m_wszText) <= wcslen(wsztemp)+1));
					if(wsztemp)
						delete[] wsztemp;
				}
				else if (item->m_wszText && selection)
				{
					item->SetSelect(wcsstr(item->m_wszText, selection) != NULL);
				}

				if (m_pMaskedName)
				{
					LPWSTR pStr = NULL;
					//Replace Tokens
					g_pOSD->Data()->ReplaceTokens(m_pMaskedName, pStr);
					if (pStr)
					{

						if (pStr && item->m_wszMask)
							if (wcsstr(item->m_wszMask, pStr) != NULL)
									item->SetMask(FALSE);
								else
									item->SetMask(TRUE);

						delete[] pStr;
						pStr = NULL;
					}
				}

				item->SetHighlight((itemID == m_nHighlightedItem));
				item->Render(tickCount);
			}
			nYOffset += item->m_nHeight + item->m_nGap;

			itemID++;
		}
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
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);
	CAutoLock itemsLock(&m_itemsLock);

	BOOL bNeedsRefresh = FALSE;

	std::vector<DWOSDListItem *>::iterator  itItemsToRender = m_itemsToRender.begin();
	std::vector<DWOSDListEntry *>::iterator itItems = m_items.begin();
	for ( ; itItems < m_items.end() ; itItems++ )
	{
		if (itItemsToRender >= m_itemsToRender.end())
		{
			bNeedsRefresh = TRUE;
			break;
		}

		DWOSDListEntry* entry = *itItems;
		DWOSDListItem* listItem = *itItemsToRender;

		DWOSDListItem* item = dynamic_cast<DWOSDListItem*>(entry);
		if (item)
		{
			if (item->Equals(listItem) == FALSE)
			{
				bNeedsRefresh = TRUE;
				break;
			}
			itItemsToRender++;
		}

		DWOSDListItemList* itemList = dynamic_cast<DWOSDListItemList*>(entry);
		if (itemList && itemList->m_pSource)
		{
			LPWSTR pStr = NULL;
			g_pOSD->Data()->ReplaceTokens(itemList->m_pSource, pStr);
			IDWOSDDataList* list = g_pOSD->Data()->GetListFromListName(pStr);
			delete[] pStr;
			pStr = NULL;

			if (list)
			{
				long listSize = list->GetListSize();

				for (int i=0 ; i<listSize ; i++ )
				{
					listItem = *itItemsToRender;

					g_pOSD->Data()->ReplaceTokens(itemList->m_pText, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_wszText || _wcsicmp(listItem->m_wszText, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_wszText, m_pListItemTemplate->m_wszText) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pOnSelect, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_pwcsOnSelect || _wcsicmp(listItem->m_pwcsOnSelect, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_pwcsOnSelect, m_pListItemTemplate->m_pwcsOnSelect) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pOnLeft, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_pwcsOnLeft || _wcsicmp(listItem->m_pwcsOnLeft, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_pwcsOnLeft, m_pListItemTemplate->m_pwcsOnLeft) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pOnRight, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_pwcsOnRight || _wcsicmp(listItem->m_pwcsOnRight, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_pwcsOnRight, m_pListItemTemplate->m_pwcsOnRight) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pSelectedName, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_pSelectedName || _wcsicmp(listItem->m_pSelectedName, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_pSelectedName, m_pListItemTemplate->m_pSelectedName) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pMaskedName, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_pMaskedName || _wcsicmp(listItem->m_pMaskedName, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_pMaskedName, m_pListItemTemplate->m_pMaskedName) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					g_pOSD->Data()->ReplaceTokens(itemList->m_pMaskText, pStr, i);
					if (pStr[0] != '\0')
					{
						if (!listItem->m_wszMask || _wcsicmp(listItem->m_wszMask, pStr) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}
					else
					{
						if (strCmp(listItem->m_wszMask, m_pListItemTemplate->m_wszMask) != 0)
						{
							bNeedsRefresh = TRUE;
							break;
						}
					}

					delete[] pStr;
					pStr = NULL;

					itItemsToRender++;
				}
			}
		}
	}


	if (bNeedsRefresh)
	{
		ClearItemsToRender();

		std::vector<DWOSDListEntry *>::iterator it = m_items.begin();
		for ( ; it < m_items.end() ; it++ )
		{
			DWOSDListEntry* entry = *it;

			DWOSDListItem* item = dynamic_cast<DWOSDListItem*>(entry);
			if (item)
			{
				DWOSDListItem* listItem = new DWOSDListItem(m_pListSurface);
				listItem->SetLogCallback(m_pLogCallback);
				item->CopyTo(listItem);
				m_itemsToRender.push_back(listItem);
				continue;
			}

			DWOSDListItemList* itemList = dynamic_cast<DWOSDListItemList*>(entry);
			if (itemList && itemList->m_pSource)
			{
				LPWSTR pStr = NULL;
				g_pOSD->Data()->ReplaceTokens(itemList->m_pSource, pStr);
				IDWOSDDataList* list = g_pOSD->Data()->GetListFromListName(pStr);
				delete[] pStr;
				pStr = NULL;

				if (list)
				{
					long listSize = list->GetListSize();

					for (int i=0 ; i<listSize ; i++ )
					{
						DWOSDListItem* listItem = new DWOSDListItem(m_pListSurface);
						listItem->SetLogCallback(m_pLogCallback);

						m_pListItemTemplate->CopyTo(listItem);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pText, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_wszText, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pOnSelect, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_pwcsOnSelect, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pOnLeft, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_pwcsOnLeft, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pOnRight, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_pwcsOnRight, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pSelectedName, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_pSelectedName, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pMaskedName, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_pMaskedName, pStr);

						g_pOSD->Data()->ReplaceTokens(itemList->m_pMaskText, pStr, i);
						if (pStr[0] != '\0')
							strCopy(listItem->m_wszMask, pStr);

						delete[] pStr;
						pStr = NULL;

						m_itemsToRender.push_back(listItem);
					}
				}
			}
		}
		if (m_nHighlightedItem >= (long)m_itemsToRender.size())
			m_nHighlightedItem = m_itemsToRender.size()-1;
		UpdateScrolling();
	}
	return S_OK;
}

void DWOSDList::UpdateScrolling()
{
	CAutoLock itemsToRenderLock(&m_itemsToRenderLock);

	long nOffsetLast = 0;
	long nOffsetHighlighted = 0;
	long nOffsetNext = 0;

	long nOffsetLastCentre = 0;
	long nOffsetHighlightedCentre = 0;

	long nMinimumOffset = 0;
	long nMaximumOffset = 0;

	long nLastItemHeight = 0;

	long nItemID = 0;
	std::vector<DWOSDListItem *>::iterator it = m_itemsToRender.begin();
	for ( ; it < m_itemsToRender.end() ; it++ )
	{
		DWOSDListItem* item = *it;

		nOffsetLast = nOffsetHighlighted;
		nOffsetHighlighted = nOffsetNext;
		nOffsetNext += item->m_nHeight + item->m_nGap;

		nOffsetLastCentre = nOffsetHighlightedCentre;
		nOffsetHighlightedCentre += (item->m_nHeight / 2);

		if (nItemID == m_nHighlightedItem)
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
			if (it < m_itemsToRender.end())
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

		nItemID++;
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

