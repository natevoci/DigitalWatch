/**
 *	DWGraph.cpp
 *	Copyright (C) 2004 Nate
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

#include "DWGraph.h"
#include "Globals.h"
#include "FilterGraphTools.h"

#include <mpconfig.h>
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include "bdamedia.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWGraph::DWGraph()
{
	m_piGraphBuilder = NULL;
	m_piMediaControl = NULL;
	m_bInitialised = FALSE;
	m_rotEntry = 0;
	log.AddCallback(&g_DWLogWriter);
}

DWGraph::~DWGraph()
{
	Destroy();
}

BOOL DWGraph::Initialise()
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DigitalWatch graph tried to initialise a second time\n").Write();

	//--- COM should already be initialized ---

	//--- Create Graph ---
	if (FAILED(hr = m_piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER)))
		return (log << "Failed Creating Graph Builder\n").Write();

	//--- Add To Running Object Table --- (for graphmgr.exe)
	if (g_pData->settings.application.addToROT)
	{
		if (FAILED(hr = AddToRot(m_piGraphBuilder, &m_rotEntry)))
		{
			//TODO: release graphbuilder
			return (log << "Failed adding graph to ROT\n").Write();
		}
	}

	//--- Get InterfacesInFilters ---
	if (FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_piMediaControl.p)))
		return (log << "Failed to get Media Control interface\n").Write();

//	m_pOverlayCallback = new OverlayCallback(g_pData->hWnd, &hr);
	m_bInitialised = TRUE;

	return TRUE;
}

BOOL DWGraph::Destroy()
{
	if (m_piMediaControl)
		m_piMediaControl.Release();

	if (m_rotEntry)
	{
		RemoveFromRot(m_rotEntry);
		m_rotEntry = 0;
	}

	if (m_piGraphBuilder)
		m_piGraphBuilder.Release();

	m_bInitialised = FALSE;

	return TRUE;
}

HRESULT DWGraph::QueryGraphBuilder(IGraphBuilder** piGraphBuilder)
{
	if (!m_piGraphBuilder)
		return S_FALSE;
	return m_piGraphBuilder->QueryInterface(IID_IGraphBuilder, (void **)piGraphBuilder);
}
	
HRESULT DWGraph::QueryMediaControl(IMediaControl** piMediaControl)
{
	if (!m_piMediaControl)
		return S_FALSE;
	return m_piMediaControl->QueryInterface(IID_IMediaControl, (void **)piMediaControl);
}
	
HRESULT DWGraph::Start()
{
	HRESULT hr;

	//Set the video renderer to use our window.
	IVideoWindow *piVideoWindow;
	if (SUCCEEDED(hr = m_piGraphBuilder->QueryInterface(IID_IVideoWindow, (void **)&piVideoWindow)))
	{
		if (FAILED(hr = piVideoWindow->put_Owner((OAHWND)g_pData->hWnd)))
			return (log << "could not set IVideoWindow Window Handle\n").Write(hr);

		if (FAILED(hr = piVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS)))
			return (log << "could not set IVideoWindow Window Style\n").Write(hr);

		if (FAILED(hr = piVideoWindow->put_MessageDrain((OAHWND)g_pData->hWnd)))
			return (log << "could not set IVideoWindow Message Drain\n").Write(hr);

		//if (FAILED(hr = piVideoWindow->put_AutoShow(OAFALSE)))
		//	return (log << "could not set IVideoWindow AutoShow\n").Write(hr);

		piVideoWindow->Release();
	}

	RefreshVideoPosition();

/*	IBaseFilter *pfOverlayMixer = NULL;
	hr = GetOverlayMixer(m_piGraphBuilder, &pfOverlayMixer);
	if (hr == S_OK)
	{
		IDDrawExclModeVideo* piDDrawExclModeVideo = NULL;
		hr = pfOverlayMixer->QueryInterface(IID_IDDrawExclModeVideo, (void **)&piDDrawExclModeVideo);
		if (hr == S_OK)
		{
			piDDrawExclModeVideo->SetCallbackInterface(m_pOverlayCallback, 0);
			HelperRelease(piDDrawExclModeVideo);
		}
		HelperRelease(pfOverlayMixer);
	}
*/
	return m_piMediaControl->Run();
}

