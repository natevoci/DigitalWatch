/**
 *	DWMediaTypes.h
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

#ifndef DWMEDIATYPES_H
#define DWMEDIATYPES_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWDecoders.h"
#include <vector>

class DWMediaTypes;
class DWMediaType
{
	friend DWMediaTypes;
public:
	DWMediaType();
	virtual ~DWMediaType();

	LPWSTR name;
	GUID majortype;
    GUID subtype;
    GUID formattype;

	DWDecoder *get_Decoder();

private:
	DWDecoder *m_pDecoder;
};

class DWMediaTypes
{
public:
	DWMediaTypes();
	virtual ~DWMediaTypes();

	void SetDecoders(DWDecoders *pDecoders);

	HRESULT Load(LPWSTR filename);

	DWMediaType *FindMediaType(AM_MEDIA_TYPE *mt);

private:
	std::vector<DWMediaType *> m_mediaTypes;
	DWDecoders *m_pDecoders;

	LPWSTR m_filename;

	LogMessage log;
};

#endif
