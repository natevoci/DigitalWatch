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
#include "MediaFormats.h"
#include "BDATYPES.H"
#include "ks.h"
#include "ksmedia.h"
#include "bdamedia.h"

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
	m_piGraphBuilder = NULL;
	g_pData = NULL;
}

DWMediaTypes::~DWMediaTypes()
{
	Destroy();

	if (m_filename)
		delete[] m_filename;

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DWMediaTypes::Destroy()
{
	CAutoLock mediaTypesLock(&m_mediaTypesLock);
	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it < m_mediaTypes.end() ; it++ )
	{
		DWMediaType *mediaType = *it;
		delete mediaType;
	}
	m_mediaTypes.clear();

	return S_OK;
}

void DWMediaTypes::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	CAutoLock mediaTypesLock(&m_mediaTypesLock);
	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it != m_mediaTypes.end() ; it++ )
	{
		DWMediaType *mediaType = *it;
		mediaType->SetLogCallback(callback);
	}
}

HRESULT DWMediaTypes::Initialise(IGraphBuilder *piGraphBuilder, AppData *pData, LPWSTR listName)
{
	(log << "Initialising the MediaTypes List \n").Write();

	m_piGraphBuilder = piGraphBuilder;
	
	if (listName)
		strCopy(m_dataListName, listName);

	if(pData)
		g_pData = pData;

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
		(log << "Could not load media types file: " << m_filename <<"\n").Show();
		if FAILED(hr = MakeFile(filename))
			return (log << "Could not load or make the Media Types File: " << m_filename << "\n").Show(hr);

		Destroy();

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

	if (m_pDecoders == NULL)
		return (log << "m_pDecoders must be set before calling DWMediaTypes::MakeFile\n").Write(E_FAIL);

	strCopy(m_filename, filename);

	Destroy();
	
	CAutoLock mediaTypesLock(&m_mediaTypesLock);

	//MPEG2 Video
	DWMediaType *mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"1");
	strCopy(mt->name, L"MPEG2 Video");
	CComBSTR bstrCLSID = KSDATAFORMAT_TYPE_VIDEO;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = MEDIASUBTYPE_MPEG2_VIDEO;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_MPEG2Video;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{73646976-0000-0010-8000-00AA00389B71}", &mt->majortype); //KSDATAFORMAT_TYPE_VIDEO
//	CLSIDFromString(L"{E06D8026-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIASUBTYPE_MPEG2_VIDEO
//	CLSIDFromString(L"{E06D80E3-DB46-11CF-B4D1-00805F6CBBEA}", &mt->formattype); //FORMAT_MPEG2Video
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//H264 Video
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"2");
	strCopy(mt->name, L"H264 Video");
	bstrCLSID = KSDATAFORMAT_TYPE_VIDEO;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = H264_SubType;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_VideoInfo;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{73646976-0000-0010-8000-00AA00389B71}", &mt->majortype); //KSDATAFORMAT_TYPE_VIDEO
//	CLSIDFromString(L"{8D2D71CB-243F-45E3-B2D8-5FD7967EC09B}", &mt->subtype); //H264_SubType
//	CLSIDFromString(L"{05589f80-c356-11ce-bf01-00aa0055595a}", &mt->formattype); //FORMAT_VideoInfo
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//MPEG4 Video
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"3");
	strCopy(mt->name, L"MPEG4 Video");
	bstrCLSID = MEDIATYPE_Video;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = FOURCCMap(MAKEFOURCC('A','V','C','1'));
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_MPEG2Video;
	CLSIDFromString(bstrCLSID, &mt->formattype);
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//MPEG Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"4");
	strCopy(mt->name, L"MPEG Audio");
	bstrCLSID = MEDIATYPE_Audio;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = MEDIASUBTYPE_MPEG1AudioPayload;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = MEDIASUBTYPE_MPEG1Payload;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{05589f81-c356-11ce-bf01-00aa0055595a}", &mt->majortype); //MEDIATYPE_Audio
