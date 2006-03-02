/**
 *	BDADVBTSinkTShift.cpp
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


#include "BDADVBTSinkTShift.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "TSFileSinkGuids.h"


//////////////////////////////////////////////////////////////////////
// BDADVBTSinkTShift
//////////////////////////////////////////////////////////////////////

BDADVBTSinkTShift::BDADVBTSinkTShift(BDADVBTSink *pBDADVBTSink) :
	m_pBDADVBTSink(pBDADVBTSink)
{
	m_pDWGraph = NULL;
	m_pTelexDWDump = NULL;
	m_pVideoDWDump = NULL;
	m_pAudioDWDump = NULL;
	m_pMPGDWDump = NULL;
	m_pTSDWDump = NULL;
	m_pFTSDWDump = NULL;

	m_pTelexFileName = NULL;
	m_pVideoFileName = NULL;
	m_pAudioFileName = NULL;
	m_pMPGFileName = NULL;
	m_pTSFileName = NULL;
	m_pFTSFileName = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;

	m_rotEntry = 0;

	m_intSinkType = 0;
}

BDADVBTSinkTShift::~BDADVBTSinkTShift()
{
	DestroyAll();
}

void BDADVBTSinkTShift::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTSinkTShift::Initialise(DWGraph *pDWGraph, int intSinkType)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T TimeShift Sink tried to initialise a second time\n").Write(E_FAIL);

	if (!pDWGraph)
		return (log << "Must pass a valid DWGraph object to Initialise a Sink\n").Write(E_FAIL);

	m_pDWGraph = pDWGraph;

	//--- COM should already be initialized ---

	if FAILED(hr = m_pDWGraph->QueryGraphBuilder(&m_piGraphBuilder))
		return (log << "Failed to get graph: " << hr << "\n").Write(hr);

	if FAILED(hr = m_pDWGraph->QueryMediaControl(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	m_intSinkType = intSinkType;
	m_bInitialised = TRUE;
	return S_OK;
}

HRESULT BDADVBTSinkTShift::DestroyAll()
{
    HRESULT hr = S_OK;

	RemoveSinkFilters();

	if(m_pTelexFileName)
		delete[] m_pTelexFileName;

	if(m_pVideoFileName)
		delete[] m_pVideoFileName;

	if(m_pAudioFileName)
		delete[] m_pAudioFileName;

	if(m_pMPGFileName)
		delete[] m_pMPGFileName;

	if(m_pTSFileName)
		delete[] m_pTSFileName;

	if(m_pFTSFileName)
		delete[] m_pFTSFileName;

	m_piMediaControl.Release();
	m_piGraphBuilder.Release();

	return S_OK;
}

HRESULT BDADVBTSinkTShift::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	//--- Add & connect the TimeShifting filters ---

	if (m_intSinkType& 0x1)
	{
		//FileWriter (Full TS TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pFTSSink, L"Full TS TimeShift FileWriter"))
		{
			(log << "Failed to add Full TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else		//Connect FileWriter (Full TS TimeShifting)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS TimeShift FileWriter: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
			}
			else	//Add Demux Pins (TS TimeShifting)
				if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pFTSFileName, pService, m_pFTSSink, 1))
				{
					(log << "Failed to Set the File Name on the Full TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
					DestroyFTSFilters();
				}
	}
	else if (m_intSinkType& 0x2)
	{
		//MPEG-2 Demultiplexer (TS TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pTSMpeg2Demux, L"TS TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else		//FileWriter (TS TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pTSSink, L"TS TimeShift FileWriter"))
			{
				(log << "Failed to add TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else		//Connect Demux (TS TimeShifting)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS TimeShifting)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyTSFilters();
					}
					else		//Connect FileWriter (TS TimeShifting)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pTSMpeg2Demux, m_pTSSink))
						{
							(log << "Failed to connect TS TimeShift MPEG-2 Demultiplexer to TS TimeShift FileWriter: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else	//Add Demux Pins (TS TimeShifting)
							if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTSFileName, pService, m_pTSSink, 11))
							{
								(log << "Failed to Set the File Name on the TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
								DestroyTSFilters();
							}
	}
	else if (m_intSinkType& 0x4)
	{
		//MPEG-2 Demultiplexer (MPG TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pMPGMpeg2Demux, L"MPG TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.mpgmuxclsid, &m_pMPGMpeg2Mux, L"MPG TimeShift MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPG TimeShift MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyMPGFilters();
			}
			else //FileWriter (MPG TimeShifting)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pMPGSink, L"MPG TimeShift FileWriter"))
				{
					(log << "Failed to add MPG TimeShift  FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG TimeShifting)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG TimeShifting)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
						}
						else	//Connect Mux (MPG TimeShifting)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Audio to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else	//Connect Mux (MPG TimeShifting)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect FileWriter (MPG TimeShifting)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Add Demux Pins (MPG TimeShifting)
										if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 111))
										{
											(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
	}


	m_bActive = TRUE;
	return hr;
}

void BDADVBTSinkTShift::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

void BDADVBTSinkTShift::DestroyFTSFilters()
{
	DestroyFilter(m_pFTSSink);
	DeleteFilter(&m_pFTSDWDump);
}

void BDADVBTSinkTShift::DestroyTSFilters()
{
	DeleteFilter(&m_pTSDWDump);
	DestroyFilter(m_pTSSink);
	DestroyFilter(m_pTSMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyMPGFilters()
{
	DeleteFilter(&m_pMPGDWDump);
	DestroyFilter(m_pMPGSink);
	DestroyFilter(m_pMPGMpeg2Mux);
	DestroyFilter(m_pMPGMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyAVFilters()
{
	DeleteFilter(&m_pVideoDWDump);
	DestroyFilter(m_pVideoSink);
	DeleteFilter(&m_pTelexDWDump);
	DestroyFilter(m_pTelexSink);
	DeleteFilter(&m_pAudioDWDump);
	DestroyFilter(m_pAudioSink);
	DestroyFilter(m_pAVMpeg2Demux);
}

void BDADVBTSinkTShift::DeleteFilter(DWDump **pfDWDump)
{
	if (pfDWDump)
		return;

	if (*pfDWDump)
		delete *pfDWDump;

	*pfDWDump = NULL;

}

HRESULT BDADVBTSinkTShift::RemoveSinkFilters()
{
	m_bActive = FALSE;

	DestroyFTSFilters();
	DestroyTSFilters();
	DestroyMPGFilters();
	DestroyAVFilters();

	return S_OK;
}

HRESULT BDADVBTSinkTShift::SetTransportStreamPin(IPin* piPin)
{
	if (!piPin)
		return E_FAIL;

	HRESULT hr;
	PIN_INFO pinInfo;
	if FAILED(hr = piPin->QueryPinInfo(&pinInfo))
		return E_FAIL;

	m_pInfinitePinTee = pinInfo.pFilter;

	return S_OK;
}

BOOL BDADVBTSinkTShift::IsActive()
{
	return m_bActive;
}

HRESULT BDADVBTSinkTShift::GetCurFile(LPOLESTR *ppszFileName)
{
	if (!ppszFileName)
		return E_INVALIDARG;

	if(*ppszFileName)
		delete[] *ppszFileName;

	if ((m_intSinkType & 0x1))
	{
		if (m_pFTSFileName)
			*ppszFileName = m_pFTSFileName;
	}
	else if ((m_intSinkType & 0x2) && m_pTSSink)
	{
		if (m_pTSFileName)
			*ppszFileName = m_pTSFileName;
	}
	else if ((m_intSinkType & 0x4) && m_pMPGSink)
	{
		if (m_pMPGFileName)
			*ppszFileName = m_pMPGFileName;
	}
	else if ((m_intSinkType & 0x8) && m_pVideoSink)
	{
		if (m_pVideoFileName)
			*ppszFileName = m_pVideoFileName;
	}
	else if ((m_intSinkType & 0x8) && m_pAudioSink)
	{
		if (m_pAudioFileName)
			*ppszFileName = m_pAudioFileName;
	}
	else if ((m_intSinkType & 0x8) && m_pTelexSink)
	{
		if (m_pTelexFileName)
			*ppszFileName = m_pTelexFileName;
	}

	if (*ppszFileName)
		return S_OK;
	else
		return E_FAIL;
}

HRESULT BDADVBTSinkTShift::GetCurFileSize(__int64 *pllFileSize)
{
	USES_CONVERSION;

	if (!pllFileSize)
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	if ((m_intSinkType & 0x1) && m_pFTSSink)
		return m_pBDADVBTSink->GetSinkSize(m_pFTSFileName, pllFileSize);

	else if ((m_intSinkType & 0x2) && m_pTSSink)
		return m_pBDADVBTSink->GetSinkSize(m_pTSFileName, pllFileSize);

	else if ((m_intSinkType & 0x4) && m_pMPGSink)
		return m_pBDADVBTSink->GetSinkSize(m_pMPGFileName, pllFileSize);

	else if ((m_intSinkType & 0x8) && m_pVideoSink)
		return m_pBDADVBTSink->GetSinkSize(m_pVideoFileName, pllFileSize);

	else if ((m_intSinkType & 0x8) && m_pAudioSink)
		return m_pBDADVBTSink->GetSinkSize(m_pAudioFileName, pllFileSize);

	else if ((m_intSinkType & 0x8) && m_pTelexSink)
		return m_pBDADVBTSink->GetSinkSize(m_pTelexFileName, pllFileSize);

	return hr;
}

