/**
 *	TSFileSource.cpp
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

#include "StdAfx.h"
#include "TSFileSource.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "LogMessage.h"
#include <initguid.h>
#include "TSFileSourceGuids.h"
#include "MediaFormats.h"
#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>

#include <process.h>

//////////////////////////////////////////////////////////////////////
// TSFileSource
//////////////////////////////////////////////////////////////////////

TSFileSource::TSFileSource() : m_strSourceType(L"TSFileSource")
{
	m_bInitialised = FALSE;
	m_pDWGraph = NULL;
	g_pOSD->Data()->AddList(&streamList);
}

TSFileSource::~TSFileSource()
{
	Destroy();
}

void TSFileSource::SetLogCallback(LogMessageCallback *callback)
{
	DWSource::SetLogCallback(callback);
	streamList.SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

LPWSTR TSFileSource::GetSourceType()
{
	return m_strSourceType;
}

DWGraph *TSFileSource::GetFilterGraph(void)
{
	return m_pDWGraph;
}

IGraphBuilder *TSFileSource::GetGraphBuilder(void)
{
	return m_piGraphBuilder;
}

HRESULT TSFileSource::Initialise(DWGraph* pFilterGraph)
{
	m_bInitialised = TRUE;

	(log << "Initialising TSFileSource Source\n").Write();
	LogMessageIndent indent(&log);

	for (int i = 0; i < g_pOSD->Data()->GetListCount(streamList.GetListName()); i++)
	{
		if (g_pOSD->Data()->GetListFromListName(streamList.GetListName()) != &streamList)
		{
			(log << "Streams List is not the same, Rotating List\n").Write();
			g_pOSD->Data()->RotateList(g_pOSD->Data()->GetListFromListName(streamList.GetListName()));
			continue;
		}
		(log << "Streams List found to be the same\n").Write();
		break;
	};

	HRESULT hr;

	m_pDWGraph = pFilterGraph;

	if (!m_piGraphBuilder)
		if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
			return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	streamList.Initialise(pFilterGraph);

	wchar_t file[MAX_PATH];

	swprintf((LPWSTR)&file, L"%sTSFileSource\\Keys.xml", g_pData->application.appPath);
	if FAILED(hr = m_sourceKeyMap.LoadFromFile((LPWSTR)&file))
		return hr;

	// Start the background thread for updating statistics
	if FAILED(hr = StartThread())
		return (log << "Failed to start background thread: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Initialising TSFileSource Source\n").Write();

	return S_OK;
}

HRESULT TSFileSource::Destroy()
{
	(log << "Destroying TSFileSource Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	// Stop background thread
	if FAILED(hr = StopThread())
		return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);
	if (hr == S_FALSE)
		(log << "Killed thread\n").Write();

	if (m_pDWGraph)
	{
		if FAILED(hr = m_pDWGraph->Stop())
			(log << "Failed to stop DWGraph\n").Write();

//		if FAILED(hr = m_pDWGraph->Cleanup())
//			(log << "Failed to cleanup DWGraph\n").Write();

		if FAILED(hr = UnloadFilters())
			(log << "Failed to unload filters\n").Write();
	}

	m_pDWGraph = NULL;

	m_piGraphBuilder.Release();
	streamList.Destroy();

	indent.Release();
	(log << "Finished Destroying TSFileSource Source\n").Write();

	return S_OK;
}

void TSFileSource::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

HRESULT TSFileSource::ExecuteCommand(ParseLine* command)
{
	(log << "TSFileSource::ExecuteCommand - " << command->LHS.Function << "\n").Write();
	LogMessageIndent indent(&log);

	int n1, n2, n3, n4;
	LPWSTR pCurr = command->LHS.FunctionName;

	if (_wcsicmp(pCurr, L"LoadFile") == 0)
	{
		if (command->LHS.ParameterCount > 1)
			return (log << "Expecting 0 or 1 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount == 1)
			return LoadFile(command->LHS.Parameter[0]);
		else
			return LoadFile(NULL);
	}
	else if (_wcsicmp(pCurr, L"PlayPause") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return PlayPause();
	}
	else if (_wcsicmp(pCurr, L"Seek") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return Skip(n1);
	}
	else if (_wcsicmp(pCurr, L"SeekTo") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return SeekTo(n1);
	}
	if (_wcsicmp(pCurr, L"SetStream") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return SetStream(n1);
	}
	else if (_wcsicmp(pCurr, L"GetStreamList") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return GetStreamList();
	}


	//Just referencing these variables to stop warnings.
	n1 = 0;
	n2 = 0;
	n3 = 0;
	n4 = 0;
	return S_FALSE;
}

BOOL TSFileSource::CanLoad(LPWSTR pCmdLine)
{
	long length = wcslen(pCmdLine);
	if ((length >= 9) && (_wcsicmp(pCmdLine+length-9, L".tsbuffer") == 0))
	{
		return TRUE;
	}
	if ((length >= 4) && (_wcsicmp(pCmdLine+length-4, L".mpg") == 0))
	{
		return TRUE;
	}
	if ((length >= 4) && (_wcsicmp(pCmdLine+length-4, L".vob") == 0))
	{
		return TRUE;
	}
	if ((length >= 3) && (_wcsicmp(pCmdLine+length-3, L".ts") == 0))
	{
		return TRUE;
	}
	return FALSE;
}

HRESULT TSFileSource::Load(LPWSTR pCmdLine)
{
	if (!m_pDWGraph)
		return (log << "Filter graph not set in TSFileSource::Play\n").Write(E_FAIL);

	HRESULT hr;

	if (!m_piGraphBuilder)
		if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
			return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	return LoadFile(pCmdLine);
}

HRESULT TSFileSource::LoadTSFile(LPWSTR pCmdLine, DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt)
{
	if (!m_pDWGraph)
		return (log << "Filter graph not set in TSFileSource::Play\n").Write(E_FAIL);

	HRESULT hr;

	if (!m_piGraphBuilder)
		if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
			return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	return LoadFile(pCmdLine, pService, pmt);
}

HRESULT TSFileSource::ReLoad(LPWSTR pCmdLine)
{
	return ReLoadFile(pCmdLine);
}

void TSFileSource::ThreadProc()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	while (!ThreadIsStopping())
	{
		UpdateData();
		Sleep(100);
	}
}

HRESULT TSFileSource::LoadFile(LPWSTR pFilename, DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt)
{
	BOOL bOwnFilename = FALSE;
	if (!pFilename)
	{
		bOwnFilename = TRUE;
		TCHAR tmpFile[MAX_PATH];
		LPTSTR ptFilename = (LPTSTR)&tmpFile;
		ptFilename[0] = '\0';

		// Setup the OPENFILENAME structure
		OPENFILENAME ofn = { sizeof(OPENFILENAME), g_pData->hWnd, NULL,
							 TEXT("Transport Stream Files (*.mpg, *.ts, *.tsbuffer, *.vob)\0*.mpg;*.ts;*.tsbuffer;*.vob\0All Files\0*.*\0\0"), NULL,
//							 TEXT("Transport Stream Files (*.ts, *.tsbuffer)\0*.ts;*.tsbuffer\0All Files\0*.*\0\0"), NULL,
							 0, 1,
							 ptFilename, MAX_PATH,
							 NULL, 0,
							 NULL,
							 TEXT("Load File"),
							 OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
							 NULL, 0, NULL, NULL };

		// Display the SaveFileName dialog.
		if( GetOpenFileName( &ofn ) == FALSE )
			return S_FALSE;

		USES_CONVERSION;
		strCopy(pFilename, T2W(ptFilename));
	}

	(log << "Building Graph (" << pFilename << ")\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if FAILED(hr = m_pDWGraph->Stop())
		return (log << "Failed to stop DWGraph\n").Write(hr);

	if FAILED(hr = UnloadFilters())
		return (log << "Failed to unload previous filters\n").Write(hr);

	// TSFileSource
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.filesourceclsid, &m_piTSFileSource, L"TSFileSource"))
		return (log << "Failed to add TSFileSource to the graph: " << hr << "\n").Write(hr);

	// Set Filename
	CComQIPtr<IFileSourceFilter> piFileSourceFilter(m_piTSFileSource);
	if (!piFileSourceFilter)
		return (log << "Cannot QI TSFileSource filter for IFileSourceFilter: " << hr << "\n").Write(hr);

	if FAILED(hr = piFileSourceFilter->Load(pFilename, pmt))
		return (log << "Failed to load filename: " << hr << "\n").Write(hr);

	if (bOwnFilename)
		delete[] pFilename;
	
g_pOSD->Data()->SetItem(L"warnings", L"Setting up to play the TimeShift File");
g_pTv->ShowOSDItem(L"Warnings", 2);
	// MPEG-2 Demultiplexer (DW's)
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
		return (log << "Failed to add DW MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);

	// Set Demux pids if loaded from a TimeShift Source
	if (pService)
	{
		if FAILED(hr = AddDemuxPins(pService, m_piBDAMpeg2Demux, pmt))
		{
			(log << "Failed to Add Demux Pins\n").Write();
		}
	}

	if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piTSFileSource, m_piBDAMpeg2Demux))
		return (log << "Failed to connect TSFileSource to MPEG2 Demultiplexer: " << hr << "\n").Write(hr);

	// Render output pins
	CComPtr <IEnumPins> piEnumPins;
	if SUCCEEDED(hr = m_piBDAMpeg2Demux->EnumPins( &piEnumPins ))
	{
		CComPtr <IPin> piPin;
		while (piPin.Release(), piEnumPins->Next(1, &piPin, 0) == NOERROR )
		{
			PIN_INFO pinInfo;
			piPin->QueryPinInfo(&pinInfo);
			if (pinInfo.pFilter)
				pinInfo.pFilter->Release();	//QueryPinInfo adds a reference to the filter.

			if (pinInfo.dir == PINDIR_OUTPUT)
			{
				if FAILED(hr = m_pDWGraph->RenderPin(piPin))
				{
					(log << "Failed to render " << pinInfo.achName << " stream : " << hr << "\n").Write();
				}
			}

			piPin.Release();
		}
	}

	//Set reference clock
	CComQIPtr<IReferenceClock> piRefClock(m_piTSFileSource);
	if (!piRefClock)
		return (log << "Failed to get reference clock interface on Sink demux filter: " << hr << "\n").Write(hr);

	CComQIPtr<IMediaFilter> piMediaFilter(m_piGraphBuilder);
	if (!piMediaFilter)
		return (log << "Failed to get IMediaFilter interface from graph: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaFilter->SetSyncSource(piRefClock))
		return (log << "Failed to set reference clock: " << hr << "\n").Write(hr);

g_pOSD->Data()->SetItem(L"warnings", L"Starting to play the TimeShift File");
g_pTv->ShowOSDItem(L"Warnings", 2);

	if FAILED(hr = m_pDWGraph->Start())
		return (log << "Failed to Start Graph.\n").Write(hr);

	indent.Release();
	(log << "Finished Loading File\n").Write();

	UpdateData();
	g_pTv->ShowOSDItem(L"Position", 10);
/*
	// If it's a .tsbuffer file then seek to the end
	long length = wcslen(pFilename);
	if ((length >= 9) && (_wcsicmp(pFilename+length-9, L".tsbuffer") == 0))
	{
		SeekTo(100);
	}
*/

	return S_OK;
}