HRESULT DWGraph::Stop()
{
	return m_piMediaControl->Stop();
}

HRESULT DWGraph::Cleanup()
{
	HRESULT hr;
/*	CComQIPtr<IFilterChain> piFilterChain(m_piGraphBuilder);
	if (piFilterChain == NULL)
		return (log << "Failed to query IFilterChain\n").Write(E_NOINTERFACE);

	piFilterChain->RemoveChain(filter, NULL);*/

	hr = DisconnectAllPins(m_piGraphBuilder);
	hr = RemoveAllFilters(m_piGraphBuilder);

	return S_OK;
}

HRESULT DWGraph::RenderPin(IPin *piPin)
{
	if (!piPin)
		return E_INVALIDARG;

	HRESULT hr;

	CComPtr<IEnumMediaTypes> piMediaTypes;
	if (FAILED(hr = piPin->EnumMediaTypes(&piMediaTypes)))
	{
		return (log << "Failed to enum media types\n").Write(hr);
	}

	AM_MEDIA_TYPE *mediaType;
	while (piMediaTypes->Next(1, &mediaType, 0) == NOERROR)
	{
		if ((mediaType->majortype  == KSDATAFORMAT_TYPE_VIDEO) &&
			(mediaType->subtype    == MEDIASUBTYPE_MPEG2_VIDEO) &&
			(mediaType->formattype == FORMAT_MPEG2Video))
		{
			//TODO: Use mpeg2 video definition from config file
			if (SUCCEEDED(hr = m_piGraphBuilder->Render(piPin)))
			{
				return hr;
			}
		}
		else
		if ((mediaType->majortype == MEDIATYPE_Audio) &&
			(mediaType->subtype    == MEDIASUBTYPE_MPEG1AudioPayload) &&
			(mediaType->formattype == FORMAT_WaveFormatEx))
		{
			//TODO: Use mpeg audio definition from config file
			if (SUCCEEDED(hr = m_piGraphBuilder->Render(piPin)))
			{
				return hr;
			}
		}
		else
		if ((mediaType->majortype == MEDIATYPE_Audio) &&
			(mediaType->subtype    == MEDIASUBTYPE_DOLBY_AC3) &&
			(mediaType->formattype == FORMAT_WaveFormatEx))
		{
			//TODO: Use ac3 audio definition from config file
			if (SUCCEEDED(hr = m_piGraphBuilder->Render(piPin)))
			{
				return hr;
			}
		}
		else
		if ((mediaType->majortype  == KSDATAFORMAT_TYPE_MPEG2_SECTIONS) &&
			(mediaType->subtype    == KSDATAFORMAT_SUBTYPE_NONE) &&
			(mediaType->formattype == KSDATAFORMAT_SPECIFIER_NONE))
		{
			//TODO: Use teletext definition from config file
			return S_OK;
		}
	}
	piMediaTypes.Release();

	(log << "Not a recognised media type. Trying to render.\n").Write();
	hr = m_piGraphBuilder->Render(piPin);
	return hr;
}