//	CLSIDFromString(L"{00000050-0000-0010-8000-00AA00389B71}", &mt->subtype); //MEDIASUBTYPE_MPEG1AudioPayload
////	CLSIDFromString(L"{e436eb81-524f-11ce-9f53-0020af0ba770}", &mt->subtype); //MEDIASUBTYPE_MPEG1Payload
//	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//MPEG2 Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"5");
	strCopy(mt->name, L"MPEG2 Audio");
	bstrCLSID = MEDIATYPE_Audio;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = MEDIASUBTYPE_MPEG2_AUDIO;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_WaveFormatEx;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
//	CLSIDFromString(L"{E06D802B-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIASUBTYPE_MPEG2_AUDIO
//	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//AC3 Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"6");
	strCopy(mt->name, L"AC3 Audio");
	bstrCLSID = MEDIATYPE_Audio;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = L"{E06D802C-DB46-11CF-B4D1-00805F6CBBEA}";//MEDIATYPE_DOLBY_AC3;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_WaveFormatEx;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
//	CLSIDFromString(L"{E06D802C-DB46-11CF-B4D1-00805F6CBBEA}", &mt->subtype); //MEDIATYPE_DOLBY_AC3
//	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//AAC Audio
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"7");
	strCopy(mt->name, L"AAC Audio");
	bstrCLSID = MEDIATYPE_Audio;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = MEDIASUBTYPE_AAC;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = FORMAT_WaveFormatEx;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{73647561-0000-0010-8000-00AA00389B71}", &mt->majortype); //MEDIATYPE_Audio
//	CLSIDFromString(L"{000000FF-0000-0010-8000-00AA00389B71}", &mt->subtype); //MEDIASUBTYPE_AAC
//	CLSIDFromString(L"{05589F81-C356-11CE-BF01-00AA0055595A}", &mt->formattype); //FORMAT_WaveFormatEx
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	//Teletext
	mt = new DWMediaType();
	mt->SetLogCallback(m_pLogCallback);
	strCopy(mt->index, L"8");
	strCopy(mt->name, L"Teletext");
	bstrCLSID = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	CLSIDFromString(bstrCLSID, &mt->majortype); 
	bstrCLSID = KSDATAFORMAT_SUBTYPE_NONE;
	CLSIDFromString(bstrCLSID, &mt->subtype); 
	bstrCLSID = KSDATAFORMAT_SPECIFIER_NONE;
	CLSIDFromString(bstrCLSID, &mt->formattype);
//	CLSIDFromString(L"{455F176C-4B06-47CE-9AEF-8CAEF73DF7B5}", &mt->majortype); //KSDATAFORMAT_TYPE_MPEG2_SECTIONS
//	CLSIDFromString(L"{E436EB8E-524F-11CE-9F53-0020AF0BA770}", &mt->subtype); //KSDATAFORMAT_SUBTYPE_NONE
//	CLSIDFromString(L"{0F6417D6-C318-11D0-A43F-00A0C9223196}", &mt->formattype); //KSDATAFORMAT_SPECIFIER_NONE
	m_mediaTypes.push_back(mt);
	GetAutoDecoder(mt);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
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


	indent.Release();
	(log << "Finished Making the Media Types File.\n").Write();
	return S_OK;
}

