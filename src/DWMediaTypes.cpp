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
	index = NULL;
	decoder = NULL;
	name = NULL;
	memset((void*)&majortype, 0, sizeof(GUID));
	memset((void*)&subtype, 0, sizeof(GUID));
	memset((void*)&formattype, 0, sizeof(GUID));
	m_pDecoder = NULL;
}

DWMediaType::~DWMediaType()
{
	if (index)
		delete[] index;
	if (decoder)
		delete[] decoder;
	if (name)
		delete[] name;
}

DWDecoder *DWMediaType::GetDecoder()
{
	return m_pDecoder;
}

HRESULT DWMediaType::SaveToXML(XMLElement *pElement)
{
	pElement->Attributes.Add(new XMLAttribute(L"name", name));

	LPOLESTR clsid = NULL;

	XMLElement *xmlMediaType;
	xmlMediaType = new XMLElement(L"MajorType");
	StringFromCLSID(majortype, &clsid);
	xmlMediaType->Attributes.Add(new XMLAttribute(L"clsid", clsid));
	pElement->Elements.Add(xmlMediaType);

	xmlMediaType = new XMLElement(L"SubType");
	StringFromCLSID(subtype, &clsid);
	xmlMediaType->Attributes.Add(new XMLAttribute(L"clsid", clsid));
	pElement->Elements.Add(xmlMediaType);

	xmlMediaType = new XMLElement(L"FormatType");
	StringFromCLSID(formattype, &clsid);
	xmlMediaType->Attributes.Add(new XMLAttribute(L"clsid", clsid));
	pElement->Elements.Add(xmlMediaType);

	XMLElement *xmlDecoder;
	xmlDecoder = new XMLElement(L"Decoder");
	if (decoder)
		xmlDecoder->Attributes.Add(new XMLAttribute(L"name", decoder));
	else
		xmlDecoder->Attributes.Add(new XMLAttribute(L"name", L""));

	pElement->Elements.Add(xmlDecoder);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWMediaTypes
//////////////////////////////////////////////////////////////////////

DWMediaTypes::DWMediaTypes()
{
	m_filename = NULL;
	m_pDecoders = NULL;
	m_dataListName = NULL;
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

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DWMediaTypes::Destroy()
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it < m_mediaTypes.end() ; it++ )
	{
		delete *it;
	}
	m_mediaTypes.clear();
	return S_OK;
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

HRESULT DWMediaTypes::Initialise(IGraphBuilder *piGraphBuilder, LPWSTR listName)
{
	(log << "Initialising the MediaTypes List \n").Write();

	m_piGraphBuilder = piGraphBuilder;
	
	if (listName)
		strCopy(m_dataListName, listName);

	(log << "Finished Initialising the MediaTypes List \n").Write();
	
	return S_OK;

}

LPWSTR DWMediaTypes::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"MediaTypeInfo");
	return m_dataListName;
}

LPWSTR DWMediaTypes::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	if (nIndex >= (long)m_mediaTypes.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DWMediaType *item = m_mediaTypes.at(nIndex);
		if (_wcsicmp(name, L".index") == 0)
			return item->index;
		else if (_wcsicmp(name, L".decoder") == 0)
			return item->decoder;
		else if (_wcsicmp(name, L".name") == 0)
			return item->name;
	}
	return NULL;
}

void DWMediaTypes::SetListItem(LPWSTR name, LPWSTR value, int index)
{
	if (!name || !value)
		return;

	CAutoLock mediaTypesLock(&m_mediaTypesLock);
	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it < m_mediaTypes.end() ; it++ )
	{
		if (_wtoi((*it)->index) == index + 1)
		{
			if (_wcsicmp(name, L"decoder") == 0)
			{
				strCopy((*it)->decoder, value);
				(*it)->m_pDecoder = m_pDecoders->Item(value);
				return;
			}
			else if (_wcsicmp(name, L"majortype") == 0)
			{
				CComBSTR bstrCLSID(value);
				CLSIDFromString(bstrCLSID, &(*it)->majortype);
				return;
			}
			else if (_wcsicmp(name, L"subtype") == 0)
			{
				CComBSTR bstrCLSID(value);
				CLSIDFromString(bstrCLSID, &(*it)->subtype);
				return;
			}
			else if (_wcsicmp(name, L"formattype") == 0)
			{
				CComBSTR bstrCLSID(value);
				CLSIDFromString(bstrCLSID, &(*it)->formattype);
				return;
			}
		}
	}
	return;
}

long DWMediaTypes::GetListSize()
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);
	return m_mediaTypes.size();
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
		(log << "Could not load media types file: " << m_filename << "\n").Show();
		if FAILED(MakeFile(filename))
			return (log << "Could not load or make the Media Types File: " << m_filename << "\n").Show(hr);

		if FAILED(hr = file.Load(m_filename))
			return (log << "Could not load or make the Media Types File: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;
	XMLAttribute *attr;

	CAutoLock mediaTypesLock(&m_mediaTypesLock);

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
			strCopy(mt->index, item+1);

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
					if (mt->m_pDecoder)
						strCopy(mt->decoder, mt->m_pDecoder->Name());
					else
						strCopy(mt->decoder, L"None");
				}
			}

			m_mediaTypes.push_back(mt);
		}
	}

	(log << "Loaded " << (long)m_mediaTypes.size() << " media types\n").Write();

	indent.Release();
	(log << "Finished loading media types file : " << hr << "\n").Write();
	return S_OK;
}

