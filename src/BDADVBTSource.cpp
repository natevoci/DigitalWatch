/**
 *	BDADVBTSource.cpp
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

#include "BDADVBTSource.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "LogMessage.h"

#include "StreamFormats.h"
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include "bdamedia.h"

//////////////////////////////////////////////////////////////////////
// BDADVBTSource
//////////////////////////////////////////////////////////////////////

BDADVBTSource::BDADVBTSource() : m_strSourceType(L"BDA")
{
	m_pCurrentTuner = NULL;
	m_pCurrentNetwork = NULL;
	m_pCurrentService = NULL;
	m_pDWGraph = NULL;

	g_pOSD->Data()->AddList(&channels);
	g_pOSD->Data()->AddList(&frequencyList);
}

BDADVBTSource::~BDADVBTSource()
{
	Destroy();
}

void BDADVBTSource::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock tunersLock(&m_tunersLock);
	CAutoLock sinksLock(&m_sinksLock);

	DWSource::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
	channels.SetLogCallback(callback);
	frequencyList.SetLogCallback(callback);
	cardList.SetLogCallback(callback);

//	std::vector<BDADVBTSourceTuner *>::iterator it = m_tuners.begin();
	std::vector<TUNERINFO>::iterator it = m_tuners.begin();
	for ( ; it != m_tuners.end() ; it++ )
	{
		BDADVBTSourceTuner *tuner = (*it).tuners;
		tuner->SetLogCallback(callback);
		BDADVBTSink *sink = (*it).sinks;
		if (sink)
			sink->SetLogCallback(callback);
	}
}

LPWSTR BDADVBTSource::GetSourceType()
{
	return m_strSourceType;
}

HRESULT BDADVBTSource::Initialise(DWGraph* pFilterGraph)
{
	(log << "Initialising BDA Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;
	m_pDWGraph = pFilterGraph;
	
	wchar_t file[MAX_PATH];

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Channels.xml", g_pData->application.appPath);
	hr = channels.LoadChannels((LPWSTR)&file);

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\FrequencyList.xml", g_pData->application.appPath);
	hr = frequencyList.LoadFrequencyList((LPWSTR)&file);

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Keys.xml", g_pData->application.appPath);
	if FAILED(hr = m_sourceKeyMap.LoadFromFile((LPWSTR)&file))
		return hr;

	//Get list of BDA capture cards
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Cards.xml", g_pData->application.appPath);
	cardList.LoadCards((LPWSTR)&file);
	cardList.SaveCards();
	if (cardList.cards.size() == 0)
		return (log << "Could not find any BDA cards\n").Show(E_FAIL);
	
	CAutoLock sinksLock(&m_sinksLock);
	CAutoLock tunersLock(&m_tunersLock);

	std::vector<BDACard *>::iterator it = cardList.cards.begin();
	for ( ; it != cardList.cards.end() ; it++ )
	{
		BDACard *tmpCard = *it;
		if (tmpCard->bActive)
		{
			m_pCurrentTuner = new BDADVBTSourceTuner(this, tmpCard);

			m_pCurrentTuner->SetLogCallback(m_pLogCallback);
			if SUCCEEDED(m_pCurrentTuner->Initialise(pFilterGraph))
			{
//				m_tuners.push_back(m_pCurrentTuner);

				TUNERINFO itInfo;
				itInfo.tuners = m_pCurrentTuner;

				m_pCurrentSink = new BDADVBTSink(this, m_pCurrentTuner);
				m_pCurrentSink->SetLogCallback(m_pLogCallback);
				if SUCCEEDED(m_pCurrentSink->Initialise(pFilterGraph))
				{
					itInfo.sinks = m_pCurrentSink;
					m_tuners.push_back(itInfo);
					continue;
				}
				delete m_pCurrentSink;
				m_pCurrentSink = NULL;
				itInfo.sinks = m_pCurrentSink;

				m_tuners.push_back(itInfo);
				continue;
			}
			delete m_pCurrentTuner;
			m_pCurrentTuner = NULL;
		}
	};
	if (m_tuners.size() == 0)
		return (log << "There are no active BDA cards\n").Show(E_FAIL);

	m_pCurrentTuner = NULL;
	m_pCurrentSink = NULL;

	indent.Release();
	(log << "Finished Initialising BDA Source\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::Destroy()
{
	(log << "Destroying BDA Source\n").Write();
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

		if FAILED(hr = UnloadSink())
			(log << "Failed to unload Sink Filters\n").Write();

		if FAILED(hr = UnloadTuner())
			(log << "Failed to unload tuner\n").Write();

		if FAILED(hr = m_pDWGraph->Cleanup())
			(log << "Failed to cleanup DWGraph\n").Write();

		CAutoLock sinksLock(&m_sinksLock);
		CAutoLock tunersLock(&m_tunersLock);

//		std::vector<BDADVBTSourceTuner *>::iterator it = m_tuners.begin();
		std::vector<TUNERINFO>::iterator it = m_tuners.begin();
		for ( ; it != m_tuners.end() ; it++ )
		{
//			BDADVBTSourceTuner *tuner = *it;
//			delete tuner;
			TUNERINFO *itInfo = it;
			if(itInfo->sinks)
				delete itInfo->sinks;

			if(itInfo->tuners)
				delete itInfo->tuners;
		}
		m_tuners.clear();
	}

	m_pDWGraph = NULL;

	m_piGraphBuilder.Release();

	cardList.Destroy();
	frequencyList.Destroy();
	channels.Destroy();

	indent.Release();
	(log << "Finished Destroying BDA Source\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::ExecuteCommand(ParseLine* command)
{
	(log << "BDADVBTSource::ExecuteCommand - " << command->LHS.Function << "\n").Write();
	LogMessageIndent indent(&log);

	int n1, n2, n3, n4;
	LPWSTR pCurr = command->LHS.FunctionName;

	if (_wcsicmp(pCurr, L"SetChannel") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 2)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		n2 = 0;
		if (command->LHS.ParameterCount >= 2)
			n2 = StringToLong(command->LHS.Parameter[1]);

		return SetChannel(n1, n2);
	}
	else if (_wcsicmp(pCurr, L"NetworkUp") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return NetworkUp();
	}
	else if (_wcsicmp(pCurr, L"NetworkDown") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return NetworkDown();
	}
	else if (_wcsicmp(pCurr, L"ProgramUp") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ProgramUp();
	}
	else if (_wcsicmp(pCurr, L"ProgramDown") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ProgramDown();
	}
	else if (_wcsicmp(pCurr, L"LastChannel") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		//return LastChannel();
	}
	else if (_wcsicmp(pCurr, L"SetFrequency") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 2)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		n2 = 0;
		if (command->LHS.ParameterCount >= 2)
			n2 = StringToLong(command->LHS.Parameter[1]);

		return SetFrequency(n1, n2);
	}
	else if (_wcsicmp(pCurr, L"UpdateChannels") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return UpdateChannels();
	}
	else if (_wcsicmp(pCurr, L"ChangeFrequencySelectionOffset") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return ChangeFrequencySelectionOffset(n1);
	}
	else if (_wcsicmp(pCurr, L"MoveNetworkUp") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return MoveNetworkUp(n1);
	}
	else if (_wcsicmp(pCurr, L"MoveNetworkDown") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return MoveNetworkDown(n1);
	}
	else if (_wcsicmp(pCurr, L"Recording") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return ToggleRecording(n1);
	}
	else if (_wcsicmp(pCurr, L"RecordingPause") == 0)
	{
		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return TogglePauseRecording(n1);
	}


	//Just referencing these variables to stop warnings.
	n3 = 0;
	n4 = 0;
	return S_FALSE;
}

HRESULT BDADVBTSource::Start()
{
	(log << "Playing BDA Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (!m_pDWGraph)
		return (log << "Filter graph not set in BDADVBTSource::Play\n").Write(E_FAIL);

	if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
		return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	//TODO: replace this with last selected channel, or menu depending on options.
	DVBTChannels_Network* pNetwork = channels.FindDefaultNetwork();
	DVBTChannels_Service* pService = (pNetwork ? pNetwork->FindDefaultService() : NULL);
	if (pService)
	{
		hr = RenderChannel(pNetwork, pService);
	}
	else
	{
		hr = g_pTv->ShowMenu(L"TVMenu");
		(log << "No channel found. Loading TV Menu : " << hr << "\n").Write();
		hr = E_FAIL;
	}

	indent.Release();
	(log << "Finished Playing BDA Source : " << hr << "\n").Write();

	return hr;
}

BOOL BDADVBTSource::CanLoad(LPWSTR pCmdLine)
{
	long length = wcslen(pCmdLine);
	if ((length >= 5) && (_wcsnicmp(pCmdLine, L"tv://", 5) == 0))
	{
		return TRUE;
	}
	return FALSE;
}

HRESULT BDADVBTSource::Load(LPWSTR pCmdLine)
{
	if (!pCmdLine)
		return S_FALSE;

	if (_wcsnicmp(pCmdLine, L"tv://", 5) != 0)
		return S_FALSE;

	pCmdLine += 5;
	if (pCmdLine[0] == '\0')
	{
		(log << "Loading default network and service\n").Write();
		DVBTChannels_Network* pNetwork = channels.FindDefaultNetwork();
		DVBTChannels_Service* pService = (pNetwork ? pNetwork->FindDefaultService() : NULL);
		if (pService)
		{
			return RenderChannel(pNetwork, pService);
		}
		else
		{
			return (log << "No default network and service found\n").Write(S_FALSE);
		}
	}

	long originalNetworkId = 0;
	long transportStreamId = 0;
	long networkId = 0;
	long serviceId = 0;

	LPWSTR pStart = wcschr(pCmdLine, L'/');
	if (pStart)
	{
		pStart[0] = 0;
		pStart++;
		serviceId = StringToLong(pStart);
	}

	pStart = pCmdLine;
	LPWSTR pColon = wcschr(pStart, L':');
	if (!pColon)
		return (log << "bad format - originalNetworkId:transportStreamId:networkId[/serviceId]\n").Write(S_FALSE);
	pColon[0] = 0;
	originalNetworkId = StringToLong(pStart);
	pColon[0] = ':';

	pStart = pColon+1;
	pColon = wcschr(pStart, L':');
	if (!pColon)
		return (log << "bad format - originalNetworkId:transportStreamId:networkId[/serviceId]\n").Write(S_FALSE);
	pColon[0] = 0;
	transportStreamId = StringToLong(pStart);
	pColon[0] = ':';
	pStart = pColon+1;

	networkId = StringToLong(pStart);

	return SetChannel(originalNetworkId, transportStreamId, networkId, serviceId);
}

DVBTChannels *BDADVBTSource::GetChannels()
{
	return &channels;
}

void BDADVBTSource::ThreadProc()
{
	while (!ThreadIsStopping())
	{
		UpdateData();
		Sleep(100);
	}
}



HRESULT BDADVBTSource::SetChannel(long originalNetworkId, long serviceId)
{
	(log << "Setting Channel (" << originalNetworkId << ", " << serviceId << ")\n").Write();
	LogMessageIndent indent(&log);

	//TODO: Check if recording

	DVBTChannels_Network* pNetwork = channels.FindNetworkByONID(originalNetworkId);

	if (!pNetwork)
		return (log << "Network with original network id " << originalNetworkId << " not found\n").Write(E_POINTER);

	//TODO: nProgram < 0 then move to next program
	DVBTChannels_Service* pService;
	if (serviceId == 0)
	{
		pService = pNetwork->FindDefaultService();
		if (!pService)
			return (log << "There are no services for the original network " << originalNetworkId << "\n").Write(E_POINTER);
	}
	else
	{
		pService = pNetwork->FindServiceByServiceId(serviceId);
		if (!pService)
			return (log << "Service with service id " << serviceId << " not found\n").Write(E_POINTER);
	}

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTSource::SetChannel(long originalNetworkId, long transportStreamId, long networkId, long serviceId)
{
	(log << "Setting Channel (" << originalNetworkId << ", " << serviceId << ")\n").Write();
	LogMessageIndent indent(&log);

	//TODO: Check if recording

	DVBTChannels_Network* pNetwork = channels.FindNetwork(originalNetworkId, transportStreamId, networkId);

	if (!pNetwork)
		return (log << "Network with original network id " << originalNetworkId << ", transport stream id " << transportStreamId << ", and network id " << networkId << " not found\n").Write(E_POINTER);

	//TODO: nProgram < 0 then move to next program
	DVBTChannels_Service* pService;
	if (serviceId == 0)
	{
		pService = pNetwork->FindDefaultService();
		if (!pService)
			return (log << "There are no services for the original network " << originalNetworkId << "\n").Write(E_POINTER);
	}
	else
	{
		pService = pNetwork->FindServiceByServiceId(serviceId);
		if (!pService)
			return (log << "Service with service id " << serviceId << " not found\n").Write(E_POINTER);
	}

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTSource::SetFrequency(long frequency, long bandwidth)
{
	(log << "Setting Frequency (" << frequency << ", " << bandwidth << ")\n").Write();
	LogMessageIndent indent(&log);

	//TODO: Check if recording

	m_pCurrentNetwork = NULL;
	m_pCurrentService = NULL;

	m_pCurrentNetwork = channels.FindNetworkByFrequency(frequency);
	if (m_pCurrentNetwork)
	{
		if (bandwidth == 0)
		{
			(log << "Using bandwidth " << m_pCurrentNetwork->bandwidth
				 << " from the channels file network with frequency "
				 << frequency << "\n").Write();
			bandwidth = m_pCurrentNetwork->bandwidth;
		}
		else
		{
			if (m_pCurrentNetwork->bandwidth != bandwidth)
				(log << "Changing the bandwidth from "
					 << m_pCurrentNetwork->bandwidth << " to "
					 << bandwidth << " for the network on frequency "
					 << frequency << "\n").Write();
			m_pCurrentNetwork->bandwidth = bandwidth;
		}

		m_pCurrentService = m_pCurrentNetwork->FindDefaultService();
		if (m_pCurrentService)
		{
			return RenderChannel(m_pCurrentNetwork, m_pCurrentService);
		}
	}
	else
	{
		if (bandwidth == 0)
			bandwidth = channels.GetDefaultBandwidth();
	}

	return RenderChannel(frequency, bandwidth);
}

HRESULT BDADVBTSource::NetworkUp()
{
	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindNextNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindDefaultService();
	if (!pService)
		return (log << "There are no services for the network " << pNetwork->originalNetworkId << "\n").Write(E_POINTER);

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTSource::NetworkDown()
{
	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindPrevNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindDefaultService();
	if (!pService)
		return (log << "There are no services for the network " << pNetwork->originalNetworkId << "\n").Write(E_POINTER);

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTSource::ProgramUp()
{
	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	if (pNetwork->GetListSize() <= 0)
		return (log << "There are no services in the channels file for the network on frequency " << frequency << "\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindNextServiceByServiceId(m_pCurrentService->serviceId);
	if (!pService)
			return (log << "Current service is not in the channels file\n").Write(E_POINTER);

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTSource::ProgramDown()
{
	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	if (pNetwork->GetListSize() <= 0)
		return (log << "There are no services in the channels file for the network on frequency " << frequency << "\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindPrevServiceByServiceId(m_pCurrentService->serviceId);
	if (!pService)
			return (log << "Current service is not in the channels file\n").Write(E_POINTER);

	return RenderChannel(pNetwork, pService);
}


//////////////////////////////////////////////////////////////////////
// graph building methods
//////////////////////////////////////////////////////////////////////

HRESULT BDADVBTSource::RenderChannel(DVBTChannels_Network* pNetwork, DVBTChannels_Service* pService)
{
	m_pCurrentNetwork = pNetwork;
	m_pCurrentService = pService;

	return RenderChannel(pNetwork->frequency, pNetwork->bandwidth);
}

HRESULT BDADVBTSource::RenderChannel(int frequency, int bandwidth)
{
	(log << "Building Graph (" << frequency << ", " << bandwidth << ")\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	//TODO: Check if recording

	// Stop background thread
	if FAILED(hr = StopThread())
		return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);
	if (hr == S_FALSE)
		(log << "Killed thread\n").Write();

	// Do data stuff
	UpdateData(frequency, bandwidth);
	if (m_pCurrentNetwork)
		g_pTv->ShowOSDItem(L"Channel", 10);
	else
		g_pTv->ShowOSDItem(L"Channel", 300);
	// End data stuff

	if FAILED(hr = m_pDWGraph->Stop())
		(log << "Failed to stop DWGraph\n").Write();

	CAutoLock sinksLock(&m_sinksLock);
	CAutoLock tunersLock(&m_tunersLock);

//	std::vector<BDADVBTSourceTuner *>::iterator it = m_tuners.begin();
	std::vector<TUNERINFO>::iterator it = m_tuners.begin();
	for ( ; TRUE /*check for end of list done after graph is cleaned up*/ ; it++ )
	{
		hr = UnloadSink();
		hr = UnloadTuner();

		if FAILED(hr = m_pDWGraph->Cleanup())
			(log << "Failed to cleanup DWGraph\n").Write();

		// check for end of list done here
		if (it == m_tuners.end())
			break;

		m_pCurrentTuner = (*it).tuners;
		m_pCurrentSink = (*it).sinks;

		if FAILED(hr = LoadTuner())
		{
			(log << "Failed to load Source Tuner\n").Write();
			continue;
		}

		if FAILED(hr = m_pCurrentTuner->LockChannel(frequency, bandwidth))
		{
			(log << "Failed to Lock Channel\n").Write();
			continue;
		}

		if (m_pCurrentService)
		{
			if FAILED(hr = AddDemuxPins(m_pCurrentService))
			{
				(log << "Failed to Add Demux Pins\n").Write();
				continue;
			}
		}

		if (m_pCurrentService)
		{
			if FAILED(hr = LoadSink())
			{
				(log << "Failed to load Sink Filters\n").Write();
				continue;
			}
		}

		if FAILED(hr = m_pDWGraph->Start())
		{
			(log << "Failed to Start Graph. Possibly tuner already in use.\n").Write();
			continue;
		}

		if FAILED(hr = m_pCurrentTuner->StartScanning())
		{
			(log << "Failed to start channel scanning\n").Write();
			continue;
		}

		// Start the background thread for updating statistics
		if FAILED(hr = StartThread())
		{
			(log << "Failed to start background thread: " << hr << "\n").Write();
		}

		//Move current tuner to back of list so that other cards will be used next
//		m_tuners.erase(it);
//		m_tuners.push_back(m_pCurrentTuner);
		TUNERINFO itInfo;
		itInfo.sinks = m_pCurrentSink;
		itInfo.tuners = m_pCurrentTuner;
		m_tuners.erase(it);
		m_tuners.push_back(itInfo);
		
		g_pOSD->Data()->SetItem(L"CurrentDVBTCard", m_pCurrentTuner->GetCardName());

		indent.Release();
		(log << "Finished Setting Channel\n").Write();

		return S_OK;
	}

	return (log << "Failed to start the graph: " << hr << "\n").Write(hr);
}

