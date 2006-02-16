/**
 *	BDADVBTSinkTShift.cpp
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


#include "BDADVBTSinkTShift.h"
#include "BDADVBTSink.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "TSFileSinkGuids.h"


//////////////////////////////////////////////////////////////////////
// BDADVBTSinkTShift
//////////////////////////////////////////////////////////////////////

BDADVBTSinkTShift::BDADVBTSinkTShift(BDADVBTSink *pBDADVBTSink, BDADVBTSourceTuner *pCurrentTuner) :
	m_pBDADVBTSink(pBDADVBTSink)
{
	m_pCurrentTuner = pCurrentTuner;

	m_pDWGraph = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;


	m_pMpeg2DataParser = NULL;
	m_pMpeg2DataParser = new DVBMpeg2DataParser();

	m_rotEntry = 0;

	m_intSinkType = 0;
}

BDADVBTSinkTShift::~BDADVBTSinkTShift()
{
	DestroyAll();
	if (m_pMpeg2DataParser)
	{
		m_pMpeg2DataParser->ReleaseFilter();
		delete m_pMpeg2DataParser;
		m_pMpeg2DataParser = NULL;
	}
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
	/*
    CComPtr <IBaseFilter> pFilter;
    CComPtr <IEnumFilters> pFilterEnum;

    hr = m_piGraphBuilder->EnumFilters(&pFilterEnum);
	switch (hr)
	{
	case S_OK:
		break;
	case E_OUTOFMEMORY:
		return (log << "Insufficient memory to create the enumerator.\n").Write(hr);
	case E_POINTER:
		return (log << "Null pointer argument.\n").Write(hr);
	default:
		return (log << "Unknown Error enumerating graph: " << hr << "\n").Write(hr);
	}

    if FAILED(hr = pFilterEnum->Reset())
		return (log << "Failed to reset graph enumerator: " << hr << "\n").Write(hr);

	while (pFilterEnum->Next(1, &pFilter, 0) == S_OK) // addrefs filter
	{
		if FAILED(hr = m_piGraphBuilder->RemoveFilter(pFilter))
		{
			FILTER_INFO info;
			pFilter->QueryFilterInfo(&info);
			if (info.pGraph)
				info.pGraph->Release();
            return (log << "Failed to remove filter: " << info.achName << " : " << hr << "\n").Write(hr);
		}
		pFilter.Release();
	}
	pFilterEnum.Release();
	*/

	RemoveSinkFilters();

	m_piMediaControl.Release();
	m_piGraphBuilder.Release();

	return S_OK;
}

HRESULT BDADVBTSinkTShift::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr;

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
				DestroyFilter(m_piFTSSink);
			}
			else	//Add Demux Pins (TS TimeShifting)
				if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piFTSSink, 1))
				{
					(log << "Failed to Set the File Name on the Full TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
					DestroyFilter(m_piFTSSink);
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
				DestroyFilter(m_piTSMpeg2Demux);
			}
			else		//Connect Demux (TS TimeShifting)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyFilter(m_piTSSink);
					DestroyFilter(m_piTSMpeg2Demux);
				}
				else	//Add Demux Pins (TS TimeShifting)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyFilter(m_piTSSink);
							DestroyFilter(m_piTSMpeg2Demux);
					}
					else		//Connect FileWriter (TS TimeShifting)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piTSMpeg2Demux, m_piTSSink))
						{
							(log << "Failed to connect TS TimeShift MPEG-2 Demultiplexer to TS TimeShift FileWriter: " << hr << "\n").Write(hr);
							DestroyFilter(m_piTSSink);
							DestroyFilter(m_piTSMpeg2Demux);
						}
						else	//Add Demux Pins (TS TimeShifting)
							if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTSSink, 11))
							{
								(log << "Failed to Set the File Name on the TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
								DestroyFilter(m_piTSSink);
								DestroyFilter(m_piTSMpeg2Demux);
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
				DestroyFilter(m_piMPGMpeg2Demux);
			}
			else //FileWriter (MPG TimeShifting)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_TSFileSink, &m_piMPGSink, L"MPG TimeShift FileWriter"))
				{
					(log << "Failed to add MPG TimeShift  FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyFilter(m_piMPGMpeg2Mux);
					DestroyFilter(m_piMPGMpeg2Demux);
				}
				else	//Connect Demux (MPG TimeShifting)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyFilter(m_piMPGSink);
						DestroyFilter(m_piMPGMpeg2Mux);
						DestroyFilter(m_piMPGMpeg2Demux);
					}
					else	//Add Demux Pins (MPG TimeShifting)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyFilter(m_piMPGSink);
								DestroyFilter(m_piMPGMpeg2Mux);
								DestroyFilter(m_piMPGMpeg2Demux);
						}
						else	//Connect Mux (MPG TimeShifting)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Audio to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyFilter(m_piMPGSink);
								DestroyFilter(m_piMPGMpeg2Mux);
								DestroyFilter(m_piMPGMpeg2Demux);
							}
							else	//Connect Mux (MPG TimeShifting)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyFilter(m_piMPGSink);
									DestroyFilter(m_piMPGMpeg2Mux);
									DestroyFilter(m_piMPGMpeg2Demux);
								}
								else	//Connect FileWriter (MPG TimeShifting)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Mux, m_piMPGSink))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
										DestroyFilter(m_piMPGSink);
										DestroyFilter(m_piMPGMpeg2Mux);
										DestroyFilter(m_piMPGMpeg2Demux);
									}
									else	//Add Demux Pins (MPG TimeShifting)
										if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piMPGSink, 111))
										{
											(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
											DestroyFilter(m_piMPGSink);
											DestroyFilter(m_piMPGMpeg2Mux);
											DestroyFilter(m_piMPGMpeg2Demux);
										}
	}


	m_bActive = TRUE;
	return S_OK;
}

void BDADVBTSinkTShift::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}



HRESULT BDADVBTSinkTShift::RemoveSinkFilters()
{
	m_bActive = FALSE;

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();

	DestroyFilter(m_piMPGSink);
	DestroyFilter(m_piMPGMpeg2Mux);
	DestroyFilter(m_piMPGMpeg2Demux);
	DestroyFilter(m_piTSSink);
	DestroyFilter(m_piTSMpeg2Demux);
	DestroyFilter(m_piFTSSink);

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();

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


