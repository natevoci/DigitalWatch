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
#include "DWOnScreenDisplay.h"

#include <math.h>
#include <mpconfig.h>
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include "bdamedia.h"


HRESULT GetProcAmpControlValue(float *controlValue, IVMRMixerControl9 *piMixerControl, VMR9ProcAmpControlFlags dwProperty, long value, long minValue, long maxValue, long defaultValue)
{
	HRESULT hr;

	VMR9ProcAmpControlRange controlRange;
	memset(&controlRange, 0, sizeof(VMR9ProcAmpControlRange));

	controlRange.dwSize = sizeof(VMR9ProcAmpControlRange);
	controlRange.dwProperty = dwProperty;

	hr = piMixerControl->GetProcAmpControlRange(0, &controlRange);
	if (hr != S_OK)
		return hr;

	float result;
	float perc;

	if (value < defaultValue)
	{
		result = controlRange.MinValue;
		perc = (value-minValue) / (float)(defaultValue-minValue);
		result += (controlRange.DefaultValue - controlRange.MinValue) * perc;
	}
	else if (value > defaultValue)
	{
		result = controlRange.DefaultValue;
		perc = (value-defaultValue) / (float)(maxValue-defaultValue);
		result += (controlRange.MaxValue - controlRange.DefaultValue) * perc;
	}
	else
		result = controlRange.DefaultValue;
	*controlValue = result;
	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// DWGraph
//////////////////////////////////////////////////////////////////////

DWGraph::DWGraph()
{
	m_piGraphBuilder = NULL;
	m_piMediaControl = NULL;
	m_bInitialised = FALSE;
	m_bPlaying = FALSE;
	m_bPaused = FALSE;
	SetRect(&m_videoRect, 0, 0, 0, 0);
	m_rotEntry = 0;
}

DWGraph::~DWGraph()
{
	Destroy();
}

void DWGraph::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	m_mediaTypes.SetLogCallback(callback);
	m_decoders.SetLogCallback(callback);
	graphTools.SetLogCallback(callback);
	m_resumeList.SetLogCallback(callback);
	m_multicastList.SetLogCallback(callback);
}

BOOL DWGraph::SaveSettings()
{
	(log << "Saving Settings DWGraph\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%sResume.xml", g_pData->application.appPath);
	if FAILED(hr = m_resumeList.Save((LPWSTR)&file))
		(log << "Failed to load Resume List: " << hr << "\n").Write();

	swprintf((LPWSTR)&file, L"%sMulticast.xml", g_pData->application.appPath);
	if FAILED(hr = m_multicastList.Save((LPWSTR)&file))
		(log << "Failed to load Multicast List: " << hr << "\n").Write();

	indent.Release();
	(log << "Finished Saving Settings DWGraph\n").Write();
	return TRUE;
}

long DWGraph::GetResumePosition(LPWSTR name)
{
	if (!name)
		return 0;

	int index = 0;
	if SUCCEEDED(m_resumeList.FindListItem(name, &index))
	{
		LPWSTR lpwTemp = NULL;
		strCopy(lpwTemp, L"resumeinfo.resume");
		LPWSTR lpwPosition = m_resumeList.GetListItem(lpwTemp, index);
		delete[] lpwTemp;
		if (lpwPosition)
			return _wtol(lpwPosition);
	}

	return 0;
}

void DWGraph::SetResumePosition(LPWSTR name, long lPos)
{
	LPWSTR lpwPosition = NULL;
	strCopy(lpwPosition, lPos);
	m_resumeList.SetListItem(name, lpwPosition);
}

