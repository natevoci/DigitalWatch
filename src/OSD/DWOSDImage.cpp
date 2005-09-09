/**
 *	DWOSDImage.cpp
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

#include "DWOSDImage.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOSDImage
//////////////////////////////////////////////////////////////////////

DWOSDImage::DWOSDImage()
{
	m_pwszName = NULL;

	m_pwszFilename = NULL;
	m_bUseColorKey = FALSE;
	m_dwColorKey = 0;
	SetRect(&m_rectStretchArea, -1, -1, -1, -1);

	m_pImage = NULL;
}

DWOSDImage::~DWOSDImage()
{
	if (m_pwszName)
		delete[] m_pwszName;
	if (m_pwszFilename)
		delete[] m_pwszFilename;
	if (m_pImage)
		delete m_pImage;
}

LPWSTR DWOSDImage::Name()
{
	return m_pwszName;
}

HRESULT DWOSDImage::Draw(DWSurface *pSurface, long x, long y, long width, long height)
{
	USES_CONVERSION;
	HRESULT hr;

	RECT rcSrc, rcDest;
		
	if (!m_pImage)
	{
		m_pImage = new DWSurface();
		hr = m_pImage->LoadBitmap(W2T(m_pwszFilename));
		if (m_bUseColorKey)
			m_pImage->SetColorKey(m_dwColorKey & 0x00FFFFFF);
		if FAILED(hr)
			return (log << "Failed to load image: " << hr << "\n").Write(hr);
	}

	long srcWidth = m_pImage->GetWidth();
	long srcHeight = m_pImage->GetHeight();

	//Stretching
	if ((m_rectStretchArea.left >= 0) && (m_rectStretchArea.right >= 0) && 
		(m_rectStretchArea.top >= 0)  && (m_rectStretchArea.bottom >= 0))
	{
		long leftoffset   = x + m_rectStretchArea.left;
		long rightoffset  = x + width + m_rectStretchArea.right - srcWidth;
		long topoffset    = y + m_rectStretchArea.top;
		long bottomoffset = y + height + m_rectStretchArea.bottom - srcHeight;

		//top left
		SetRect(&rcSrc, 0, 0, m_rectStretchArea.left, m_rectStretchArea.top);
		SetRect(&rcDest, x, y, leftoffset-x, topoffset-y);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//bottom left
		SetRect(&rcSrc, 0, m_rectStretchArea.bottom, m_rectStretchArea.left, srcHeight);
		SetRect(&rcDest, x, bottomoffset, leftoffset-x, y + height - bottomoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//top right
		SetRect(&rcSrc, m_rectStretchArea.right, 0, srcWidth, m_rectStretchArea.top);
		SetRect(&rcDest, rightoffset, y, x + width - rightoffset, topoffset-y);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;
		
		//bottom right
		SetRect(&rcSrc, m_rectStretchArea.right, m_rectStretchArea.bottom, srcWidth, srcHeight);
		SetRect(&rcDest, rightoffset, bottomoffset, x + width - rightoffset, y + height - bottomoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//middle left
		SetRect(&rcSrc, 0, m_rectStretchArea.top, m_rectStretchArea.left, m_rectStretchArea.bottom-m_rectStretchArea.top);
		SetRect(&rcDest, x, topoffset, leftoffset-x, bottomoffset-topoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//middle right
		SetRect(&rcSrc, m_rectStretchArea.right, m_rectStretchArea.top, srcWidth-m_rectStretchArea.right, m_rectStretchArea.bottom-m_rectStretchArea.top);
		SetRect(&rcDest, rightoffset, topoffset, x + width - rightoffset, bottomoffset-topoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//middle top
		SetRect(&rcSrc, m_rectStretchArea.left, 0, m_rectStretchArea.right-m_rectStretchArea.left, m_rectStretchArea.top);
		SetRect(&rcDest, leftoffset, y, rightoffset-leftoffset, topoffset-y);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//middle bottom
		SetRect(&rcSrc, m_rectStretchArea.left, m_rectStretchArea.bottom, m_rectStretchArea.right-m_rectStretchArea.left, srcHeight-m_rectStretchArea.bottom);
		SetRect(&rcDest, leftoffset, bottomoffset, rightoffset-leftoffset, y + height - bottomoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;
		
		//centre
		SetRect(&rcSrc, m_rectStretchArea.left, m_rectStretchArea.top, m_rectStretchArea.right-m_rectStretchArea.left, m_rectStretchArea.bottom-m_rectStretchArea.top);
		SetRect(&rcDest, leftoffset, topoffset, rightoffset-leftoffset, bottomoffset-topoffset);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;
	}
	else if ((m_rectStretchArea.left >= 0) && (m_rectStretchArea.right >= 0))
	{
		double fract = height / (double)srcHeight;
		long leftoffset  = x + (m_rectStretchArea.left * fract);
		long rightoffset = x + width + (long)((m_rectStretchArea.right - srcWidth) * fract);

		//left
		SetRect(&rcSrc, 0, 0, m_rectStretchArea.left, srcHeight);
		SetRect(&rcDest, x, y, leftoffset-x, height);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//centre
		SetRect(&rcSrc, m_rectStretchArea.left, 0, m_rectStretchArea.right-m_rectStretchArea.left, srcHeight);
		SetRect(&rcDest, leftoffset, y, rightoffset-leftoffset, height);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;

		//right
		SetRect(&rcSrc, m_rectStretchArea.right, 0, srcWidth-m_rectStretchArea.right, srcHeight);
		SetRect(&rcDest, rightoffset, y, x+width-rightoffset, height);
		if FAILED(hr = m_pImage->Blt(pSurface, &rcDest, &rcSrc))
			return hr;
	}
	else
	{
		SetRect(&rcDest, x, y, width, height);
		hr = m_pImage->Blt(pSurface, &rcDest);
		if FAILED(hr)
			return hr;
	}
	
	return S_OK;
}

HRESULT DWOSDImage::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;

	attr = pElement->Attributes.Item(L"name");
	if (attr == NULL)
		return (log << "Cannot have a window without a name\n").Write();
	if (attr->value[0] == '\0')
		return (log << "Cannot have a blank window name\n").Write();
	strCopy(m_pwszName, attr->value);

	XMLElement *element = NULL;

	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = pElement->Elements.Item(item);
		if (_wcsicmp(element->name, L"filename") == 0)
		{
			strCopy(m_pwszFilename, element->value);
		}
		else if (_wcsicmp(element->name, L"colorkey") == 0)
		{
			m_bUseColorKey = TRUE;
			m_dwColorKey = wcsToColor(element->value);
		}
		else if (_wcsicmp(element->name, L"stretcharea") == 0)
		{
			attr = element->Attributes.Item(L"left");
			if (attr)
				m_rectStretchArea.left = _wtoi(attr->value);

			attr = element->Attributes.Item(L"top");
			if (attr)
				m_rectStretchArea.top = _wtoi(attr->value);

			attr = element->Attributes.Item(L"right");
			if (attr)
				m_rectStretchArea.right = _wtoi(attr->value);

			attr = element->Attributes.Item(L"bottom");
			if (attr)
				m_rectStretchArea.bottom = _wtoi(attr->value);

		}

	};

	return S_OK;
}