HRESULT TSFileSource::ReLoadFile(LPWSTR pFilename)
{
	(log << "Reloading the FileSource Filter with fileName : (" << pFilename << ")\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr = E_FAIL;

	if (m_piTSFileSource)
	{
		// Set Filename
		CComQIPtr<IFileSourceFilter> piFileSourceFilter(m_piTSFileSource);
		if (!piFileSourceFilter)
			return (log << "Cannot QI TSFileSource filter for IFileSourceFilter: " << hr << "\n").Write(hr);

		if FAILED(hr = piFileSourceFilter->Load(pFilename, NULL))
			return (log << "Failed to load filename: " << hr << "\n").Write(hr);

		indent.Release();
		(log << "Finished Reloading File\n").Write();

		return hr;
	}
	return hr;
}


HRESULT TSFileSource::UnloadFilters()
{
	HRESULT hr;

	if (m_piMpeg2Demux)
		m_piMpeg2Demux.Release();

	if FAILED(hr = graphTools.DisconnectAllPins(m_piGraphBuilder))
		(log << "Failed to DisconnectAllPins : " << hr << "\n").Write();

	DestroyFilter(m_piTSFileSource);
	DestroyFilter(m_piBDAMpeg2Demux);

	if FAILED(hr = m_pDWGraph->Cleanup())
		return (log << "Failed to cleanup DWGraph\n").Write(hr);

	return S_OK;
}

HRESULT TSFileSource::AddDemuxPins(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, AM_MEDIA_TYPE *pmt, int intPinType)
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

	(log << "Adding Demux Pins\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (m_piMpeg2Demux)
		m_piMpeg2Demux.Release();

	if FAILED(hr = pFilter->QueryInterface(&m_piMpeg2Demux))
	{
		(log << "Failed to get the IMeg2Demultiplexer Interface on the Sink Demux.\n").Write();
		return E_FAIL;
	}

	long videoStreamsRendered;
	long audioStreamsRendered;

	// render video
	hr = AddDemuxPinsVideo(pService, pmt, &videoStreamsRendered);

	// render teletext if video was rendered
	if (videoStreamsRendered > 0)
		hr = AddDemuxPinsTeletext(pService, pmt);

	// render mp2 audio
	hr = AddDemuxPinsMp2(pService, pmt, &audioStreamsRendered);

	// render ac3 audio if no mp2 was rendered
	if (audioStreamsRendered == 0)
		hr = AddDemuxPinsAC3(pService, pmt, &audioStreamsRendered);

	if (m_piMpeg2Demux)
		m_piMpeg2Demux.Release();

	indent.Release();
	(log << "Finished Adding Demux Pins\n").Write();

	return S_OK;
}

HRESULT TSFileSource::AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, AM_MEDIA_TYPE *pmt, long *streamsRendered)
{
	if (pService == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	long renderedStreams = 0;
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
		if SUCCEEDED(hr = m_piMpeg2Demux->QueryInterface(&pFilter))
		{
			if FAILED(hr = pFilter->FindPin(pPinName, &piPin))
			{
				// Create the Pin
				(log << "Creating pin: PID=" << (long)Pid << "   Name=\"" << (LPWSTR)&text << "\"\n").Write();
				LogMessageIndent indent(&log);

				if (S_OK != (hr = m_piMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
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
			if (S_OK != (hr = m_piMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
			{
				(log << "Failed to create demux " << pPinName << " pin : " << hr << "\n").Write();
				return hr;
			}
			indent.Release();
		}

		// Map the PID.
		CComPtr <IMPEG2PIDMap> piPidMap;
		CComPtr <IMPEG2StreamIdMap> piStreamIdMap;
		if(pmt->subtype != MEDIASUBTYPE_MPEG2_PROGRAM)
		{
			if (SUCCEEDED(hr = piPin.QueryInterface(&piPidMap)))
			{
				if FAILED(hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM))
				{
					(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();
					continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
				}
			}
		}
		else if SUCCEEDED(hr = piPin.QueryInterface(&piStreamIdMap))
		{
			USHORT pidId = 0xC0;
			if (pMediaType->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
			{
				if FAILED(hr = piStreamIdMap->MapStreamId(0xE0, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0))
				{
					(log << "Failed to map demux Stream ID's " << pPinName << " pin : " << hr << "\n").Write();
					continue;	//it's safe to not piStreamIdMap.Release() because it'll go out of scope
				}
			}
			else if (pMediaType->subtype == MEDIASUBTYPE_MPEG1Payload)
			{
				if FAILED(hr = piStreamIdMap->MapStreamId(0xC0, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0))
				{
					(log << "Failed to map demux Stream ID's " << pPinName << " pin : " << hr << "\n").Write();
					continue;	//it's safe to not piStreamIdMap.Release() because it'll go out of scope
				}
			}
			else if (pMediaType->subtype == MEDIASUBTYPE_MPEG2_AUDIO)
			{
				if FAILED(hr = piStreamIdMap->MapStreamId(0xC0, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0))
				{
					(log << "Failed to map demux Stream ID's " << pPinName << " pin : " << hr << "\n").Write();
					continue;	//it's safe to not piStreamIdMap.Release() because it'll go out of scope
				}
			}
			else if (pMediaType->subtype == MEDIASUBTYPE_DOLBY_AC3)
			{
				if FAILED(hr = piStreamIdMap->MapStreamId(0xBD, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0x80, 0x04))
				{
					(log << "Failed to map demux Stream ID's " << pPinName << " pin : " << hr << "\n").Write();
					continue;	//it's safe to not piStreamIdMap.Release() because it'll go out of scope
				}
			}
		}
		else
			(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();

		if (renderedStreams != 0)
			continue;

		renderedStreams++;
	}

	if (streamsRendered)
		*streamsRendered = renderedStreams;

	return hr;
}

HRESULT TSFileSource::AddDemuxPinsVideo(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
	GetVideoMedia(&mediaType);
	return AddDemuxPins(pService, video, L"Video", &mediaType, pmt, streamsRendered);
}

HRESULT TSFileSource::AddDemuxPinsMp2(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	GetMP2Media(&mediaType);
	return AddDemuxPins(pService, mp2, L"Audio", &mediaType, pmt, streamsRendered);
}

HRESULT TSFileSource::AddDemuxPinsAC3(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	GetAC3Media(&mediaType);
	return AddDemuxPins(pService, ac3, L"AC3", &mediaType, pmt, streamsRendered);
}

HRESULT TSFileSource::GetAC3Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_DOLBY_AC3;
	pintype->cbFormat = sizeof(MPEG1AudioFormat);//sizeof(AC3AudioFormat); //
	pintype->pbFormat = MPEG1AudioFormat;//AC3AudioFormat; //
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->formattype = FORMAT_WaveFormatEx;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT TSFileSource::GetMP2Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_MPEG2_AUDIO; 
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(MPEG2AudioFormat);
	pintype->pbFormat = MPEG2AudioFormat; 
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT TSFileSource::GetMP1Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_MPEG1Payload;
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(MPEG1AudioFormat);
	pintype->pbFormat = MPEG1AudioFormat;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT TSFileSource::GetAACMedia(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_AAC;
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(AACAudioFormat);
	pintype->pbFormat = AACAudioFormat;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT TSFileSource::GetVideoMedia(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_VIDEO;
	pintype->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = FALSE;
	pintype->lSampleSize = 1;
	pintype->formattype = FORMAT_MPEG2Video;
	pintype->pUnk = NULL;
//	pintype->cbFormat = sizeof(Mpeg2ProgramVideo);
//	pintype->pbFormat = Mpeg2ProgramVideo;
	pintype->cbFormat = sizeof(g_Mpeg2ProgramVideo);
	pintype->pbFormat = g_Mpeg2ProgramVideo;

	return S_OK;
}
static GUID H264_SubType = {0x8D2D71CB, 0x243F, 0x45E3, {0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B}};

HRESULT TSFileSource::GetH264Media(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Video;
//	pintype->subtype = FOURCCMap(MAKEFOURCC('h','2','6','4'));
	pintype->subtype = H264_SubType;
	pintype->bFixedSizeSamples = FALSE;
	pintype->bTemporalCompression = TRUE;
	pintype->lSampleSize = 1;

	pintype->formattype = FORMAT_VideoInfo;
	pintype->pUnk = NULL;
	pintype->cbFormat = sizeof(H264VideoFormat);
	pintype->pbFormat = H264VideoFormat;

	return S_OK;
}

HRESULT TSFileSource::GetMpeg4Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Video;
	pintype->subtype = FOURCCMap(MAKEFOURCC('h','2','6','4'));
	pintype->bFixedSizeSamples = FALSE;
	pintype->bTemporalCompression = TRUE;
	pintype->lSampleSize = 1;

	pintype->formattype = FORMAT_VideoInfo;
	pintype->pUnk = NULL;
	pintype->cbFormat = sizeof(H264VideoFormat);
	pintype->pbFormat = H264VideoFormat;

	return S_OK;
}

HRESULT TSFileSource::GetTIFMedia(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	pintype->subtype = MEDIASUBTYPE_DVB_SI; 
	pintype->formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return S_OK;
}

HRESULT TSFileSource::GetTelexMedia(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	pintype->subtype = KSDATAFORMAT_SUBTYPE_NONE; 
	pintype->formattype = KSDATAFORMAT_SPECIFIER_NONE; 

	return S_OK;
}

HRESULT TSFileSource::GetTSMedia(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Stream;
	pintype->subtype = KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT; 
	pintype->formattype = FORMAT_None; 

	return S_OK;
}

HRESULT TSFileSource::AddDemuxPinsTeletext(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	mediaType.subtype = KSDATAFORMAT_SUBTYPE_NONE;
	mediaType.formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return AddDemuxPins(pService, teletext, L"Teletext", &mediaType, pmt, streamsRendered);
}


HRESULT TSFileSource::PlayPause()
{
	HRESULT hr;

	if (!m_pDWGraph)
		return S_FALSE;

	if (!m_pDWGraph->IsPlaying())
	{
		if FAILED(hr = m_pDWGraph->Start())
			return (log << "Failed to Unpause Graph.\n").Write(hr);

		g_pOSD->Data()->SetItem(L"warnings", L"Playing");
		g_pTv->ShowOSDItem(L"Warnings", 5);
	}
	else
	{
		if (m_pDWGraph->IsPaused())
		{
			if FAILED(hr = m_pDWGraph->Pause(FALSE))
				return (log << "Failed to Unpause Graph.\n").Write(hr);

			g_pOSD->Data()->SetItem(L"warnings", L"Playing");
			g_pTv->ShowOSDItem(L"Warnings", 5);
		}
		else
		{
			if FAILED(hr = m_pDWGraph->Pause(TRUE))
				return (log << "Failed to Pause Graph.\n").Write(hr);

			g_pOSD->Data()->SetItem(L"warnings", L"Paused");
			g_pTv->ShowOSDItem(L"Warnings", 10000);
		}
	}
	return S_OK;
}

HRESULT TSFileSource::Skip(long seconds)
{
	HRESULT hr;

	CComQIPtr<IMediaSeeking> piMediaSeeking(m_piGraphBuilder);

	REFERENCE_TIME rtNow, rtStop, rtEarliest, rtLatest;

	if FAILED(hr = piMediaSeeking->GetCurrentPosition(&rtNow))
		return (log << "Failed to get current time: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaSeeking->GetAvailable(&rtEarliest, &rtLatest))
		return (log << "Failed to get available times: " << hr << "\n").Write(hr);

	rtLatest = (__int64)max(rtEarliest, (__int64)(rtLatest-(__int64)20000000));

	rtStop = 0;
	rtNow += ((__int64)seconds * (__int64)10000000);
	if (rtNow < rtEarliest)
		rtNow = rtEarliest;
	if (rtNow > rtLatest)
		rtNow = rtLatest;

	if FAILED(piMediaSeeking->SetPositions(&rtNow, AM_SEEKING_AbsolutePositioning, &rtStop, AM_SEEKING_NoPositioning))
		return (log << "Failed to set positions: " << hr << "\n").Write(hr);

	UpdateData();

	return S_OK;
}

HRESULT TSFileSource::SeekTo(long percentage)
{
	HRESULT hr;

	CComQIPtr<IMediaSeeking> piMediaSeeking(m_piGraphBuilder);

	REFERENCE_TIME rtNow, rtStop, rtEarliest, rtLatest;

	if FAILED(hr = piMediaSeeking->GetAvailable(&rtEarliest, &rtLatest))
		return (log << "Failed to get available times: " << hr << "\n").Write(hr);

	rtLatest = (__int64)max(rtEarliest, (__int64)(rtLatest-(__int64)20000000));

	if (percentage > 100)
		percentage = 100;
	if (percentage < 0)
		percentage = 0;

	rtStop = 0;
	rtNow = rtLatest - rtEarliest;
	rtNow *= (__int64)percentage;
	rtNow /= (__int64)100;
	rtNow += rtEarliest;

	if FAILED(piMediaSeeking->SetPositions(&rtNow, AM_SEEKING_AbsolutePositioning, &rtStop, AM_SEEKING_NoPositioning))
		return (log << "Failed to set positions: " << hr << "\n").Write(hr);

	UpdateData();

	return S_OK;
}

HRESULT TSFileSource::UpdateData()
{
	HRESULT hr;

	if (!m_pDWGraph->IsPlaying())
		return S_OK;

	CComQIPtr<IMediaSeeking> piMediaSeeking(m_piGraphBuilder);

	REFERENCE_TIME rtCurrent, rtEarliest, rtLatest;

	if FAILED(hr = piMediaSeeking->GetCurrentPosition(&rtCurrent))
		return (log << "Failed to get current time: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaSeeking->GetAvailable(&rtEarliest, &rtLatest))
		return (log << "Failed to get available times: " << hr << "\n").Write(hr);

	WCHAR sz[MAX_PATH];
	long milli, secs, mins, hours;

	milli = (long)(rtEarliest / 10000);
	secs = milli / 1000;
	mins = secs / 60;
	hours = mins / 60;
	milli %= 1000;
	secs %= 60;
	mins %= 60;
	swprintf((LPWSTR)&sz, L"%02i:%02i:%02i", hours, mins, secs, milli);
	g_pOSD->Data()->SetItem(L"PositionEarliest", (LPWSTR)&sz);

	milli = (long)(rtCurrent / 10000);
	secs = milli / 1000;
	mins = secs / 60;
	hours = mins / 60;
	milli %= 1000;
	secs %= 60;
	mins %= 60;
	swprintf((LPWSTR)&sz, L"%02i:%02i:%02i", hours, mins, secs, milli);
	g_pOSD->Data()->SetItem(L"PositionCurrent", (LPWSTR)&sz);

	milli = (long)(rtLatest / 10000);
	secs = milli / 1000;
	mins = secs / 60;
	hours = mins / 60;
	milli %= 1000;
	secs %= 60;
	mins %= 60;
	swprintf((LPWSTR)&sz, L"%02i:%02i:%02i", hours, mins, secs, milli);
	g_pOSD->Data()->SetItem(L"PositionLatest", (LPWSTR)&sz);

	if (!streamList.GetListSize())
		if FAILED(hr = streamList.LoadStreamList(FALSE))
			return (log << "Failed to get a Stream List: " << hr << "\n").Write(hr);
		else
		{
			if (streamList.GetServiceName())
			{
				g_pOSD->Data()->SetItem(L"CurrentService", streamList.GetServiceName() + 2);
				g_pTv->ShowOSDItem(L"Channel", 5);
			}
		}

	return S_OK;
}

HRESULT TSFileSource::SetStream(long index)
{
	IAMStreamSelect *pIAMStreamSelect;
	HRESULT hr = m_piTSFileSource->QueryInterface(IID_IAMStreamSelect, (void**)&pIAMStreamSelect);
	if (SUCCEEDED(hr))
	{
		hr = pIAMStreamSelect->Enable((index & 0xff), AMSTREAMSELECTENABLE_ENABLE);
		pIAMStreamSelect->Release();
	}
	return hr;
}

HRESULT TSFileSource::GetStreamList(void)
{
	HRESULT hr = S_OK;

	if FAILED(hr = streamList.LoadStreamList())
		return (log << "Failed to get a Stream List: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT TSFileSource::SetStreamName(LPWSTR pService, BOOL bEnable)
{
	if (!pService)
		return S_FALSE;

	HRESULT hr;

	int index = -1;
	if FAILED(hr = streamList.FindServiceName(pService, &index))
		return hr;

	if(hr == S_OK && bEnable) 
		SetStream(index);

	return hr;
}

BOOL TSFileSource::IsRecording()
{
		return FALSE;
}

