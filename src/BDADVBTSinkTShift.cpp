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
	m_piTelexDWDump = NULL;
	m_piVideoDWDump = NULL;
	m_piAudioDWDump = NULL;
	m_piMPGDWDump = NULL;
	m_piTSDWDump = NULL;
	m_piFTSDWDump = NULL;

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
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_TSFileSink, &m_piFTSSink, L"Full TS TimeShift FileWriter"))
		{
			(log << "Failed to add Full TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else		//Connect FileWriter (Full TS TimeShifting)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS TimeShift FileWriter: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
			}
			else	//Add Demux Pins (TS TimeShifting)
				if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piFTSSink, 1))
				{
					(log << "Failed to Set the File Name on the Full TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
					DestroyFTSFilters();
				}
	}
	else if (m_intSinkType& 0x2)
	{
		//MPEG-2 Demultiplexer (TS TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piTSMpeg2Demux, L"TS TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else		//FileWriter (TS TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_TSFileSink, &m_piTSSink, L"TS TimeShift FileWriter"))
			{
				(log << "Failed to add TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else		//Connect Demux (TS TimeShifting)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS TimeShifting)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyTSFilters();
					}
					else		//Connect FileWriter (TS TimeShifting)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piTSMpeg2Demux, m_piTSSink))
						{
							(log << "Failed to connect TS TimeShift MPEG-2 Demultiplexer to TS TimeShift FileWriter: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else	//Add Demux Pins (TS TimeShifting)
							if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTSSink, 11))
							{
								(log << "Failed to Set the File Name on the TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
								DestroyTSFilters();
							}
	}
	else if (m_intSinkType& 0x4)
	{
		//MPEG-2 Demultiplexer (MPG TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piMPGMpeg2Demux, L"MPG TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Multiplexer, &m_piMPGMpeg2Mux, L"MPG TimeShift MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPG TimeShift MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyMPGFilters();
			}
			else //FileWriter (MPG TimeShifting)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_TSFileSink, &m_piMPGSink, L"MPG TimeShift FileWriter"))
				{
					(log << "Failed to add MPG TimeShift  FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG TimeShifting)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG TimeShifting)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
						}
						else	//Connect Mux (MPG TimeShifting)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Audio to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else	//Connect Mux (MPG TimeShifting)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect FileWriter (MPG TimeShifting)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Mux, m_piMPGSink))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Add Demux Pins (MPG TimeShifting)
										if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piMPGSink, 111))
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
	DestroyFilter(m_piFTSSink);
	DeleteFilter(&m_piFTSDWDump);
}

void BDADVBTSinkTShift::DestroyTSFilters()
{
	DeleteFilter(&m_piTSDWDump);
	DestroyFilter(m_piTSSink);
	DestroyFilter(m_piTSMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyMPGFilters()
{
	DeleteFilter(&m_piMPGDWDump);
	DestroyFilter(m_piMPGSink);
	DestroyFilter(m_piMPGMpeg2Mux);
	DestroyFilter(m_piMPGMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyAVFilters()
{
	DeleteFilter(&m_piVideoDWDump);
	DestroyFilter(m_piVideoSink);
	DeleteFilter(&m_piTelexDWDump);
	DestroyFilter(m_piTelexSink);
	DeleteFilter(&m_piAudioDWDump);
	DestroyFilter(m_piAudioSink);
	DestroyFilter(m_piAVMpeg2Demux);
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

	m_piInfinitePinTee = pinInfo.pFilter;

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

	CComPtr <IFileSinkFilter> piFileSinkFilter;

	if ((m_intSinkType & 0x1))
	{
		if (m_piFTSSink)
			m_piFTSSink->QueryInterface(&piFileSinkFilter);
	}
	else if ((m_intSinkType & 0x2) && m_piTSSink)
	{
		if (m_piTSSink)
			m_piTSSink->QueryInterface(&piFileSinkFilter);
	}
	else if ((m_intSinkType & 0x4) && m_piMPGSink)
	{
		if (m_piMPGSink)
			m_piMPGSink->QueryInterface(&piFileSinkFilter);
	}

	if (piFileSinkFilter)
		return piFileSinkFilter->GetCurFile(ppszFileName, NULL);

	return E_FAIL;
}

