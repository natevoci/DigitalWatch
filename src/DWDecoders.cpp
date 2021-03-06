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
#include "Globals.h"
#include "DWOnScreenDisplay.h"
#include "DirectDraw/DWRendererDirectDraw.h"
#include "VMR9Bitmap/DWRendererVMR9Bitmap.h"
/*
// {B87BEB7B-8D29-423f-AE4D-6582C10175AC}
static const GUID CLSID_VideoMixingRenderer = {0xB87BEB7B, 0x8D29, 0x423f, {0xAE, 0x4D, 0x65, 0x82, 0xC1, 0x01, 0x75, 0xAC}};

// {51b4abf3-748f-4e3b-a276-c828330e926a}
static const GUID CLSID_VideoMixingRenderer9 = {0x51b4abf3, 0x748f, 0x4e3b, {0xa2, 0x76, 0xc8, 0x28, 0x33, 0x0e, 0x92, 0x6a}};

*/
//////////////////////////////////////////////////////////////////////
// DWDecoder
//////////////////////////////////////////////////////////////////////

DWDecoder::DWDecoder(XMLElement *pElement)
{
	m_pElement = pElement;
	m_pElement->AddRef();
	index = NULL;
	name = NULL;
	maskname = NULL;
}

DWDecoder::~DWDecoder()
{
	m_pElement->Release();
	if (index)
		delete[] index;
	if (name)
		delete[] name;
	if (maskname)
		delete[] maskname;
}

void DWDecoder::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
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

LPWSTR DWDecoder::MaskName()
{
	XMLAttribute *attr = m_pElement->Attributes.Item(L"maskname");
	if (attr)
	{
		return attr->value;
	}
	return L"";
}

