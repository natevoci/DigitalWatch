/**
 *	BDADVBTSinkFile.cpp
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


#include "BDADVBTSinkFile.h"
#include "BDADVBTSink.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>

//////////////////////////////////////////////////////////////////////
// BDADVBTSinkFile
//////////////////////////////////////////////////////////////////////

BDADVBTSinkFile::BDADVBTSinkFile(BDADVBTSink *pBDADVBTSink, BDADVBTSourceTuner *pCurrentTuner) :
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

BDADVBTSinkFile::~BDADVBTSinkFile()
{
	DestroyAll();
	if (m_pMpeg2DataParser)
	{
		m_pMpeg2DataParser->ReleaseFilter();
		delete m_pMpeg2DataParser;
		m_pMpeg2DataParser = NULL;
	}
}

void BDADVBTSinkFile::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTSinkFile::Initialise(DWGraph *pDWGraph, int intSinkType)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T Source Tuner tried to initialise a second time\n").Write(E_FAIL);

	if (!pDWGraph)
		return (log << "Must pass a valid DWGraph object to Initialise a tuner\n").Write(E_FAIL);

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

HRESULT BDADVBTSinkFile::DestroyAll()
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

HRESULT BDADVBTSinkFile::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr;


	//--- Add & connect the Sink filters ---


	
	if (m_intSinkType& 0x1)
	{
		//FileWriter (Full TS Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piFTSSink, L"Full TS Sink FileWriter"))
		{
			(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else	//Connect FileWriter (Full TS Sink's)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS Sink FileWriter: " << hr << "\n").Write(hr);
				DestroyFilter(m_piFTSSink);
			}
			else	//Add Demux Pins (Full TS Sink's)
				if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piFTSSink, 2))
				{
					(log << "Failed to Set the File Name on the Full TS Sink FileWriter Interface: " << hr << "\n").Write(hr);
					DestroyFilter(m_piFTSSink);
				}
	}

	if (m_intSinkType& 0x2)
	{
		//MPEG-2 Demultiplexer (TS Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piTSMpeg2Demux, L"TS Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//FileWriter (TS Sink's)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piTSSink, L"TS Sink FileWriter"))
			{
				(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyFilter(m_piTSMpeg2Demux);
			}
			else	//Connect Demux (TS Sink's)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyFilter(m_piTSSink);
					DestroyFilter(m_piTSMpeg2Demux);
				}
				else	//Add Demux Pins (TS Sink's)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyFilter(m_piTSSink);
							DestroyFilter(m_piTSMpeg2Demux);
					}
					else	//Connect FileWriter (TS Sink's)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piTSMpeg2Demux, m_piTSSink))
						{
							(log << "Failed to connect TS Sink MPEG-2 Demultiplexer to TS Sink FileWriter: " << hr << "\n").Write(hr);
							DestroyFilter(m_piTSSink);
							DestroyFilter(m_piTSMpeg2Demux);
						}
						else	//Add Demux Pins (TS Sink's)
							if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTSSink, 22))
							{
								(log << "Failed to Set the File Name on the TS Sink FileWriter Interface: " << hr << "\n").Write(hr);
								DestroyFilter(m_piTSSink);
								DestroyFilter(m_piTSMpeg2Demux);
							}
	};

	if (m_intSinkType& 0x4)
	{
		//MPEG-2 Demultiplexer (MPG Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piMPGMpeg2Demux, L"MPG Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG Sink's)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Multiplexer, &m_piMPGMpeg2Mux, L"MPG Sink MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyFilter(m_piMPGMpeg2Demux);
			}
			else //FileWriter (MPG Sink's)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piMPGSink, L"MPG Sink FileWriter"))
				{
					(log << "Failed to add Full MPG Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyFilter(m_piMPGMpeg2Mux);
					DestroyFilter(m_piMPGMpeg2Demux);
				}
				else	//Connect Demux (MPG Sink's)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyFilter(m_piMPGSink);
						DestroyFilter(m_piMPGMpeg2Mux);
						DestroyFilter(m_piMPGMpeg2Demux);
					}
					else	//Add Demux Pins (MPG Sink's)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyFilter(m_piMPGSink);
								DestroyFilter(m_piMPGMpeg2Mux);
								DestroyFilter(m_piMPGMpeg2Demux);
						}
						else	//Connect Mux (MPG Sink's)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Audio to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyFilter(m_piMPGSink);
								DestroyFilter(m_piMPGMpeg2Mux);
								DestroyFilter(m_piMPGMpeg2Demux);
							}
							else	//Connect Mux (MPG Sink's)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyFilter(m_piMPGSink);
									DestroyFilter(m_piMPGMpeg2Mux);
									DestroyFilter(m_piMPGMpeg2Demux);
								}
								else	//Connect FileWriter (MPG Sink's)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Mux, m_piMPGSink))
									{
										(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyFilter(m_piMPGSink);
										DestroyFilter(m_piMPGMpeg2Mux);
										DestroyFilter(m_piMPGMpeg2Demux);
									}
									else	//Add Demux Pins (MPG Sink's)
										if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piMPGSink, 3))
										{
											(log << "Failed to Set the File Name on the MPG Sink FileWriter Interface: " << hr << "\n").Write(hr);
											DestroyFilter(m_piMPGSink);
											DestroyFilter(m_piMPGMpeg2Mux);
											DestroyFilter(m_piMPGMpeg2Demux);
										}
	}

	if (m_intSinkType& 0x8)
	{
		//MPEG-2 Demultiplexer (Seperate A/V Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piAVMpeg2Demux, L"A/V Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add A/V Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//FileWriter (Seperate A/V Sink's) Audio
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piAudioSink, L"A/V Audio Sink FileWriter"))
			{
				(log << "Failed to add Audio Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyFilter(m_piAVMpeg2Demux);
			}
			else		//FileWriter (Seperate A/V Sink's) Teletext
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piTelexSink, L"A/V Teletext Sink FileWriter"))
				{
					(log << "Failed to add Teletext Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyFilter(m_piAudioSink);
					DestroyFilter(m_piAVMpeg2Demux);
				}
				else		//FileWriter (Seperate A/V Sink's) Video
					if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_FileWriterDump, &m_piVideoSink, L"A/V Video Sink FileWriter"))
					{
						(log << "Failed to add Video Sink FileWriter to the graph: " << hr << "\n").Write(hr);
						DestroyFilter(m_piTelexSink);
						DestroyFilter(m_piAudioSink);
						DestroyFilter(m_piAVMpeg2Demux);
					}
					else		//Connect Demux (Seperate A/V Sink's)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piAVMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyFilter(m_piVideoSink);
						DestroyFilter(m_piTelexSink);
						DestroyFilter(m_piAudioSink);
						DestroyFilter(m_piAVMpeg2Demux);
					}
					else	//Add Demux Pins (Seperate A/V Sink's)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piAVMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyFilter(m_piVideoSink);
								DestroyFilter(m_piTelexSink);
								DestroyFilter(m_piAudioSink);
								DestroyFilter(m_piAVMpeg2Demux);
						}
						else		//Connect FileWriter (Seperate A/V Sink's) Audio
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piAudioSink))
							{
								(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Audio Sink FileWriter: " << hr << "\n").Write(hr);
								DestroyFilter(m_piVideoSink);
								DestroyFilter(m_piTelexSink);
								DestroyFilter(m_piAudioSink);
								DestroyFilter(m_piAVMpeg2Demux);
							}
							else		//Connect FileWriter (Seperate A/V Sink's) Teletext
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piTelexSink))
								{
									(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Teletext Sink FileWriter: " << hr << "\n").Write(hr);
									DestroyFilter(m_piVideoSink);
									DestroyFilter(m_piTelexSink);
									DestroyFilter(m_piAudioSink);
									DestroyFilter(m_piAVMpeg2Demux);
								}
								else		//Connect FileWriter (Seperate A/V Sink's) Video
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piVideoSink))
									{
										(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Video Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyFilter(m_piVideoSink);
										DestroyFilter(m_piTelexSink);
										DestroyFilter(m_piAudioSink);
										DestroyFilter(m_piAVMpeg2Demux);
									}
									else	//Add Demux Pins (Seperate A/V Sink's) Video
										if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piVideoSink, 4))
										{
											(log << "Failed to Set the File Name on the Video Sink FileWriter Interface: " << hr << "\n").Write(hr);
											DestroyFilter(m_piVideoSink);
											DestroyFilter(m_piTelexSink);
											DestroyFilter(m_piAudioSink);
											DestroyFilter(m_piAVMpeg2Demux);
										}
										else	//Add Demux Pins (Seperate A/V Sink's) Teletext
											if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTelexSink, 6))
											{
												(log << "Failed to Set the File Name on the Teletext Sink FileWriter Interface: " << hr << "\n").Write(hr);
												DestroyFilter(m_piVideoSink);
												DestroyFilter(m_piTelexSink);
												DestroyFilter(m_piAudioSink);
												DestroyFilter(m_piAVMpeg2Demux);
											}
											else	//Add Demux Pins (Seperate A/V Sink's) Audio
												if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piAudioSink, 5))
												{
													(log << "Failed to Set the File Name on the Audio Sink FileWriter Interface: " << hr << "\n").Write(hr);
													DestroyFilter(m_piVideoSink);
													DestroyFilter(m_piTelexSink);
													DestroyFilter(m_piAudioSink);
													DestroyFilter(m_piAVMpeg2Demux);
												}
	}



	m_bActive = TRUE;
	return S_OK;
}

void BDADVBTSinkFile::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}



HRESULT BDADVBTSinkFile::RemoveSinkFilters()
{
	m_bActive = FALSE;

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();

	DestroyFilter(m_piVideoSink);
	DestroyFilter(m_piAudioSink);
	DestroyFilter(m_piAVMpeg2Demux);
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

HRESULT BDADVBTSinkFile::SetTransportStreamPin(IPin* piPin)
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

BOOL BDADVBTSinkFile::IsActive()
{
	return m_bActive;
}