BOOL DWMediaTypes::SaveMediaTypes(LPWSTR filename)
{
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	CAutoLock mediaTypesLock(&m_mediaTypesLock);
	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it < m_mediaTypes.end() ; it++ )
	{
		XMLElement *pElement = new XMLElement(L"MediaType");
		DWMediaType *pMediaType = *it;
		pMediaType->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);
		
	return TRUE;
}

LPWSTR DWMediaTypes::GetMediaTypeDecoder(int index)
{
	LPWSTR searchItem = new WCHAR[21];
	strCopy(searchItem, L"MediaTypeInfo.decoder");
	return GetListItem(searchItem, index);
}

HRESULT DWMediaTypes::SetMediaTypeDecoder(int index, LPWSTR decoderName, BOOL bKeep)
{

	SetListItem(L"decoder", decoderName, index);

	if (!bKeep)
		return S_OK;

	if (!SaveMediaTypes())
		return (log << "Unable to save the Media Types File \n").Write(E_FAIL);

	return S_OK;
}

HRESULT DWMediaTypes::MakeFile(LPWSTR filename)
{
	(log << "Making the Media Types file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

//	HRESULT hr;

	if (m_pDecoders == NULL)
		return (log << "m_pDecoders must be set before calling DWMediaTypes::MakeFile\n").Write(E_FAIL);

	strCopy(m_filename, filename);

	std::vector<DWMediaType *> mediaTypes;

	DWMediaType *mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	//MPEG2 Video
	mt->index = L"1";
	mt->name = L"MPEG2 Video";
	CLSIDFromString(L"{73646976-0000-0010-8000-00AA00389B71}", &mt->majortype); //KSDATAFORMAT_TYPE_VIDEO
	CLSIDFromString(L"{E06D8026-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIASUBTYPE_MPEG2_VIDEO
	CLSIDFromString(L"{E06D80E3-DB46-11CF-B4D1-00805F6CBBEA}", &mt->formattype); //FORMAT_MPEG2Video
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//H264 Video
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"2";
	mt->name = L"H264 Video";
	CLSIDFromString(L"{73646976-0000-0010-8000-00AA00389B71}", &mt->majortype); //KSDATAFORMAT_TYPE_VIDEO
	CLSIDFromString(L"{8D2D71CB-243F-45E3-B2D8-5FD7967EC09B}", &mt->subtype); //H264_SubType
	CLSIDFromString(L"{05589f80-c356-11ce-bf01-00aa0055595a}", &mt->formattype); //FORMAT_VideoInfo
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//MPEG Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"3";
	mt->name = L"MPEG Audio";
	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
	CLSIDFromString(L"{00000050-0000-0010-8000-00AA00389B71}", &mt->subtype); //MEDIASUBTYPE_MPEG1AudioPayload
	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//MPEG2 Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"4";
	mt->name = L"MPEG2 Audio";
	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
	CLSIDFromString(L"{E06D802B-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIASUBTYPE_MPEG2_AUDIO
	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//AC3 Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"5";
	mt->name = L"AC3 Audio";
	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
	CLSIDFromString(L"{E06D802C-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIATYPE_DOLBY_AC3
	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//AAC Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"6";
	mt->name = L"AAC Audio";
	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
	CLSIDFromString(L"{000000FF-0000-0010-8000-00AA00389B71}", &mt->subtype); //MEDIASUBTYPE_AAC
	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	//Teletext
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	mt->index = L"7";
	mt->name = L"Teletext";
	CLSIDFromString(L"{455F176C-4B06-47CE-9AEF-8CAEF73DF7B5}", &mt->majortype); //KSDATAFORMAT_TYPE_MPEG2_SECTIONS
	CLSIDFromString(L"{E436EB8E-524F-11CE-9F53-0020AF0BA770}", &mt->subtype); //KSDATAFORMAT_SUBTYPE_NONE
	CLSIDFromString(L"{0F6417D6-C318-11D0-A43F-00A0C9223196}", &mt->formattype); //KSDATAFORMAT_SPECIFIER_NONE
	mt->m_pDecoder = GetAutoDecoder(mt);
	if (mt->m_pDecoder)
		strCopy(mt->decoder, mt->m_pDecoder->Name());
	mediaTypes.push_back(mt);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	std::vector<DWMediaType *>::iterator it = mediaTypes.begin();
	for ( ; it < mediaTypes.end() ; it++ )
	{
		XMLElement *pElement = new XMLElement(L"MediaType");
		DWMediaType *pMediaType = *it;
		pMediaType->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);

	it = mediaTypes.begin();
	for ( ; it < mediaTypes.end() ; it++ )
	{
		delete (*it);
	}
	mediaTypes.clear();

	indent.Release();
	(log << "Finished Making the Media Types File.\n").Write();
	return S_OK;
}

DWDecoder *DWMediaTypes::GetAutoDecoder(DWMediaType *mediaType)
{
	return NULL;
}