HRESULT BDADVBTSource::LoadTuner()
{
	(log << "Loading Tuner\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if FAILED(hr = m_pCurrentTuner->AddSourceFilters())
		return (log << "Failed to add source filters: " << hr << "\n").Write(hr);

	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin: " << hr << "\n").Write(hr);

	//MPEG-2 Demultiplexer (DW's)
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
		return (log << "Failed to add DW MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);

	m_piBDAMpeg2Demux.QueryInterface(&m_piMpeg2Demux);

	CComPtr <IPin> piDemuxPin;
	if FAILED(hr = graphTools.FindFirstFreePin(m_piBDAMpeg2Demux, &piDemuxPin, PINDIR_INPUT))
		return (log << "Failed to get input pin on DW Demux: " << hr << "\n").Write(hr);

	if FAILED(hr = m_piGraphBuilder->ConnectDirect(piTSPin, piDemuxPin, NULL))
		return (log << "Failed to connect TS Pin to DW Demux: " << hr << "\n").Write(hr);

	//Set reference clock
	CComQIPtr<IReferenceClock> piRefClock(m_piBDAMpeg2Demux);
	if (!piRefClock)
		return (log << "Failed to get reference clock interface on demux filter: " << hr << "\n").Write(hr);

	CComQIPtr<IMediaFilter> piMediaFilter(m_piGraphBuilder);
	if (!piMediaFilter)
		return (log << "Failed to get IMediaFilter interface from graph: " << hr << "\n").Write(hr);

	if FAILED(hr = piMediaFilter->SetSyncSource(piRefClock))
		return (log << "Failed to set reference clock: " << hr << "\n").Write(hr);

	piDemuxPin.Release();
	piTSPin.Release();

	indent.Release();
	(log << "Finished Loading Tuner\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::UnloadTuner()
{
	(log << "Unloading Tuner\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	m_piMpeg2Demux.Release();

	if (m_piBDAMpeg2Demux)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDAMpeg2Demux);
		m_piBDAMpeg2Demux.Release();
	}

	if (m_pCurrentTuner)
	{
		if FAILED(hr = m_pCurrentTuner->RemoveSourceFilters())
			return (log << "Failed to remove source filters: " << hr << "\n").Write(hr);
		m_pCurrentTuner = NULL;
	}

	indent.Release();
	(log << "Finished Unloading Tuner\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::LoadSink()
{
	(log << "Loading Sink Filters\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr = S_OK;

	if (!m_pCurrentSink)
		return (log << "No Sink Filters loaded: " << hr << "\n").Write(hr);

	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin: " << hr << "\n").Write(hr);

	m_pCurrentSink->SetTransportStreamPin(piTSPin);

	if FAILED(hr = m_pCurrentSink->AddSinkFilters(m_pCurrentService))
		return (log << "Failed to add Sink filters: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Loading Tuner\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::UnloadSink()
{
	(log << "Unloading Sink Filters\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (m_pCurrentSink)
	{
		if FAILED(hr = m_pCurrentSink->RemoveSinkFilters())
			return (log << "Failed to remove Sink filters: " << hr << "\n").Write(hr);
		m_pCurrentSink = NULL;
	}

	indent.Release();
	(log << "Finished Unloading Sink Filters\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::AddDemuxPins(DVBTChannels_Service* pService)
{
	if (pService == NULL)
	{
		(log << "Skipping Demux Pins. No service passed.\n").Write();
		return E_INVALIDARG;
	}

	(log << "Adding Demux Pins\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	long videoStreamsRendered;
	long audioStreamsRendered;

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

	indent.Release();
	(log << "Finished Adding Demux Pins\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered)
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
		if (S_OK != (hr = m_piMpeg2Demux->CreateOutputPin(pMediaType, (wchar_t*)&text, &piPin)))
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

		if FAILED(hr = m_pDWGraph->RenderPin(piPin))
		{
			(log << "Failed to render " << pPinName << " stream : " << hr << "\n").Write();
			continue;
		}

		renderedStreams++;
	}

	if (streamsRendered)
		*streamsRendered = renderedStreams;

	return hr;
}


HRESULT BDADVBTSource::AddDemuxPinsVideo(DVBTChannels_Service* pService, long *streamsRendered)
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

HRESULT BDADVBTSource::AddDemuxPinsMp2(DVBTChannels_Service* pService, long *streamsRendered)
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

HRESULT BDADVBTSource::AddDemuxPinsAC3(DVBTChannels_Service* pService, long *streamsRendered)
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

HRESULT BDADVBTSource::AddDemuxPinsTeletext(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	mediaType.subtype = KSDATAFORMAT_SUBTYPE_NONE;
	mediaType.formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return AddDemuxPins(pService, teletext, L"Teletext", &mediaType, streamsRendered);
}

void BDADVBTSource::UpdateData(long frequency, long bandwidth)
{
//	HRESULT hr;
	LPWSTR str = NULL;
	strCopy(str, L"");

	// Set CurrentBandwidth
	if (bandwidth != 0)
	{
		strCopy(str, bandwidth);
		g_pOSD->Data()->SetItem(L"CurrentBandwidth", str);
	}
	else
	{
		g_pOSD->Data()->SetItem(L"CurrentBandwidth", L"");
	}

	// Set CurrentFrequency
	if (frequency != 0)
	{
		strCopy(str, frequency);
		g_pOSD->Data()->SetItem(L"CurrentFrequency", str);
	}
	else
	{
		g_pOSD->Data()->SetItem(L"CurrentFrequency", L"");
	}

	// Set CurrentNetwork
	if (m_pCurrentNetwork && m_pCurrentNetwork->networkName)
		g_pOSD->Data()->SetItem(L"CurrentNetwork", m_pCurrentNetwork->networkName);
	else
		g_pOSD->Data()->SetItem(L"CurrentNetwork", str); // str should be whatever CurrentFrequency is

	// Set CurrentService
	if (m_pCurrentService && m_pCurrentService->serviceName)
	{
		g_pOSD->Data()->SetItem(L"CurrentService", m_pCurrentService->serviceName);
	}
	else if (m_pCurrentService && m_pCurrentService->serviceId != 0)
	{
		strCopy(str, m_pCurrentService->serviceId);
		g_pOSD->Data()->SetItem(L"CurrentService", str);
	}
	else
	{
		g_pOSD->Data()->SetItem(L"CurrentService", L"");
	}
/*
	// Set Signal Statistics
	BOOL locked;
	long strength, quality;

	REFERENCE_TIME rtStart = timeGetTime();

	std::vector<BDADVBTSourceTuner *>::iterator it = m_tuners.begin();
	for ( ; it != m_tuners.end() ; it++ )
	{
		BDADVBTSourceTuner *tuner = *it;
		tuner->GetSignalStats(locked, strength, quality);
	}

//	if (m_pCurrentTuner)
//	{
//		if SUCCEEDED(hr = m_pCurrentTuner->GetSignalStats(locked, strength, quality))
//		{
//			if (locked)
//				g_pOSD->Data()->SetItem(L"SignalLocked", L"Locked");
//			else
//				g_pOSD->Data()->SetItem(L"SignalLocked", L"Not Locked");
//
//			strCopy(str, strength);
//			g_pOSD->Data()->SetItem(L"SignalStrength", str);
//
//			strCopy(str, quality);
//			g_pOSD->Data()->SetItem(L"SignalQuality", str);
//		}
//	}

	REFERENCE_TIME rtEnd = timeGetTime();
	long timespan = rtEnd - rtStart;
	if (timespan > 1000)
		(log << "Retrieving signal stats took " << timespan << " for " << m_pCurrentTuner->GetCardName() << "\n").Write();
*/
	delete[] str;
}

HRESULT BDADVBTSource::UpdateChannels()
{
	if (channels.GetListSize() <= 0)
		return S_OK;

	(log << "UpdateChannels()\n").Write();
	LogMessageIndent indent(&log);

	if (!m_pCurrentNetwork)
	{
		(log << "No current network. Finding it\n").Write();

		long frequency = m_pCurrentTuner->GetCurrentFrequency();
		DVBTChannels_Network* pNetwork = channels.FindNetworkByFrequency(frequency);
		if (!pNetwork)
			return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

		DVBTChannels_Service* pService = pNetwork->FindDefaultService();
		if (!pService)
			return (log << "There are no services for the original network " << pNetwork->originalNetworkId << "\n").Write(E_POINTER);

		return RenderChannel(pNetwork, pService);
	}
	else
	{
		// TODO: Check if the streams in a service we're using were changed.
		//       A difficulty with this is that if the service was deleted then
		//       m_pCurrentService won't be valid any more so we'll need to clone it.

		//(log << "Starting updating data\n").Write();
		//UpdateData();
		//(log << "Finished updating data\n").Write();

		if (m_pCurrentNetwork)
			g_pTv->ShowOSDItem(L"Channel", 10);
		else
			g_pTv->ShowOSDItem(L"Channel", 300);
	}
	return S_OK;
}

HRESULT BDADVBTSource::ChangeFrequencySelectionOffset(long change)
{
	return frequencyList.ChangeOffset(change);
}

HRESULT BDADVBTSource::MoveNetworkUp(long originalNetworkId)
{
	return channels.MoveNetworkUp(originalNetworkId);
}

HRESULT BDADVBTSource::MoveNetworkDown(long originalNetworkId)
{
	return channels.MoveNetworkDown(originalNetworkId);
}

HRESULT BDADVBTSource::ToggleRecording(long mode)
{
	HRESULT hr = S_OK;

	WCHAR sz[32];

	if(m_pCurrentSink->IsRecording())
	{

		hr = m_pCurrentSink->StopRecording();
		lstrcpyW(sz, L"Recording Stopped");
	}
	else
	{
		hr = m_pCurrentSink->StartRecording(m_pCurrentService);
		lstrcpyW(sz, L"Recording");
	}

	g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
	g_pTv->ShowOSDItem(L"Recording", 5);

	return hr;
}

HRESULT BDADVBTSource::TogglePauseRecording(long mode)
{
	HRESULT hr = S_OK;

	WCHAR sz[32];

	if(m_pCurrentSink->IsPaused())
	{
		m_pCurrentSink->UnPauseRecording(m_pCurrentService);
		lstrcpyW(sz, L"Recording");
		g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
		g_pTv->ShowOSDItem(L"Recording", 5);
	}
	else
	{
		m_pCurrentSink->PauseRecording();
		lstrcpyW(sz, L"Recording Paused");
		g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
		g_pTv->ShowOSDItem(L"Recording", 10000);
	}


	return hr;
}