BOOL DWGraph::Initialise()
{
	(log << "Initialising DWGraph\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;
	if (m_bInitialised)
		return (log << "DigitalWatch graph tried to initialise a second time\n").Write();

	wchar_t file[MAX_PATH];

	swprintf((LPWSTR)&file, L"%sDecoders.xml", g_pData->application.appPath);
	if FAILED(hr = m_decoders.Load((LPWSTR)&file))
		return (log << "Failed to load decoders: " << hr << "\n").Write(hr);

	g_pOSD->Data()->AddList(&m_decoders);

	m_mediaTypes.SetDecoders(&m_decoders);
	m_mediaTypes.Initialise(m_piGraphBuilder, g_pData);
	swprintf((LPWSTR)&file, L"%sMediaTypes.xml", g_pData->application.appPath);
	if FAILED(hr = m_mediaTypes.Load((LPWSTR)&file))
		return (log << "Failed to load mediatypes: " << hr << "\n").Write(hr);

	g_pOSD->Data()->AddList(&m_mediaTypes);

	m_resumeList.Initialise(g_pData->settings.application.resumesize);
	swprintf((LPWSTR)&file, L"%sResume.xml", g_pData->application.appPath);
	if FAILED(hr = m_resumeList.Load((LPWSTR)&file))
		return (log << "Failed to load Resume List: " << hr << "\n").Write(hr);

	g_pOSD->Data()->AddList(&m_resumeList);

	m_multicastList.Initialise(g_pData->settings.application.resumesize);
	swprintf((LPWSTR)&file, L"%sMulticast.xml", g_pData->application.appPath);
	if FAILED(hr = m_multicastList.Load((LPWSTR)&file))
		return (log << "Failed to load Multicast List: " << hr << "\n").Write(hr);

	g_pOSD->Data()->AddList(&m_multicastList);

	//--- COM should already be initialized ---

	//--- Create Graph ---
	if FAILED(hr = m_piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER))
		return (log << "Failed Creating Graph Builder: " << hr << "\n").Write();

	//--- Add To Running Object Table --- (for graphmgr.exe)
	if (g_pData->settings.application.addToROT)
	{
		if FAILED(hr = graphTools.AddToRot(m_piGraphBuilder, &m_rotEntry))
		{
			return (log << "Failed adding graph to ROT: " << hr << "\n").Write();
		}
	}

	//--- Get InterfacesInFilters ---
	if FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&m_piMediaControl)))
		return (log << "Failed to get Media Control interface: " << hr << "\n").Write();

	m_bInitialised = TRUE;

	indent.Release();
	(log << "Finished Initialising DWGraph\n").Write();

	return TRUE;
}

BOOL DWGraph::Destroy()
{
	if (m_piMediaControl)
		m_piMediaControl.Release();

	if (m_rotEntry)
	{
		graphTools.RemoveFromRot(m_rotEntry);
		m_rotEntry = 0;
	}

	m_multicastList.Destroy();
	m_resumeList.Destroy();
	m_decoders.Destroy();
	m_mediaTypes.Destroy();

	if (m_piGraphBuilder)
		m_piGraphBuilder.Release();

	m_bInitialised = FALSE;

	return TRUE;
}

HRESULT DWGraph::QueryGraphBuilder(IGraphBuilder** piGraphBuilder)
{
	if (!m_piGraphBuilder)
		return E_POINTER;
	*piGraphBuilder = m_piGraphBuilder;
	(*piGraphBuilder)->AddRef();
	return S_OK;
}
	
HRESULT DWGraph::QueryMediaControl(IMediaControl** piMediaControl)
{
	if (!m_piMediaControl)
		return E_POINTER;
	*piMediaControl = m_piMediaControl;
	(*piMediaControl)->AddRef();
	return S_OK;
}

HRESULT DWGraph::Start()
{
	HRESULT hr;

	if FAILED(hr = Start(m_piGraphBuilder, FALSE))
		return hr;

	m_bPlaying = TRUE;
	m_bPaused = FALSE;


	return hr;
}

