/**
 *	BDADVBTSink.cpp
 *	Copyright (C) 2004 Nate
 *	Copyright (C) 2006 Bear
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


#include "BDADVBTSink.h"
#include "BDADVBTSinkTShift.h"
#include "BDADVBTSinkDSNet.h"
#include "BDADVBTSinkFile.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "dsnetifc.h"
#include "TSFileSinkGuids.h"
#include "Winsock.h"
#include "StreamFormats.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define toIPAddress(a, b, c, d) (a + (b << 8) + (c << 16) + (d << 24))

//////////////////////////////////////////////////////////////////////
// BDADVBTSink
//////////////////////////////////////////////////////////////////////

BDADVBTSink::BDADVBTSink()
{

	m_pDWGraph = NULL;

	m_pCurrentTShiftSink = NULL;
	m_pCurrentFileSink = NULL;
	m_pCurrentDSNetSink = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;

	m_rotEntry = 0;

	m_intTimeShiftType = 0;
	m_intFileSinkType = 0;
	m_intDSNetworkType = 0;
}

BDADVBTSink::~BDADVBTSink()
{
	DestroyAll();
}

void BDADVBTSink::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTSink::Initialise(DWGraph *pDWGraph)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T Sink tried to initialise a second time\n").Write(E_FAIL);

	if (!pDWGraph)
		return (log << "Must pass a valid DWGraph object to Initialise a Sink\n").Write(E_FAIL);

	m_pDWGraph = pDWGraph;

	//--- COM should already be initialized ---

	if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
		return (log << "Failed to get graph: " << hr << "\n").Write(hr);

	if FAILED(hr = m_pDWGraph->QueryMediaControl(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	m_intTimeShiftType = g_pData->values.timeshift.format;
	if(m_intTimeShiftType)
	{
		m_pCurrentTShiftSink = new BDADVBTSinkTShift(this);
		m_pCurrentTShiftSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentTShiftSink->Initialise(m_pDWGraph, m_intTimeShiftType))
			return (log << "Failed to Initalise the TimeShift Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intFileSinkType = g_pData->values.capture.format;
	if(m_intFileSinkType)
	{
		m_pCurrentFileSink = new BDADVBTSinkFile(this);
		m_pCurrentFileSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentFileSink->Initialise(m_pDWGraph, m_intFileSinkType))
			return (log << "Failed to Initalise the File Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intDSNetworkType = g_pData->values.dsnetwork.format;
	if(m_intDSNetworkType)
	{
		m_pCurrentDSNetSink = new BDADVBTSinkDSNet(this);
		m_pCurrentDSNetSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentDSNetSink->Initialise(m_pDWGraph, m_intDSNetworkType))
			return (log << "Failed to Initalise the DSNetwork Sink Filters: " << hr << "\n").Write(hr);
	}

	m_bInitialised = TRUE;
	return S_OK;
}

HRESULT BDADVBTSink::DestroyAll()
{
    HRESULT hr = S_OK;


	if (m_pCurrentTShiftSink)
	{
		m_pCurrentTShiftSink->RemoveSinkFilters();
		delete m_pCurrentTShiftSink;
	}

	if (m_pCurrentFileSink)
	{
		m_pCurrentFileSink->RemoveSinkFilters();
		delete m_pCurrentFileSink;
	}

	if (m_pCurrentDSNetSink)
	{
		m_pCurrentDSNetSink->RemoveSinkFilters();
		delete m_pCurrentDSNetSink;
	}

	m_piMediaControl.Release();
	m_piGraphBuilder.Release();

	return S_OK;
}

HRESULT BDADVBTSink::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;


	//--- Add & connect the Sink filters ---
	if (m_intFileSinkType && m_pCurrentFileSink)
	{
		if FAILED(hr = m_pCurrentFileSink->AddSinkFilters(pService))
			(log << "Failed to add all the File Sink Filters to the graph: " << hr << "\n").Write(hr);
	}

	//--- Now add & connect the TimeShifting filters ---
	if (m_intTimeShiftType && m_pCurrentTShiftSink)
	{
		if FAILED(hr = m_pCurrentTShiftSink->AddSinkFilters(pService))
			(log << "Failed to add all the TimeShift Sink Filters to the graph: " << hr << "\n").Write(hr);
	}

	//--- Now add & connect the DSNetworking filters ---
	if (m_intDSNetworkType && m_pCurrentDSNetSink)
	{
		if FAILED(hr = m_pCurrentDSNetSink->AddSinkFilters(pService))
			(log << "Failed to add all the DSNetworking Sink Filters to the graph: " << hr << "\n").Write(hr);
	}


	m_bActive = TRUE;
	return hr;
}

void BDADVBTSink::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

HRESULT BDADVBTSink::RemoveSinkFilters()
{
	m_bActive = FALSE;
	
	//--- Remove the Sink filters ---
	if (m_intFileSinkType && m_pCurrentFileSink)
		m_pCurrentFileSink->RemoveSinkFilters();

	//--- Remove the TimeShifting filters ---
	if (m_intTimeShiftType && m_pCurrentTShiftSink)
		m_pCurrentTShiftSink->RemoveSinkFilters();

	//--- Remove the DSNetworking filters ---
	if (m_intDSNetworkType && m_pCurrentDSNetSink)
		m_pCurrentDSNetSink->RemoveSinkFilters();

	return S_OK;
}

HRESULT BDADVBTSink::SetTransportStreamPin(IPin* piPin)
{
	if (!piPin)
		return E_FAIL;

	HRESULT hr;
	PIN_INFO pinInfo;
	if FAILED(hr = piPin->QueryPinInfo(&pinInfo))
		return E_FAIL;

	m_piInfinitePinTee = pinInfo.pFilter;

	if(m_pCurrentTShiftSink)
		m_pCurrentTShiftSink->SetTransportStreamPin(piPin);

	if(m_pCurrentFileSink)
		m_pCurrentFileSink->SetTransportStreamPin(piPin);

	if(m_pCurrentDSNetSink)
		m_pCurrentDSNetSink->SetTransportStreamPin(piPin);

	return S_OK;
}

BOOL BDADVBTSink::IsActive()
{
	return m_bActive;
}

HRESULT BDADVBTSink::AddFileName(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intSinkType)
{
	if (pService == NULL)
	{
		(log << "Skipping Add File Name. No service passed.\n").Write();
		return E_INVALIDARG;
	}

	if (pFilter == NULL)
	{
		(log << "Skipping Add File Name. No Sink Filter passed.\n").Write();
		return E_INVALIDARG;
	}

	(log << "Adding The Sink File Name\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	//
	// Get the Sink Filter Interface 
	//
	if (intSinkType)
	{
		if (m_piFileSink)
			m_piFileSink.Release();

		if FAILED(hr = pFilter->QueryInterface(&m_piFileSink))
		{
			(log << "Failed to get the IFileSinkFilter Interface on the Sink Filter.\n").Write(hr);
			return E_FAIL;
		}

		//
		// Add the Date/Time Stamp to the FileName 
		//
		WCHAR wPathName[MAX_PATH] = L""; 
		WCHAR wTimeStamp[MAX_PATH] = L"";
		WCHAR wfileName[MAX_PATH] = L"";
		WCHAR fileName[MAX_PATH] = L"";
		WCHAR wServiceName[MAX_PATH] = L"";

		lstrcpyW(wServiceName, pService->serviceName);
	
		_tzset();
		struct tm *tmTime;
		time_t lTime = timeGetTime();
		time(&lTime);
		tmTime = localtime(&lTime);
		wcsftime(wTimeStamp, 32, L"(%Y-%m-%d %H-%M-%S)", tmTime);


		if ((intSinkType == 1 || intSinkType == 11 || intSinkType == 111) &&
				g_pData->settings.timeshift.folder)
			lstrcpyW(wPathName, g_pData->settings.timeshift.folder);
		else
		{
			lstrcpyW(wPathName, g_pData->settings.capture.folder);
			if (wcslen(g_pData->settings.capture.fileName))
				lstrcpyW(wServiceName, g_pData->settings.capture.fileName);
		}

		if (intSinkType == 1)
			wsprintfW(fileName, L"%S%S %S.full.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 11)
			wsprintfW(fileName, L"%S%S %S.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 111)
			wsprintfW(fileName, L"%S%S %S.mpg.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 2) 
			wsprintfW(fileName, L"%S%S %S.full.ts", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 22) 
			wsprintfW(fileName, L"%S%S %S.ts", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 3) 
			wsprintfW(fileName, L"%S%S %S.mpg", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 4) 
			wsprintfW(fileName, L"%S%S %S.mv2", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 5) 
			wsprintfW(fileName, L"%S%S %S.mp2", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 6)
			wsprintfW(fileName, L"%S%S %S.txt", wPathName, wTimeStamp, wServiceName);

		//
		// Set the Sink Filter File Name 
		//
		if FAILED(hr = m_piFileSink->SetFileName(fileName, NULL))
		{
			(log << "Failed to Set the IFileSinkFilter Interface with a filename.\n").Write(hr);
			return hr;
		}
	}
	else
	{
		(log << "Setting The DSNetwork Configeration\n").Write();
		LogMessageIndent indent(&log);

		//Setup dsnet sender
		IMulticastConfig *piMulticastConfig = NULL;
		if FAILED(hr = pFilter->QueryInterface(IID_IMulticastConfig, reinterpret_cast<void**>(&piMulticastConfig)))
			return (log << "Failed to query Sink filter for IMulticastConfig: " << hr << "\n").Write(hr);

		TCHAR sz[32];
		sprintf(sz,"%S",g_pData->settings.dsnetwork.nicaddr);
		if FAILED(hr = piMulticastConfig->SetNetworkInterface(inet_addr (sz))) //0 == INADDR_ANY
			return (log << "Failed to set network interface for Sink filter: " << hr << "\n").Write(hr);

		sprintf(sz,"%S",g_pData->settings.dsnetwork.ipaddr);
		ULONG ulIP = inet_addr(sz);
		if FAILED(hr = piMulticastConfig->SetMulticastGroup(ulIP, htons((unsigned short)g_pData->settings.dsnetwork.port)))
			return (log << "Failed to set multicast group for Sink filter: " << hr << "\n").Write(hr);
		piMulticastConfig->Release();

		indent.Release();
		(log << "Finished Setting The DSNetwork Configeration\n").Write();
	}

	indent.Release();
	(log << "Finished Adding The Sink File Name\n").Write();

	return S_OK;
} 

HRESULT BDADVBTSink::NullFileName(CComPtr<IBaseFilter>& pFilter, int intSinkType)
{
	if (pFilter == NULL)
	{
		(log << "Skipping Null Set File Name. No Sink Filter passed.\n").Write();
		return E_INVALIDARG;
	}

	(log << "Nulling the Sink Demux File Name\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (intSinkType)
	{
		if (m_piFileSink)
			m_piFileSink.Release();

		if FAILED(hr = pFilter->QueryInterface(&m_piFileSink))
		{
			(log << "Failed to get the IFileSinkFilter Interface on the Sink Filter.\n").Write(hr);
			return E_FAIL;
		}

		//
		// Set the Sink Filter File Name 
		//
		if FAILED(hr = m_piFileSink->SetFileName(L"", NULL))
		{
			(log << "Failed to Set the IFileSinkFilter Interface with a Null filename.\n").Write(hr);
			return hr;
		}
	}
	else
	{
		//Setup dsnet sender
		IMulticastConfig *piMulticastConfig = NULL;
		if FAILED(hr = pFilter->QueryInterface(IID_IMulticastConfig, reinterpret_cast<void**>(&piMulticastConfig)))
			return (log << "Failed to query Sink filter for IMulticastConfig: " << hr << "\n").Write(hr);

//		if FAILED(hr = piMulticastConfig->SetNetworkInterface(inet_addr ("192.168.0.1"))) //0 == INADDR_ANY
		if FAILED(hr = piMulticastConfig->SetNetworkInterface(NULL)) //0 == INADDR_ANY
			return (log << "Failed to set network interface for Sink filter: " << hr << "\n").Write(hr);

		ULONG ulIP = NULL;
		if FAILED(hr = piMulticastConfig->SetMulticastGroup(ulIP, htons(0)))
			return (log << "Failed to set multicast group for Sink filter: " << hr << "\n").Write(hr);
		piMulticastConfig->Release();
	}
	indent.Release();

	return S_OK;
}

HRESULT BDADVBTSink::AddDemuxPins(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intPinType)
{
	if (pService == NULL)
	{
		(log << "Skipping Demux Pins. No service passed.\n").Write();
		return E_INVALIDARG;
	}

	if (pFilter == NULL)
	{
		(log << "Skipping Demux Pins. No Demultiplexer passed.\n").Write();
		return E_INVALIDARG;
	}

	(log << "Adding Sink Demux Pins\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (m_piIMpeg2Demux)
		m_piIMpeg2Demux.Release();

	if FAILED(hr = pFilter->QueryInterface(&m_piIMpeg2Demux))
	{
		(log << "Failed to get the IMeg2Demultiplexer Interface on the Sink Demux.\n").Write();
		return E_FAIL;
	}

	long videoStreamsRendered;
	long audioStreamsRendered;
	long tsStreamsRendered;

	if (intPinType)
	{
		// render TS
		hr = AddDemuxPinsTS(pService, &tsStreamsRendered);
	}
	else
	{
		// render video
		hr = AddDemuxPinsVideo(pService, &videoStreamsRendered);

		// render teletext if video was rendered
		if (videoStreamsRendered > 0)
			hr = AddDemuxPinsTeletext(pService);

		// render mp2 audio
		hr = AddDemuxPinsMp2(pService, &audioStreamsRendered);

		// render ac3 audio if no mp2 was rendered
		if (audioStreamsRendered == 0)
			hr = AddDemuxPinsAC3(pService, &audioStreamsRendered);
	}
/*
	//Set reference clock
	CComQIPtr<IReferenceClock> piRefClock(pFilter);
	if (!piRefClock)
		return (log << "Failed to get reference clock interface on Sink demux filter: " << hr << "\n").Write(hr);

	CComQIPtr<IMediaFilter> piMediaFilter(m_piGraphBuilder);
	if (!piMediaFilter)
		return (log << "Failed to get IMediaFilter interface from graph: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaFilter->SetSyncSource(piRefClock))
		return (log << "Failed to set reference clock: " << hr << "\n").Write(hr);
*/
	if (m_piIMpeg2Demux)
		m_piIMpeg2Demux.Release();

	indent.Release();
	(log << "Finished Adding Demux Pins\n").Write();
