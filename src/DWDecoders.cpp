/**
 *	DWDecoders.cpp
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

#include "DWDecoders.h"
#include "GlobalFunctions.h"
#include "FilterGraphTools.h"

//////////////////////////////////////////////////////////////////////
// DWDecoder
//////////////////////////////////////////////////////////////////////

DWDecoder::DWDecoder()
{
}

DWDecoder::~DWDecoder()
{
}

LPWSTR DWDecoder::Name()
{
	XMLAttribute *attr = m_pElement->Attributes.Item(L"name");
	if (attr)
	{
		return attr->value;
	}
	return L"";
}

HRESULT DWDecoder::AddFilters(IGraphBuilder *piGraphBuilder, IPin *piSourcePin)
{
	HRESULT hr;

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;
	XMLAttribute *attr = NULL;
	int elementCount = m_pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = m_pElement->Elements.Item(item);

		if (_wcsicmp(element->name, L"LoadFilter") == 0)
		{
			attr = element->Attributes.Item(L"clsid");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;

			CComBSTR bstrCLSID(attr->value);
			GUID clsid;
			if FAILED(hr = CLSIDFromString(bstrCLSID, &clsid))
				continue;

			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;

			CComPtr <IBaseFilter> piNewFilter;
			hr = AddFilter(piGraphBuilder, clsid, &piNewFilter, attr->value);
			if (hr != S_OK)
				return (log << "Error: Can't Add " << attr->value << "\n").Show(hr);
		}
		else if (_wcsicmp(element->name, L"VideoRenderer") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pName = attr->value;

			if (_wcsicmp(pName, L"Overlay Mixer") == 0)
			{
				CComPtr <IBaseFilter> piOMFilter;
				hr = AddFilter(piGraphBuilder, CLSID_OverlayMixer, &piOMFilter, L"Overlay Mixer");
				if (hr != S_OK)
					return (log << "Error: Can't Add Overlay Mixer\n").Show(hr);

				CComPtr <IBaseFilter> piVRFilter;
				hr = AddFilter(piGraphBuilder, CLSID_VideoRenderer, &piVRFilter, L"Video Renderer");
				if (hr != S_OK)
					return (log << "Error: Can't Add Video Renderer\n").Show(hr);

				hr = ConnectFilters(piGraphBuilder, piOMFilter, piVRFilter);
				if (hr != S_OK)
					return (log << "Error: Can't connect overlay mixer to video renderer\n").Show(hr);
			}
			else if (_wcsicmp(pName, L"VMR7") == 0)
			{
				CComPtr <IBaseFilter> piVMR7Filter;
				hr = AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer, &piVMR7Filter, L"VMR7");
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR7\n").Show(hr);
			}
			else if (_wcsicmp(pName, L"VMR9") == 0)
			{
				CComPtr <IBaseFilter> piVMR9Filter;
				hr = AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter, L"VMR9");
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR9\n").Show(hr);
			}
			else
			{
				return (log << "Unrecognised VideoRenderer: " << pName << "\n").Show(hr);
			}
		}
		else if (_wcsicmp(element->name, L"AudioRenderer") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pName = attr->value;

			CComPtr <IBaseFilter> piDSFilter;
			hr = AddFilterByName(piGraphBuilder, &piDSFilter, CLSID_AudioRendererCategory, pName);
			if (hr != S_OK)
				return (log << "Error: Can't Add Audio Renderer: " << pName << "\n").Show(hr);
		}
		else if (_wcsicmp(element->name, L"InputFilter") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pFilterName = attr->value;

			CComPtr <IBaseFilter> piTarget;
			hr = FindFilter(piGraphBuilder, pFilterName, &piTarget);
			if (hr != S_OK)
				return (log << "Error: Can't Find " << pFilterName << " filter\n").Show(hr);

			attr = element->Attributes.Item(L"pin");
			if ((attr == NULL) || (attr->value[0] == '\0'))
			{
				hr = ConnectFilters(piGraphBuilder, piSourcePin, piTarget);
				if (hr != S_OK)
					return (log << "Error: Can't Connect Source Pin to input filter " << pFilterName << "\n").Show(hr);
			}
			else
			{
				LPWSTR pPinName = attr->value;
				CComPtr <IPin> piTargetPin;
				hr = FindPin(piTarget, pPinName, &piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Find " << pPinName << " pin on " << pFilterName << " filter\n").Show(hr);

				hr = ConnectFilters(piGraphBuilder, piSourcePin, piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Connect Source Pin to input pin " << pPinName << " on " << pFilterName << " filter\n").Show(hr);
			}
		}
		else if (_wcsicmp(element->name, L"Connect") == 0)
		{
			LPWSTR pSourceFilterName = NULL;
			LPWSTR pSourceFilterPin  = NULL;
			LPWSTR pTargetFilterName = NULL;
			LPWSTR pTargetFilterPin  = NULL;

			//Read source filter details
			subelement = element->Elements.Item(L"From");
			if (subelement == NULL)
				continue;

			attr = subelement->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			pSourceFilterName = attr->value;

			attr = subelement->Attributes.Item(L"pin");
			if ((attr != NULL) && (attr->value[0] != '\0'))
				pSourceFilterPin = attr->value;

			//Read target filter details
			subelement = element->Elements.Item(L"To");
			if (subelement == NULL)
				continue;

			attr = subelement->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			pTargetFilterName = attr->value;

			attr = subelement->Attributes.Item(L"pin");
			if ((attr != NULL) && (attr->value[0] != '\0'))
				pTargetFilterPin = attr->value;

			//Find source filter
			CComPtr <IBaseFilter> piSource;
			hr = FindFilter(piGraphBuilder, pSourceFilterName, &piSource);
			if (hr != S_OK)
				return (log << "Error: Can't Find Source Filter: " << pSourceFilterName << "\n").Show(hr);
			
			//Find target filter
			CComPtr <IBaseFilter> piTarget;
			hr = FindFilter(piGraphBuilder, pTargetFilterName, &piTarget);
			if (hr != S_OK)
				return (log << "Error: Can't Find Target Filter" << pTargetFilterName << "\n").Show(hr);
			
			CComPtr <IPin> piSourcePin;
			if (pSourceFilterPin)
			{
				hr = FindPin(piSource, pSourceFilterPin, &piSourcePin);
				if (hr != S_OK)
					return (log << "Error: Can't Find find " << pSourceFilterPin << " pin on " << pSourceFilterName << " filter\n").Show(hr);
			}

			CComPtr <IPin> piTargetPin;
			if (pTargetFilterPin)
			{
				hr = FindPin(piTarget, pTargetFilterPin, &piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Find find " << pTargetFilterPin << " pin on " << pTargetFilterName << " filter\n").Show(hr);
			}

			if (piSourcePin && piTargetPin)
			{
				hr = ConnectFilters(piGraphBuilder, piSourcePin, piTargetPin);
			}
			else if (piSourcePin)
			{
				hr = ConnectFilters(piGraphBuilder, piSourcePin, piTarget);
			}
			else if (piTargetPin)
			{
				hr = ConnectFilters(piGraphBuilder, piSource, piTargetPin);
			}
			else
			{
				hr = ConnectFilters(piGraphBuilder, piSource, piTarget);
			}
			if (hr != S_OK)
				return (log << "Error: Failed to connect " << pSourceFilterName << " to " << pTargetFilterName << "\n").Show(hr);
		}
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWDecoders
//////////////////////////////////////////////////////////////////////

DWDecoders::DWDecoders() : m_filename(0)
{

}

DWDecoders::~DWDecoders()
{
	if (m_filename)
		delete m_filename;

	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it != m_decoders.end() ; it++ )
	{
		delete *it;
	}
	m_decoders.clear();
}

HRESULT DWDecoders::Load(LPWSTR filename)
{
	strCopy(m_filename, filename);

	XMLDocument file;
	if (file.Load(m_filename) != S_OK)
	{
		return (log << "Could not load decoders file: " << m_filename << "\n").Show(FALSE);
	}

	(log << "Loading decoders file: " << m_filename << "\n").Write();

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"Decoder") == 0)
		{
			DWDecoder *dec = new DWDecoder();
			dec->m_pElement = element;
			dec->m_pElement->AddRef();
			m_decoders.push_back(dec);
		}
	}

	return S_OK;
}

DWDecoder *DWDecoders::Item(LPWSTR pName)
{
	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it < m_decoders.end() ; it++ )
	{
		DWDecoder *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}