HRESULT DWGraph::Start(IGraphBuilder *piGraphBuilder, BOOL bSink)
{
	(log << "Starting DW Graph\n").Write();
	LogMessageIndent indent(&log);

	if (!piGraphBuilder)
		return (log << "Sink Graph Builder interface is NULL\n").Write(E_POINTER);

	ASSERT(piGraphBuilder != NULL);

	HRESULT hr;

	CComPtr<IMediaControl> piMediaControl;
	if FAILED(hr = piGraphBuilder->QueryInterface(&piMediaControl))
		return (log << "Failed to get Graph media control: " << hr << "\n").Write(hr);

	if (!bSink)
	{
		hr = InitialiseVideoPosition(piGraphBuilder);
		if FAILED(hr)
			return (log << "Failed to Initialise Video Rendering: " << hr << "\n").Write(hr);
	}

	//Start the graph
	hr = piMediaControl->Run();
	if FAILED(hr)
		return (log << "Failed to start graph: " << hr << "\n").Write(hr);

	if (!bSink)
	{
		hr = ApplyColorControls(piGraphBuilder);
		hr = SetVolume(piGraphBuilder, g_pData->values.audio.volume);
		hr = Mute(piGraphBuilder, g_pData->values.audio.bMute);
	}

	//Log the reference clock
	do
	{
		CComPtr<IReferenceClock> piRefClock;
		CComQIPtr<IMediaFilter> piMediaFilter(piGraphBuilder);
		if (!piMediaFilter)
		{
			(log << "Failed to get IMediaFilter interface from graph: " << hr << "\n").Write();
			break;
		}

		if FAILED(hr = piMediaFilter->GetSyncSource(&piRefClock))
		{
			(log << "Failed to get reference clock: " << hr << "\n").Write();
			break;
		}

		if (!piRefClock)
		{
			(log << "Reference Clock is not set\n").Write();
			break;
		}
		
		CComQIPtr<IBaseFilter> piFilter(piRefClock);
		if (!piFilter)
		{
			(log << "Failed to get IBaseFilter interface from reference clock\n").Write();
			break;
		}

		FILTER_INFO filterInfo;
		if SUCCEEDED(hr = piFilter->QueryFilterInfo(&filterInfo))
		{
			(log << "Reference Clock is \"" << filterInfo.achName << "\"\n").Write();
			if (filterInfo.pGraph)
				filterInfo.pGraph->Release();
		}
		else
			(log << "Failed to get filter info: " << hr << "\n").Write();

		break;

	} while (FALSE);

	indent.Release();
	(log << "Finished Starting DW Graph : " << hr << "\n").Write();

	return hr;
}

HRESULT DWGraph::Stop()
{
	m_bPlaying = FALSE;
	m_bPaused = FALSE;

	HRESULT hr;

	if FAILED(hr = Stop(m_piGraphBuilder))
		return hr;

(log << "Reset renderer method\n").Write();
LogMessageIndent indent(&log);

		//Reset renderer method
	if (g_pOSD)
		g_pOSD->SetRenderMethod(RENDER_METHOD_DEFAULT);

indent.Release();
(log << "Finished Reset renderer method : " << hr << "\n").Write();


	return hr;
}

HRESULT DWGraph::Stop(IGraphBuilder *piGraphBuilder)
{
	(log << "Stopping DW Graph\n").Write();
	LogMessageIndent indent(&log);

	if (!piGraphBuilder)
		return (log << "Graph Builder interface is NULL\n").Write(E_POINTER);

//(log << "got piGraphBuilder ok: " << (int)piGraphBuilder << "\n").Write();
	ASSERT(piGraphBuilder != NULL);
//(log << "got ASSERT piGraphBuilder ok: " << (int)piGraphBuilder << "\n").Write();

	HRESULT hr;

	CComPtr<IMediaControl> piMediaControl;
	if FAILED(hr = piGraphBuilder->QueryInterface(&piMediaControl))
		return (log << "Failed to get Graph media control: " << hr << "\n").Write(hr);

//(log << "got the Graph media control: " << (int)piMediaControl << "\n").Write();

	if FAILED(hr = piMediaControl->Stop())
		return (log << "Failed to stop graph: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Stopping DW Graph : " << hr << "\n").Write();

	return hr;
}

HRESULT DWGraph::Pause(BOOL bPause)
{
	HRESULT hr;
	LPWSTR pStr;
	if (bPause)
		pStr = L"Pause";
	else
		pStr = L"Unpause";

	if (m_bPaused == bPause)
		return (log << "Graph already in " << pStr << " state\n").Write();

	(log << pStr << " DW Graph\n").Write();
	LogMessageIndent indent(&log);

	m_bPaused = bPause;

	if (m_bPaused)
		hr = m_piMediaControl->Pause();
	else
		hr = m_piMediaControl->Run();

	if FAILED(hr)
		return (log << "Failed to " << pStr << " graph: " << hr << "\n").Write(hr);

	indent.Release();

	return hr;
}

