/**
 *	BDADVBTimeShift.cpp
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

#include "BDADVBTimeShift.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "LogMessage.h"

#include "StreamFormats.h"
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include "bdamedia.h"

//////////////////////////////////////////////////////////////////////
// BDADVBTimeShift
//////////////////////////////////////////////////////////////////////

BDADVBTimeShift::BDADVBTimeShift() : m_strSourceType(L"BDATimeShift")
{
	m_bInitialised = FALSE;
	m_pCurrentTuner = NULL;
	m_pCurrentSink  = NULL;
	m_pCurrentFilterList = NULL;
	m_pCurrentFileSource = NULL;
	m_piSinkGraphBuilder = NULL;
	m_pCurrentNetwork = NULL;
	m_pCurrentService = NULL;
	m_pDWGraph = NULL;
	m_bFileSourceActive = TRUE;
	m_rotEntry = 0;
	m_rtTimeShiftStart = 0;
	m_rtTimeShiftDuration = 0;
	m_cardId = 0;

	g_pOSD->Data()->AddList(&channels);
	g_pOSD->Data()->AddList(&frequencyList);
}

BDADVBTimeShift::~BDADVBTimeShift()
{
	Destroy();
}

void BDADVBTimeShift::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock tunersLock(&m_tunersLock);

	DWSource::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
	channels.SetLogCallback(callback);
	frequencyList.SetLogCallback(callback);
	cardList.SetLogCallback(callback);

	std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
	for ( ; it != m_tuners.end() ; it++ )
	{
		if ((*it)->pTuner)
			(*it)->pTuner->SetLogCallback(callback);

		if ((*it)->pSink)
			(*it)->pSink->SetLogCallback(callback);

		if ((*it)->pFilterList)
			(*it)->pFilterList->SetLogCallback(callback);
	}

	if (m_pCurrentFileSource)
		m_pCurrentFileSource->SetLogCallback(callback);

}

LPWSTR BDADVBTimeShift::GetSourceType()
{
	return m_strSourceType;
}

DWGraph *BDADVBTimeShift::GetFilterGraph(void)
{
	return m_pDWGraph;
}

IGraphBuilder *BDADVBTimeShift::GetGraphBuilder(void)
{
	if(m_bFileSourceActive)
		return m_piGraphBuilder;
	else
		return m_piSinkGraphBuilder;
}

void BDADVBTimeShift::RotateFilterList(void)
{
	if (m_pCurrentFilterList)
		for (int i = 0; i < g_pOSD->Data()->GetListCount(m_pCurrentFilterList->GetListName()); i++)
		{
			if (g_pOSD->Data()->GetListFromListName(m_pCurrentFilterList->GetListName()) != m_pCurrentFilterList)
			{
				(log << "Filter List is not the same, Rotating List\n").Write();
				g_pOSD->Data()->RotateList(g_pOSD->Data()->GetListFromListName(m_pCurrentFilterList->GetListName()));
				continue;
			}
			(log << "Filter List found to be the same\n").Write();
			break;
		};
}

HRESULT BDADVBTimeShift::Initialise(DWGraph* pFilterGraph)
{
	m_bFileSourceActive = TRUE;
	m_bInitialised = TRUE;

	(log << "Initialising BDATimeShift Source\n").Write();
	LogMessageIndent indent(&log);

	for (int i = 0; i < g_pOSD->Data()->GetListCount(channels.GetListName()); i++)
	{
		if (g_pOSD->Data()->GetListFromListName(channels.GetListName()) != &channels)
		{
			(log << "Channels List is not the same, Rotating List\n").Write();
			g_pOSD->Data()->RotateList(g_pOSD->Data()->GetListFromListName(channels.GetListName()));
		}
		(log << "Channels List found to be the same\n").Write();
		break;
	};

	for (i = 0; i < g_pOSD->Data()->GetListCount(frequencyList.GetListName()); i++)
	{
		if (g_pOSD->Data()->GetListFromListName(frequencyList.GetListName()) != &frequencyList)
		{
			(log << "Frequency List is not the same, Rotating List\n").Write();
			g_pOSD->Data()->RotateList(g_pOSD->Data()->GetListFromListName(frequencyList.GetListName()));
		}
		(log << "Frequency List found to be the same\n").Write();
		break;
	};

	(log << "Clearing All TVChannels.Services Lists\n").Write();
	g_pOSD->Data()->ClearAllListNames(L"TVChannels.Services");

	HRESULT hr;
	m_pDWGraph = pFilterGraph;

	g_pData->values.timeshift.format = g_pData->settings.timeshift.format;
	g_pData->values.capture.format = g_pData->settings.capture.format; 
	g_pData->values.dsnetwork.format = g_pData->settings.dsnetwork.format;
	g_pData->values.application.multicard = g_pData->settings.application.multicard;

	m_pCurrentFileSource = new TSFileSource();
	m_pCurrentFileSource->SetLogCallback(m_pLogCallback);
	if FAILED(hr = m_pCurrentFileSource->Initialise(m_pDWGraph))
		return (log << "Failed to Initialise the TSFileSource Filters" << hr << "\n").Write(hr);

	wchar_t file[MAX_PATH];

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Channels.xml", g_pData->application.appPath);
	hr = channels.LoadChannels((LPWSTR)&file);

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\FrequencyList.xml", g_pData->application.appPath);
	hr = frequencyList.LoadFrequencyList((LPWSTR)&file);

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\TimeShiftKeys.xml", g_pData->application.appPath);
	if FAILED(hr = m_sourceKeyMap.LoadFromFile((LPWSTR)&file))
		return hr;

	//Get list of BDA capture cards
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Cards.xml", g_pData->application.appPath);
	cardList.LoadCards((LPWSTR)&file);
	cardList.SaveCards();
	if (cardList.cards.size() == 0)
		return (log << "Could not find any BDA cards\n").Show(E_FAIL);

	CAutoLock tunersLock(&m_tunersLock);
	std::vector<BDACard *>::iterator it = cardList.cards.begin();
	for ( ; it != cardList.cards.end() ; it++ )
	{
		BDACard *tmpCard = *it;
		if (tmpCard->bActive)
		{
			TUNER_GRAPH *tuner = new TUNER_GRAPH;
			tuner->pSink = NULL;
			tuner->pTuner = NULL;
			tuner->pFilterList = NULL;
			tuner->piGraphBuilder = NULL;
			tuner->isRecording = FALSE;
			tuner->rotEntry = 0;
			tuner->networkId = 0;
			tuner->serviceId = 0;
			tuner->cardId = m_tuners.size();

			//--- Create Graph ---
			if FAILED(hr = tuner->piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER))
				return (log << "Failed Creating TimeShift Sink Graph Builder: " << hr << "\n").Write();

			if (g_pData->settings.application.addToROT)
			{
				if FAILED(hr = graphTools.AddToRot(tuner->piGraphBuilder, &tuner->rotEntry))
				{
					return (log << "Failed adding the TimeShift Sink graph to ROT: " << hr << "\n").Write();
				}
			}

			tuner->pSink = new BDADVBTSink();
			tuner->pSink->SetLogCallback(m_pLogCallback);
			if FAILED(hr = tuner->pSink->Initialise(tuner->piGraphBuilder, m_tuners.size()))
				return (log << "Failed to Initialise Sink Filters" << hr << "\n").Write(hr);

			tuner->pFilterList = new FilterPropList();
			tuner->pFilterList->SetLogCallback(m_pLogCallback);
			if FAILED(hr = tuner->pFilterList->Initialise(tuner->piGraphBuilder, L"FilterTSInfo"))
				return (log << "Failed to Initialise Filter Properties List" << hr << "\n").Write(hr);

			g_pOSD->Data()->AddList(tuner->pFilterList);

			tuner->pTuner = new BDADVBTimeShiftTuner(this, tmpCard);
			tuner->pTuner->SetLogCallback(m_pLogCallback);
			if SUCCEEDED(tuner->pTuner->Initialise(tuner->piGraphBuilder))
			{
				m_tuners.push_back(tuner);
				continue;
			}
		}
	};

	if (m_tuners.size() == 0)
	{
		g_pOSD->Data()->SetItem(L"warnings", L"There are no active BDA cards");
		g_pTv->ShowOSDItem(L"Warnings", 5);
		return (log << "There are no active BDA cards\n").Show(E_FAIL);
	}

	//Restore to the first tuner sink graph
	m_pCurrentSink = (*m_tuners.begin())->pSink;
	m_pCurrentTuner = (*m_tuners.begin())->pTuner;
	m_pCurrentFilterList = (*m_tuners.begin())->pFilterList;
	m_piSinkGraphBuilder = (*m_tuners.begin())->piGraphBuilder;
	m_rotEntry = (*m_tuners.begin())->rotEntry;
	m_cardId = (*m_tuners.begin())->cardId;

	indent.Release();
	(log << "Finished Initialising BDATimeShift Source\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::Destroy()
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
		if FAILED(hr = UnloadFileSource())
			(log << "Failed to unload Sink Filters\n").Write();

		CAutoLock tunersLock(&m_tunersLock);
		std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
		for ( ; it != m_tuners.end() ; it++ )
		{
			if(*it)
			{
				m_pCurrentSink = (*it)->pSink;
				m_pCurrentTuner = (*it)->pTuner;
				m_pCurrentFilterList = (*it)->pFilterList;
				m_piSinkGraphBuilder = (*it)->piGraphBuilder;
				m_rotEntry = (*it)->rotEntry;
				m_cardId = (*it)->cardId;

				if FAILED(hr = m_pDWGraph->Stop(m_piSinkGraphBuilder))
					(log << "Failed to stop DW Sink Graph\n").Write();

				if FAILED(hr = UnloadSink())
					(log << "Failed to unload Sink Filters\n").Write();

				if FAILED(hr = UnloadTuner())
					(log << "Failed to unload tuner\n").Write();

				if FAILED(hr = m_pDWGraph->Cleanup(m_piSinkGraphBuilder))
					(log << "Failed to cleanup DW Sink Graph\n").Write();

				if (m_rotEntry)
				{
					graphTools.RemoveFromRot(m_rotEntry);
					m_rotEntry = 0;
				}

				if (m_pCurrentFilterList)
				{
					m_pCurrentFilterList->Destroy();
					delete m_pCurrentFilterList;
				}

				if (m_pCurrentSink)
					delete m_pCurrentSink;

				if (m_pCurrentTuner)
					delete m_pCurrentTuner;

				if (m_piSinkGraphBuilder)
					m_piSinkGraphBuilder.Release();

				delete *it;
			}

		}
		m_tuners.clear();

	}

	m_pDWGraph = NULL;

	if (m_piGraphBuilder)
		m_piGraphBuilder.Release();
	
	cardList.Destroy();
	frequencyList.Destroy();
	channels.Destroy();

	indent.Release();
	(log << "Finished Destroying BDA Source\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::ExecuteCommand(ParseLine* command)
{
	(log << "BDADVBTimeShift::ExecuteCommand - " << command->LHS.Function << "\n").Write();
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
/*DWS		if (command->LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		if (command->LHS.ParameterCount > 1)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		return ToggleRecording(n1);
*/
		if (command->LHS.ParameterCount <= 0)
