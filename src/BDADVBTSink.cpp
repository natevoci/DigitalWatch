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
#include "TSFileSource/MediaFormats.h"
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

	m_piGraphBuilder = NULL;

	m_pCurrentTShiftSink = NULL;
	m_pCurrentFileSink = NULL;
	m_pCurrentDSNetSink = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;

	m_intTimeShiftType = 0;
	m_intFileSinkType = 0;
	m_intDSNetworkType = 0;
	m_cardId = 0;
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

HRESULT BDADVBTSink::Initialise(IGraphBuilder *piGraphBuilder, int cardID)
{
	HRESULT hr;
	m_cardId = cardID;

	if (m_bInitialised)
		return (log << "DVB-T Sink tried to initialise a second time\n").Write(E_FAIL);

	if (!piGraphBuilder)
		return (log << "Must pass a valid DWGraph object to Initialise a Sink\n").Write(E_FAIL);

	m_piGraphBuilder = piGraphBuilder;

	if FAILED(hr = m_piGraphBuilder->QueryInterface(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	m_intDSNetworkType = g_pData->values.dsnetwork.format;
	if(m_intDSNetworkType)
	{
		m_pCurrentDSNetSink = new BDADVBTSinkDSNet(this);
		m_pCurrentDSNetSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentDSNetSink->Initialise(m_piGraphBuilder, m_intDSNetworkType))
			return (log << "Failed to Initalise the DSNetwork Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intFileSinkType = g_pData->values.capture.format;
	if(m_intFileSinkType)
	{
		m_pCurrentFileSink = new BDADVBTSinkFile(this);
		m_pCurrentFileSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentFileSink->Initialise(m_piGraphBuilder, m_intFileSinkType))
			return (log << "Failed to Initalise the File Sink Filters: " << hr << "\n").Write(hr);
	}

	m_intTimeShiftType = g_pData->values.timeshift.format;
	if(m_intTimeShiftType)
	{
		m_pCurrentTShiftSink = new BDADVBTSinkTShift(this);
		m_pCurrentTShiftSink->SetLogCallback(m_pLogCallback);
		if FAILED(hr = m_pCurrentTShiftSink->Initialise(m_piGraphBuilder, m_intTimeShiftType))
			return (log << "Failed to Initalise the TimeShift Sink Filters: " << hr << "\n").Write(hr);
	}

	m_bInitialised = TRUE;
	return S_OK;
}

HRESULT BDADVBTSink::DestroyAll()
{
    HRESULT hr = S_OK;

	if (m_pCurrentDSNetSink)
	{
		m_pCurrentDSNetSink->RemoveSinkFilters();
		delete m_pCurrentDSNetSink;
	}

	if (m_pCurrentFileSink)
	{
		m_pCurrentFileSink->RemoveSinkFilters();
		delete m_pCurrentFileSink;
	}

	if (m_pCurrentTShiftSink)
	{
		m_pCurrentTShiftSink->RemoveSinkFilters();
		delete m_pCurrentTShiftSink;
	}

	m_piMediaControl.Release();

	return S_OK;
}

HRESULT BDADVBTSink::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	//--- Now add & connect the DSNetworking filters ---
	if (m_intDSNetworkType && m_pCurrentDSNetSink)
	{
		if FAILED(hr = m_pCurrentDSNetSink->AddSinkFilters(pService))
			(log << "Failed to add all the DSNetworking Sink Filters to the graph: " << hr << "\n").Write(hr);
	}

	//--- Now add & connect the TimeShifting filters ---
	if (m_intTimeShiftType && m_pCurrentTShiftSink)
	{
		if FAILED(hr = m_pCurrentTShiftSink->AddSinkFilters(pService))
			(log << "Failed to add all the TimeShift Sink Filters to the graph: " << hr << "\n").Write(hr);
	}

	//--- Add & connect the Sink filters ---
	if (m_intFileSinkType && m_pCurrentFileSink)
	{
		if FAILED(hr = m_pCurrentFileSink->AddSinkFilters(pService))
			(log << "Failed to add all the File Sink Filters to the graph: " << hr << "\n").Write(hr);
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
	
	//--- Remove the TimeShifting filters ---
	if (m_intTimeShiftType && m_pCurrentTShiftSink)
		m_pCurrentTShiftSink->RemoveSinkFilters();

	//--- Remove the Sink filters ---
	if (m_intFileSinkType && m_pCurrentFileSink)
		m_pCurrentFileSink->RemoveSinkFilters();

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

	if(m_pCurrentDSNetSink)
		m_pCurrentDSNetSink->SetTransportStreamPin(piPin);

	if(m_pCurrentFileSink)
		m_pCurrentFileSink->SetTransportStreamPin(piPin);

	if(m_pCurrentTShiftSink)
		m_pCurrentTShiftSink->SetTransportStreamPin(piPin);

	return S_OK;
}

BOOL BDADVBTSink::IsActive()
{
	return m_bActive;
}

//HRESULT BDADVBTSink::AddFileName(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intSinkType, LPWSTR pFileName)
//DWS28-02-2006 HRESULT BDADVBTSink::AddFileName(LPOLESTR *ppFileName, DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intSinkType, LPWSTR pFileName)
HRESULT BDADVBTSink::AddFileName(LPOLESTR *ppFileName, DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intSinkType, LPWSTR pFileName, LPWSTR pPath)
{
	if (ppFileName == NULL)
	{
		(log << "Skipping Add File Name. No Valid Pointer passed.\n").Write();
		return E_INVALIDARG;
	}

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

		if(*ppFileName)
			delete[] *ppFileName;

		*ppFileName = new WCHAR[MAX_PATH];
		wcscpy(*ppFileName, L"");

		//
		// Add the Date/Time Stamp to the FileName 
		//
		WCHAR wPathName[MAX_PATH] = L""; 
		WCHAR wTimeStamp[MAX_PATH] = L"";
		WCHAR wfileName[MAX_PATH] = L"";
		WCHAR wServiceName[MAX_PATH] = L"";

		wcscpy(wServiceName, pService->serviceName);
	
		_tzset();
		struct tm *tmTime;
		time_t lTime = timeGetTime();
		time(&lTime);
		tmTime = localtime(&lTime);
		wcsftime(wTimeStamp, 32, L"(%Y-%m-%d %H-%M-%S)", tmTime);


		//DWS28-02-2006
		if (pPath != NULL)
			  wcscpy(wPathName, pPath);
		else
			if ((intSinkType == 1 || intSinkType == 11 || intSinkType == 111) &&
					g_pData->settings.timeshift.folder)
				wcscpy(wPathName, g_pData->settings.timeshift.folder);
			else
			{
				wcscpy(wPathName, g_pData->settings.capture.folder);
				if (wcslen(g_pData->settings.capture.fileName))
					wcscpy(wServiceName, g_pData->settings.capture.fileName);
			}

		//DWS28-02-2006
        //ensures that there's always a back slash at the end
        wPathName[wcslen(wPathName)] = char(92*(int)(wPathName[wcslen(wPathName)-1]!=char(92)));

		if (intSinkType == 1)
			wsprintfW(*ppFileName, L"%S%S %S.full.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 11)
			wsprintfW(*ppFileName, L"%S%S %S.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 111)
			wsprintfW(*ppFileName, L"%S%S %S.mpg.tsbuffer", wPathName, wTimeStamp, wServiceName);
		else if (pFileName == NULL)
		{	
		     if (intSinkType == 2) 
			wsprintfW(*ppFileName, L"%S%S %S.full.ts", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 22) 
			wsprintfW(*ppFileName, L"%S%S %S.ts", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 3) 
			wsprintfW(*ppFileName, L"%S%S %S.mpg", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 4) 
			wsprintfW(*ppFileName, L"%S%S %S.mv2", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 5) 
			wsprintfW(*ppFileName, L"%S%S %S.mp2", wPathName, wTimeStamp, wServiceName);
		else if (intSinkType == 6)
			wsprintfW(*ppFileName, L"%S%S %S.txt", wPathName, wTimeStamp, wServiceName);
		}
		else
		//DWS28-02-2006
		{	 if (intSinkType == 2) 
			wsprintfW(*ppFileName, L"%S%S.full.ts", wPathName, pFileName);
		else if (intSinkType == 22) 
			wsprintfW(*ppFileName, L"%S%S.ts", wPathName, pFileName);
		else if (intSinkType == 3) 
			wsprintfW(*ppFileName, L"%S%S.mpg", wPathName, pFileName);
		else if (intSinkType == 4) 
			wsprintfW(*ppFileName, L"%S%S.mv2", wPathName, pFileName);
		else if (intSinkType == 5) 
			wsprintfW(*ppFileName, L"%S%S.mp2", wPathName, pFileName);
		else if (intSinkType == 6)
			wsprintfW(*ppFileName, L"%S%S.txt", wPathName, pFileName);
		}

		//
		// Set the Sink Filter File Name 
		//
		if FAILED(hr = m_piFileSink->SetFileName(*ppFileName, NULL))
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
		ULONG ulIP = inet_addr(sz) + 256*256*256*m_cardId;
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

//DWS28-02-2006	HRESULT BDADVBTSink::StartRecording(DVBTChannels_Service* pService, LPWSTR pFilename)
HRESULT BDADVBTSink::StartRecording(DVBTChannels_Service* pService, LPWSTR pFilename, LPWSTR pPath)
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
//DWS28-02-2006		return m_pCurrentFileSink->StartRecording(pService, pFilename);
		return m_pCurrentFileSink->StartRecording(pService, pFilename, pPath);

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

HRESULT BDADVBTSink::GetCurFile(LPOLESTR *ppszFileName, BOOL bTimeShiftSink)
{
	if (!ppszFileName)
		return E_INVALIDARG;

	if(bTimeShiftSink && m_intTimeShiftType && m_pCurrentTShiftSink)
		return m_pCurrentTShiftSink->GetCurFile(ppszFileName);

	if(m_intFileSinkType < 4 && m_pCurrentFileSink)
		return m_pCurrentFileSink->GetCurFile(ppszFileName);

	return FALSE;
}

HRESULT BDADVBTSink::GetCurFileSize(__int64 *pllFileSize, BOOL bTimeShiftSink)
{
	if (!pllFileSize)
		return E_INVALIDARG;

	*pllFileSize = 0;

	if(bTimeShiftSink && m_intTimeShiftType && m_pCurrentTShiftSink)
		return m_pCurrentTShiftSink->GetCurFileSize(pllFileSize);

	if(m_intFileSinkType < 4 && m_pCurrentFileSink)
		return m_pCurrentFileSink->GetCurFileSize(pllFileSize);

	return E_FAIL;
}

HRESULT BDADVBTSink::GetSinkSize(LPOLESTR pFileName, __int64 *pllFileSize)
{
	USES_CONVERSION;

	if (!pllFileSize)
		return E_INVALIDARG;

	if (!pFileName)
		return E_FAIL;

	*pllFileSize = 0;

	HRESULT hr = E_FAIL;


	// If it's a .tsbuffer file then get info from file
	long length = wcslen(pFileName);
	if ((length >= 9) && (_wcsicmp(pFileName+length-9, L".tsbuffer") == 0))
	{

	//	(log << "Opening File to get size: " << pFileName << "\n").Write();
		HANDLE hFile = CreateFile(W2T(pFileName),   // The filename
							 GENERIC_READ,          // File access
							 FILE_SHARE_READ |
							 FILE_SHARE_WRITE,       // Share access
							 NULL,                  // Security
							 OPEN_EXISTING,         // Open flags
							 (DWORD) 0,             // More flags
							 NULL);                 // Template
		if (hFile != INVALID_HANDLE_VALUE)
		{
			__int64 length = -1;
			DWORD read = 0;
			LARGE_INTEGER li;
			li.QuadPart = 0;
			::SetFilePointer(hFile, li.LowPart, &li.HighPart, FILE_BEGIN);
			ReadFile(hFile, (PVOID)&length, (DWORD)sizeof(__int64), &read, NULL);
			CloseHandle(hFile);

			if(length > -1)
			{
				*pllFileSize = length;
				return S_OK;
			}
		}
		else
		{
			wchar_t msg[MAX_PATH];
			DWORD dwErr = GetLastError();
			swprintf((LPWSTR)&msg, L"Failed to open file %s : 0x%x\n", pFileName, dwErr);
			::OutputDebugString(W2T((LPWSTR)&msg));
			return HRESULT_FROM_WIN32(dwErr);
		}
	}
	else //Normal File type & info file type
	{
		TCHAR infoName[512];
		strcpy(infoName, W2T(pFileName));
		strcat(infoName, ".info");

		HANDLE hInfoFile = CreateFile((LPCTSTR) infoName, // The filename
										GENERIC_READ,    // File access
										FILE_SHARE_READ |
										FILE_SHARE_WRITE,   // Share access
										NULL,      // Security
										OPEN_EXISTING,    // Open flags
										FILE_ATTRIBUTE_NORMAL, // More flags
										NULL);

		if (hInfoFile != INVALID_HANDLE_VALUE)
		{
			__int64 length = -1;
			DWORD read = 0;
			LARGE_INTEGER li;
			li.QuadPart = 0;
			::SetFilePointer(hInfoFile, li.LowPart, &li.HighPart, FILE_BEGIN);
			ReadFile(hInfoFile, (PVOID)&length, (DWORD)sizeof(__int64), &read, NULL);
			CloseHandle(hInfoFile);

			if(length > -1)
			{
				*pllFileSize = length;
				return S_OK;
			}
		}

		//Test file is being recorded to
		HANDLE hFile = CreateFile(W2T(pFileName),		// The filename
							GENERIC_READ,				// File access
							FILE_SHARE_READ |
							FILE_SHARE_WRITE,			// Share access
							NULL,						// Security
							OPEN_EXISTING,				// Open flags
							FILE_ATTRIBUTE_NORMAL,		// More flags
							NULL);						// Template

		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD dwErr = GetLastError();
			return HRESULT_FROM_WIN32(dwErr);
		}

		DWORD dwSizeLow;
		DWORD dwSizeHigh;

		dwSizeLow = ::GetFileSize(hFile, &dwSizeHigh);
		if ((dwSizeLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR ))
		{
			CloseHandle(hFile);
			return E_FAIL;
		}

		CloseHandle(hFile);

		LARGE_INTEGER li;
		li.LowPart = dwSizeLow;
		li.HighPart = dwSizeHigh;
		*pllFileSize = li.QuadPart;
	}
	return S_OK;
}
	
HRESULT BDADVBTSink::UpdateTSFileSink(BOOL bAutoMode)
{
	if(m_intTimeShiftType && m_pCurrentTShiftSink)
		return m_pCurrentTShiftSink->UpdateTSFileSink(bAutoMode);

	return E_FAIL;
}