HRESULT DWGraph::Pause(IGraphBuilder *piGraphBuilder, BOOL bSink)
{
	(log << "Pausing DW Graph\n").Write();
	LogMessageIndent indent(&log);

	if (!piGraphBuilder)
		return (log << "Graph Builder interface is NULL\n").Write(E_POINTER);

	ASSERT(piGraphBuilder);

	HRESULT hr;

	if (!bSink)
	{
		hr = InitialiseVideoPosition(piGraphBuilder);
		if FAILED(hr)
			return (log << "Failed to Initialise Video Rendering: " << hr << "\n").Write(hr);
	}

	CComPtr<IMediaControl> piMediaControl;
	if FAILED(hr = piGraphBuilder->QueryInterface(&piMediaControl))
		return (log << "Failed to get Graph media control: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaControl->Pause())
		return (log << "Failed to Pause graph: " << hr << "\n").Write(hr);

	if (!bSink)
	{
		hr = ApplyColorControls(piGraphBuilder);
		hr = SetVolume(piGraphBuilder, g_pData->values.audio.volume);
		hr = Mute(piGraphBuilder, g_pData->values.audio.bMute);
	}

	indent.Release();
	(log << "Finished Pausing DW Graph : " << hr << "\n").Write();

	return hr;
}

HRESULT DWGraph::Cleanup()
{
	return Cleanup(m_piGraphBuilder);
}

HRESULT DWGraph::Cleanup(IGraphBuilder *piGraphBuilder)
{
	(log << "Cleaning up DW Graph\n").Write();
	LogMessageIndent indent(&log);

	if (!piGraphBuilder)
		return (log << "Graph Builder interface is NULL\n").Write(E_POINTER);

	HRESULT hr;

	hr = graphTools.DisconnectAllPins(piGraphBuilder);
	if FAILED(hr)
		(log << "Failed to disconnect pins: " << hr << "\n").Write(hr);

	hr = graphTools.RemoveAllFilters(piGraphBuilder);
	if FAILED(hr)
		(log << "Failed to remove filters: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Cleaning up DW Graph\n").Write();

	return S_OK;
}

HRESULT DWGraph::RenderPin(IPin *piPin)
{
	return RenderPin(m_piGraphBuilder, piPin);
}

HRESULT DWGraph::RenderPin(IGraphBuilder *piGraphBuilder, IPin *piPin)
{
	if (!piPin)
		return E_INVALIDARG;

	HRESULT hr;

	CComPtr <IEnumMediaTypes> piMediaTypes;
	if FAILED(hr = piPin->EnumMediaTypes(&piMediaTypes))
	{
		return (log << "Failed to enum media types: " << hr << "\n").Write(hr);
	}

	hr = S_FALSE;
	AM_MEDIA_TYPE *mediaType;
	while (piMediaTypes->Next(1, &mediaType, 0) == NOERROR)
	{
		DWMediaType *dwMediaType = m_mediaTypes.FindMediaType(mediaType);
		if (dwMediaType == NULL)
			continue;

		DWDecoder *dwDecoder = dwMediaType->GetDecoder();
		if (dwDecoder == NULL)
			continue;

		(log << "Rendering stream of type \"" << dwMediaType->name << "\" with decoder \"" << dwDecoder->Name() << "\"\n").Write();

		if SUCCEEDED(hr = dwDecoder->AddFilters(piGraphBuilder, piPin))
		{
			break;
		}
	}

	piMediaTypes.Release();

	return hr;
}

HRESULT DWGraph::InitialiseVideoPosition()
{
	return InitialiseVideoPosition(m_piGraphBuilder);
}

HRESULT DWGraph::InitialiseVideoPosition(IGraphBuilder *piGraphBuilder)
{
	HRESULT hr = S_OK;

	if (!piGraphBuilder)
		return (log << "IGraphBuilder doesn't exist yet\n").Write(E_POINTER);
	if (!g_pOSD)
		return (log << "OSD is gone!\n").Write(E_POINTER);

	RENDER_METHOD renderMethod = g_pOSD->GetRenderMethod();

	if ((renderMethod == RENDER_METHOD_OverlayMixer) ||
		(renderMethod == RENDER_METHOD_VMR7) ||
		(renderMethod == RENDER_METHOD_VMR9))
	{
		//Set the video renderer to use our window.
		CComQIPtr <IVideoWindow> piVideoWindow(piGraphBuilder);
		if (piVideoWindow)
		{
			if FAILED(hr = piVideoWindow->put_Owner((OAHWND)g_pData->hWnd))
				return (log << "could not set IVideoWindow Window Handle: " << hr << "\n").Write(hr);

			if FAILED(hr = piVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS))
				return (log << "could not set IVideoWindow Window Style: " << hr << "\n").Write(hr);

			if FAILED(hr = piVideoWindow->put_MessageDrain((OAHWND)g_pData->hWnd))
				return (log << "could not set IVideoWindow Message Drain: " << hr << "\n").Write(hr);

			//if FAILED(hr = piVideoWindow->put_AutoShow(OAFALSE))
			//	return (log << "could not set IVideoWindow AutoShow: " << hr << "\n").Write(hr);
		}
	}

	if (renderMethod == RENDER_METHOD_OverlayMixer)
	{
		//Set overlay to streched AR mode
		CComPtr <IPin> piPin;
		hr = graphTools.GetOverlayMixerInputPin(piGraphBuilder, L"Input0", &piPin);
		if (hr == S_OK)
		{
			CComPtr <IMixerPinConfig> piMixerPinConfig;
			hr = piPin->QueryInterface(IID_IMixerPinConfig, reinterpret_cast<void**>(&piMixerPinConfig));
			if (hr == S_OK)
			{
				hr = piMixerPinConfig->SetAspectRatioMode(AM_ARMODE_STRETCHED);
			}
		}
		else
		{
			(log << "Error: Failed to find input pin of overlay mixer: " << hr << "\n").Write();
		}
	}

	return RefreshVideoPosition(piGraphBuilder);
}

