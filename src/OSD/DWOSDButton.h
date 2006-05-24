/**
 *	DWOSDButton.h
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

#ifndef DWOSDBUTTON_H
#define DWOSDBUTTON_H

#include "StdAfx.h"
#include "DWOSDControl.h"
#include "XMLDocument.h"
#include "DWOSDImage.h"

class DWOSDButton : public DWOSDControl  
{
public:
	DWOSDButton(DWSurface* pSurface);
	virtual ~DWOSDButton();

	HRESULT LoadFromXML(XMLElement *pElement);

protected:
	virtual HRESULT Draw(long tickCount);

	long m_nPosX;
	long m_nPosY;
	long m_nWidth;
	long m_nHeight;

	unsigned int m_uAlignHorizontal;
	unsigned int m_uAlignVertical;

	LPWSTR m_wszText;
	LPWSTR m_wszFont;
	COLORREF m_dwTextColor;
	long m_nTextHeight;
	long m_nTextWeight;

	DWOSDImage* m_pBackgroundImage;
	DWOSDImage* m_pHighlightImage;
	DWOSDImage* m_pSelectedImage;
	DWOSDImage* m_pMaskedImage;

	LPWSTR m_wszMask;
	

};

#endif
