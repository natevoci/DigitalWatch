/**
 *	DWOSDLabel.cpp
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

#include "DWOSDLabel.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSDLabel
//////////////////////////////////////////////////////////////////////

DWOSDLabel::DWOSDLabel(DWSurface* pSurface) : DWOSDControl(pSurface)
{
	m_nPosX = 0;
	m_nPosY = 0;
	m_wszText = NULL;
	m_wszFont = NULL;
	m_dwTextColor = 0;
	m_nHeight = 40;
	m_nWeight = 400;
	m_uAlignHorizontal = TA_LEFT;
	m_uAlignVertical = TA_TOP;

	m_pBackgroundImage = NULL;
	SetRect(&m_rectBackgroundPadding, 0, 0, 0, 0);
}

DWOSDLabel::~DWOSDLabel()
{
	if (m_wszText)
		delete[] m_wszText;

}

HRESULT DWOSDLabel::LoadFromXML(XMLElement *pElement)
{
	DWOSDControl::LoadFromXML(pElement);

	XMLAttribute *attr;
	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

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
		else if (_wcsicmp(element->name, L"text") == 0)
		{
			if (element->value)
				strCopy(m_wszText, element->value);
		}
		else if (_wcsicmp(element->name, L"font") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if (attr)
				strCopy(m_wszFont, attr->value);

			attr = element->Attributes.Item(L"height");
			if (attr)
				m_nHeight = _wtoi(attr->value);

			attr = element->Attributes.Item(L"weight");
			if (attr)
				m_nWeight = _wtoi(attr->value);

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
				else if (_wcsicmp(subelement->name, L"padding") == 0)
				{
					attr = subelement->Attributes.Item(L"left");
					if (attr)
						m_rectBackgroundPadding.left = _wtoi(attr->value);

					attr = subelement->Attributes.Item(L"top");
					if (attr)
						m_rectBackgroundPadding.top = _wtoi(attr->value);

					attr = subelement->Attributes.Item(L"right");
					if (attr)
						m_rectBackgroundPadding.right = _wtoi(attr->value);

					attr = subelement->Attributes.Item(L"bottom");
					if (attr)
						m_rectBackgroundPadding.bottom = _wtoi(attr->value);
				}
			}
		}
	}

	return S_OK;
}

HRESULT DWOSDLabel::Draw(long tickCount)
{
	USES_CONVERSION;

	HRESULT hr;

	LPWSTR pStr = NULL;
	//Replace Tokens
	g_pOSD->Data()->ReplaceTokens(m_wszText, pStr);

	if (pStr[0] == '\0')
		return S_OK;

	DWSurfaceText text;
	strCopy(text.text, pStr);

	//Set Font
	ZeroMemory(&text.font, sizeof(LOGFONT));
	text.font.lfHeight = m_nHeight;
	text.font.lfWeight = m_nWeight;
	text.font.lfOutPrecision = OUT_OUTLINE_PRECIS; //OUT_DEVICE_PRECIS;
	text.font.lfQuality = ANTIALIASED_QUALITY;
	lstrcpy(text.font.lfFaceName, (m_wszFont) ? W2A(m_wszFont) : TEXT("Arial"));

	text.crTextColor = m_dwTextColor;

	long nPosX = m_nPosX;
	long nPosY = m_nPosY;

	if (m_pBackgroundImage || (m_uAlignHorizontal != TA_LEFT) || (m_uAlignVertical != TA_TOP))
	{
		SIZE extent;
		hr = text.GetTextExtent(&extent);

		if (m_uAlignHorizontal == TA_CENTER)
			nPosX -= (extent.cx / 2);
		else if (m_uAlignHorizontal == TA_RIGHT)
			nPosX -= (extent.cx);
		
		if (m_uAlignVertical == TA_CENTER)
			nPosY -= (extent.cy / 2);
		else if (m_uAlignVertical == TA_BOTTOM)
			nPosY -= (extent.cy);

		if (m_pBackgroundImage)
		{
			m_pBackgroundImage->Draw(m_pSurface,
									 nPosX - m_rectBackgroundPadding.left,
									 nPosY - m_rectBackgroundPadding.top,
									 extent.cx + m_rectBackgroundPadding.left + m_rectBackgroundPadding.right,
									 extent.cy + m_rectBackgroundPadding.top + m_rectBackgroundPadding.bottom);
		}
	}

	m_pSurface->DrawText(&text, nPosX, nPosY);

	return S_OK;
}

