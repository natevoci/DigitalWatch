/**
 *	DWMediaTypes.cpp
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

#include "DWMediaTypes.h"
#include "ParseLine.h"
#include "GlobalFunctions.h"
#include "XMLDocument.h"

//////////////////////////////////////////////////////////////////////
// DWMediaType
//////////////////////////////////////////////////////////////////////

DWMediaType::DWMediaType()
{
	name = NULL;
	memset((void*)&majortype, 0, sizeof(GUID));
	memset((void*)&subtype, 0, sizeof(GUID));
	memset((void*)&formattype, 0, sizeof(GUID));
	m_pDecoder = NULL;
}

DWMediaType::~DWMediaType()
{
	if (name)
		delete name;
}

DWDecoder *DWMediaType::get_Decoder()
{
	return m_pDecoder;
}

//////////////////////////////////////////////////////////////////////
// DWMediaTypes
//////////////////////////////////////////////////////////////////////

DWMediaTypes::DWMediaTypes()
{
	m_filename = NULL;
	m_pDecoders = NULL;
}

DWMediaTypes::~DWMediaTypes()
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	if (m_filename)
		delete m_filename;

	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it != m_mediaTypes.end() ; it++ )
	{
		delete *it;
	}
	m_mediaTypes.clear();
}

void DWMediaTypes::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	LogMessageCaller::SetLogCallback(callback);

	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it != m_mediaTypes.end() ; it++ )
	{
		DWMediaType *mediaType = *it;
		mediaType->SetLogCallback(callback);
	}
}

void DWMediaTypes::SetDecoders(DWDecoders *pDecoders)
{
	m_pDecoders = pDecoders;
}

DWMediaType *DWMediaTypes::FindMediaType(AM_MEDIA_TYPE *mt)
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it < m_mediaTypes.end() ; it++ )
	{
		DWMediaType *item = *it;
		if ((item->majortype  != GUID_NULL) && (item->majortype  != mt->majortype ))
			continue;
		if ((item->subtype    != GUID_NULL) && (item->subtype    != mt->subtype   ))
			continue;
		if ((item->formattype != GUID_NULL) && (item->formattype != mt->formattype))
			continue;
		return item;
	}
	return NULL;
}

HRESULT DWMediaTypes::Load(LPWSTR filename)
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	(log << "Loading Media Types file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (m_pDecoders == NULL)
		return (log << "m_pDecoders must be set before calling DWMediaTypes::Load\n").Write(E_FAIL);

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		return (log << "Could not load media types file: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;
	XMLAttribute *attr;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"MediaType") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if (!attr)
				continue;

			DWMediaType *mt = new DWMediaType();
			mt->SetLogCallback(m_pLogCallback);

			strCopy(mt->name, attr->value);

			subelement = element->Elements.Item(L"MajorType");
			if (subelement)
			{
				attr = subelement->Attributes.Item(L"clsid");
				if (attr)
				{
					CComBSTR bstrCLSID(attr->value);
					if FAILED(hr = CLSIDFromString(bstrCLSID, &mt->majortype))
						(log << "Could not convert Network Type to CLSID: " << hr << "\n").Write(hr);
				}
			}

			subelement = element->Elements.Item(L"SubType");
			if (subelement)
			{
				attr = subelement->Attributes.Item(L"clsid");
				if (attr)
				{
					CComBSTR bstrCLSID(attr->value);
					if FAILED(hr = CLSIDFromString(bstrCLSID, &mt->subtype))
						(log << "Could not convert Network Type to CLSID: " << hr << "\n").Write(hr);
				}
			}

			subelement = element->Elements.Item(L"FormatType");
			if (subelement)
			{
				attr = subelement->Attributes.Item(L"clsid");
				if (attr)
				{
					CComBSTR bstrCLSID(attr->value);
					if FAILED(hr = CLSIDFromString(bstrCLSID, &mt->formattype))
						(log << "Could not convert Network Type to CLSID: " << hr << "\n").Write(hr);
				}
			}

			subelement = element->Elements.Item(L"Decoder");
			if (subelement)
			{
				attr = subelement->Attributes.Item(L"name");
				if (attr)
				{
					mt->m_pDecoder = m_pDecoders->Item(attr->value);
				}
			}

			m_mediaTypes.push_back(mt);
		}
	}

	return S_OK;
}

