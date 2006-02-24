/**
 *	BDADVBTSinkFile.cpp
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


#include "BDADVBTSinkFile.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>

//////////////////////////////////////////////////////////////////////
// BDADVBTSinkFile
//////////////////////////////////////////////////////////////////////

BDADVBTSinkFile::BDADVBTSinkFile(BDADVBTSink *pBDADVBTSink) :
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
	m_bRecording = FALSE;
	m_bPaused = FALSE;

}

BDADVBTSinkFile::~BDADVBTSinkFile()
{
	DestroyAll();
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
		return (log << "DVB-T File Sink tried to initialise a second time\n").Write(E_FAIL);

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

HRESULT BDADVBTSinkFile::DestroyAll()
{
    HRESULT hr = S_OK;

	RemoveSinkFilters();

	m_piMediaControl.Release();
	m_piGraphBuilder.Release();

	return S_OK;
}

//--- Add & connect the Sink filters ---
HRESULT BDADVBTSinkFile::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	if (m_intSinkType& 0x1)
	{
		//FileWriter (Full TS Sink's)
		if FAILED(hr = AddDWDumpFilter(L"Full TS Sink FileWriter", &m_piFTSDWDump, m_piFTSSink))
		{
			(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else	//Connect FileWriter (Full TS Sink's)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS Sink FileWriter: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
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
			if FAILED(hr = AddDWDumpFilter(L"TS Sink FileWriter", &m_piTSDWDump, m_piTSSink))
			{
				(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else	//Connect Demux (TS Sink's)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS Sink's)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyTSFilters();
					}
					else	//Connect FileWriter (TS Sink's)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piTSMpeg2Demux, m_piTSSink))
						{
							(log << "Failed to connect TS Sink MPEG-2 Demultiplexer to TS Sink FileWriter: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else
							m_pBDADVBTSink->ClearDemuxPids(m_piTSMpeg2Demux);
	}

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
				DestroyMPGFilters();
			}
			else //FileWriter (MPG Sink's)
				if FAILED(hr = AddDWDumpFilter(L"MPG Sink FileWriter", &m_piMPGDWDump, m_piMPGSink))
				{
					(log << "Failed to add Full MPG Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG Sink's)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG Sink's)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyMPGFilters();
						}
						else	//Connect Mux (MPG Sink's)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Audio to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else	//Connect Mux (MPG Sink's)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Demux, m_piMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect FileWriter (MPG Sink's)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piMPGMpeg2Mux, m_piMPGSink))
									{
										(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
//									else
//										m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);

	}

	if (m_intSinkType& 0x8)
	{
		//MPEG-2 Demultiplexer (Seperate A/V Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, &m_piAVMpeg2Demux, L"A/V Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add A/V Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//FileWriter (Seperate A/V Sink's) Audio
			if FAILED(hr = AddDWDumpFilter(L"A/V Audio Sink FileWriter", &m_piAudioDWDump, m_piAudioSink))
			{
				(log << "Failed to add Audio Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyAVFilters();
			}
			else		//FileWriter (Seperate A/V Sink's) Teletext
				if FAILED(hr = AddDWDumpFilter(L"A/V Teletext Sink FileWriter", &m_piTelexDWDump, m_piTelexSink))
				{
					(log << "Failed to add Teletext Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyAVFilters();
				}
				else		//FileWriter (Seperate A/V Sink's) Video
					if FAILED(hr = AddDWDumpFilter(L"A/V Video Sink FileWriter", &m_piVideoDWDump, m_piVideoSink))
					{
						(log << "Failed to add Video Sink FileWriter to the graph: " << hr << "\n").Write(hr);
						DestroyAVFilters();
					}
					else		//Connect Demux (Seperate A/V Sink's)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piAVMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyAVFilters();
					}
					else	//Add Demux Pins (Seperate A/V Sink's)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piAVMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyAVFilters();
						}
						else		//Connect FileWriter (Seperate A/V Sink's) Audio
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piAudioSink))
							{
								(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Audio Sink FileWriter: " << hr << "\n").Write(hr);
								DestroyAVFilters();
							}
							else		//Connect FileWriter (Seperate A/V Sink's) Teletext
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piTelexSink))
								{
									(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Teletext Sink FileWriter: " << hr << "\n").Write(hr);
									DestroyAVFilters();
								}
								else		//Connect FileWriter (Seperate A/V Sink's) Video
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piAVMpeg2Demux, m_piVideoSink))
									{
										(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Video Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyAVFilters();
									}
									else
										m_pBDADVBTSink->ClearDemuxPids(m_piAVMpeg2Demux);
	}

	m_bActive = TRUE;
	return hr;
}

void BDADVBTSinkFile::DestroyFTSFilters()
{
	DestroyFilter(m_piFTSSink);
	DeleteFilter(&m_piFTSDWDump);
}

void BDADVBTSinkFile::DestroyTSFilters()
{
	DeleteFilter(&m_piTSDWDump);
	DestroyFilter(m_piTSSink);
	DestroyFilter(m_piTSMpeg2Demux);
}

void BDADVBTSinkFile::DestroyMPGFilters()
{
	DeleteFilter(&m_piMPGDWDump);
	DestroyFilter(m_piMPGSink);
	DestroyFilter(m_piMPGMpeg2Mux);
	DestroyFilter(m_piMPGMpeg2Demux);
}

void BDADVBTSinkFile::DestroyAVFilters()
{
	DeleteFilter(&m_piVideoDWDump);
	DestroyFilter(m_piVideoSink);
	DeleteFilter(&m_piTelexDWDump);
	DestroyFilter(m_piTelexSink);
	DeleteFilter(&m_piAudioDWDump);
	DestroyFilter(m_piAudioSink);
	DestroyFilter(m_piAVMpeg2Demux);
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

	DestroyFTSFilters();
	DestroyTSFilters();
	DestroyMPGFilters();
	DestroyAVFilters();

	return S_OK;
}

void BDADVBTSinkFile::DeleteFilter(DWDump **pfDWDump)
{
	if (pfDWDump)
		return;

	if (*pfDWDump)
		delete *pfDWDump;

	*pfDWDump = NULL;

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

HRESULT BDADVBTSinkFile::AddDWDumpFilter(LPWSTR name, DWDump **pfDWDump, CComPtr <IBaseFilter> &pFilter)
{
	HRESULT hr = S_OK;

	// Dump filter for recording
	*pfDWDump = new DWDump(&hr);
	if (FAILED(hr))
	{
		(log << "Failed to Create the DWDump Filter Instance on the " << name << " : " << hr << "\n").Write(hr);
		return hr;
	}

	if FAILED(hr = (*pfDWDump)->QueryInterface(IID_IBaseFilter, reinterpret_cast<void **>(&pFilter)))
	{
		(log << "Failed to Get the DWDump Filter IBaseFilter Interface on the " << name << " : " << hr << "\n").Write(hr);
		delete *pfDWDump;
		*pfDWDump = NULL;
		return hr;
	}

	return m_piGraphBuilder->AddFilter(pFilter, name);
}

HRESULT BDADVBTSinkFile::StartRecording(DVBTChannels_Service* pService)
{
	(log << "Recording Starting on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_piFTSSink)
	{
		//Add Demux Pins (Full TS Sink's)
		if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piFTSSink, 2))
			(log << "Failed to Set the File Name on the Full TS Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piFTSDWDump->Record())
			(log << "Failed to Start Recording on the Full TS Sink Filter: " << hr << "\n").Write(hr);
	}

	if ((m_intSinkType& 0x2) && m_piTSSink)
	{
		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux, 1))
			(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTSSink, 22))
			(log << "Failed to Set the File Name on the TS Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piTSDWDump->Record())
			(log << "Failed to Start Recording on the TS Sink Filter: " << hr << "\n").Write(hr);
	}

	if ((m_intSinkType& 0x4) && m_piMPGSink)
	{
		if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piMPGSink, 3))
			(log << "Failed to Set the File Name on the MPG Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
			(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piMPGDWDump->Record())
			(log << "Failed to Start Recording on the MPG Sink Filter: " << hr << "\n").Write(hr);
/*
		IReferenceClock *piReferenceClock;
		m_piMPGMpeg2Demux->SetSyncSource(NULL);
		Sleep(100);
		m_piMPGMpeg2Demux->GetSyncSource(&piReferenceClock);
		m_piMPGMpeg2Demux->SetSyncSource(piReferenceClock);
		m_piMPGMpeg2Mux->SetSyncSource(piReferenceClock);
*/
	}

	if ((m_intSinkType& 0x8) && (m_piVideoSink || m_piAudioSink || m_piTelexSink))
	{
		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piAVMpeg2Demux))
			(log << "Failed to Add Output Pins to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		//Add Demux Pins (Seperate A/V Sink's) Video
		if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piVideoSink, 4))
			(log << "Failed to Set the File Name on the Video Sink FileWriter Interface: " << hr << "\n").Write(hr);
		else	//Add Demux Pins (Seperate A/V Sink's) Teletext
			if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piTelexSink, 6))
				(log << "Failed to Set the File Name on the Teletext Sink FileWriter Interface: " << hr << "\n").Write(hr);
			else	//Add Demux Pins (Seperate A/V Sink's) Audio
				if FAILED(hr = m_pBDADVBTSink->AddFileName(pService, m_piAudioSink, 5))
					(log << "Failed to Set the File Name on the Audio Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piVideoDWDump->Record())
			(log << "Failed to Start Recording on the Video AV Sink Filter: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piTelexDWDump->Record())
			(log << "Failed to Start Recording on the Teletext AV Sink Filter: " << hr << "\n").Write(hr);

		if FAILED(hr = m_piAudioDWDump->Record())
			(log << "Failed to Start Recording on the Audio AV Sink Filter: " << hr << "\n").Write(hr);
	}

	m_bRecording = TRUE;

	return hr;
}