HRESULT DWGraph::RefreshVideoPosition()
{
//	if (!m_bInitialised || (m_pAppData->bPlaying == FALSE))
//		return FALSE;
//	if (!m_bVideo)
//		return TRUE;
	HRESULT hr = S_OK;

	CComPtr<IVideoWindow> piVideoWindow;
	if (FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IVideoWindow, (void **)&piVideoWindow)))
		return (log << "Could not query graph builder for IVideoWindow\n").Write(S_FALSE);

	CComPtr<IBasicVideo> piBasicVideo;
	if (FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IBasicVideo, (void **)&piBasicVideo)))
	{
		piVideoWindow.Release();
		return (log << "could not query graph builder for IBasicVideo\n").Write(S_FALSE);
	}

	RECT mainRect, zoomRect;
	GetVideoRect(&zoomRect);
	GetClientRect(g_pData->hWnd, &mainRect);
	
	//RECT srcRect;
	//srcRect.left = 0; srcRect.top = 0;
	//hr = piBasicVideo->GetVideoSize(&srcRect.right, &srcRect.bottom);
	//if (hr == S_OK) hr = piBasicVideo->SetSourcePosition(srcRect.left, srcRect.top, srcRect.right-srcRect.left, srcRect.bottom-srcRect.top);

	//SetWindowPos(g_pData->hWnd, NULL, mainRect.left, mainRect.top, mainRect.right-mainRect.left, mainRect.bottom-mainRect.top, /*SWP_NOMOVE | */SWP_NOZORDER);
	if (FAILED(hr = piVideoWindow->SetWindowPosition(0, 0, mainRect.right-mainRect.left, mainRect.bottom-mainRect.top)))
		return (log << "could not set IVideoWindow position\n").Write(hr);

	if (hr == S_OK)
		hr = piBasicVideo->SetDestinationPosition(zoomRect.left, zoomRect.top, zoomRect.right-zoomRect.left, zoomRect.bottom-zoomRect.top);

	//Set overlay to streched AR mode
	IPin* piPin = NULL;
	hr = GetOverlayMixerInputPin(m_piGraphBuilder, L"Input0", &piPin);
	if (hr == S_OK)
	{
		IMixerPinConfig* piMixerPinConfig = NULL;
		hr = piPin->QueryInterface(IID_IMixerPinConfig, (void **)&piMixerPinConfig);
		if (hr == S_OK)
		{
			hr = piMixerPinConfig->SetAspectRatioMode(AM_ARMODE_STRETCHED);
			HelperRelease(piMixerPinConfig);
		}
		HelperRelease(piPin);
	}

	if (FAILED(hr = piVideoWindow->put_Visible(OATRUE)))
		return (log << "could not set IVideoWindow visible\n").Write(hr);

	Sleep(10);

	return TRUE;
}

void DWGraph::GetVideoRect(RECT *rect)
{
	GetClientRect(g_pData->hWnd, rect);

	double ar   = 0;
	if (g_pData->values.display.aspectRatio.height > 0)
		ar = g_pData->values.display.aspectRatio.width / g_pData->values.display.aspectRatio.height;
	double zoom = g_pData->values.display.zoom / 100.0;
	
	if (ar >= 0)
	{
		long windowWidth  = (rect->right  - rect->left);
		long windowHeight = (rect->bottom - rect->top );
		long newWidth, newHeight;

		switch (g_pData->values.display.zoomMode)
		{
		case 1:
			{
				newWidth  = long(windowHeight * ar * zoom);
				newHeight = long(windowHeight * zoom);

				rect->left   = long((windowWidth  - newWidth ) / 2);
				rect->top    = long((windowHeight - newHeight) / 2);
				rect->right  = rect->left + newWidth;
				rect->bottom = rect->top  + newHeight;
			}
			break;
		default:
			{
				if ((windowWidth/(double)windowHeight) < ar)
				{
					newWidth  = long(windowWidth * zoom);
					newHeight = long(windowWidth * (1.0 / ar) * zoom);

					rect->left   = long((windowWidth  - newWidth ) / 2);
					rect->top    = long((windowHeight - newHeight) / 2);
					rect->right  = rect->left + newWidth;
					rect->bottom = rect->top  + newHeight;
				}
				else
				{
					newWidth  = long(windowHeight * ar * zoom);
					newHeight = long(windowHeight * zoom);

					rect->left   = long((windowWidth  - newWidth ) / 2);
					rect->top    = long((windowHeight - newHeight) / 2);
					rect->right  = rect->left + newWidth;
					rect->bottom = rect->top  + newHeight;
				}
			}
			break;
		}
	}
}

