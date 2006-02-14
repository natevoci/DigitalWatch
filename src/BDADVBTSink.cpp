/**
 *	BDADVBTSink.cpp
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


#include "BDADVBTSink.h"
#include "BDADVBTSource.h"
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

BDADVBTSink::BDADVBTSink(BDADVBTSource *pBDADVBTSource, BDADVBTSourceTuner *pCurrentTuner) :
	m_pBDADVBTSource(pBDADVBTSource)
{
	m_pCurrentTuner = pCurrentTuner;

	m_pDWGraph = NULL;

	m_pCurrentTShiftSink = NULL;
	m_pCurrentFileSink = NULL;
	m_pCurrentDSNetSink = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;


	m_pMpeg2DataParser = NULL;
	m_pMpeg2DataParser = new DVBMpeg2DataParser();

	m_rotEntry = 0;

	m_intTimeShiftType = 0;
	m_intFileSinkType = 0;
	m_intDSNetworkType = 0;
}

BDADVBTSink::~BDADVBTSink()
{
	DestroyAll();
	if (m_pMpeg2DataParser)
	{
		m_pMpeg2DataParser->ReleaseFilter();
		delete m_pMpeg2DataParser;
		m_pMpeg2DataParser = NULL;
	}
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

	m_intTimeShiftType = 0;
	if(m_intTimeShiftType)
	{
		m_pCurrentTShiftSink = new BDADVBTSinkTShift(this, m_pCurrentTuner);
		m_pCurrentTShiftSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentTShiftSink->Initialise(m_pDWGraph, m_intTimeShiftType))
			return (log << "Failed to Initalise the TimeShift Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intFileSinkType = 0;
	if(m_intFileSinkType)
	{
		m_pCurrentFileSink = new BDADVBTSinkFile(this, m_pCurrentTuner);
		m_pCurrentFileSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentFileSink->Initialise(m_pDWGraph, m_intFileSinkType))
			return (log << "Failed to Initalise the File Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intDSNetworkType = 0;
	if(m_intDSNetworkType)
	{
		m_pCurrentDSNetSink = new BDADVBTSinkDSNet(this, m_pCurrentTuner);
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
	HRESULT hr;


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
	return S_OK;
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

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();

	DestroyFilter(m_piMPGDSNetworkSink);
	DestroyFilter(m_piMPGDSNetworkMpeg2Mux);
	DestroyFilter(m_piMPGDSNetworkMpeg2Demux);
	DestroyFilter(m_piTSDSNetworkSink);
	DestroyFilter(m_piTSDSNetworkMpeg2Demux);
	DestroyFilter(m_piFTSDSNetworkSink);
	DestroyFilter(m_piMPGTShiftFileWriter);
	DestroyFilter(m_piMPGTShiftMpeg2Mux);
	DestroyFilter(m_piMPGTShiftMpeg2Demux);
	DestroyFilter(m_piTShiftFileWriter);
	DestroyFilter(m_piTShiftMpeg2Demux);
	DestroyFilter(m_piFTShiftFileWriter);
	DestroyFilter(m_piVideoCapFileWriter);
	DestroyFilter(m_piAudioCapFileWriter);
	DestroyFilter(m_piAVCapMpeg2Demux);
	DestroyFilter(m_piMPGCapFileWriter);
	DestroyFilter(m_piMPGCapMpeg2Mux);
	DestroyFilter(m_piMPGCapMpeg2Demux);
	DestroyFilter(m_piTSCapFileWriter);
	DestroyFilter(m_piTSCapMpeg2Demux);
	DestroyFilter(m_piFTSCapFileWriter);

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();

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

	(log << "Adding Sink Demux File Name\n").Write();
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

//		WCHAR fileName[MAX_PATH] = L"G:\\Capture\\"; 
		WCHAR fileName[MAX_PATH] = L""; 
		//
		// Add the Date/Time Stamp to the FileName 
		//
		WCHAR wfileName[MAX_PATH] = L"";
	
		_tzset();
		struct tm *tmTime;
		time_t lTime = timeGetTime();
		time(&lTime);
		tmTime = localtime(&lTime);
		wcsftime(wfileName, 32, L"(%Y-%m-%d %H-%M)", tmTime);

		if (intSinkType == 1)
			wsprintfW(fileName, L"%S%S%S.full.tsbuffer", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 11)
			wsprintfW(fileName, L"%S%S%S.tsbuffer", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 111)
			wsprintfW(fileName, L"%S%S%S.mpg.tsbuffer", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 2) 
			wsprintfW(fileName, L"%S%S%S.full.ts", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 22) 
			wsprintfW(fileName, L"%S%S%S.ts",	fileName, wfileName, pService->serviceName);
		else if (intSinkType == 3) 
			wsprintfW(fileName, L"%S%S%S.mpg", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 4) 
			wsprintfW(fileName, L"%S%S%S.mv2", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 5) 
			wsprintfW(fileName, L"%S%S%S.mp2", fileName, wfileName, pService->serviceName);
		else if (intSinkType == 6)
			wsprintfW(fileName, L"%S%S%S.txt", fileName, wfileName, pService->serviceName);

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
		//Setup dsnet sender
		IMulticastConfig *piMulticastConfig = NULL;
		if FAILED(hr = pFilter->QueryInterface(IID_IMulticastConfig, reinterpret_cast<void**>(&piMulticastConfig)))
			return (log << "Failed to query Sink filter for IMulticastConfig: " << hr << "\n").Write(hr);

//		if FAILED(hr = piMulticastConfig->SetNetworkInterface(inet_addr ("192.168.0.1"))) //0 == INADDR_ANY
		if FAILED(hr = piMulticastConfig->SetNetworkInterface(inet_addr ("127.0.0.1"))) //0 == INADDR_ANY
			return (log << "Failed to set network interface for Sink filter: " << hr << "\n").Write(hr);

		ULONG ulIP = toIPAddress(224,0,0,1);
		if FAILED(hr = piMulticastConfig->SetMulticastGroup(ulIP, htons(1234)))
			return (log << "Failed to set multicast group for Sink filter: " << hr << "\n").Write(hr);
		piMulticastConfig->Release();
	}
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

	if (m_piIMpeg2Demux)
		m_piIMpeg2Demux.Release();

	indent.Release();
	(log << "Finished Adding Demux Pins\n").Write();
TCHAR sz[128];
sprintf(sz, "%u", 0);
//MessageBox(NULL, sz,"test", NULL);

	return S_OK;
}

HRESULT BDADVBTSink::AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered)
{
	if (pService == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	long count = pService->GetStreamCount(streamType);
	long renderedStreams = 0;
	BOOL bMultipleStreams = (pService->GetStreamCount(streamType) > 1) ? 1 : 0;

	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		ULONG Pid = pService->GetStreamPID(streamType, currentStream);

		wchar_t text[16];
		swprintf((wchar_t*)&text, pPinName);
		if (bMultipleStreams)
			swprintf((wchar_t*)&text, L"%s %i", pPinName, currentStream+1);

		(log << "Creating pin: PID=" << (long)Pid << "   Name=\"" << (LPWSTR)&text << "\"\n").Write();
		LogMessageIndent indent(&log);

		// Create the Pin
		CComPtr <IPin> piPin;
		if (S_OK != (hr = m_piIMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
		{
			(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
			continue;
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

//		if FAILED(hr = m_pDWGraph->RenderPin(piPin))
//		{
//			(log << "Failed to render " << pPinName << " stream : " << hr << "\n").Write();
//			continue;
//		}

		renderedStreams++;
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

	return AddDemuxPins(pService, ts, L"TS", &mediaType, streamsRendered);
}