HRESULT DWGraph::RefreshVideoPosition()
{
	return RefreshVideoPosition(m_piGraphBuilder);
}

HRESULT DWGraph::RefreshVideoPosition(IGraphBuilder *piGraphBuilder)
{
	HRESULT hr = S_OK;

	double aspectRatio = 0;

	if (!piGraphBuilder)
		return (log << "IGraphBuilder doesn't exist yet").Write(S_FALSE);
	if (!g_pOSD)
		return (log << "OSD is gone!\n").Write(E_POINTER);

	RENDER_METHOD renderMethod = g_pOSD->GetRenderMethod();

	if (renderMethod == RENDER_METHOD_VMR9Windowless)
	{
		CComPtr <IBaseFilter> piVMR9Filter;
		hr = graphTools.FindFilterByCLSID(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter);
		if (hr == S_OK)
		{
			// Fill client area of our window with VMR9
			RECT clientRect;
			GetClientRect(g_pData->hWnd, &clientRect);

			CComQIPtr<IVMRWindowlessControl9> piWindowlessControl(piVMR9Filter);
			if (piWindowlessControl == NULL)
				return (log << "Error: Failed to get IVMRWindowlessControl9 interface\n").Show(E_NOINTERFACE);

			hr = piWindowlessControl->SetVideoPosition(NULL, &clientRect);
			if (hr != S_OK)
				return (log << "Error: Failed to set clipping window: " << hr << "\n").Show(hr);

			// Position the video within our application window
			long lWidth, lHeight, lARWidth, lARHeight;
			piWindowlessControl->GetNativeVideoSize(&lWidth, &lHeight, &lARWidth, &lARHeight);
			aspectRatio = lARWidth / (double)lARHeight;
			CalculateVideoRect(aspectRatio);

			CComQIPtr<IVMRMixerControl9> piMixerControl(piVMR9Filter);
			if (piMixerControl == NULL)
				return (log << "Error: Failed to get IVMRMixerControl9 interface\n").Show(E_NOINTERFACE);

			VMR9NormalizedRect zoomRect;
			lWidth = (clientRect.right - clientRect.left);
			lHeight = (clientRect.bottom - clientRect.top);
			zoomRect.left   = m_videoRect.left   / (float)lWidth;
			zoomRect.top    = m_videoRect.top    / (float)lHeight;
			zoomRect.right  = m_videoRect.right  / (float)lWidth;
			zoomRect.bottom = m_videoRect.bottom / (float)lHeight;
			hr = piMixerControl->SetOutputRect(0, &zoomRect);
			if (hr != S_OK)
				return (log << "Error: Failed to set output rectangle: " << hr << "\n").Show(hr);
		}
	}
	else if (renderMethod == RENDER_METHOD_VMR9Renderless)
	{
	}
	else if (renderMethod == RENDER_METHOD_OverlayMixer)
	{
		CComQIPtr <IVideoWindow> piVideoWindow(piGraphBuilder);
		if (!piVideoWindow)
			return (log << "Could not query graph builder for IVideoWindow\n").Write(S_FALSE);

		CComQIPtr <IBasicVideo> piBasicVideo(piGraphBuilder);
		if (!piBasicVideo)
			return (log << "could not query graph builder for IBasicVideo\n").Write(S_FALSE);

		CComQIPtr <IBasicVideo2> piBasicVideo2(piGraphBuilder);
		if (piBasicVideo2)
		{
			long lARWidth, lARHeight;
			hr = piBasicVideo2->GetPreferredAspectRatio(&lARWidth, &lARHeight);
			if (hr == S_OK)
				aspectRatio = lARWidth / (double)lARHeight;
		}
		else
			(log << "could not query graph builder for IBasicVideo2\n").Write();

		RECT mainRect;
		GetClientRect(g_pData->hWnd, &mainRect);
		if FAILED(hr = piVideoWindow->SetWindowPosition(0, 0, mainRect.right-mainRect.left, mainRect.bottom-mainRect.top))
			return (log << "could not set IVideoWindow position: " << hr << "\n").Write(hr);

		CalculateVideoRect(aspectRatio);
		if FAILED(hr = piBasicVideo->SetDestinationPosition(m_videoRect.left, m_videoRect.top, m_videoRect.right-m_videoRect.left, m_videoRect.bottom-m_videoRect.top))
			return (log << "count not set IBasicVideo destination position: " << hr << "\n").Write(hr);
	}

	return S_OK;
}