//TCHAR sz[128];
//sprintf(sz, "%u", 0);
//MessageBox(NULL, sz,"test", NULL);

	return S_OK;
}

HRESULT BDADVBTSink::AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered)
{
	if (pService == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	long renderedStreams = 0;

	if (!wcsicmp(pPinName, L"TS"))
	{
		long count = pService->GetStreamCount();

		wchar_t text[16];
		swprintf((wchar_t*)&text, pPinName);

		CComPtr <IPin> piPin;

		// Get the Pin
		CComPtr<IBaseFilter>pFilter;
		if SUCCEEDED(hr = m_piIMpeg2Demux->QueryInterface(&pFilter))
		{
			if FAILED(hr = pFilter->FindPin(pPinName, &piPin))
			{
				// Create the Pin
				if (S_OK != (hr = m_piIMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
				{
					(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
					return hr;
				}
			}
		}
		else
		{
			// Create the Pin
			if (S_OK != (hr = m_piIMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
			{
				(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
				return hr;
			}
		}

		// Map the PID.
		CComPtr <IMPEG2PIDMap> piPidMap;
		if FAILED(hr = piPin.QueryInterface(&piPidMap))
		{
			(log << "Failed to query demux " << pPinName << " pin : " << hr << "\n").Write();
			return hr;	//it's safe to not piPin.Release() because it'll go out of scope
		}

		for ( long currentStream=0 ; currentStream<count ; currentStream++ )
		{
			ULONG Pid = pService->GetStreamPID(currentStream);
			if FAILED(hr = piPidMap->MapPID(1, &Pid, MEDIA_TRANSPORT_PACKET))
			{
				(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();
				continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
			}
			renderedStreams++;
		}

		ULONG Pidarray[6] = {0x00, 0x10, 0x11, 0x12, 0x13, 0x14};
		if FAILED(hr = piPidMap->MapPID(6, &Pidarray[0], MEDIA_TRANSPORT_PACKET))
		{
			(log << "Failed to map demux " << pPinName << " pin Fixed Pids : " << hr << "\n").Write();
		}

		renderedStreams++;
	}
	else
	{
		long count = pService->GetStreamCount(streamType);
		BOOL bMultipleStreams = (pService->GetStreamCount(streamType) > 1) ? 1 : 0;

		for ( long currentStream=0 ; currentStream<count ; currentStream++ )
		{
			ULONG Pid = pService->GetStreamPID(streamType, currentStream);

			wchar_t text[16];
			swprintf((wchar_t*)&text, pPinName);
			if (bMultipleStreams)
				swprintf((wchar_t*)&text, L"%s %i", pPinName, currentStream+1);

			CComPtr <IPin> piPin;

			// Get the Pin
			CComPtr<IBaseFilter>pFilter;
			if SUCCEEDED(hr = m_piIMpeg2Demux->QueryInterface(&pFilter))
			{
				if FAILED(hr = pFilter->FindPin(pPinName, &piPin))
				{
					// Create the Pin
					(log << "Creating pin: PID=" << (long)Pid << "   Name=\"" << (LPWSTR)&text << "\"\n").Write();
					LogMessageIndent indent(&log);

					if (S_OK != (hr = m_piIMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
					{
						(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
						return hr;
					}
					indent.Release();
				}
			}
			else
			{
				(log << "Creating pin: PID=" << (long)Pid << "   Name=\"" << (LPWSTR)&text << "\"\n").Write();
				LogMessageIndent indent(&log);

				// Create the Pin
				if (S_OK != (hr = m_piIMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
				{
					(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
					return hr;
				}
				indent.Release();
			}

			// Map the PID.
			CComPtr <IMPEG2PIDMap> piPidMap;
			if FAILED(hr = piPin.QueryInterface(&piPidMap))
			{
				(log << "Failed to query demux " << pPinName << " pin : " << hr << "\n").Write();
				continue;	//it's safe to not piPin.Release() because it'll go out of scope
			}

			if FAILED(hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM))
			{
				(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();
				continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
			}

			if (renderedStreams != 0)
				continue;

			renderedStreams++;
		}
	}

	if (streamsRendered)
		*streamsRendered = renderedStreams;

	return hr;
}


HRESULT BDADVBTSink::AddDemuxPinsVideo(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = KSDATAFORMAT_TYPE_VIDEO;
	mediaType.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = 0;
	mediaType.lSampleSize = 1;
	mediaType.formattype = FORMAT_MPEG2Video;
	mediaType.pUnk = NULL;
	mediaType.cbFormat = sizeof(g_Mpeg2ProgramVideo);
	mediaType.pbFormat = g_Mpeg2ProgramVideo;

	return AddDemuxPins(pService, video, L"Video", &mediaType, streamsRendered);
}

HRESULT BDADVBTSink::AddDemuxPinsMp2(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	//mediaType.majortype = KSDATAFORMAT_TYPE_AUDIO;
	mediaType.majortype = MEDIATYPE_Audio;
	//mediaType.subtype = MEDIASUBTYPE_MPEG2_AUDIO;
	mediaType.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = 0;
	mediaType.lSampleSize = 1;
	mediaType.formattype = FORMAT_WaveFormatEx;
	mediaType.pUnk = NULL;
	mediaType.cbFormat = sizeof g_MPEG1AudioFormat;
	mediaType.pbFormat = g_MPEG1AudioFormat;

	return AddDemuxPins(pService, mp2, L"Audio", &mediaType, streamsRendered);
}

HRESULT BDADVBTSink::AddDemuxPinsAC3(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	//mediaType.majortype = KSDATAFORMAT_TYPE_AUDIO;
	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_DOLBY_AC3;
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = 0;
	mediaType.lSampleSize = 1;
	mediaType.formattype = FORMAT_WaveFormatEx;
	mediaType.pUnk = NULL;
	mediaType.cbFormat = sizeof g_MPEG1AudioFormat;
	mediaType.pbFormat = g_MPEG1AudioFormat;


/*	mediaType.majortype = MEDIATYPE_Audio;
	mediaType.subtype = MEDIASUBTYPE_DOLBY_AC3;
	mediaType.cbFormat = sizeof(MPEG1AudioFormat);//sizeof(AC3AudioFormat); //
	mediaType.pbFormat = MPEG1AudioFormat;//AC3AudioFormat; //
	mediaType.bFixedSizeSamples = TRUE;
	mediaType.bTemporalCompression = 0;
	mediaType.lSampleSize = 1;
	mediaType.formattype = FORMAT_WaveFormatEx;
	mediaType.pUnk = NULL;
*/
	return AddDemuxPins(pService, ac3, L"AC3", &mediaType, streamsRendered);
}

HRESULT BDADVBTSink::AddDemuxPinsTeletext(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	mediaType.subtype = KSDATAFORMAT_SUBTYPE_NONE;
	mediaType.formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return AddDemuxPins(pService, teletext, L"Teletext", &mediaType, streamsRendered);
}

HRESULT BDADVBTSink::AddDemuxPinsTS(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = MEDIATYPE_Stream;
	mediaType.subtype = KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT; 
	mediaType.formattype = FORMAT_None; 

	return AddDemuxPins(pService, unknown, L"TS", &mediaType, streamsRendered);
}

HRESULT BDADVBTSink::ClearDemuxPids(CComPtr<IBaseFilter>& pFilter)
{
	HRESULT hr = E_INVALIDARG;

	if(pFilter == NULL)
		return hr;

	CComPtr<IPin> pOPin;
	PIN_DIRECTION  direction;
	// Enumerate the Demux pins
	CComPtr<IEnumPins> pIEnumPins;
	if (SUCCEEDED(pFilter->EnumPins(&pIEnumPins)))
	{
		ULONG pinfetch(0);
		while(pIEnumPins->Next(1, &pOPin, &pinfetch) == S_OK)
		{
			pOPin->QueryDirection(&direction);
			if(direction == PINDIR_OUTPUT)
			{
				WCHAR *pinName;
				pOPin->QueryId(&pinName);
				if FAILED(hr = ClearDemuxPins(pOPin))
					(log << "Failed to Clear demux Pin" << pinName << " pin : " << hr << "\n").Write();
			}
			pOPin.Release();
			pOPin = NULL;
		}
	}
	return S_OK;
}

HRESULT BDADVBTSink::ClearDemuxPins(IPin *pIPin)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid))){

		IEnumPIDMap *pIEnumPIDMap;
		if (SUCCEEDED(muxMapPid->EnumPIDMap(&pIEnumPIDMap))){
			ULONG pNumb = 0;
			PID_MAP pPidMap;
			while(pIEnumPIDMap->Next(1, &pPidMap, &pNumb) == S_OK){
				ULONG pid = pPidMap.ulPID;
				hr = muxMapPid->UnmapPID(1, &pid);
			}
		}
		muxMapPid->Release();
	}
	else {

		IMPEG2StreamIdMap* muxStreamMap;
		if(SUCCEEDED(pIPin->QueryInterface (&muxStreamMap))){

			IEnumStreamIdMap *pIEnumStreamMap;
			if (SUCCEEDED(muxStreamMap->EnumStreamIdMap(&pIEnumStreamMap))){
				ULONG pNumb = 0;
				STREAM_ID_MAP pStreamIdMap;
				while(pIEnumStreamMap->Next(1, &pStreamIdMap, &pNumb) == S_OK){
					ULONG pid = pStreamIdMap.stream_id;
					hr = muxStreamMap->UnmapStreamId(1, &pid);
				}
			}
			muxStreamMap->Release();
		}
	}
	return S_OK;
}

HRESULT BDADVBTSink::StartSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd)
{
	HRESULT hr = E_INVALIDARG;

	if(pFilterStart == NULL)
		return hr;

	CComPtr<IFilterChain>pIFilterChain;
	if FAILED(hr = m_piGraphBuilder->QueryInterface(&pIFilterChain))
	{
		(log << "Failed to the IFilterChain Interface" << hr << "\n").Write();
		return hr;
	}

	FILTER_INFO filterInfoStart;
	FILTER_INFO filterInfoEnd;
	pFilterStart->QueryFilterInfo(&filterInfoStart);
	pFilterEnd->QueryFilterInfo(&filterInfoEnd);

	if FAILED(hr = pIFilterChain->StartChain(pFilterEnd, NULL))
		(log << "Failed to Start Filter Chain:" << filterInfoStart.achName << " to : " << filterInfoEnd.achName << "\n").Write();

	return hr;
}

HRESULT BDADVBTSink::StopSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd)
{
	HRESULT hr = E_INVALIDARG;

	if(pFilterStart == NULL)
		return hr;

	CComPtr<IFilterChain>pIFilterChain;
	if FAILED(hr = m_piGraphBuilder->QueryInterface(&pIFilterChain))
	{
		(log << "Failed to the IFilterChain Interface" << hr << "\n").Write();
		return hr;
	}

	FILTER_INFO filterInfoStart;
	FILTER_INFO filterInfoEnd;
	pFilterStart->QueryFilterInfo(&filterInfoStart);
	pFilterEnd->QueryFilterInfo(&filterInfoEnd);
	
	if FAILED(hr = pIFilterChain->StopChain(pFilterStart, pFilterEnd))
		(log << "Failed to Stop Filter Chain:" << filterInfoStart.achName << " to : " << filterInfoEnd.achName << "\n").Write();

	return hr;
}

HRESULT BDADVBTSink::PauseSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd)
{
	HRESULT hr = E_INVALIDARG;

	if(pFilterStart == NULL)
		return hr;

	CComPtr<IFilterChain>pIFilterChain;
	if FAILED(hr = m_piGraphBuilder->QueryInterface(&pIFilterChain))
	{
		(log << "Failed to the IFilterChain Interface" << hr << "\n").Write();
		return hr;
	}

	FILTER_INFO filterInfoStart;
	FILTER_INFO filterInfoEnd;
	pFilterStart->QueryFilterInfo(&filterInfoStart);
	pFilterEnd->QueryFilterInfo(&filterInfoEnd);
	
	if FAILED(hr = pIFilterChain->PauseChain(pFilterStart, pFilterEnd))
		(log << "Failed to Pause Filter Chain:" << filterInfoStart.achName << " to : " << filterInfoEnd.achName << "\n").Write();

	return hr;
}

HRESULT BDADVBTSink::StartRecording(DVBTChannels_Service* pService, LPWSTR pFilename)
{
	if (!m_pCurrentFileSink)
	{

//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;
/*
	if (wcslen(g_pData->settings.capture.fileName))
		g_pOSD->Data()->ReplaceTokens(g_pData->settings.capture.fileName, pFilename);
	else
		g_pOSD->Data()->ReplaceTokens(L"$(CurrentService)", pFilename);
*/			

	if(m_intFileSinkType)
		return m_pCurrentFileSink->StartRecording(pService);

	return hr;
}

HRESULT BDADVBTSink::StopRecording(void)
{
	if (!m_pCurrentFileSink)
	{
//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;

	if(m_intFileSinkType)
		return m_pCurrentFileSink->StopRecording();

	return hr;
}

HRESULT BDADVBTSink::PauseRecording(void)
{
	if (!m_pCurrentFileSink)
	{
//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;

	if(m_intFileSinkType)
		return m_pCurrentFileSink->PauseRecording();

	return hr;
}

HRESULT BDADVBTSink::UnPauseRecording(DVBTChannels_Service* pService)
{
	if (!m_pCurrentFileSink)
	{
//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;

	if(m_intFileSinkType)
		return m_pCurrentFileSink->UnPauseRecording(pService);

	return hr;
}

BOOL BDADVBTSink::IsRecording(void)
{
	if(!m_pCurrentFileSink)
		return FALSE;

	return m_pCurrentFileSink->IsRecording();
}

BOOL BDADVBTSink::IsPaused(void)
{
	if(!m_pCurrentFileSink)
		return FALSE;

	return m_pCurrentFileSink->IsPaused();
}

HRESULT BDADVBTSink::GetCurFile(LPOLESTR *ppszFileName)
{
	if (!ppszFileName)
		return E_INVALIDARG;

	if(m_intTimeShiftType && m_pCurrentTShiftSink)
		return m_pCurrentTShiftSink->GetCurFile(ppszFileName);

	if((m_intFileSinkType & 0x07) && m_pCurrentFileSink)
		return m_pCurrentFileSink->GetCurFile(ppszFileName);

	return FALSE;

}