void DWMediaTypes::GetAutoDecoder(DWMediaType *mediaType)
{
	if (!mediaType || !g_pData || !g_pData->settings.application.autoDecoderTest)
		return;

	HRESULT hr;

	(log << "Searching for a suitable decoder for " << mediaType->name << " Media Type\n").Write();
	LogMessageIndent indent(&log);

	//--- Create Graph ---
	CComPtr <IGraphBuilder> piGraphBuilder;
	if FAILED(hr = piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER))
	{
		(log << "Failed Creating the Decoder Auto Test Graph Builder: " << hr << "\n").Write();
		return;
	}

	if (mediaType->GetDecoder() == NULL)
	{
		DVBTChannels_Service* pService = new DVBTChannels_Service();
		DVBTChannels_Stream *pStream = new DVBTChannels_Stream();

		BOOL bFound = FALSE;
		for (int n=0 ; n<DVBTChannels_Service_PID_Types_Count ; n++ )
		{
			if (_wcsicmp(mediaType->name, DVBTChannels_Service_PID_Types_String[n]) == 0)
			{
				pStream->Type = (DVBTChannels_Service_PID_Types)n;
				bFound = TRUE;
			}
		}

		if (bFound)
			pService->AddStream(pStream);
		else
		{
			delete pStream;
			delete pService;
			(log << "Failed to find a matching Media Type\n").Write();
			return; 
		}

		g_pData->application.forceConnect = TRUE;

		for ( int i = 0 ; i < m_pDecoders->GetListSize() ; i++ )
		{
			DWDecoder *pDecoder = m_pDecoders->Item(i);
			LPWSTR name = pDecoder->Name();

			if (pDecoder && pDecoder->maskname && mediaType->name &&  
				(wcsstr(pDecoder->maskname, mediaType->name) != NULL))
			{
				CComPtr <IBaseFilter> piBDAMpeg2Demux;
				DWORD rotEntry = 0;

				while (TRUE)
				{
					CLSID clsid = GUID_NULL;
					CLSIDFromString(L"{afb6c280-2c41-11d3-8a60-0000f81e0e4a}", &clsid);
					if FAILED(hr = graphTools.AddFilter(piGraphBuilder, clsid, &piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
					{
						(log << "Failed to add Test MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write();
						break;
					}

					if FAILED(hr = graphTools.AddDemuxPins(pService, piBDAMpeg2Demux, 0, TRUE))
					{
						(log << "Failed to Add Demux Pins and render the graph\n").Write();
						break;
					}

					CComPtr <IPin> piPin;
					if FAILED(hr = graphTools.FindAnyPin(piBDAMpeg2Demux, NULL, &piPin, REQUESTED_PINDIR_OUTPUT))
					{
						(log << "Failed to find an output pin: " << hr << "\n").Write();
						break;
					}

					if FAILED(hr = pDecoder->AddFilters(piGraphBuilder, piPin))
					{
						(log << "Failed to render decoder\n").Write();
						break;
					}

					delete pService;

					if (piBDAMpeg2Demux)
						piBDAMpeg2Demux.Release();

					hr = graphTools.DisconnectAllPins(piGraphBuilder);
					if FAILED(hr)
						(log << "Failed to disconnect pins: " << hr << "\n").Write(hr);

					hr = graphTools.RemoveAllFilters(piGraphBuilder);
					if FAILED(hr)
						(log << "Failed to remove filters: " << hr << "\n").Write(hr);

					if (pDecoder && pDecoder->Name())
						strCopy(mediaType->decoder, pDecoder->Name());
					else
						strCopy(mediaType->decoder, L"None");

					g_pData->application.forceConnect = FALSE;

					(log << "Found a working Decoder: " << name << " for " << mediaType->name << " Media Type has succeeded\n").Write();
					indent.Release();

					return;
				};

				(log << "Testing of the Decoder: " << name << " for " << mediaType->name << " Media Type Failed\n").Write();

				if (piBDAMpeg2Demux)
					piBDAMpeg2Demux.Release();

				hr = graphTools.DisconnectAllPins(piGraphBuilder);
				if FAILED(hr)
					(log << "Failed to disconnect pins: " << hr << "\n").Write(hr);

				hr = graphTools.RemoveAllFilters(piGraphBuilder);
				if FAILED(hr)
					(log << "Failed to remove filters: " << hr << "\n").Write(hr);
			}
		}
		delete pService;
	}

	strCopy(mediaType->decoder, L"None");

	g_pData->application.forceConnect = FALSE;

	(log << "Finished Searching for a decoder, None Found.\n").Write();
	indent.Release();

	return;
}