HRESULT DWDecoder::AddFilters(IGraphBuilder *piGraphBuilder, IPin *piSourcePin)
{
	HRESULT hr;

	// Note: Every video renderer must set renderMethod to something
	//       other than RENDER_METHOD_DEFAULT. If it doesn't then
	//       the renderer's window won't get owned by DW.
	RENDER_METHOD renderMethod = RENDER_METHOD_DEFAULT;

	CComPtr <IBaseFilter> piNewFilter;

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

//			CComPtr <IBaseFilter> piNewFilter;
			if(piNewFilter) 
				piNewFilter.Release();

			hr = graphTools.AddFilter(piGraphBuilder, clsid, &piNewFilter, attr->value);
			if (hr != S_OK)
				return (log << "Error: Can't Add " << attr->value << " : " << hr << "\n").Show(hr);
		}
		else if (_wcsicmp(element->name, L"VideoRenderer") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pName = attr->value;

			if (renderMethod != RENDER_METHOD_DEFAULT)
			{
				(log << "Cannot add a second video renderer to the graph. Ignoring " << pName << "\n").Write();
				continue;
			}

			if (((_wcsicmp(pName, L"Overlay Mixer") == 0) ||
				(_wcsicmp(pName, L"OverlayMixer") == 0)) &&
				!g_pData->values.application.multiple)
			{
				CComPtr <IBaseFilter> piOMFilter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_OverlayMixer, &piOMFilter, pName);
				if (hr != S_OK)
				{
					if(g_pData->application.forceConnect)
						return (log << "Error: Can't Add Overlay Mixer: " << hr << "\n").Show(hr);

					(log << "Error: Can't Add Overlay Mixer: " << hr << "\n").Write();
					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				//Check if overlay is in use
				if(piNewFilter) 
				{
					hr = graphTools.ConnectFilters(piGraphBuilder, piNewFilter, piOMFilter);
					if (hr != S_OK) //-2147467259)// == 0x800040207) //DDERR_CURRENTLYNOTAVAIL)
					{
						DestroyFilter(piGraphBuilder, piOMFilter);

						if(g_pData->application.forceConnect)
							return (log << "Error: Can't connect to overlay mixer, already be in use " << hr << "\n").Show(hr);
						else
							(log << "Error: Can't connect to overlay mixer, already be in use: " << hr << "\n").Write();

						hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
						if (hr != S_OK)
							return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

						continue;
					}
					else if (hr == S_OK)
					{
						graphTools.DisconnectOutputPins(piNewFilter);
						graphTools.DisconnectInputPins(piOMFilter);
					}
				}

				CComPtr <IDDrawExclModeVideo> piDDrawExclMode;
				piOMFilter.QueryInterface(&piDDrawExclMode);
				if (piDDrawExclMode == NULL)
				{
					DestroyFilter(piGraphBuilder, piOMFilter);
					if(g_pData->application.forceConnect)
						return (log << "Error: Could not QI for IDDrawExclModeVideo\n").Write(hr);
					else
						(log << "Error: Could not QI for IDDrawExclModeVideo\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				DWRenderer *pOSDRenderer;
				hr = g_pOSD->GetOSDRenderer(&pOSDRenderer);
				if FAILED(hr)
				{
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);
					else
						(log << "Failed to get OSD Renderer: " << hr << "\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				DWRendererDirectDraw *pOSDRendererDirectDraw = dynamic_cast<DWRendererDirectDraw *>(pOSDRenderer);
				if (!pOSDRendererDirectDraw)
				{
					pOSDRenderer->Destroy();
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Failed to cast OSD Renderer as DirectDraw OSD Renderer\n").Write(E_FAIL);
					else
						(log << "Failed to cast OSD Renderer as DirectDraw OSD Renderer\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				IDDrawExclModeVideoCallback *pOverlayCallback;
				hr = pOSDRendererDirectDraw->GetDirectDraw()->GetOverlayCallbackInterface(&pOverlayCallback);
				if FAILED(hr)
				{
					pOSDRendererDirectDraw->Destroy();
					pOSDRenderer->Destroy();
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Failed to get overlay callback interface: " << hr << "\n").Write(hr);
					else
						(log << "Failed to get overlay callback interface: " << hr << "\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				hr = piDDrawExclMode->SetCallbackInterface(pOverlayCallback, 0);
				if FAILED(hr)
				{
					pOverlayCallback->Release();
					pOSDRendererDirectDraw->Destroy();
					pOSDRenderer->Destroy();
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Error: Failed to set Callback interface on overlay mixer: " << hr << "\n").Show(hr);
					else
						(log << "Error: Failed to set Callback interface on overlay mixer: " << hr << "\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				CComPtr <IBaseFilter> piVRFilter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoRenderer, &piVRFilter, L"Video Renderer");
				if (hr != S_OK)
				{
					pOverlayCallback->Release();
					pOSDRendererDirectDraw->Destroy();
					pOSDRenderer->Destroy();
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Error: Can't Add Video Renderer: " << hr << "\n").Show(hr);
					else
						(log << "Error: Can't Add Video Renderer: " << hr << "\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}

				hr = graphTools.ConnectFilters(piGraphBuilder, piOMFilter, piVRFilter);
				if (hr != S_OK)
				{
					DestroyFilter(piGraphBuilder, piVRFilter);
					pOverlayCallback->Release();
					pOSDRendererDirectDraw->Destroy();
					pOSDRenderer->Destroy();
					piDDrawExclMode.Release();
					DestroyFilter(piGraphBuilder, piOMFilter);

					if(g_pData->application.forceConnect)
						return (log << "Error: Can't connect overlay mixer to video renderer: " << hr << "\n").Show(hr);
					else
						(log << "Error: Can't connect overlay mixer to video renderer: " << hr << "\n").Write();

					hr = RenderWindowLess(piGraphBuilder, piSourcePin, &renderMethod, pName);
					if (hr != S_OK)
						return (log << "Error: Can't use Windowless Render either: " << hr << "\n").Show(hr);;

					continue;
				}
				else
				{
					//Set renderer method
					renderMethod = RENDER_METHOD_OverlayMixer;
					g_pOSD->SetRenderMethod(renderMethod);
				}
			}
			else if (_wcsicmp(pName, L"VMR7") == 0 && !g_pData->values.application.multiple)
			{
				CComPtr <IBaseFilter> piVMR7Filter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer, &piVMR7Filter, pName);
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR7: " << hr << "\n").Show(hr);

				//Set renderer method
				renderMethod = RENDER_METHOD_VMR7;
				g_pOSD->SetRenderMethod(renderMethod);
			}
			else if (_wcsicmp(pName, L"VMR9") == 0 && !g_pData->values.application.multiple)
			{
				CComPtr <IBaseFilter> piVMR9Filter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter, pName);
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR9: " << hr << "\n").Show(hr);

				//Set renderer method to clear overlay first
				renderMethod = RENDER_METHOD_VMR7;
				g_pOSD->SetRenderMethod(renderMethod);

				//Set renderer method
				renderMethod = RENDER_METHOD_VMR9;
				g_pOSD->SetRenderMethod(renderMethod);
			}
			else if (_wcsicmp(pName, L"VMR9Windowless") == 0 || g_pData->values.application.multiple)
			{
				CComPtr <IBaseFilter> piVMR9Filter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter, pName);
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR9: " << hr << "\n").Show(hr);

				// Set Windowless Mode
				CComQIPtr<IVMRFilterConfig9> piFilterConfig(piVMR9Filter);
				if (piFilterConfig == NULL)
					return (log << "Error: Failed to get IVMRFilterConfig9 interface\n").Show(hr);

				hr = piFilterConfig->SetRenderingMode(VMR9Mode_Windowless);
				if (hr != S_OK)
					return (log << "Error: Can't set windowless mode: " << hr << "\n").Show(hr);

				// Set the clipping window
				CComQIPtr<IVMRWindowlessControl9> piWindowlessControl(piVMR9Filter);
				if (piWindowlessControl == NULL)
					return (log << "Error: Failed to get IVMRWindowlessControl9 interface\n").Show(hr);

				hr = piWindowlessControl->SetVideoClippingWindow(g_pData->hWnd);
				if (hr != S_OK)
					return (log << "Error: Failed to set clipping window: " << hr << "\n").Show(hr);

				//Set renderer method
				renderMethod = RENDER_METHOD_VMR9Windowless;
				g_pOSD->SetRenderMethod(renderMethod);
			}
			else if (_wcsicmp(pName, L"VMR9Renderless") == 0 && !g_pData->values.application.multiple)
			{
				CComPtr <IBaseFilter> piVMR9Filter;
				hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter, pName);
				if (hr != S_OK)
					return (log << "Error: Can't Add VMR9: " << hr << "\n").Show(hr);

				//Set renderer method
				renderMethod = RENDER_METHOD_VMR9Renderless;
				g_pOSD->SetRenderMethod(renderMethod);
			}
			else
			{
				return (log << "Unrecognised VideoRenderer: " << pName << " : " << hr << "\n").Show(hr);
			}
		}
		else if (_wcsicmp(element->name, L"AudioRenderer") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pName = attr->value;

			CComPtr <IBaseFilter> piDSFilter;
			hr = graphTools.AddFilterByName(piGraphBuilder, &piDSFilter, CLSID_AudioRendererCategory, pName);
			if (hr != S_OK)
				return (log << "Error: Can't Add Audio Renderer: " << pName << " : " << hr << "\n").Show(hr);
		}
		else if (_wcsicmp(element->name, L"InputFilter") == 0)
		{
			attr = element->Attributes.Item(L"name");
			if ((attr == NULL) || (attr->value[0] == '\0'))
				continue;
			LPWSTR pFilterName = attr->value;

			CComPtr <IBaseFilter> piTarget;
			hr = graphTools.FindFilter(piGraphBuilder, pFilterName, &piTarget);
			if (hr != S_OK)
				return (log << "Error: Can't Find " << pFilterName << " filter: " << hr << "\n").Show(hr);

			attr = element->Attributes.Item(L"pin");
			if ((attr == NULL) || (attr->value[0] == '\0'))
			{
				hr = graphTools.ConnectFilters(piGraphBuilder, piSourcePin, piTarget);
				if (hr != S_OK)
					return (log << "Error: Can't Connect Source Pin to input filter " << pFilterName << " : " << hr << "\n").Show(hr);
			}
			else
			{
				LPWSTR pPinName = attr->value;
				CComPtr <IPin> piTargetPin;
				hr = graphTools.FindPin(piTarget, pPinName, &piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Find " << pPinName << " pin on " << pFilterName << " filter: " << hr << "\n").Show(hr);

				hr = graphTools.ConnectFilters(piGraphBuilder, piSourcePin, piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Connect Source Pin to input pin " << pPinName << " on " << pFilterName << " filter: " << hr << "\n").Show(hr);
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
			hr = graphTools.FindFilter(piGraphBuilder, pSourceFilterName, &piSource);
			if (hr != S_OK)
				return (log << "Error: Can't Find Source Filter: " << pSourceFilterName << " : " << hr << "\n").Show(hr);
			
			//Find target filter
			CComPtr <IBaseFilter> piTarget;
			hr = graphTools.FindFilter(piGraphBuilder, pTargetFilterName, &piTarget);
			if (hr != S_OK)
				return (log << "Error: Can't Find Target Filter" << pTargetFilterName << " : " << hr << "\n").Show(hr);
			
			CComPtr <IPin> piSourcePin;
			if (pSourceFilterPin)
			{
				hr = graphTools.FindPin(piSource, pSourceFilterPin, &piSourcePin);
				if (hr != S_OK)
					return (log << "Error: Can't Find find " << pSourceFilterPin << " pin on " << pSourceFilterName << " filter: " << hr << "\n").Show(hr);
			}

			CComPtr <IPin> piTargetPin;
			if (pTargetFilterPin)
			{
				hr = graphTools.FindPin(piTarget, pTargetFilterPin, &piTargetPin);
				if (hr != S_OK)
					return (log << "Error: Can't Find find " << pTargetFilterPin << " pin on " << pTargetFilterName << " filter: " << hr << "\n").Show(hr);
			}

			if (piSourcePin && piTargetPin)
			{
				hr = graphTools.ConnectFilters(piGraphBuilder, piSourcePin, piTargetPin);
			}
			else if (piSourcePin)
			{
				hr = graphTools.ConnectFilters(piGraphBuilder, piSourcePin, piTarget);
			}
			else if (piTargetPin)
			{
				hr = graphTools.ConnectFilters(piGraphBuilder, piSource, piTargetPin);
			}
			else
			{
				hr = graphTools.ConnectFilters(piGraphBuilder, piSource, piTarget);
			}
			if (hr != S_OK)
				return (log << "Error: Failed to connect " << pSourceFilterName << " to " << pTargetFilterName << " : " << hr << "\n").Show(hr);
		}
	}
	return S_OK;
}

HRESULT DWDecoder::RenderWindowLess(IGraphBuilder *piGraphBuilder,
									IPin *piSourcePin,
									RENDER_METHOD *pRenderMethod,
									LPWSTR pName)
{
	HRESULT hr;
	(log << "OverLay Failed so now trying to use WindowLess Render\n").Write();

	CComPtr <IBaseFilter> piVMR9Filter;
	hr = graphTools.AddFilter(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter, pName);
	if (hr != S_OK)
		return (log << "Error: Can't Add VMR9: " << hr << "\n").Show(hr);

	// Set Windowless Mode
	CComQIPtr<IVMRFilterConfig9> piFilterConfig(piVMR9Filter);
	if (piFilterConfig == NULL)
		return (log << "Error: Failed to get IVMRFilterConfig9 interface\n").Show(hr);

	hr = piFilterConfig->SetRenderingMode(VMR9Mode_Windowless);
	if (hr != S_OK)
		return (log << "Error: Can't set windowless mode: " << hr << "\n").Show(hr);

	// Set the clipping window
	CComQIPtr<IVMRWindowlessControl9> piWindowlessControl(piVMR9Filter);
	if (piWindowlessControl == NULL)
		return (log << "Error: Failed to get IVMRWindowlessControl9 interface\n").Show(hr);

	hr = piWindowlessControl->SetVideoClippingWindow(g_pData->hWnd);
	if (hr != S_OK)
		return (log << "Error: Failed to set clipping window: " << hr << "\n").Show(hr);

	//Set renderer method
	*pRenderMethod = RENDER_METHOD_VMR9Windowless;
	g_pOSD->SetRenderMethod(RENDER_METHOD_VMR9Windowless);

	return hr;
}

void DWDecoder::DestroyFilter(IGraphBuilder *piGraphBuilder, CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter && piGraphBuilder)
	{
		piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

//////////////////////////////////////////////////////////////////////
// DWDecoders
//////////////////////////////////////////////////////////////////////

DWDecoders::DWDecoders() : m_filename(0)
{
	m_dataListName = NULL;
}

DWDecoders::~DWDecoders()
{
	CAutoLock decodersLock(&m_decodersLock);

	if (m_filename)
		delete[] m_filename;

	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it != m_decoders.end() ; it++ )
	{
		if (*it) delete *it;
	}
	m_decoders.clear();

	if (m_dataListName)
		delete[] m_dataListName;
}

HRESULT DWDecoders::Destroy()
{
	CAutoLock decodersLock(&m_decodersLock);

	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it < m_decoders.end() ; it++ )
	{
		if (*it) delete *it;
	}
	m_decoders.clear();
	return S_OK;
}

void DWDecoders::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock decodersLock(&m_decodersLock);

	LogMessageCaller::SetLogCallback(callback);

	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it != m_decoders.end() ; it++ )
	{
		DWDecoder *decoder = *it;
		decoder->SetLogCallback(callback);
	}
}
HRESULT DWDecoders::Initialise(IGraphBuilder *piGraphBuilder, LPWSTR listName)
{
	(log << "Initialising the Decoders List \n").Write();

	m_piGraphBuilder = piGraphBuilder;
	
	if (listName)
		strCopy(m_dataListName, listName);

	(log << "Finished Initialising the Decoders List \n").Write();
	
	return S_OK;

}

LPWSTR DWDecoders::GetListName()
{
	if (!m_dataListName)
		strCopy(m_dataListName, L"DecoderInfo");
	return m_dataListName;
}

LPWSTR DWDecoders::GetListItem(LPWSTR name, long nIndex)
{
	CAutoLock decodersLock(&m_decodersLock);

	if (nIndex >= (long)m_decoders.size())
		return NULL;

	long startsWithLength = strStartsWith(name, m_dataListName);
	if (startsWithLength > 0)
	{
		name += startsWithLength;

		DWDecoder *item = m_decoders.at(nIndex);
		if (_wcsicmp(name, L".index") == 0)
			return item->index;
		else if (_wcsicmp(name, L".name") == 0)
			return item->name;
		else if (_wcsicmp(name, L".maskname") == 0)
			return item->maskname;
	}
	else
	{
		startsWithLength = strStartsWith(name, L"GetMaskFromName.");
		if (startsWithLength > 0)
		{
			name += startsWithLength;

			std::vector<DWDecoder *>::iterator it = m_decoders.begin();
			for ( ; it != m_decoders.end() ; it++ )
			{
				DWDecoder *item = *it;
				if (_wcsicmp(name, item->name) == 0)
					return item->maskname;
			}
		}
		else
		{
			startsWithLength = strStartsWith(name, L"GetIndexFromName.");
			if (startsWithLength > 0)
			{
				name += startsWithLength;

				std::vector<DWDecoder *>::iterator it = m_decoders.begin();
				for ( ; it != m_decoders.end() ; it++ )
				{
					DWDecoder *item = *it;
					if (_wcsicmp(name, item->name) == 0)
						return item->index;
				}
			}
		}
	}

	return NULL;
}

HRESULT DWDecoders::FindListItem(LPWSTR name, int *pIndex)
{
	if (!pIndex)
        return E_INVALIDARG;

	*pIndex = 0;

	CAutoLock decodersLock(&m_decodersLock);
	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it < m_decoders.end() ; it++ )
	{
		if (_wcsicmp((*it)->name, name) == 0)
			return S_OK;

		(*pIndex)++;
	}

	return E_FAIL;
}

long DWDecoders::GetListSize()
{
	CAutoLock decodersLock(&m_decodersLock);
	return m_decoders.size();
}

HRESULT DWDecoders::Load(LPWSTR filename)
{
	CAutoLock decodersLock(&m_decodersLock);

	(log << "Loading Decoders file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	strCopy(m_filename, filename);

	HRESULT hr;
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		return (log << "Could not load decoders file: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"Decoder") == 0)
		{
			DWDecoder *dec = new DWDecoder(element);
			strCopy((*dec).name, (*dec).Name());
			strCopy((*dec).index, item+1);
			(*dec).maskname = new wchar_t[1];
			(*dec).maskname[0] = 0;

			LPWSTR pMaskName = new WCHAR[MAX_PATH];
			int element2Count = element->Elements.Count();
			for ( int item2=0 ; item2<element2Count ; item2++ )
			{
				XMLElement *element2 = element->Elements.Item(item2);
				if (_wcsicmp(element2->name, L"MediaType") == 0)
				{
					XMLAttribute *attr = element2->Attributes.Item(L"name");
					if (attr)
					{
						StringCchPrintfW(pMaskName, MAX_PATH, L"%s%s ", (*dec).maskname, attr->value);
						strCopy((*dec).maskname, pMaskName);
					}
				}
			}

			dec->SetLogCallback(m_pLogCallback);
			m_decoders.push_back(dec);
		}
	}

	(log << "Loaded " << (long)m_decoders.size() << " decoders\n").Write();

	indent.Release();
	(log << "Finished loading decoders file : " << hr << "\n").Write();

	return S_OK;
}

DWDecoder *DWDecoders::Item(LPWSTR pName)
{
	CAutoLock decodersLock(&m_decodersLock);

	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it < m_decoders.end() ; it++ )
	{
		DWDecoder *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}

DWDecoder *DWDecoders::Item(int index)
{
	CAutoLock decodersLock(&m_decodersLock);

	int count = 0;
	std::vector<DWDecoder *>::iterator it = m_decoders.begin();
	for ( ; it < m_decoders.end() ; it++ )
	{
		DWDecoder *item = *it;
		if (index == count)
			return item;

		count++;
	}
	return NULL;
}
