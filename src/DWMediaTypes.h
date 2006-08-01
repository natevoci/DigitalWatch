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
#include "IDWOSDDataList.h"
#include "FilterGraphTools.h"
#include "AppData.h"

class DWMediaTypes;
class DWMediaType : public LogMessageCaller
{
	friend DWMediaTypes;
public:
	DWMediaType();
	virtual ~DWMediaType();

	LPWSTR index;
	LPWSTR decoder;
	LPWSTR name;
	GUID majortype;
    GUID subtype;
    GUID formattype;

	DWDecoder *GetDecoder();
	HRESULT SaveToXML(XMLElement *pElement);

private:
	DWDecoder *m_pDecoder;
};

class DWMediaTypes : public LogMessageCaller, public IDWOSDDataList
{
public:
	DWMediaTypes();
	virtual ~DWMediaTypes();

	//IDWOSDDataList Methods
	virtual LPWSTR GetListName();
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex = 0);
	virtual long GetListSize();
	virtual HRESULT FindListItem(LPWSTR name, int *pIndex);

	HRESULT Destroy();
	virtual void SetLogCallback(LogMessageCallback *callback);
	HRESULT Initialise(IGraphBuilder *piGraphBuilder, AppData *pData, LPWSTR listName = NULL);

	void SetDecoders(DWDecoders *pDecoders);

	HRESULT Load(LPWSTR filename);
	BOOL SaveMediaTypes(LPWSTR filename = NULL);
	LPWSTR GetMediaTypeDecoder(int index);
	HRESULT SetMediaTypeDecoder(int index, LPWSTR decoderName, BOOL bKeep = TRUE);
	HRESULT MakeFile(LPWSTR filename);
	void SetListItem(LPWSTR name, LPWSTR value, int index = -1);
	void GetAutoDecoder(DWMediaType *mediaType);

	DWMediaType *FindMediaType(AM_MEDIA_TYPE *mt);

private:
	CComPtr <IGraphBuilder> m_piGraphBuilder;
	FilterGraphTools graphTools;

	std::vector<DWMediaType *> m_mediaTypes;
	CCritSec m_mediaTypesLock;
	DWDecoders *m_pDecoders;

	LPWSTR m_filename;
	LPWSTR m_dataListName;
	AppData *g_pData;
};

#endif
