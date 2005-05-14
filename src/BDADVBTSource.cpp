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
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BDADVBTSource::BDADVBTSource() : m_strSourceType(L"BDA")
{
	m_pCurrentTuner = NULL;
	m_pDWGraph = NULL;

	g_pOSD->Data()->AddList(L"TVChannels.Networks", &channels);
}

BDADVBTSource::~BDADVBTSource()
{
	Destroy();
}

void BDADVBTSource::SetLogCallback(LogMessageCallback *callback)
{
	DWSource::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
	channels.SetLogCallback(callback);
	cardList.SetLogCallback(callback);

	std::vector<BDADVBTSourceTuner *>::iterator it = m_Tuners.begin();
	for ( ; it != m_Tuners.end() ; it++ )
	{
		BDADVBTSourceTuner *tuner = *it;
		tuner->SetLogCallback(callback);
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
	if FAILED(channels.LoadChannels((LPWSTR)&file))
		return E_FAIL;

	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Keys.xml", g_pData->application.appPath);
	if FAILED(hr = m_sourceKeyMap.LoadFromFile((LPWSTR)&file))
		return hr;

	//Get list of BDA capture cards
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Cards.xml", g_pData->application.appPath);
	cardList.LoadCards((LPWSTR)&file);
	cardList.SaveCards();
	if (cardList.cards.size() == 0)
		return (log << "Could not find any BDA cards\n").Show(E_FAIL);
	
	std::vector<BDACard *>::iterator it = cardList.cards.begin();
	for ( ; it != cardList.cards.end() ; it++ )
	{
		BDACard *tmpCard = *it;
		if (tmpCard->bActive)
		{
			m_pCurrentTuner = new BDADVBTSourceTuner(tmpCard);
			m_pCurrentTuner->SetLogCallback(m_pLogCallback);
			if SUCCEEDED(m_pCurrentTuner->Initialise(pFilterGraph))
			{
				m_Tuners.push_back(m_pCurrentTuner);
				continue;
			}
			delete m_pCurrentTuner;
			m_pCurrentTuner = NULL;
		}
	}
	if (m_Tuners.size() == 0)
		return (log << "There are no active BDA cards\n").Show(E_FAIL);

	m_pCurrentTuner = NULL;

	indent.Release();
	(log << "Finished Initialising BDA Source\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::Destroy()
{
	(log << "Destroying BDA Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (m_pDWGraph)
	{
		if FAILED(hr = m_pDWGraph->Stop())
			(log << "Failed to stop DWGraph\n").Write();

		if FAILED(hr = UnloadTuner())
			(log << "Failed to unload tuner\n").Write();

		if FAILED(hr = m_pDWGraph->Cleanup())
			(log << "Failed to cleanup DWGraph\n").Write();

		std::vector<BDADVBTSourceTuner *>::iterator it = m_Tuners.begin();
		for ( ; it != m_Tuners.end() ; it++ )
		{
			BDADVBTSourceTuner *tuner = *it;
			delete tuner;
		}
		m_Tuners.clear();
	}

	m_pDWGraph = NULL;

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

		n1 = _wtoi(command->LHS.Parameter[0]);

		n2 = 0;
		if (command->LHS.ParameterCount >= 2)
			n2 = _wtoi(command->LHS.Parameter[1]);

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


	//Just referencing these variables to stop warnings.
	n3 = 0;
	n4 = 0;
	return S_FALSE;
}

HRESULT BDADVBTSource::Play()
{
	(log << "Playing BDA Source\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	if (!m_pDWGraph)
		return (log << "Filter graph not set in BDADVBTSource::Play\n").Write(E_FAIL);

	if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
		return (log << "Failed to get graph: " << hr <<"\n").Write(hr);

	//TODO: replace this with last selected channel, or menu depending on options.
	if SUCCEEDED(hr = channels.SetCurrentNetworkId(1))
	{
		DVBTChannels_Network* network = channels.GetCurrentNetwork();
		if SUCCEEDED(hr = network->SetCurrentProgramId(1))
		{
			return SetChannel(1, 1);
		}
	}

	indent.Release();
	(log << "Finished Playing BDA Source\n").Write();

	return E_FAIL;
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

	CComPtr <IPin> piDemuxPin;
	if FAILED(hr = graphTools.FindFirstFreePin(m_piBDAMpeg2Demux, &piDemuxPin, PINDIR_INPUT))
		return (log << "Failed to get input pin on DW Demux: " << hr << "\n").Write(hr);

	if FAILED(hr = m_piGraphBuilder->ConnectDirect(piTSPin, piDemuxPin, NULL))
		return (log << "Failed to connect TS Pin to DW Demux: " << hr << "\n").Write(hr);

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

HRESULT BDADVBTSource::AddDemuxPins(DVBTChannels_Program* program)
{
	(log << "Adding Demux Pins\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	CComQIPtr <IMpeg2Demultiplexer> piMpeg2Demux(m_piBDAMpeg2Demux);
	if (!piMpeg2Demux)
	{
		return (log << "Failed to QI demux filter for IMpeg2Demultiplexer\n").Write(E_FAIL);
	}

	CComPtr <IMPEG2PIDMap> piPidMap;
	CComPtr <IPin> piPin;
	ULONG Pid;

	long unkCount = (program->GetStreamCount(unknown)  > 1) ? 1 : 0;
	long vidCount = (program->GetStreamCount(video)    > 1) ? 1 : 0;
	long mp2Count = (program->GetStreamCount(mp2)      > 1) ? 1 : 0;
	long ac3Count = (program->GetStreamCount(ac3)      > 1) ? 1 : 0;
	long txtCount = (program->GetStreamCount(teletext) > 1) ? 1 : 0;

	BOOL bAudioRendered = FALSE;

	long count = program->GetStreamCount();
	for (int i=0 ; i<count ; i++ )
	{
		DVBTChannels_Program_PID_Types type = program->GetStreamType(i);
		Pid = program->GetStreamPID(i);

		switch (type)
		{
		case unknown:
			{
				if (unkCount > 0)
					unkCount++;
			}
			break;
		case video:
			{
				wchar_t text[10];
				if (vidCount > 0)
					swprintf((wchar_t*)&text, L"Video %i", vidCount);
				else
				{
					swprintf((wchar_t*)&text, L"Video");
					vidCount = 1;
				}

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

				hr = piMpeg2Demux->CreateOutputPin(&mediaType, (wchar_t*)&text, &piPin);
				if (hr != S_OK) return FALSE;

				// Map the PID.
				if SUCCEEDED(hr = piPin.QueryInterface(&piPidMap))
				{
					hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM);
					piPidMap.Release();
				}

				if (vidCount == 1)
				{
					if FAILED(hr = m_pDWGraph->RenderPin(piPin))
						(log << "Failed to render video stream\n").Write();
				}

				piPin.Release();

				vidCount++;
			}
			break;
		case mp2:
			{
				wchar_t text[10];
				if (mp2Count > 0)
					swprintf((wchar_t*)&text, L"Audio %i", mp2Count);
				else
				{
					swprintf((wchar_t*)&text, L"Audio");
					mp2Count = 1;
				}

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

				hr = piMpeg2Demux->CreateOutputPin(&mediaType, (wchar_t*)&text, &piPin);
				if (hr != S_OK) return FALSE;

				// Map the PID.
				if SUCCEEDED(hr = piPin.QueryInterface(&piPidMap))
				{
					hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM);
					piPidMap.Release();
				}

				if (!bAudioRendered)
				{
					if SUCCEEDED(hr = m_pDWGraph->RenderPin(piPin))
					{
						bAudioRendered = TRUE;
					}
					else
					{
						(log << "Failed to render audio stream\n").Write();
					}
				}

				piPin.Release();

				mp2Count++;
			}
			break;
		case ac3:
			{
				wchar_t text[10];
				if (ac3Count > 0)
					swprintf((wchar_t*)&text, L"AC3 %i", ac3Count);
				else
				{
					swprintf((wchar_t*)&text, L"AC3");
					ac3Count = 1;
				}

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

				hr = piMpeg2Demux->CreateOutputPin(&mediaType, (wchar_t*)&text, &piPin);
				if (hr != S_OK) return FALSE;

				// Map the PID.
				if SUCCEEDED(hr = piPin.QueryInterface(&piPidMap))
				{
					hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM);
					piPidMap.Release();
				}

				if (!bAudioRendered)
				{
					if SUCCEEDED(hr = m_pDWGraph->RenderPin(piPin))
					{
						bAudioRendered = TRUE;
					}
					else
					{
						(log << "Failed to render AC3 stream\n").Write();
					}
				}

				piPin.Release();

				ac3Count++;
			}
			break;
		case teletext:
			{
				wchar_t text[10];
				if (txtCount > 0)
					swprintf((wchar_t*)&text, L"Teletext %i", txtCount);
				else
				{
					swprintf((wchar_t*)&text, L"Teletext");
					txtCount = 1;
				}

				AM_MEDIA_TYPE mediaType;
				ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));

				mediaType.majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
				mediaType.subtype = KSDATAFORMAT_SUBTYPE_NONE;
				mediaType.formattype = KSDATAFORMAT_SPECIFIER_NONE;

				hr = piMpeg2Demux->CreateOutputPin(&mediaType, (wchar_t*)&text, &piPin);
				if (hr != S_OK) return FALSE;

				// Map the PID.
				if SUCCEEDED(hr = piPin.QueryInterface(&piPidMap))
				{
					hr = piPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM);
					piPidMap.Release();
				}

				if (txtCount == 1)
				{
					if FAILED(hr = m_pDWGraph->RenderPin(piPin))
						(log << "Failed to render teletext stream\n").Write();
				}

				piPin.Release();

				txtCount++;
			}
			break;
		default:
			break;
		}
	}

	indent.Release();
	(log << "Finished Adding Demux Pins\n").Write();

	return S_OK;
}

HRESULT BDADVBTSource::SetChannel(int nNetwork, int nProgram)
{
	(log << "Setting Channel (" << nNetwork << ", " << nProgram << ")\n").Write();
	LogMessageIndent indent(&log);

	HRESULT hr;

	//TODO: Check if recording

	if FAILED(hr = channels.SetCurrentNetworkId(nNetwork))
		return (log << "Network number is not valid\n").Write(hr);

	DVBTChannels_Network* network = channels.GetCurrentNetwork();

	if (nProgram == 0)
		nProgram = network->GetCurrentProgramId();
	//TODO: nProgram < 0 then move to next program

	if FAILED(hr = network->SetCurrentProgramId(nProgram))
		return (log << "Program number is not valid\n").Write(hr);

	DVBTChannels_Program* program = network->GetCurrentProgram();

	g_pOSD->Data()->SetItem(L"CurrentNetwork", network->name);
	g_pOSD->Data()->SetItem(L"CurrentProgram", program->name);
	g_pTv->ShowOSDItem(L"Channel", 10);

	//Check if already on this network
	/*if (nNetwork == m_nCurrentNetwork)
	{
		if ((nProgram == channels.Network(nNetwork)
	}*/

	if FAILED(hr = m_pDWGraph->Stop())
		(log << "Failed to stop DWGraph\n").Write();

	std::vector<BDADVBTSourceTuner *>::iterator it = m_Tuners.begin();
	for ( ; TRUE /*check for end of list done after graph is cleaned up*/ ; it++ )
	{
		hr = UnloadTuner();

		if FAILED(hr = m_pDWGraph->Cleanup())
			(log << "Failed to cleanup DWGraph\n").Write();

		if (it == m_Tuners.end())
			break;

		m_pCurrentTuner = *it;

		if FAILED(hr = LoadTuner())
		{
			(log << "Failed to load Source Tuner\n").Write();
			continue;
		}

		if FAILED(hr = m_pCurrentTuner->LockChannel(network->frequency, network->bandwidth))
		{
			(log << "Failed to Lock Channel\n").Write();
			continue;
		}

		if FAILED(hr = AddDemuxPins(program))
		{
			(log << "Failed to Add Demux Pins\n").Write();
			continue;
		}

		if FAILED(hr = m_pDWGraph->Start())
		{
			(log << "Failed to Start Graph. Possibly tuner already in use.\n").Write();
			continue;
		}

		//Move current tuner to back of list so that other cards will be used next
		m_Tuners.erase(it);
		m_Tuners.push_back(m_pCurrentTuner);
		
		g_pOSD->Data()->SetItem(L"CurrentDVBTCard", m_pCurrentTuner->GetCardName());

		indent.Release();
		(log << "Finished Setting Channel\n").Write();

		return S_OK;
	}

	return (log << "Failed to start the graph: " << hr << "\n").Write(hr);
}

HRESULT BDADVBTSource::NetworkUp()
{
	long nNetwork = channels.GetNextNetworkId();

	if (nNetwork > 0)
		return SetChannel(nNetwork, 0);
	return E_FAIL;
}

HRESULT BDADVBTSource::NetworkDown()
{
	long nNetwork = channels.GetPrevNetworkId();

	if (nNetwork > 0)
		return SetChannel(nNetwork, 0);
	return E_FAIL;
}

HRESULT BDADVBTSource::ProgramUp()
{
	DVBTChannels_Network* network = channels.GetCurrentNetwork();
	if (network == NULL)
	{
		if FAILED(channels.SetCurrentNetworkId(1))
			return E_FAIL;
		network = channels.GetCurrentNetwork();
	}

	long nNetwork = channels.GetCurrentNetworkId();
	long nProgram = network->GetNextProgramId();

	if (nProgram > 0)
		return SetChannel(nNetwork, nProgram);
	return E_FAIL;
}

HRESULT BDADVBTSource::ProgramDown()
{
	DVBTChannels_Network* network = channels.GetCurrentNetwork();
	if (network == NULL)
	{
		if FAILED(channels.SetCurrentNetworkId(1))
			return E_FAIL;
		network = channels.GetCurrentNetwork();
	}

	long nNetwork = channels.GetCurrentNetworkId();
	long nProgram = network->GetPrevProgramId();

	if (nProgram > 0)
		return SetChannel(nNetwork, nProgram);
	return E_FAIL;
}