//DWS28-02-2006			return (log << "Expecting 1 or 2 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);
			return (log << "Expecting 1 or 2 or 3 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

//DWS28-02-2006		if (command->LHS.ParameterCount > 2)
		if (command->LHS.ParameterCount > 3)
			return (log << "Too many parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		n1 = StringToLong(command->LHS.Parameter[0]);

		n2 = 0;
/*DWS28-02-2006		
		if (command->LHS.ParameterCount >= 2)
		{
			n2 = (int)command->LHS.Parameter[1];
			return ToggleRecording(n1, (LPWSTR)n2);
		}
		else
			return ToggleRecording(n1);
*/
		if (command->LHS.ParameterCount >= 3)
		{
			n2 = (int)command->LHS.Parameter[1];
			n3 = (int)command->LHS.Parameter[2];
			return ToggleRecording(n1, (LPWSTR)n2, (LPWSTR)n3);
		}
		else
		{
			if (command->LHS.ParameterCount >= 2)
			{
				n2 = (int)command->LHS.Parameter[1];
				return ToggleRecording(n1, (LPWSTR)n2);
			}
			else
				return ToggleRecording(n1);
		}

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
	else if (_wcsicmp(pCurr, L"ReLoadTimeShiftFile") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting no parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ReLoadTimeShiftFile();
	}
	if (_wcsicmp(pCurr, L"ShowTSFilter") == 0)
	{
		if (command->LHS.ParameterCount != 1)
			return (log << "Expecting 1 parameter: " << command->LHS.Function << "\n").Show(E_FAIL);

		return ShowFilter(command->LHS.Parameter[0]);
	}
	else if (_wcsicmp(pCurr, L"GetTSFilterList") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return GetFilterList();
	}
	else if (_wcsicmp(pCurr, L"CloseBuffers") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return CloseBuffers();
	}
	else if (_wcsicmp(pCurr, L"MinimiseBuffers") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pTv->MinimiseScreen();
		return CloseBuffers();
	}
	else if (_wcsicmp(pCurr, L"MinimiseDisplay") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		g_pTv->MinimiseScreen();
		return CloseDisplay();
	}
	else if (_wcsicmp(pCurr, L"CloseDisplay") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return CloseDisplay();
	}
	else if (_wcsicmp(pCurr, L"OpenDisplay") == 0)
	{
		if (command->LHS.ParameterCount != 0)
			return (log << "Expecting 0 parameters: " << command->LHS.Function << "\n").Show(E_FAIL);

		return OpenDisplay();
	}




	if (m_pCurrentFileSource)
		return m_pCurrentFileSource->ExecuteCommand(command);

	//Just referencing these variables to stop warnings.
	n3 = 0;
	n4 = 0;
	return S_FALSE;
}

BOOL BDADVBTimeShift::CanLoad(LPWSTR pCmdLine)
{
	long length = wcslen(pCmdLine);
	if ((length >= 5) && (_wcsnicmp(pCmdLine, L"ts://", 5) == 0))
	{
		return TRUE;
	}
	return FALSE;
}

HRESULT BDADVBTimeShift::Load(LPWSTR pCmdLine)
{
	if (!m_pDWGraph)
		return (log << "Filter graph not set in BDADVBTimeShift::Load\n").Write(E_FAIL);

	HRESULT hr;

	if(!m_piGraphBuilder)
		if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
			return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	if (!pCmdLine)
	{
		//TODO: replace this with last selected channel, or menu depending on options.
		DVBTChannels_Network* pNetwork = channels.FindDefaultNetwork();
		DVBTChannels_Service* pService = (pNetwork ? pNetwork->FindDefaultService() : NULL);

		if (pService)
		{
			if (pService == m_pCurrentService)
				return S_OK;

			return RenderChannel(pNetwork, pService);
		}
		else
		{
			hr = g_pTv->ShowMenu(L"TVMenu");
			(log << "No channel found. Loading TV Menu : " << hr << "\n").Write();
			return E_FAIL;
		}
	}

	if (_wcsnicmp(pCmdLine, L"ts://", 5) != 0)
		return S_FALSE;

	pCmdLine += 5;
	if (pCmdLine[0] == '\0')
	{
		(log << "Loading default network and service\n").Write();
		DVBTChannels_Network* pNetwork = channels.FindDefaultNetwork();
		DVBTChannels_Service* pService = (pNetwork ? pNetwork->FindDefaultService() : NULL);
		if (pService)
		{
			if (pService == m_pCurrentService)
				return S_OK;

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

DVBTChannels *BDADVBTimeShift::GetChannels()
{
	return &channels;
}

void BDADVBTimeShift::ThreadProc()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	m_rtTimeShiftStart = timeGetTime()/60000;
	m_rtTimeShiftDuration = m_rtTimeShiftStart;

	while (!ThreadIsStopping())
	{
		UpdateData();
		Sleep(100);
	}
}

HRESULT BDADVBTimeShift::SetChannel(long originalNetworkId, long serviceId)
{
	(log << "Setting Channel (" << originalNetworkId << ", " << serviceId << ")\n").Write();
	LogMessageIndent indent(&log);

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

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTimeShift::SetChannel(long originalNetworkId, long transportStreamId, long networkId, long serviceId)
{
	(log << "Setting Channel (" << originalNetworkId << ", " << serviceId << ")\n").Write();
	LogMessageIndent indent(&log);

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

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTimeShift::SetFrequency(long frequency, long bandwidth)
{
	(log << "Setting Frequency (" << frequency << ", " << bandwidth << ")\n").Write();
	LogMessageIndent indent(&log);

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

		if (m_pCurrentNetwork->FindDefaultService() == m_pCurrentService)
			return S_OK;

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

HRESULT BDADVBTimeShift::NetworkUp()
{
	if(!m_pCurrentTuner)
		return (log << "There are no Tuner Class Loaded\n").Write(E_POINTER);;

	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindNextNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindDefaultService();
	if (!pService)
		return (log << "There are no services for the network " << pNetwork->originalNetworkId << "\n").Write(E_POINTER);

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTimeShift::NetworkDown()
{
	if(!m_pCurrentTuner)
		return (log << "There are no Tuner Class Loaded\n").Write(E_POINTER);;

	if (channels.GetListSize() <= 0)
		return (log << "There are no networks in the channels file\n").Write(E_POINTER);

	long frequency = m_pCurrentTuner->GetCurrentFrequency();
	DVBTChannels_Network* pNetwork = channels.FindPrevNetworkByFrequency(frequency);
	if (!pNetwork)
		return (log << "Currently tuned frequency (" << frequency << ") is not in the channels file\n").Write(E_POINTER);

	DVBTChannels_Service* pService = pNetwork->FindDefaultService();
	if (!pService)
		return (log << "There are no services for the network " << pNetwork->originalNetworkId << "\n").Write(E_POINTER);

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTimeShift::ProgramUp()
{
	if(!m_pCurrentTuner)
		return (log << "There are no Tuner Class Loaded\n").Write(E_POINTER);;

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

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}

HRESULT BDADVBTimeShift::ProgramDown()
{
	if(!m_pCurrentTuner)
		return (log << "There are no Tuner Class Loaded\n").Write(E_POINTER);;

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

	if (pService == m_pCurrentService)
		return S_OK;

	return RenderChannel(pNetwork, pService);
}


//////////////////////////////////////////////////////////////////////
// graph building methods
//////////////////////////////////////////////////////////////////////

HRESULT BDADVBTimeShift::RenderChannel(DVBTChannels_Network* pNetwork, DVBTChannels_Service* pService)
{
	if (pService == m_pCurrentService)
		return S_OK;

	m_pCurrentNetwork = pNetwork;
	m_pCurrentService = pService;

	return RenderChannel(pNetwork->frequency, pNetwork->bandwidth);
}

HRESULT BDADVBTimeShift::RenderChannel(int frequency, int bandwidth)
{
	if(m_pCurrentFileSource && m_pCurrentService && m_pCurrentService->serviceName &&
			g_pData->values.timeshift.format == 1)
	{
		if SUCCEEDED(m_pCurrentFileSource->SetStreamName(m_pCurrentService->serviceName, FALSE))
			return S_OK;
	}

	(log << "Building BDATimeShift Graph (" << frequency << ", " << bandwidth << ")\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;
	g_pOSD->Data()->SetItem(L"recordingicon", L"C");
	g_pTv->ShowOSDItem(L"RecordingIcon", 100000);

	(log << "Checking if service already running or we have multicard option set\n").Write();
	//Check if service already running or we have multicard set
	if (m_pDWGraph->IsPlaying() || g_pData->values.application.multicard)
	{
		//Save the current sink tuner graph	
		TUNER_GRAPH *tuner = new TUNER_GRAPH;
		tuner->pSink = m_pCurrentSink;
		tuner->pTuner = m_pCurrentTuner;
		tuner->pFilterList = m_pCurrentFilterList;
		tuner->piGraphBuilder = m_piSinkGraphBuilder;
		tuner->rotEntry = m_rotEntry;
		tuner->cardId = m_cardId;

		CAutoLock tunersLock(&m_tunersLock);
		std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
		for ( ; TRUE /*check for end of list done after graph is cleaned up*/ ; it++ )
		{
			if (it == m_tuners.end())
				break; //exit if no more graphs

			if ((*it)->pTuner->IsActive())
			{
				// if graph is not running then its free to use
				m_pCurrentSink = (*it)->pSink;
				m_pCurrentTuner	= (*it)->pTuner;
				m_pCurrentFilterList = (*it)->pFilterList;
				m_piSinkGraphBuilder = (*it)->piGraphBuilder;
				m_rotEntry = (*it)->rotEntry;
				m_cardId = (*it)->cardId;
				
				if (((*it)->networkId && m_pCurrentNetwork && //Full Mux
					(*it)->networkId == m_pCurrentNetwork->networkId &&
					g_pData->values.timeshift.format == 1) ||
					((*it)->networkId && m_pCurrentNetwork && //Not Full Mux
					(*it)->networkId == m_pCurrentNetwork->networkId &&
					(*it)->serviceId && m_pCurrentService &&
					(*it)->serviceId == m_pCurrentService->serviceId &&
					g_pData->values.timeshift.format != 1))
				{
					// Stop background thread
					if FAILED(hr = StopThread())
						return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);
					if (hr == S_FALSE)
						(log << "Killed thread\n").Write();

					m_rtTimeShiftStart = timeGetTime()/60000;
					m_rtTimeShiftDuration = m_rtTimeShiftStart;

					//Check if service already running
					if (m_pDWGraph->IsPlaying())
					{
						if FAILED(hr = ReLoadTimeShiftFile())
						{
							(log << "Failed to Reload Timeshift File\n").Write();
							continue;
						}
					}
					else //If not playing then load player
					{
						if FAILED(hr = LoadFileSource())
						{
							(log << "Failed to load File Source Filters\n").Write();
							continue;
						}
					}

					// Start the background thread for updating statistics
					if FAILED(hr = StartThread())
						(log << "Failed to start background thread: " << hr << "\n").Write();

					//Move current tuner to back of list so that other cards will be used next
					tuner->pSink = m_pCurrentSink;
					tuner->pTuner = m_pCurrentTuner;
					tuner->pFilterList = m_pCurrentFilterList;
					tuner->piGraphBuilder = m_piSinkGraphBuilder;
					tuner->isRecording = m_pCurrentSink->IsRecording();
					tuner->rotEntry = m_rotEntry;
					tuner->networkId = m_pCurrentNetwork->networkId;
					tuner->serviceId = m_pCurrentService->serviceId;
					tuner->cardId = m_cardId;
					m_tuners.erase(it);
					m_tuners.push_back(tuner);

					RotateFilterList();

					UpdateStatusDisplay();

					// Start the background thread for updating statistics
					if FAILED(hr = StartThread())
						(log << "Failed to start background thread: " << hr << "\n").Write();

					indent.Release();
					(log << "Finished Setting Channel\n").Write();
					return S_OK;
				}
			}
		};

		//Restore the old tuner sink graph
		m_pCurrentSink = tuner->pSink;
		m_pCurrentTuner = tuner->pTuner;
		m_pCurrentFilterList = tuner->pFilterList;
		m_piSinkGraphBuilder = tuner->piGraphBuilder;
		m_rotEntry = tuner->rotEntry;
		m_cardId = tuner->cardId;

		RotateFilterList();

		delete tuner;
	}

	(log << "If service is not already running then look for free card and load it\n").Write();
	//If service is not already running then look for free card and load it
	if (m_pDWGraph->IsPlaying() &&	g_pData->settings.application.multicard)
	{
		//Save the current sink tuner graph	
		TUNER_GRAPH *tuner = new TUNER_GRAPH;
		tuner->pSink = m_pCurrentSink;
		tuner->pTuner = m_pCurrentTuner;
		tuner->pFilterList = m_pCurrentFilterList;
		tuner->piGraphBuilder = m_piSinkGraphBuilder;
		tuner->rotEntry = m_rotEntry;
		tuner->cardId = m_cardId;

		CAutoLock tunersLock(&m_tunersLock);
		std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
		for ( ; TRUE /*check for end of list done after graph is cleaned up*/ ; it++ )
		{
			if (it == m_tuners.end())
				break; //exit if no more graphs

			if ((*it)->pSink->IsRecording())
				continue; //do next card if this graph is recording

			if (!(*it)->pTuner->IsActive())	// if graph is not running then its free to use
			{
				m_pCurrentSink = (*it)->pSink;
				m_pCurrentTuner	= (*it)->pTuner;
				m_pCurrentFilterList = (*it)->pFilterList;
				m_piSinkGraphBuilder = (*it)->piGraphBuilder;
				m_rotEntry = (*it)->rotEntry;
				m_cardId = (*it)->cardId;

				if FAILED(hr = LoadSinkGraph(frequency, bandwidth))
				{
					(log << "Failed to Load the Sink Graph\n").Write();
					if FAILED(hr = UnLoadSinkGraph())
						(log << "Failed to Unload Sink Graph\n").Write();

					continue;
				}

				if (tuner->pTuner && tuner->pTuner->IsActive())
					if FAILED(hr = tuner->pTuner->StopScanning())
						(log << "Failed to Stop the previous Tuner from Scanning\n").Write();

				//Move current tuner to back of list so that other cards will be used next
				tuner->pSink = m_pCurrentSink;
				tuner->pTuner = m_pCurrentTuner;
				tuner->pFilterList = m_pCurrentFilterList;
				tuner->piGraphBuilder = m_piSinkGraphBuilder;
				tuner->isRecording = m_pCurrentSink->IsRecording();
				tuner->rotEntry = m_rotEntry;
				tuner->networkId = m_pCurrentNetwork->networkId;
				tuner->serviceId = m_pCurrentService->serviceId;
				tuner->cardId = m_cardId;
				m_tuners.erase(it);
				m_tuners.push_back(tuner);

				RotateFilterList();

				UpdateStatusDisplay();

				indent.Release();
				(log << "Finished Setting Channel\n").Write();

				return S_OK;
			}
		};

		//Restore the old tuner sink graph
		m_pCurrentSink = tuner->pSink;
		m_pCurrentTuner = tuner->pTuner;
		m_pCurrentFilterList = tuner->pFilterList;
		m_piSinkGraphBuilder = tuner->piGraphBuilder;
		m_rotEntry = tuner->rotEntry;
		m_cardId = tuner->cardId;

		RotateFilterList();

		delete tuner;
	}

	hr = S_OK;
	//Save the current sink tuner graph	
	TUNER_GRAPH *tuner = new TUNER_GRAPH;
	tuner->pSink = m_pCurrentSink;
	tuner->pTuner = m_pCurrentTuner;
	tuner->pFilterList = m_pCurrentFilterList;
	tuner->piGraphBuilder = m_piSinkGraphBuilder;
	tuner->rotEntry = m_rotEntry;
	tuner->cardId = m_cardId;

	(log << "Seems to be no free graphs or we are not playing any so lets force a load\n").Write();
	//Seems to be no free graphs or we are not playing any so lets force a load 
	CAutoLock tunersLock(&m_tunersLock);
	std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
	for ( ; TRUE; /*check for end of list done after graph is cleaned up*/ it++ ) 
	{
		// check for end of list done here
		if (it == m_tuners.end())
			break;

		//TODO: Check if recording
		if ((*it)->pSink && g_pData->values.capture.format && (*it)->pSink->IsRecording())
		{
			g_pData->values.application.multicard = TRUE;
			(log << "Unable to use this sink graph, Recording Still in Progress\n").Write();
			continue;
		}

		m_pCurrentSink = (*it)->pSink;
		m_pCurrentTuner	= (*it)->pTuner;
		m_pCurrentFilterList = (*it)->pFilterList;
		m_piSinkGraphBuilder = (*it)->piGraphBuilder;
		m_rotEntry = (*it)->rotEntry;
		m_cardId = (*it)->cardId;

		if (m_pCurrentTuner->IsActive())	// if graph is running then lets unload it so its free to use
		{
			if FAILED(hr = UnLoadSinkGraph())
			{
				(log << "Failed to UnLoad the Sink Graph\n").Write();
			}
		}

		if FAILED(hr = LoadSinkGraph(frequency, bandwidth))
		{
			if FAILED(hr = UnLoadSinkGraph())
				(log << "Failed to Unload Sink Graph\n").Write();

			(log << "Failed to Load the Sink Graph\n").Write();
			continue;
		}

		if (tuner->pTuner && tuner->pTuner->IsActive())
			if FAILED(hr = tuner->pTuner->StopScanning())
				(log << "Failed to Stop the previous Tuner from Scanning\n").Write();

		//Move current tuner to back of list so that other cards will be used next
		TUNER_GRAPH *tuner = new TUNER_GRAPH;
		tuner->pSink = m_pCurrentSink;
		tuner->pTuner = m_pCurrentTuner;
		tuner->pFilterList = m_pCurrentFilterList;
		tuner->piGraphBuilder = m_piSinkGraphBuilder;
		tuner->isRecording = m_pCurrentSink->IsRecording();
		tuner->rotEntry = m_rotEntry;
		tuner->networkId = m_pCurrentNetwork->networkId;
		tuner->serviceId = m_pCurrentService->serviceId;
		tuner->cardId = m_cardId;
		m_tuners.erase(it);
		m_tuners.push_back(tuner);

		RotateFilterList();
	
		UpdateStatusDisplay();

		indent.Release();
		(log << "Finished Setting Channel\n").Write();

		return S_OK;
	}

	(log << "Seems to be no free graphs so lets use the same card.\n").Write();

	//Check if service already running
	if (m_pDWGraph->IsPlaying())
	{
		if FAILED(hr = UnloadFileSource())
			(log << "Failed to Unload the File Source Filters\n").Write();
	}

	//Release the current card
	if (!g_pData->settings.application.multicard && !(m_pCurrentSink && m_pCurrentSink->IsRecording()))
	{
		if FAILED(hr = UnLoadSinkGraph())
		{
			(log << "Failed to UnLoad the Sink Graph\n").Write();
		}
	}

	//Restore the old tuner sink graph
	m_pCurrentSink = tuner->pSink;
	m_pCurrentTuner = tuner->pTuner;
	m_pCurrentFilterList = tuner->pFilterList;
	m_piSinkGraphBuilder = tuner->piGraphBuilder;
	m_rotEntry = tuner->rotEntry;
	m_cardId = tuner->cardId;

	if (m_pCurrentTuner && m_pCurrentTuner->IsActive())	// if sink graph is still running 
	{
		//TODO: Check if recording
		if (m_pCurrentSink && g_pData->values.capture.format && m_pCurrentSink->IsRecording())
		{
			delete tuner;

			//Check if service already running
			if (m_pDWGraph->IsPlaying())
			{
				if FAILED(hr = ReLoadTimeShiftFile())
				{
					return (log << "Failed to Reload Timeshift File: " << hr << "\n").Write(hr);
				}
			}
			else //If not playing then load player
			{
				if FAILED(hr = LoadFileSource())
				{
					return (log << "Failed to load File Source Filters: " << hr << "\n").Write(hr);
				}
			}

			// Start the background thread for updating statistics
			if FAILED(hr = StartThread())
				(log << "Failed to start background thread: " << hr << "\n").Write();

			UpdateStatusDisplay();

			return (log << "Unable to SetChannel, Recording Still in Progress\n").Write(hr);
		}

		if FAILED(hr = UnLoadSinkGraph())
		{
			(log << "Failed to UnLoad the Sink Graph\n").Write();
		}
	}

	if FAILED(hr = LoadSinkGraph(frequency, bandwidth))
	{
		if FAILED(hr = UnLoadSinkGraph())
			(log << "Failed to Unload Sink Graph\n").Write();

		(log << "Failed to Load Sink Graph\n").Write();
		delete tuner;
		return (log << "Failed to start the graph: " << hr << "\n").Write(hr);
	}

	UpdateStatusDisplay();

	RotateFilterList();

	delete tuner;

	return (log << "Failed to start the graph: " << hr << "\n").Write(hr);
}

void BDADVBTimeShift::UpdateStatusDisplay()
{
	g_pOSD->Data()->SetItem(L"recordingicon", L"");
	if (m_pCurrentSink && m_pCurrentSink->IsRecording())
	{
		if (m_pCurrentSink->IsPaused())
			g_pOSD->Data()->SetItem(L"recordingicon", L"P");
		else
			g_pOSD->Data()->SetItem(L"recordingicon", L"R");

		g_pTv->ShowOSDItem(L"RecordingIcon", 100000);

		// Stop Tuner Scanner while recording
		if (m_pCurrentTuner && m_pCurrentTuner->IsActive())
			if FAILED(m_pCurrentTuner->StopScanning())
				(log << "Failed to Stop the Tuner from Scanning while recording.\n").Write();
	}
	else
	{
		g_pTv->HideOSDItem(L"RecordingIcon");
	}

	if (m_pCurrentNetwork)
		g_pTv->ShowOSDItem(L"Channel", 10);

	g_pOSD->Data()->SetItem(L"CurrentDVBTCard", m_pCurrentTuner->GetCardName());
}


HRESULT BDADVBTimeShift::ReLoadTimeShiftFile()
{
	HRESULT hr;

	//Check if service already running
	if (m_pDWGraph->IsPlaying())
	{
		LPOLESTR pFileName = NULL;
		if(m_pCurrentSink)
			m_pCurrentSink->GetCurFile(&pFileName);

		if (!pFileName)
		{
			hr = E_FAIL;
			return (log << "Failed to Get Sink Filter File Name: " << hr << "\n").Write(hr);
		}

		// Wait up to 2 sec for file to grow
		int count =0;
		__int64 fileSize = 0;
		(log << "Waiting for the Sink File to grow: " << pFileName << "\n").Write();
		while(SUCCEEDED(hr = m_pCurrentSink->GetCurFileSize(&fileSize)) &&
				fileSize < (__int64)20000 && count < 20)
		{
			Sleep(100);
			(log << "Waiting for Sink File to Build: " << fileSize << " Bytes\n").Write();
			count++;
		}

//		g_pOSD->Data()->SetItem(L"warnings", L"Now Loading TimeShift File");
//		g_pTv->ShowOSDItem(L"Warnings", 2);
		if FAILED(hr = m_pCurrentFileSource->ReLoad(pFileName))
			return (log << "Failed to ReLoad File Source filters: " << hr << "\n").Write(hr);

		m_pCurrentFileSource->SeekTo(100);

		UpdateStatusDisplay();
	}
	else if (m_pCurrentTuner->IsActive())	// if sink graph is still running 
	{
		//If not playing then load player
		if FAILED(hr = LoadFileSource())
		{
			(log << "Failed to load File Source Filters\n").Write();
			return (log << "Failed to Load File Source filters: " << hr << "\n").Write(hr);
		}

		m_pCurrentFileSource->SeekTo(100);

		UpdateStatusDisplay();

		if (!m_pCurrentSink || !m_pCurrentSink->IsRecording())
		{
			// Start the background thread for updating channels
			if FAILED(hr = m_pCurrentTuner->StartScanning())
				(log << "Failed to start channel scanning: " << hr << "\n").Write();
 		}
	}

	return hr;
}

HRESULT BDADVBTimeShift::CloseBuffers()
{
	HRESULT hr = S_OK;

	//Save the current sink tuner graph	
	TUNER_GRAPH *tuner = new TUNER_GRAPH;
	tuner->pSink = m_pCurrentSink;
	tuner->pTuner = m_pCurrentTuner;
	tuner->pFilterList = m_pCurrentFilterList;
	tuner->piGraphBuilder = m_piSinkGraphBuilder;
	tuner->rotEntry = m_rotEntry;
	tuner->cardId = m_cardId;

	CAutoLock tunersLock(&m_tunersLock);
	std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
	for ( ; TRUE; /*check for end of list done after graph is cleaned up*/ it++ ) 
	{
		// check for end of list done here
		if (it == m_tuners.end())
			break;

		if ((*it)->pTuner == tuner->pTuner)
			continue; //do next card if this graph is current

		//TODO: Check if recording
		if ((*it)->pSink && g_pData->values.capture.format && (*it)->pSink->IsRecording())
		{
			g_pData->values.application.multicard = TRUE;
			g_pTv->ShowOSDItem(L"Recording", 5);
			g_pOSD->Data()->SetItem(L"warnings", L"Recording In Progress");
			g_pTv->ShowOSDItem(L"Warnings", 5);
			g_pOSD->Data()->SetItem(L"recordingicon", L"R");
			g_pTv->ShowOSDItem(L"RecordingIcon", 100000);

			(log << "Unable to Clear Sink Graph, Recording Still in Progress\n").Write();
			continue;
		}

		m_pCurrentSink = (*it)->pSink;
		m_pCurrentTuner	= (*it)->pTuner;
		m_pCurrentFilterList = (*it)->pFilterList;
		m_piSinkGraphBuilder = (*it)->piGraphBuilder;
		m_rotEntry = (*it)->rotEntry;
		m_cardId = tuner->cardId;

		//Release the current card
		if (m_pCurrentTuner->IsActive())	// if graph is running then lets unlod it so its free to use
		{
			if FAILED(hr = UnLoadSinkGraph())
			{
				(log << "Failed to UnLoad the Sink Graph\n").Write();
			}
		}
	}

	//Restore the old tuner sink graph
	m_pCurrentSink = tuner->pSink;
	m_pCurrentTuner = tuner->pTuner;
	m_pCurrentFilterList = tuner->pFilterList;
	m_piSinkGraphBuilder = tuner->piGraphBuilder;
	m_rotEntry = tuner->rotEntry;
	m_cardId = tuner->cardId;

	RotateFilterList();

	delete tuner;

	return hr;
}

HRESULT BDADVBTimeShift::CloseDisplay()
{
	HRESULT hr = S_OK;

	// Stop background thread
	if FAILED(hr = StopThread())
		return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);
	if (hr == S_FALSE)
		(log << "Killed thread\n").Write();

	//Check if service already running
	if (m_pDWGraph->IsPlaying())
	{
		if FAILED(hr = UnloadFileSource())
			return (log << "Failed to Unload the File Source Filters: " << hr << "\n").Write(hr);
	}
	return hr;
}

HRESULT BDADVBTimeShift::OpenDisplay()
{
	//Check if service already running
	if (!m_pDWGraph->IsPlaying())
		return 	ReLoadTimeShiftFile();

	if (m_pCurrentNetwork)
		g_pTv->ShowOSDItem(L"Channel", 10);

	g_pOSD->Data()->SetItem(L"CurrentDVBTCard", m_pCurrentTuner->GetCardName());

	return S_OK;
}

HRESULT BDADVBTimeShift::LoadSinkGraph(int frequency, int bandwidth)
{
	(log << "Loading Sink Graph No: " << m_cardId << "\n").Write();
	HRESULT hr = S_OK;

	if FAILED(hr = LoadTuner())
		return (log << "Failed to load Source Tuner: " << hr << "\n").Write(hr);

	if FAILED(hr = m_pCurrentTuner->LockChannel(frequency, bandwidth))
		return (log << "Failed to Lock Channel: " << hr << "\n").Write(hr);

	BOOL sinkFail = TRUE;
	if (m_pCurrentService)
	{
		if FAILED(hr = LoadSink())
		{
			if (!g_pData->values.timeshift.format)
			{
				g_pOSD->Data()->SetItem(L"warnings", L"No TimeShift format set");
				g_pTv->ShowOSDItem(L"Warnings", 5);
				(log << "No TimeShift format set\n").Write();
				if FAILED(hr = LoadDemux())
					return (log << "Failed to Add DeMultiplexer: " << hr << "\n").Write(hr);

				if FAILED(hr = AddDemuxPins(m_pCurrentService))
					return (log << "Failed to Add Demux Pins: " << hr << "\n").Write(hr);
				else
					m_bFileSourceActive = FALSE;
			}
			else
				return (log << "Failed to Load Sink: " << hr << "\n").Write(hr);
		}
		else
			sinkFail = FALSE;
	}

	// Set the thread state to keep the computer awake
//		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

	// Set priority to HIGH
//	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	if FAILED(hr = m_pDWGraph->Start(m_piSinkGraphBuilder, TRUE))
	{
		HRESULT hr2;
		if FAILED(hr2 = m_pDWGraph->Stop(m_piSinkGraphBuilder))
			(log << "Failed to stop DW Sink Graph\n").Write();

		return (log << "Failed to Start Graph. Possibly tuner already in use: " << hr << "\n").Write(hr);
	}

	if FAILED(hr = m_pCurrentTuner->StopTIF())
		return (log << "Failed to stop the BDA TIF Filter: " << hr << "\n").Write(hr);

	if (SUCCEEDED(hr) && m_pCurrentService && !sinkFail)
	{
		// Stop background thread
		if FAILED(hr = StopThread())
			return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);
		if (hr == S_FALSE)
			(log << "Killed thread\n").Write();

		// Do data stuff
		UpdateData(frequency, bandwidth);

		if (m_pCurrentNetwork)
			g_pTv->ShowOSDItem(L"Channel", 10);

		m_rtTimeShiftStart = timeGetTime()/60000;
		m_rtTimeShiftDuration = m_rtTimeShiftStart;


		//Check if service already running
		if (m_pDWGraph->IsPlaying())
		{
			(log << "TSFileSource Already Playing Now Unloading.\n").Write();

			if FAILED(hr = UnloadFileSource())
				(log << "Failed to Unload the File Source Filters\n").Write();

			(log << "TSFileSource Now Unloaded.\n").Write();
		}

		if FAILED(hr = LoadFileSource())
			return (log << "Failed to load File Source Filters: " << hr << "\n").Write(hr);
	}

	// Start the background thread for updating channels
	if FAILED(hr = m_pCurrentTuner->StartScanning())
		(log << "Failed to start channel scanning: " << hr << "\n").Write();
 
	// Start the background thread for updating statistics
	if FAILED(hr = StartThread())
		(log << "Failed to start background thread: " << hr << "\n").Write();

	(log << "Finished Loading Sink Graph No: " << m_cardId << "\n").Write();
	return hr;
}

HRESULT BDADVBTimeShift::UnLoadSinkGraph()
{
	(log << "UnLoading Sink Graph No: " << m_cardId << "\n").Write();

	HRESULT hr = S_OK;

	if (m_pCurrentTuner && m_pCurrentTuner->IsActive())
		if FAILED(hr = m_pCurrentTuner->StopScanning())
			(log << "Failed to Stop the previous Tuner from Scanning\n").Write();

	if FAILED(hr = m_pDWGraph->Stop(m_piSinkGraphBuilder))
		(log << "Failed to Stop Sink Graph\n").Write();

	if FAILED(hr = UnloadSink())
		(log << "Failed to Unload Sink Filters\n").Write();

	if FAILED(hr = UnloadTuner())
		(log << "Failed to Unload Tuner Filters\n").Write();

	if FAILED(hr = m_pDWGraph->Cleanup(m_piSinkGraphBuilder))
		(log << "Failed to Cleanup Sink Graph\n").Write();

	(log << "Finished UnLoading Sink Graph No: " << m_cardId << "\n").Write();
	return hr;
}

HRESULT BDADVBTimeShift::LoadTuner()
{
	(log << "Loading Tuner\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if FAILED(hr = m_pCurrentTuner->AddSourceFilters())
		return (log << "Failed to add source filters: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Loading Tuner\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::LoadDemux()
{
	(log << "Loading DW DeMultiplexer\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin: " << hr << "\n").Write(hr);

	//MPEG-2 Demultiplexer (DW's)
	if FAILED(hr = graphTools.AddFilter(m_piSinkGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
		return (log << "Failed to add DW MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);

	m_piBDAMpeg2Demux.QueryInterface(&m_piMpeg2Demux);

	CComPtr <IPin> piDemuxPin;
	if FAILED(hr = graphTools.FindFirstFreePin(m_piBDAMpeg2Demux, &piDemuxPin, PINDIR_INPUT))
		return (log << "Failed to get input pin on DW Demux: " << hr << "\n").Write(hr);

	if FAILED(hr = m_piSinkGraphBuilder->ConnectDirect(piTSPin, piDemuxPin, NULL))
		return (log << "Failed to connect TS Pin to DW Demux: " << hr << "\n").Write(hr);

	//Set reference clock
	if FAILED(hr = graphTools.SetReferenceClock(m_piBDAMpeg2Demux))
		return (log << "Failed to get reference clock interface on demux filter: " << hr << "\n").Write(hr);

	piDemuxPin.Release();
	piTSPin.Release();

	indent.Release();
	(log << "Finished Loading DW DeMultiplexer\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::UnloadTuner()
{
	(log << "Unloading Tuner\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	m_piMpeg2Demux.Release();

	if (m_piBDAMpeg2Demux)
	{
		m_piSinkGraphBuilder->RemoveFilter(m_piBDAMpeg2Demux);
		m_piBDAMpeg2Demux.Release();
	}

	if (m_pCurrentTuner)
	{
		if FAILED(hr = m_pCurrentTuner->RemoveSourceFilters())
			return (log << "Failed to remove source filters: " << hr << "\n").Write(hr);

//		m_pCurrentTuner = NULL;
	}

	indent.Release();
	(log << "Finished Unloading Tuner\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::LoadSink()
{
	(log << "Loading Sink Filters\n").Write();
	LogMessageIndent indent(&log);

	if (!m_pCurrentSink)
		return (log << "No Main Sink Class loaded.\n").Write();

	HRESULT hr = S_OK;

	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin: " << hr << "\n").Write(hr);

	m_pCurrentSink->SetTransportStreamPin(piTSPin);

	if FAILED(hr = m_pCurrentSink->AddSinkFilters(m_pCurrentService))
		return (log << "Failed to add Sink filters: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Loading Sink Filters\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::UnloadSink()
{
	(log << "Unloading Sink Filters\n").Write();
	LogMessageIndent indent(&log);

	if (!m_pCurrentSink)
		return (log << "No Main Sink Class loaded.\n").Write();

	HRESULT hr;

	if FAILED(hr = m_pCurrentSink->RemoveSinkFilters())
		return (log << "Failed to remove Sink filters: " << hr << "\n").Write(hr);

	indent.Release();
	(log << "Finished Unloading Sink Filters\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::LoadFileSource()
{
	(log << "Loading File Source Filters\n").Write();
	LogMessageIndent indent(&log);

	if (!m_pCurrentFileSource)
		return (log << "No Main Sink Class loaded.\n").Write();

	m_bFileSourceActive = TRUE;

	HRESULT hr = S_OK;
/*
	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin: " << hr << "\n").Write(hr);

	m_pCurrentSink->SetTransportStreamPin(piTSPin);
*/

	//
	//Start capture sink recording if no TimeShift specified, poor mans timeshift
	//
//	if(!g_pData->values.timeshift.format && (g_pData->values.capture.format & 0x07))
//		m_pCurrentSink->StartRecording(m_pCurrentService);

	LPOLESTR pFileName = NULL;
	if(m_pCurrentSink)
		m_pCurrentSink->GetCurFile(&pFileName);

	if (!pFileName)
	{
		hr = E_FAIL;
		return (log << "Failed to Get Sink Filter File Name: " << hr << "\n").Write(hr);
	}

	//
	//Do file wait method if specified. 
	//
	if(g_pData->values.timeshift.flimit)
	{
		// Wait up to 5 sec for file to grow
		int count =0;
		int maxcount =0;
		__int64 fileSize = 0;
		__int64 fileSizeSave = 0;

		//
		//Wait until the file has grown to the specified size or break if no flow for sepcified time
		//
		(log << "Waiting for the Sink File to grow: " << pFileName << "\n").Write();
		while(SUCCEEDED(hr = m_pCurrentSink->GetCurFileSize(&fileSize)) &&
				fileSize < (__int64)max(2000000, g_pData->values.timeshift.flimit) &&
				count < max(1, (g_pData->values.timeshift.dlimit/500)) &&
				maxcount < 80)
		{
			(log << "Waiting for Sink File to Build: " << fileSize << " Bytes\n").Write();
			count++;
			maxcount++;
			fileSizeSave++;
			if (fileSize > fileSizeSave)
				count--;

			Sleep(500);
//LPWSTR sz = new WCHAR[128];
//wsprintfW(sz, L"Waiting for TimeShifting File to Build: %lu kBytes", fileSize/1024); 
//g_pOSD->Data()->SetItem(L"warnings", sz);
//g_pTv->ShowOSDItem(L"Warnings", 5);
//delete[] sz;
		}

		//
		//Check if the file has stopped growing
		//
		if (FAILED(hr = m_pCurrentSink->GetCurFileSize(&fileSize)) || fileSize <= fileSizeSave)
		{
			g_pOSD->Data()->SetItem(L"warnings", L"FAILED Building The TimeShift File");
			g_pTv->ShowOSDItem(L"Warnings", 5);
			return (log << "Data Flow Stopped on the Sink File: " << fileSize << "\n").Write(E_FAIL);
		}

//g_pOSD->Data()->SetItem(L"warnings", L"Now Loading TimeShift File");
//g_pTv->ShowOSDItem(L"Warnings", 2);

		//
		//Load the TSFileSource with the file, render & run
		//
		if FAILED(hr = m_pCurrentFileSource->Load(pFileName))//LoadTSFile(pFileName, m_pCurrentService, NULL))
		{
			g_pOSD->Data()->SetItem(L"warnings", L"FAILED Loading TimeShift File");
			g_pTv->ShowOSDItem(L"Warnings", 5);
			return (log << "Failed to Load File Source filters: " << hr << "\n").Write(hr);
		}

//		if FAILED(hr = m_pCurrentFileSource->SetRate(0.99))
//		{
//			return (log << "Failed to Set File Source Rate: " << hr << "\n").Write(hr);
//		}
	
		//
		// Do a pause if it has been requested in the settings, just adds extra delay
		//
		if(g_pData->values.timeshift.fdelay)
		{
			if FAILED(hr = m_pDWGraph->Pause(TRUE))
			{
				(log << "Failed to Pause Graph.\n").Write();
			}

			Sleep(max(50, g_pData->values.timeshift.fdelay));

			if FAILED(hr = m_pDWGraph->Pause(FALSE))
			{
				(log << "Failed to Pause Graph.\n").Write();
			}
		}
	}
	else
	{
		// Wait up to up to 2 sec for file to grow
		int count =0;
		int maxcount =0;
		__int64 fileSize = 0;
		__int64 fileSizeSave = 0;

		//
		//Wait until the file has grown to the specified size or break if no flow for sepcified time
		//
		(log << "Waiting for the Sink File to grow: " << pFileName << "\n").Write();
		while(SUCCEEDED(hr = m_pCurrentSink->GetCurFileSize(&fileSize)) &&
				fileSize < 20000 &&
				count < 20)
		{
			(log << "Waiting for Sink File to Build: " << fileSize << " Bytes\n").Write();
			count++;
			maxcount++;
			fileSizeSave++;
			Sleep(100);
		}

		//
		//Check if the file has stopped growing
		//
		if (FAILED(hr = m_pCurrentSink->GetCurFileSize(&fileSize)) || fileSize <= fileSizeSave)
		{
			g_pOSD->Data()->SetItem(L"warnings", L"FAILED Building The TimeShift File");
			g_pTv->ShowOSDItem(L"Warnings", 5);
			return (log << "Data Flow Stopped on the Sink File: " << fileSize << "\n").Write(E_FAIL);
		}

//g_pOSD->Data()->SetItem(L"warnings", L"Now Loading TimeShift File");
//g_pTv->ShowOSDItem(L"Warnings", 2);
		//
		//Set the Source pin type as we could be loading from scratch
		//
		CMediaType cmt;
		cmt.InitMediaType();
		cmt.SetType(&MEDIATYPE_Stream);
//		if (g_pData->values.timeshift.format != 4)
		if (g_pData->values.timeshift.format != 3)
			cmt.SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
		else
			cmt.SetSubtype(&MEDIASUBTYPE_MPEG2_PROGRAM);

//g_pOSD->Data()->SetItem(L"warnings", L"Now Loading TimeShift File");
//g_pTv->ShowOSDItem(L"Warnings", 2);
		//
		//Load the TSFileSource with the file, render & run
		//
		if FAILED(hr = m_pCurrentFileSource->LoadTSFile(pFileName, m_pCurrentService, &cmt))
		{
			g_pOSD->Data()->SetItem(L"warnings", L"FAILED Loading TimeShift File");
			g_pTv->ShowOSDItem(L"Warnings", 5);
			return (log << "Failed to Load File Source filters: " << hr << "\n").Write(hr);
		}
//g_pOSD->Data()->SetItem(L"warnings", L"Finished play the TimeShift File");
//g_pTv->ShowOSDItem(L"Warnings", 2);

//		if FAILED(hr = m_pCurrentFileSource->SetRate(1.00))
//		{
//			return (log << "Failed to Set File Source Rate: " << hr << "\n").Write(hr);
//		}
	
	}

	m_bFileSourceActive = TRUE;

	indent.Release();
	(log << "Finished Loading File Source Filters\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::UnloadFileSource()
{
	(log << "Unloading File Source Filters\n").Write();
	LogMessageIndent indent(&log);

	if (!m_pCurrentFileSource)
		return (log << "No Main Sink Class loaded.\n").Write();

	m_bFileSourceActive = TRUE;

	HRESULT hr;

	if(m_pDWGraph)
	{
		// Stop background thread
		if FAILED(hr = StopThread())
			return (log << "Failed to stop background thread: " << hr << "\n").Write(hr);

		if (hr == S_FALSE)
			(log << "Killed thread\n").Write();

		if FAILED(hr = m_pDWGraph->Stop())
			(log << "Failed to stop the File Source DWGraph\n").Write();

		if FAILED(hr = m_pCurrentFileSource->UnloadFilters())
			return (log << "Failed to remove File Source filters: " << hr << "\n").Write(hr);
/*
		IGraphBuilder *piGraphBuilder;
		if FAILED(hr = m_pCurrentDWGraph->QueryGraphBuilder(&piGraphBuilder))
			return (log << "Failed to get the filtergraph's IGraphBuilder Interface : " << hr << "\n").Write(hr);

		CComQIPtr<IMediaFilter> piMediaFilter(piGraphBuilder);
		if (!piMediaFilter)
		{
			piGraphBuilder->Release();
			return (log << "Failed to get IMediaFilter interface from graph: " << hr << "\n").Write(hr);
		}

		piGraphBuilder->Release();

		if FAILED(hr = piMediaFilter->SetSyncSource(NULL))
			return (log << "Failed to set reference clock: " << hr << "\n").Write(hr);
*/
	}

	indent.Release();
	(log << "Finished Unloading File Source Filters\n").Write();

	return S_OK;
}

HRESULT BDADVBTimeShift::AddDemuxPins(DVBTChannels_Service* pService)
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

HRESULT BDADVBTimeShift::AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered)
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

		if FAILED(hr = VetDemuxPin(piPin, Pid))
		{
			(log << "Failed to unmap demux " << pPinName << " pin : " << hr << "\n").Write();
			continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
		}

		if FAILED(hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM))
		{
			(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();
			continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
		}

		if (renderedStreams != 0)
			continue;

		if FAILED(hr = m_pDWGraph->RenderPin(m_piSinkGraphBuilder, piPin))
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

HRESULT BDADVBTimeShift::VetDemuxPin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	ULONG pids = 0;
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid))){

		IEnumPIDMap *pIEnumPIDMap;
		if (SUCCEEDED(muxMapPid->EnumPIDMap(&pIEnumPIDMap))){
			ULONG pNumb = 0;
			PID_MAP pPidMap;
			while(pIEnumPIDMap->Next(1, &pPidMap, &pNumb) == S_OK){
				if (pid != pPidMap.ulPID)
				{
					pids = pPidMap.ulPID;
					hr = muxMapPid->UnmapPID(1, &pids);
				}
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
					if (pid != pStreamIdMap.stream_id)
					{
						pids = pStreamIdMap.stream_id;
						hr = muxStreamMap->UnmapStreamId(1, &pids);
					}
				}
			}
			muxStreamMap->Release();
		}
	}
	return S_OK;
}

HRESULT BDADVBTimeShift::AddDemuxPinsVideo(DVBTChannels_Service* pService, long *streamsRendered)
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

HRESULT BDADVBTimeShift::AddDemuxPinsMp2(DVBTChannels_Service* pService, long *streamsRendered)
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

HRESULT BDADVBTimeShift::AddDemuxPinsAC3(DVBTChannels_Service* pService, long *streamsRendered)
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

	return AddDemuxPins(pService, ac3, L"Audio", &mediaType, streamsRendered);
//	return AddDemuxPins(pService, ac3, L"AC3", &mediaType, streamsRendered);
}

HRESULT BDADVBTimeShift::AddDemuxPinsTeletext(DVBTChannels_Service* pService, long *streamsRendered)
{
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

	mediaType.majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	mediaType.subtype = KSDATAFORMAT_SUBTYPE_NONE;
	mediaType.formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return AddDemuxPins(pService, teletext, L"Teletext", &mediaType, streamsRendered);
}

void BDADVBTimeShift::UpdateData(long frequency, long bandwidth)
{
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

	if(m_pCurrentService)
	{
		LPWSTR streamName = new WCHAR[256];
		wsprintfW(streamName, L"%i. %S", m_pCurrentService->logicalChannelNumber, m_pCurrentService->serviceName);
		if(m_pCurrentFileSource && m_pCurrentService && m_pCurrentService->serviceName &&
			g_pData->values.timeshift.format == 1)
		{
			if (m_pCurrentFileSource->SetStreamName(streamName, TRUE) == S_OK)
			{
				g_pTv->ShowOSDItem(L"Channel", 5);
			}	
		}
		delete[] streamName;
	}

	// Set Time Shift Sink Interface
	if (m_pCurrentSink)
	{
		if (g_pData->values.timeshift.maxnumbfiles != g_pData->settings.timeshift.maxnumbfiles ||
			g_pData->values.timeshift.numbfilesrecycled != g_pData->settings.timeshift.numbfilesrecycled ||
			g_pData->values.timeshift.bufferfilesize != g_pData->settings.timeshift.bufferfilesize ||
			g_pData->values.timeshift.bufferMinutes != g_pData->settings.timeshift.bufferMinutes)
		{
			g_pData->settings.timeshift.maxnumbfiles = g_pData->values.timeshift.maxnumbfiles;
			g_pData->settings.timeshift.numbfilesrecycled = g_pData->values.timeshift.numbfilesrecycled;
			g_pData->settings.timeshift.bufferfilesize = g_pData->values.timeshift.bufferfilesize;
			g_pData->settings.timeshift.bufferMinutes = g_pData->values.timeshift.bufferMinutes;
			m_pCurrentSink->UpdateTSFileSink(FALSE);
		}
	}

	// Set Time Shift Sink Interface to auto after time limit reached.
	m_rtTimeShiftDuration = timeGetTime()/60000;
	m_rtTimeShiftDuration -= m_rtTimeShiftStart;

	if (m_pCurrentSink  && g_pData->values.timeshift.bufferMinutes && (g_pData->values.timeshift.bufferMinutes <= m_rtTimeShiftDuration))
	{
		m_rtTimeShiftStart = timeGetTime()/60000;;
		m_pCurrentSink->UpdateTSFileSink(TRUE);
	}

/*
	// Set Signal Statistics
	BOOL locked;
	long strength, quality;

	REFERENCE_TIME rtStart = timeGetTime();

	std::vector<BDADVBTimeShiftTuner *>::iterator it = m_tuners.begin();
	for ( ; it != m_tuners.end() ; it++ )
	{
		BDADVBTimeShiftTuner *tuner = *it;
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

HRESULT BDADVBTimeShift::UpdateChannels()
{
	if(!m_pCurrentTuner)
		return (log << "There are no Tuner Class Loaded\n").Write(E_POINTER);;

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

		if (pService == m_pCurrentService)
			return S_OK;

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

HRESULT BDADVBTimeShift::ChangeFrequencySelectionOffset(long change)
{
	return frequencyList.ChangeOffset(change);
}

HRESULT BDADVBTimeShift::MoveNetworkUp(long originalNetworkId)
{
	return channels.MoveNetworkUp(originalNetworkId);
}

HRESULT BDADVBTimeShift::MoveNetworkDown(long originalNetworkId)
{
	return channels.MoveNetworkDown(originalNetworkId);
}

HRESULT BDADVBTimeShift::GetFilterList(void)
{
	HRESULT hr = S_FALSE;

	if (!m_pCurrentFilterList)
		return (log << "Failed to get a Filter Property List: " << hr << "\n").Write(hr);

	if FAILED(hr = m_pCurrentFilterList->LoadFilterList(TRUE))
		return (log << "Failed to get a Filter Property List: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT BDADVBTimeShift::ShowFilter(LPWSTR filterName)
{
	HRESULT hr = S_FALSE;

	if (!m_pCurrentFilterList)
		return (log << "Failed to get a Filter Property List: " << hr << "\n").Write(hr);

	if FAILED(hr = m_pCurrentFilterList->ShowFilterProperties(g_pData->hWnd, filterName, 0))
		return (log << "Failed to Show the Filter Property Page: " << hr << "\n").Write(hr);

	return hr;
}

//HRESULT BDADVBTimeShift::ToggleRecording(long mode)
//DWS28-02-2006 HRESULT BDADVBTimeShift::ToggleRecording(long mode, LPWSTR pFilename)
HRESULT BDADVBTimeShift::ToggleRecording(long mode, LPWSTR pFilename, LPWSTR pPath)
{
	if (!m_pCurrentSink)
	{
//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;

//DWS	WCHAR sz[32];
	WCHAR sz[32] = L"";

//	if(m_pCurrentSink->IsRecording())
	if (m_pCurrentSink && m_pCurrentSink->IsRecording() && ((mode == 0) || (mode == 2)))
	{

		if FAILED(hr = m_pCurrentSink->StopRecording())
			return hr;

		wcscpy(sz, L"Recording Stopped");
		g_pOSD->Data()->SetItem(L"recordingicon", L"S");
		g_pTv->ShowOSDItem(L"RecordingIcon", 2);

		// Set the thread state to keep the computer awake
//		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

		// Set priority to HIGH
//		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}
//	else
	else if (!m_pCurrentSink->IsRecording() && ((mode == 1) || (mode == 2)))
	{
//		if FAILED(hr = m_pCurrentSink->StartRecording(m_pCurrentService))
//DWS28-02-2006		if FAILED(hr = m_pCurrentSink->StartRecording(m_pCurrentService, pFilename))
		if FAILED(hr = m_pCurrentSink->StartRecording(m_pCurrentService, pFilename, pPath))
			return hr;

		wcscpy(sz, L"Recording");
		g_pOSD->Data()->SetItem(L"recordingicon", L"R");
		g_pTv->ShowOSDItem(L"RecordingIcon", 100000);
		
		// Stop Tuner Scanner while Recording
		if (m_pCurrentTuner && m_pCurrentTuner->IsActive())
			if FAILED(hr = m_pCurrentTuner->StopScanning())
				(log << "Failed to Stop the previous Tuner from Scanning while Recording.\n").Write();

			// Set the thread state to keep the computer awake
			SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

			// Set priority to HIGH
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}

	//DWS if (sz != L"") this is required to avoid OSD when nothing in sz
	//the if then else above may leave sz empty for example when trying to stop and not recording...
	if (sz != L"")
	{
		g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
		g_pTv->ShowOSDItem(L"Recording", 5);
	}

	return hr;
}

HRESULT BDADVBTimeShift::TogglePauseRecording(long mode)
{
	if (!m_pCurrentSink)
	{
//		g_pOSD->Data()->SetItem(L"RecordingStatus", L"No Capture Format Set");
//		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"warnings", L"Unable to Record: No Capture Format Set");
		g_pTv->ShowOSDItem(L"Warnings", 5);

		return E_FAIL;
	}

	HRESULT hr = S_OK;

	WCHAR sz[32];

	if(m_pCurrentSink->IsPaused())
	{
		if FAILED(hr = m_pCurrentSink->UnPauseRecording(m_pCurrentService))
			return hr;

		wcscpy(sz, L"Recording");
		g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
		g_pTv->ShowOSDItem(L"Recording", 5);
		g_pOSD->Data()->SetItem(L"recordingicon", L"R");
		g_pTv->ShowOSDItem(L"RecordingIcon", 100000);
	}
	else
	{
		if FAILED(hr = m_pCurrentSink->PauseRecording())
			return hr;

		wcscpy(sz, L"Recording Paused");
		g_pOSD->Data()->SetItem(L"RecordingStatus", (LPWSTR) &sz);
		g_pTv->ShowOSDItem(L"Recording", 100000);
		g_pOSD->Data()->SetItem(L"recordingicon", L"P");
		g_pTv->ShowOSDItem(L"RecordingIcon", 100000);
	}


	return hr;
}

BOOL BDADVBTimeShift::IsRecording()
{
	if (g_pData->values.application.multicard && g_pData->values.timeshift.format)
	{
		BOOL isRecording = FALSE;
		std::vector<TUNER_GRAPH*>::iterator it = m_tuners.begin();
		for ( ; it != m_tuners.end() ; it++ )
		{
			if ((*it)->pSink)
				isRecording |= (*it)->pSink->IsRecording();
		}
		return isRecording;
	}
	else if (m_pCurrentSink && g_pData->values.timeshift.format)
		return m_pCurrentSink->IsRecording();
	else
		return FALSE;
}