void DWGraph::CalculateVideoRect(double aspectRatio/* = 0*/)
{
	GetClientRect(g_pData->hWnd, &m_videoRect);

	double ar = aspectRatio;
	if ((aspectRatio == 0) || g_pData->values.video.aspectRatio.bOverride)
		if (g_pData->values.video.aspectRatio.height > 0)
			ar = g_pData->values.video.aspectRatio.width / g_pData->values.video.aspectRatio.height;

	double zoom = g_pData->values.video.zoom / 100.0;
	
	if (ar >= 0)
	{
		long windowWidth  = (m_videoRect.right  - m_videoRect.left);
		long windowHeight = (m_videoRect.bottom - m_videoRect.top );
		long newWidth, newHeight;

		switch (g_pData->values.video.zoomMode)
		{
		case 1:
			{
				newWidth  = long(windowHeight * ar * zoom);
				newHeight = long(windowHeight * zoom);

				m_videoRect.left   = long((windowWidth  - newWidth ) / 2);
				m_videoRect.top    = long((windowHeight - newHeight) / 2);
				m_videoRect.right  = m_videoRect.left + newWidth;
				m_videoRect.bottom = m_videoRect.top  + newHeight;
			}
			break;
		default:
			{
				if ((windowWidth/(double)windowHeight) < ar)
				{
					newWidth  = long(windowWidth * zoom);
					newHeight = long(windowWidth * (1.0 / ar) * zoom);

					m_videoRect.left   = long((windowWidth  - newWidth ) / 2);
					m_videoRect.top    = long((windowHeight - newHeight) / 2);
					m_videoRect.right  = m_videoRect.left + newWidth;
					m_videoRect.bottom = m_videoRect.top  + newHeight;
				}
				else
				{
					newWidth  = long(windowHeight * ar * zoom);
					newHeight = long(windowHeight * zoom);

					m_videoRect.left   = long((windowWidth  - newWidth ) / 2);
					m_videoRect.top    = long((windowHeight - newHeight) / 2);
					m_videoRect.right  = m_videoRect.left + newWidth;
					m_videoRect.bottom = m_videoRect.top  + newHeight;
				}
			}
			break;
		}
	}
}

