/**
 *	DWSurfaceText.cpp
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

#include "DWSurfaceText.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWSurfaceText::DWSurfaceText()
{
	crTextColor = 0xFFFFFFFF;
	crTextBkColor = 0x00000000;
	bTextBkTransparent = TRUE;

	ZeroMemory(&font, sizeof(LOGFONT));
	font.lfHeight = 14;
	font.lfWeight = 400;
	font.lfOutPrecision = OUT_OUTLINE_PRECIS;
	font.lfQuality = ANTIALIASED_QUALITY;
	lstrcpy(font.lfFaceName, TEXT("Arial"));

	text = NULL;

	m_hFont = NULL;
	m_hOldFont = NULL;
}

DWSurfaceText::~DWSurfaceText()
{
	if (text)
		delete[] text;

	if (m_hFont)
		DeleteObject(m_hFont);

}

HRESULT DWSurfaceText::GetTextExtent(SIZE *extent)
{
	USES_CONVERSION;

	HDC hDC;
	if (text == NULL)
	{
		return (log << "DWSurfaceText::GetTextExtent: No text defined\n").Write(E_FAIL);
	}

	hDC = CreateCompatibleDC(NULL);
	InitDC(hDC);
	::GetTextExtentPoint32(hDC, W2T(text), wcslen(text), extent);
	UninitDC(hDC);
	DeleteDC(hDC);

	return S_OK;
}

HRESULT DWSurfaceText::InitDC(HDC &hDC)
{
	USES_CONVERSION;

	m_hFont = CreateFontIndirect(&font);
	if (m_hFont == NULL)
	{
		return (log << "Could not load font: " << font.lfFaceName << "\n").Write(E_FAIL);
	}

	m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);
	::SetTextColor(hDC, crTextColor & 0x00FFFFFF);
	if (bTextBkTransparent)
	{
		SetBkMode(hDC, TRANSPARENT);
	}
	else
	{
		::SetBkColor(hDC, crTextBkColor & 0x00FFFFFF);
	}

	SetTextAlign(hDC, TA_LEFT);

	return S_OK;
}

HRESULT DWSurfaceText::UninitDC(HDC &hDC)
{
	SelectObject(hDC, m_hOldFont);
	DeleteObject(m_hFont);
	m_hFont = NULL;
	m_hOldFont = NULL;

	return S_OK;
}

