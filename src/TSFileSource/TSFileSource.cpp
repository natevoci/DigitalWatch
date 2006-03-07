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
//	if(m_pCurrentDWGraph && m_bFileSourceActive)
//		*ppDWGraph = m_pCurrentDWGraph;
//	else
		return m_pDWGraph;
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

HRESULT TSFileSource::Start()
{
	(log << "Playing TSFileSource Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (!m_pDWGraph)
		return (log << "Filter graph not set in TSFileSource::Play\n").Write(E_FAIL);

	if (!m_piGraphBuilder)
		if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
			return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	indent.Release();
	(log << "Finished Playing TSFileSource Source : " << hr << "\n").Write();

	return hr;
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

HRESULT TSFileSource::ReLoad(LPWSTR pCmdLine)
{
	return ReLoadFile(pCmdLine);
}

void TSFileSource::ThreadProc()
{
	while (!ThreadIsStopping())
	{
		UpdateData();
		Sleep(100);
//		Sleep(1000);
	}
}

HRESULT TSFileSource::LoadFile(LPWSTR pFilename)
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
	if FAILED(hr = piFileSourceFilter->Load(pFilename, NULL))
		return (log << "Failed to load filename: " << hr << "\n").Write(hr);

	if (bOwnFilename)
		delete[] pFilename;
	
	// MPEG-2 Demultiplexer (DW's)
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
		return (log << "Failed to add DW MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);

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

void TSFileSource::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
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