HRESULT BDADVBTSinkFile::StopRecording(void)
{
	(log << "Recording Stopping on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_piFTSSink)
	{
		m_piFTSDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_piFTSSink, m_intSinkType);
	}

	if ((m_intSinkType& 0x2) && m_piTSSink)
	{
		m_piTSDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_piTSSink, m_intSinkType);
		m_pBDADVBTSink->ClearDemuxPids(m_piTSMpeg2Demux);
	}

	if ((m_intSinkType& 0x4) && m_piMPGSink)
	{
		m_piMPGDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_piMPGSink, m_intSinkType);
//		m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);
	}

	if ((m_intSinkType& 0x8) && (m_piVideoSink || m_piAudioSink || m_piTelexSink))
	{
		m_piVideoDWDump->Record();
		m_piTelexDWDump->Record();
		m_piAudioDWDump->Record();
		m_pBDADVBTSink->ClearDemuxPids(m_piAVMpeg2Demux);
		m_pBDADVBTSink->NullFileName(m_piVideoSink, m_intSinkType);
		m_pBDADVBTSink->NullFileName(m_piTelexSink, m_intSinkType);
		m_pBDADVBTSink->NullFileName(m_piAudioSink, m_intSinkType);
	}

	m_bRecording = FALSE;
	return hr;
}