HRESULT DWGraph::GetVolume(long &volume)
{
	return GetVolume(m_piGraphBuilder, volume);
}

HRESULT DWGraph::GetVolume(IGraphBuilder *piGraphBuilder, long &volume)
{
	HRESULT hr = S_OK;

	volume = 0;
	CComQIPtr <IBasicAudio> piBasicAudio(piGraphBuilder);
	if (!piBasicAudio)
		return (log << "could not query graph builder for IBasicAudio\n").Write(E_FAIL);

	hr = piBasicAudio->get_Volume(&volume);
	if FAILED(hr)
		return (log << "Failed to retrieve volume: " << hr << "\n").Write(hr);

	volume = (int)sqrt((double)-volume);
	volume = 100 - volume;
	g_pData->values.audio.volume = volume;
	return S_OK;
}

HRESULT DWGraph::SetVolume(long volume)
{
	return SetVolume(m_piGraphBuilder, volume);
}

HRESULT DWGraph::SetVolume(IGraphBuilder *piGraphBuilder, long volume)
{
	if (volume < 0)
		volume = 0;
	if (volume > 100)
		volume = 100;
	HRESULT hr = S_OK;
	CComQIPtr <IBasicAudio> piBasicAudio(piGraphBuilder);
	if (!piBasicAudio)
		return (log << "could not query graph builder for IBasicAudio\n").Write(E_FAIL);

	g_pData->values.audio.volume = volume;
	volume -= 100;
	volume *= volume;
	volume = -volume;
	hr = piBasicAudio->put_Volume(volume);
	if FAILED(hr)
		return (log << "Failed to set volume: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWGraph::Mute(BOOL bMute)
{
	return Mute(m_piGraphBuilder, bMute);
}

HRESULT DWGraph::Mute(IGraphBuilder *piGraphBuilder, BOOL bMute)
{
	int value = 0;
	if (!bMute)
		value = g_pData->values.audio.volume;
		
	HRESULT hr = S_OK;
	CComQIPtr <IBasicAudio> piBasicAudio(piGraphBuilder);
	if (!piBasicAudio)
		return (log << "could not query graph builder for IBasicAudio\n").Write(E_FAIL);

	value -= 100;
	value *= value;
	value = -value;
	hr = piBasicAudio->put_Volume(value);
	if FAILED(hr)
		return (log << "Failed to set volume: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWGraph::SetColorControls(int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma)
{
	return SetColorControls(m_piGraphBuilder, nBrightness, nContrast, nHue, nSaturation, nGamma);
}

HRESULT DWGraph::SetColorControls(IGraphBuilder *piGraphBuilder, int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma)
{
	g_pData->values.video.overlay.brightness = nBrightness;
	g_pData->values.video.overlay.contrast = nContrast;
	g_pData->values.video.overlay.hue = nHue;
	g_pData->values.video.overlay.saturation = nSaturation;
	g_pData->values.video.overlay.gamma = nGamma;

	return ApplyColorControls(piGraphBuilder);
}

HRESULT DWGraph::ApplyColorControls(IGraphBuilder *piGraphBuilder)
{
	HRESULT hr = S_OK;

	if (!g_pOSD)
		return (log << "OSD is gone!\n").Write(E_POINTER);

	RENDER_METHOD renderMethod = g_pOSD->GetRenderMethod();

	if (renderMethod == RENDER_METHOD_OverlayMixer)
	{
		CComPtr <IPin> piPin;
		hr = graphTools.GetOverlayMixerInputPin(piGraphBuilder, L"Input0", &piPin);
		if (FAILED(hr))
			return hr;

		CComPtr <IMixerPinConfig2> piMixerPinConfig2;
		if FAILED(hr = piPin->QueryInterface(IID_IMixerPinConfig2, (void **)&piMixerPinConfig2))
			return (log << "Failed to get IMixerPinConfig2 interface from pin\n").Write(E_FAIL);

		DDCOLORCONTROL colorControl;
		colorControl.dwSize = sizeof(DDCOLORCONTROL);
		colorControl.dwFlags = DDCOLOR_BRIGHTNESS | DDCOLOR_CONTRAST | DDCOLOR_HUE | DDCOLOR_SATURATION | DDCOLOR_GAMMA;
		colorControl.lBrightness = g_pData->values.video.overlay.brightness;
		colorControl.lContrast = g_pData->values.video.overlay.contrast;
		colorControl.lHue = g_pData->values.video.overlay.hue;
		colorControl.lSaturation = g_pData->values.video.overlay.saturation;
		colorControl.lGamma = g_pData->values.video.overlay.gamma;

		hr = piMixerPinConfig2->SetOverlaySurfaceColorControls(&colorControl);
		if FAILED(hr)
			return (log << "Failed to SetOverlaySurfaceColorControls: " << hr << "\n").Write(hr);
	}
	else if ((renderMethod == RENDER_METHOD_VMR9) || 
			 (renderMethod == RENDER_METHOD_VMR9Windowless) ||
			 (renderMethod == RENDER_METHOD_VMR9Renderless))
	{
		CComPtr <IBaseFilter> piVMR9Filter;
		hr = graphTools.FindFilterByCLSID(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9Filter);
		if (hr == S_OK)
		{
			CComQIPtr<IVMRMixerControl9> piMixerControl(piVMR9Filter);
			if (piMixerControl == NULL)
				return (log << "Error: Failed to get IVMRMixerControl9 interface\n").Show(E_NOINTERFACE);

			VMR9ProcAmpControl control;

			memset(&control, 0, sizeof(VMR9ProcAmpControl));
			control.dwSize = sizeof(VMR9ProcAmpControl);
			hr = piMixerControl->GetProcAmpControl(0, &control);

			if ((control.dwFlags & ProcAmpControl9_Brightness) != 0)
				GetProcAmpControlValue(&control.Brightness, piMixerControl, ProcAmpControl9_Brightness, g_pData->values.video.overlay.brightness,    0, 10000,   750);
			if ((control.dwFlags & ProcAmpControl9_Contrast)   != 0)
				GetProcAmpControlValue(&control.Contrast  , piMixerControl, ProcAmpControl9_Contrast  , g_pData->values.video.overlay.contrast  ,    0, 20000, 10000);
			if ((control.dwFlags & ProcAmpControl9_Hue)        != 0)
				GetProcAmpControlValue(&control.Hue       , piMixerControl, ProcAmpControl9_Hue       , g_pData->values.video.overlay.hue       , -180,   180,     0);
			if ((control.dwFlags & ProcAmpControl9_Saturation) != 0)
				GetProcAmpControlValue(&control.Saturation, piMixerControl, ProcAmpControl9_Saturation, g_pData->values.video.overlay.saturation,    0, 20000, 10000);

			hr = piMixerControl->SetProcAmpControl(0, &control);
			if (hr != S_OK)
				return (log << "Error: Failed to set proc amp controls: " << hr << "\n").Write(hr);
		}
	}

	return S_OK;
}

void DWGraph::GetVideoRect(RECT *rect)
{
	CopyRect(rect, &m_videoRect);
}

BOOL DWGraph::IsPlaying()
{
	return m_bPlaying;
}

BOOL DWGraph::IsPaused()
{
	return m_bPaused;
}

LPWSTR DWGraph::GetMediaTypeDecoder(int index)
{
	return m_mediaTypes.GetMediaTypeDecoder(index);
}

HRESULT DWGraph::SetMediaTypeDecoder(int index, LPWSTR decoderName, BOOL bKeep)
{
	return m_mediaTypes.SetMediaTypeDecoder(index, decoderName, bKeep);
}

