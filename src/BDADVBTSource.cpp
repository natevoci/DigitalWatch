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
#include "FilterGraphTools.h"
#include "ParseLine.h"

#include "StreamFormats.h"
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include "bdamedia.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BDADVBTSource::BDADVBTSource()
{
	m_pDWGraph = NULL;
}

BDADVBTSource::~BDADVBTSource()
{
	Destroy();
}

void BDADVBTSource::GetSourceType(LPWSTR &type)
{
	strCopy(type, L"TV");
}

HRESULT BDADVBTSource::Initialise(DWGraph* pFilterGraph)
{
	m_pDWGraph = pFilterGraph;
	
	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Channels.xml", g_pData->application.appPath);
	if (channels.LoadChannels((LPWSTR)&file) == FALSE)
		return E_FAIL;

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

	return S_OK;
}

HRESULT BDADVBTSource::ExecuteCommand(LPWSTR command)
{
	int n1, n2, n3, n4;
	LPWSTR pBuff = (LPWSTR)command;
	LPWSTR pCurr;

	ParseLine parseLine;
	if (parseLine.Parse(pBuff) == FALSE)
		return (log << "Parse error in function: " << command << "\n").Show(E_FAIL);

	(log << "BDADVBTSource::ExecuteCommand - " << command << "\n").Write();

	if (parseLine.HasRHS())
		return (log << "Cannot have RHS for function.\n").Show(E_FAIL);

	pCurr = parseLine.LHS.FunctionName;

	if (_wcsicmp(pCurr, L"SetChannel") == 0)
	{
		if (parseLine.LHS.ParameterCount <= 0)
			return (log << "Expecting 1 or 2 parameters: " << parseLine.LHS.Function << "\n").Show(E_FAIL);

		if (parseLine.LHS.ParameterCount > 2)
			return (log << "Too many parameters: " << parseLine.LHS.Function << "\n").Show(E_FAIL);

		n1 = _wtoi(parseLine.LHS.Parameter[0]);

		n2 = -1;
		if (parseLine.LHS.ParameterCount >= 2)
			n2 = _wtoi(parseLine.LHS.Parameter[1]);

		return SetChannel(n1, n2);
	}

	//Just referencing these variables to stop warnings.
	n3 = 0;
	n4 = 0;
	return S_FALSE;
}

HRESULT BDADVBTSource::Play()
{
	HRESULT hr;

	if (!m_pDWGraph)
		return (log << "Filter graph not set in BDADVBTSource::Play\n").Write(E_FAIL);

	if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
		return (log << "Failed to get graph\n").Write(hr);

	if (channels.IsValidNetwork(1) && channels.Network(1)->IsValidProgram(1))
		return SetChannel(1, 1);

	return E_FAIL;
}

HRESULT BDADVBTSource::SetChannel(int nNetwork, int nProgram)
{
	HRESULT hr;
	//Check if recording

	if (!channels.IsValidNetwork(nNetwork))
		return (log << "BDADVBTSource::SetChannel - Network number is not valid\n").Write(E_INVALIDARG);

	if (!channels.Network(nNetwork)->IsValidProgram(nProgram))
		return (log << "BDADVBTSource::SetChannel - Program number is not valid\n").Write(E_INVALIDARG);

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

		if FAILED(hr = m_pCurrentTuner->LockChannel(channels.Network(nNetwork)->frequency, channels.Network(nNetwork)->bandwidth))
		{
			(log << "Failed to Lock Channel\n").Write();
			continue;
		}

		if FAILED(hr = AddDemuxPins(channels.Network(nNetwork)->Program(nProgram)))
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
		
		return S_OK;
	}

	return (log << "Failed to start the graph\n").Write(hr);
}

HRESULT BDADVBTSource::LoadTuner()
{
	HRESULT hr;

	if FAILED(hr = m_pCurrentTuner->AddSourceFilters())
		return (log << "Failed to add source filters\n").Write(hr);

	CComPtr <IPin> piTSPin;
	if FAILED(hr = m_pCurrentTuner->QueryTransportStreamPin(&piTSPin))
		return (log << "Could not get TSPin\n").Write(hr);

	//MPEG-2 Demultiplexer (DW's)
	if FAILED(hr = AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piBDAMpeg2Demux, L"DW MPEG-2 Demultiplexer"))
		return (log << "Failed to add DW MPEG-2 Demultiplexer to the graph\n").Write(hr);

	CComPtr <IPin> piDemuxPin;
	if FAILED(hr = FindFirstFreePin(m_piBDAMpeg2Demux, &piDemuxPin, PINDIR_INPUT))
		return (log << "Failed to get input pin on DW Demux\n").Write(hr);

	if FAILED(hr = m_piGraphBuilder->ConnectDirect(piTSPin, piDemuxPin, NULL))
		return (log << "Failed to connect TS Pin to DW Demux\n").Write(hr);

	piDemuxPin.Release();
	piTSPin.Release();

	return S_OK;
}

HRESULT BDADVBTSource::UnloadTuner()
{
	HRESULT hr;

	if (m_piBDAMpeg2Demux)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDAMpeg2Demux);
		m_piBDAMpeg2Demux.Release();
	}

	if (m_pCurrentTuner)
	{
		if FAILED(hr = m_pCurrentTuner->RemoveSourceFilters())
			return (log << "Failed to remove source filters\n").Write(hr);
		m_pCurrentTuner = NULL;
	}

	return S_OK;
}

HRESULT BDADVBTSource::AddDemuxPins(DVBTChannels_Program* program)
{
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
	return S_OK;
}

HRESULT BDADVBTSource::Destroy()
{
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

	return S_OK;
}