HRESULT BDADVBTSinkFile::PauseRecording(void)
{
	(log << "Pausing Recording on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_piFTSSink)
		hr = m_piFTSDWDump->Pause();

	if ((m_intSinkType& 0x2) && m_piTSSink)
	{
		hr = m_piTSDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_piTSMpeg2Demux);
	}

	if ((m_intSinkType& 0x4) && m_piMPGSink)
	{
		hr = m_piMPGDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);
	}
	
	if ((m_intSinkType& 0x8) && (m_piVideoSink || m_piAudioSink || m_piTelexSink))
	{
		hr = m_piVideoDWDump->Pause();
		hr = m_piTelexDWDump->Pause();
		hr = m_piAudioDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_piAVMpeg2Demux);
	}

	m_bPaused = TRUE;

	return hr;
}

HRESULT BDADVBTSinkFile::UnPauseRecording(DVBTChannels_Service* pService)
{
	(log << "UnPausing Recording on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_piFTSSink)
	{
		hr = m_piFTSDWDump->Pause();
	}

	if ((m_intSinkType& 0x2) && m_piTSSink)
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piTSMpeg2Demux))
//			(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_piTSDWDump->Pause();
	}

	if ((m_intSinkType& 0x4) && m_piMPGSink)
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piMPGMpeg2Demux))
//			(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_piMPGDWDump->Pause();
	}
	
	if ((m_intSinkType& 0x8) && (m_piVideoSink || m_piAudioSink || m_piTelexSink))
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_piAVMpeg2Demux))
//			(log << "Failed to Add Output Pins to AV Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_piVideoDWDump->Pause();
		hr = m_piTelexDWDump->Pause();
		hr = m_piAudioDWDump->Pause();
	}


	m_bPaused = FALSE;

	return hr;
}

BOOL BDADVBTSinkFile::IsRecording(void)
{
	if ((m_intSinkType & 0x1))
	{
		if (m_piFTSSink)
			return m_piFTSDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x2))
	{
		if (m_piTSSink)
			return m_piTSDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x4))
	{
		if (m_piMPGSink)
			return m_piMPGDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x8))
	{
		if (m_piVideoSink || m_piAudioSink || m_piTelexSink)
			return 	m_piVideoDWDump->IsRecording() |
					m_piTelexDWDump->IsRecording() |
					m_piAudioDWDump->IsRecording();
	}

	return m_bRecording;
}

BOOL BDADVBTSinkFile::IsPaused(void)
{
	if ((m_intSinkType & 0x1) && m_piFTSSink)
	{
		if (m_piFTSSink)
			return m_piFTSDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x2) && m_piTSSink)
	{
		if (m_piTSSink)
			return m_piTSDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x4) && m_piMPGSink)
	{
		if (m_piMPGSink)
			return m_piMPGDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x8) && (m_piVideoSink || m_piAudioSink || m_piTelexSink))
	{
		if (m_piVideoSink || m_piAudioSink || m_piTelexSink)
			return 	m_piVideoDWDump->IsPaused() |
					m_piTelexDWDump->IsPaused() |
					m_piAudioDWDump->IsPaused();
	}

	return m_bPaused;
}

HRESULT BDADVBTSinkFile::GetCurFile(LPOLESTR *ppszFileName)
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
