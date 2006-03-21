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

	m_piGraphBuilder = NULL;
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

HRESULT BDADVBTSinkFile::Initialise(IGraphBuilder *piGraphBuilder, int intSinkType)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T File Sink tried to initialise a second time\n").Write(E_FAIL);

	if (!piGraphBuilder)
		return (log << "Must pass a valid DWGraph object to Initialise a Sink\n").Write(E_FAIL);

	m_piGraphBuilder = piGraphBuilder;

	//--- COM should already be initialized ---

	if FAILED(hr = m_piGraphBuilder->QueryInterface(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	m_intSinkType = intSinkType;
	m_bInitialised = TRUE;
	return S_OK;
}

HRESULT BDADVBTSinkFile::DestroyAll()
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

	return S_OK;
}

//--- Add & connect the Sink filters ---
HRESULT BDADVBTSinkFile::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	if (m_intSinkType& 0x1)
	{
		//FileWriter (Full TS Sink's)
		if FAILED(hr = AddDWDumpFilter(L"Full TS Sink FileWriter", &m_pFTSDWDump, m_pFTSSink))
		{
			(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else	//Connect FileWriter (Full TS Sink's)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS Sink FileWriter: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
			}
	}

	if (m_intSinkType& 0x2)
	{
		//MPEG-2 Demultiplexer (TS Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pTSMpeg2Demux, L"TS Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//FileWriter (TS Sink's)
			if FAILED(hr = AddDWDumpFilter(L"TS Sink FileWriter", &m_pTSDWDump, m_pTSSink))
			{
				(log << "Failed to add Full TS Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else	//Connect Demux (TS Sink's)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS Sink's)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyTSFilters();
					}
					else	//Connect FileWriter (TS Sink's)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pTSMpeg2Demux, m_pTSSink))
						{
							(log << "Failed to connect TS Sink MPEG-2 Demultiplexer to TS Sink FileWriter: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else
						{
							graphTools.SetReferenceClock(m_pTSMpeg2Demux);
							m_pBDADVBTSink->ClearDemuxPids(m_pTSMpeg2Demux);
						}
	}

	if (m_intSinkType& 0x4)
	{
		//MPEG-2 Demultiplexer (MPG Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pMPGMpeg2Demux, L"MPG Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG Sink's)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.mpgmuxclsid, &m_pMPGMpeg2Mux, L"MPG Sink MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyMPGFilters();
			}
			else //FileWriter (MPG Sink's)
				if FAILED(hr = AddDWDumpFilter(L"MPG Sink FileWriter", &m_pMPGDWDump, m_pMPGSink))
				{
					(log << "Failed to add Full MPG Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG Sink's)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG Sink's)
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyMPGFilters();
						}
						else	//Connect Mux (MPG Sink's)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Audio to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else //Connect Mux (MPG Sink's)
								if ((CComBSTR)GUID_NULL == (CComBSTR)g_pData->settings.filterguids.quantizerclsid)
								{
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
									{
										(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Connect FileWriter (MPG Sink's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
										{
											(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG Sink FileWriter: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
//										else
//											m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);
								}
								else
								{
									if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.quantizerclsid, &m_pMPGQuantizer, L"MPG Sink Quantizer"))
									{
										(log << "Failed to add MPG Sink Quantizer to the graph: " << hr << "\n").Write(hr);
									}
									else //MPEG-2 Multiplexer (MPG Sink's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGQuantizer))
										{
											(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG Sink Quantizer: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else
											if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGQuantizer, m_pMPGMpeg2Mux))
											{
												(log << "Failed to connect MPG Sink MPG Sink Quantizer Video to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
												DestroyMPGFilters();
											}
											else	//Connect FileWriter (MPG Sink's)
												if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
												{
													(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG Sink FileWriter: " << hr << "\n").Write(hr);
													DestroyMPGFilters();
												}
												else
													graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
								}
/*
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG Sink MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect FileWriter (MPG Sink's)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
									{
										(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else
										graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
//										m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);
*/
	}

	if (m_intSinkType& 0x8)
	{
		//MPEG-2 Demultiplexer (Seperate A/V Sink's)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pAVMpeg2Demux, L"A/V Sink MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add A/V Sink MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//FileWriter (Seperate A/V Sink's) Audio
			if FAILED(hr = AddDWDumpFilter(L"A/V Audio Sink FileWriter", &m_pAudioDWDump, m_pAudioSink))
			{
				(log << "Failed to add Audio Sink FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyAVFilters();
			}
			else		//FileWriter (Seperate A/V Sink's) Teletext
				if FAILED(hr = AddDWDumpFilter(L"A/V Teletext Sink FileWriter", &m_pTelexDWDump, m_pTelexSink))
				{
					(log << "Failed to add Teletext Sink FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyAVFilters();
				}
				else		//FileWriter (Seperate A/V Sink's) Video
					if FAILED(hr = AddDWDumpFilter(L"A/V Video Sink FileWriter", &m_pVideoDWDump, m_pVideoSink))
					{
						(log << "Failed to add Video Sink FileWriter to the graph: " << hr << "\n").Write(hr);
						DestroyAVFilters();
					}
					else		//Connect Demux (Seperate A/V Sink's)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pAVMpeg2Demux))
						{
							(log << "Failed to connect Infinite Pin Tee Filter to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyAVFilters();
						}
						else	//Add Demux Pins (Seperate A/V Sink's)
							if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pAVMpeg2Demux))
							{
								(log << "Failed to Add Output Pins to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyAVFilters();
							}
							else		//Connect FileWriter (Seperate A/V Sink's) Audio
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pAVMpeg2Demux, m_pAudioSink))
								{
									(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Audio Sink FileWriter: " << hr << "\n").Write(hr);
									DestroyAVFilters();
								}
								else		//Connect FileWriter (Seperate A/V Sink's) Teletext
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pAVMpeg2Demux, m_pTelexSink))
									{
										(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Teletext Sink FileWriter: " << hr << "\n").Write(hr);
										DestroyAVFilters();
									}
									else	//Connect FileWriter (Seperate A/V Sink's) Video
										if ((CComBSTR)GUID_NULL == (CComBSTR)g_pData->settings.filterguids.quantizerclsid)
										{
											if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pAVMpeg2Demux, m_pVideoSink))
											{
												(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Video Sink FileWriter: " << hr << "\n").Write(hr);
												DestroyAVFilters();
											}
											else
												m_pBDADVBTSink->ClearDemuxPids(m_pAVMpeg2Demux);
										}
										else
										{
											if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.quantizerclsid, &m_pVideoQuantizer, L"A/V Video Sink Quantizer"))
											{
												(log << "Failed to add A/V Video Sink Quantizer to the graph: " << hr << "\n").Write(hr);
											}
											else
												if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pAVMpeg2Demux, m_pVideoQuantizer))
												{
													(log << "Failed to connect A/V Sink MPEG-2 Demultiplexer to Video Sink Quantizer: " << hr << "\n").Write(hr);
													DestroyAVFilters();
												}
												else
													if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pVideoQuantizer, m_pVideoSink))
													{
														(log << "Failed to connect A/V Sink Quantizer to Video Sink FileWriter: " << hr << "\n").Write(hr);
														DestroyAVFilters();
													}
													else
													{
														m_pBDADVBTSink->ClearDemuxPids(m_pAVMpeg2Demux);
														graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
													}
										}

	}

	m_bActive = TRUE;
	return hr;
}

void BDADVBTSinkFile::DestroyFTSFilters()
{
	DestroyFilter(m_pFTSSink);
	DeleteFilter(&m_pFTSDWDump);
}

void BDADVBTSinkFile::DestroyTSFilters()
{
	DeleteFilter(&m_pTSDWDump);
	DestroyFilter(m_pTSSink);
	DestroyFilter(m_pTSMpeg2Demux);
}

void BDADVBTSinkFile::DestroyMPGFilters()
{
	DeleteFilter(&m_pMPGDWDump);
	DestroyFilter(m_pMPGSink);
	DestroyFilter(m_pMPGMpeg2Mux);
	DestroyFilter(m_pMPGQuantizer);
	DestroyFilter(m_pMPGMpeg2Demux);
}

void BDADVBTSinkFile::DestroyAVFilters()
{
	DeleteFilter(&m_pVideoDWDump);
	DestroyFilter(m_pVideoSink);
	DeleteFilter(&m_pTelexDWDump);
	DestroyFilter(m_pTelexSink);
	DeleteFilter(&m_pAudioDWDump);
	DestroyFilter(m_pAudioSink);
	DestroyFilter(m_pVideoQuantizer);
	DestroyFilter(m_pAVMpeg2Demux);
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

	m_pInfinitePinTee = pinInfo.pFilter;

	return S_OK;
}

BOOL BDADVBTSinkFile::IsActive()
{
	return m_bActive;
}

HRESULT BDADVBTSinkFile::AddDWDumpFilter(LPWSTR name, DWDump **pfDWDump, CComPtr <IBaseFilter> &pFilter)
{
	HRESULT hr = S_OK;

	if ((CComBSTR)GUID_NULL == (CComBSTR)g_pData->settings.filterguids.filewriterclsid)
	{
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
	else
	{
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.filewriterclsid, &pFilter, name))
			return (log << "Failed to add the FileWriter Filter to the graph: " << hr << "\n").Write(hr);
	}
	return hr;

}

//DWS HRESULT BDADVBTSinkFile::StartRecording(DVBTChannels_Service* pService)
//DWS28-02-2006 HRESULT BDADVBTSinkFile::StartRecording(DVBTChannels_Service* pService, LPWSTR pFileName)
HRESULT BDADVBTSinkFile::StartRecording(DVBTChannels_Service* pService, LPWSTR pFileName, LPWSTR pPath)
{
	(log << "Recording Starting on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_pFTSSink)
	{
		//Add Demux Pins (Full TS Sink's)
//DWS28-02-2006		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pFTSFileName, pService, m_pFTSSink, 2, pFileName))		
		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pFTSFileName, pService, m_pFTSSink, 2, pFileName, pPath))		
			(log << "Failed to Set the File Name on the Full TS Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if (m_pFTSDWDump)
		{
			if FAILED(hr = m_pFTSDWDump->Record())
				(log << "Failed to Start Recording on the Full TS Sink Filter: " << hr << "\n").Write(hr);
		}
	}

	if ((m_intSinkType& 0x2) && m_pTSSink)
	{
		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pTSMpeg2Demux, 1))
			(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

//DWS28-02-2006		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTSFileName, pService, m_pTSSink, 22, pFileName))
		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTSFileName, pService, m_pTSSink, 22, pFileName, pPath))
			(log << "Failed to Set the File Name on the TS Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if (m_pTSDWDump)
		{
			if FAILED(hr = m_pTSDWDump->Record())
				(log << "Failed to Start Recording on the TS Sink Filter: " << hr << "\n").Write(hr);
		}
	}

	if ((m_intSinkType& 0x4) && m_pMPGSink)
	{
//DWS28-02-2006		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 3, pFileName))
		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 3, pFileName, pPath))
			(log << "Failed to Set the File Name on the MPG Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pMPGMpeg2Demux))
			(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		if (m_pMPGDWDump)
		{
			if FAILED(hr = m_pMPGDWDump->Record())
				(log << "Failed to Start Recording on the MPG Sink Filter: " << hr << "\n").Write(hr);
		}
/*
		IReferenceClock *piReferenceClock;
		m_piMPGMpeg2Demux->SetSyncSource(NULL);
		Sleep(100);
		m_piMPGMpeg2Demux->GetSyncSource(&piReferenceClock);
		m_piMPGMpeg2Demux->SetSyncSource(piReferenceClock);
		m_piMPGMpeg2Mux->SetSyncSource(piReferenceClock);
*/
	}

	if ((m_intSinkType& 0x8) && (m_pVideoSink || m_pAudioSink || m_pTelexSink))
	{
		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pAVMpeg2Demux))
			(log << "Failed to Add Output Pins to A/V Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		//Add Demux Pins (Seperate A/V Sink's) Video
//DWS28-02-2006		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pVideoFileName, pService, m_pVideoSink, 4, pFileName))
		if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pVideoFileName, pService, m_pVideoSink, 4, pFileName, pPath))
			(log << "Failed to Set the File Name on the Video Sink FileWriter Interface: " << hr << "\n").Write(hr);
		else	//Add Demux Pins (Seperate A/V Sink's) Teletext
//DWS28-02-2006			if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTelexFileName, pService, m_pTelexSink, 6, pFileName))
			if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTelexFileName, pService, m_pTelexSink, 6, pFileName, pPath))
				(log << "Failed to Set the File Name on the Teletext Sink FileWriter Interface: " << hr << "\n").Write(hr);
			else	//Add Demux Pins (Seperate A/V Sink's) Audio
//DWS28-02-2006				if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pAudioFileName, pService, m_pAudioSink, 5, pFileName))
				if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pAudioFileName, pService, m_pAudioSink, 5, pFileName, pPath))
					(log << "Failed to Set the File Name on the Audio Sink FileWriter Interface: " << hr << "\n").Write(hr);

		if (m_pVideoDWDump)
		{
			if FAILED(hr = m_pVideoDWDump->Record())
				(log << "Failed to Start Recording on the Video AV Sink Filter: " << hr << "\n").Write(hr);
		}

		if (m_pTelexDWDump)
		{
			if FAILED(hr = m_pTelexDWDump->Record())
				(log << "Failed to Start Recording on the Teletext AV Sink Filter: " << hr << "\n").Write(hr);
		}

		if (m_pAudioDWDump)
		{
			if FAILED(hr = m_pAudioDWDump->Record())
				(log << "Failed to Start Recording on the Audio AV Sink Filter: " << hr << "\n").Write(hr);
		}
	}

	m_bRecording = TRUE;

	return hr;
}

HRESULT BDADVBTSinkFile::StopRecording(void)
{
	(log << "Recording Stopping on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_pFTSSink)
	{
		if (m_pFTSDWDump)
			m_pFTSDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_pFTSSink, m_intSinkType);
	}

	if ((m_intSinkType& 0x2) && m_pTSSink)
	{
		if (m_pTSDWDump)
			m_pTSDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_pTSSink, m_intSinkType);
		m_pBDADVBTSink->ClearDemuxPids(m_pTSMpeg2Demux);
	}

	if ((m_intSinkType& 0x4) && m_pMPGSink)
	{
		if (m_pMPGDWDump)
			m_pMPGDWDump->Record();
		m_pBDADVBTSink->NullFileName(m_pMPGSink, m_intSinkType);
//		m_pBDADVBTSink->ClearDemuxPids(m_piMPGMpeg2Demux);
	}

	if ((m_intSinkType& 0x8) && (m_pVideoSink || m_pAudioSink || m_pTelexSink))
	{
		if (m_pVideoDWDump)
			m_pVideoDWDump->Record();
		if (m_pTelexDWDump)
			m_pTelexDWDump->Record();
		if (m_pAudioDWDump)
			m_pAudioDWDump->Record();

		m_pBDADVBTSink->ClearDemuxPids(m_pAVMpeg2Demux);
		m_pBDADVBTSink->NullFileName(m_pVideoSink, m_intSinkType);
		m_pBDADVBTSink->NullFileName(m_pTelexSink, m_intSinkType);
		m_pBDADVBTSink->NullFileName(m_pAudioSink, m_intSinkType);
	}

	m_bRecording = FALSE;
	return hr;
}

HRESULT BDADVBTSinkFile::PauseRecording(void)
{
	(log << "Pausing Recording on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_pFTSSink && m_pFTSDWDump)
		hr = m_pFTSDWDump->Pause();

	if ((m_intSinkType& 0x2) && m_pTSSink && m_pTSDWDump)
	{
		hr = m_pTSDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_pTSMpeg2Demux);
	}

	if ((m_intSinkType& 0x4) && m_pMPGSink && m_pMPGDWDump)
	{
		hr = m_pMPGDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_pMPGMpeg2Demux);
	}
	
	if ((m_intSinkType& 0x8) &&
		(m_pVideoSink || m_pAudioSink || m_pTelexSink) &&
		 m_pVideoDWDump  || m_pTelexDWDump || m_pAudioDWDump)
	{
		hr = m_pVideoDWDump->Pause();
		hr = m_pTelexDWDump->Pause();
		hr = m_pAudioDWDump->Pause();
//		m_pBDADVBTSink->ClearDemuxPids(m_pAVMpeg2Demux);
	}

	m_bPaused = TRUE;

	return hr;
}

HRESULT BDADVBTSinkFile::UnPauseRecording(DVBTChannels_Service* pService)
{
	(log << "UnPausing Recording on Sink FileWriter \n").Write();

	HRESULT hr = S_OK;

	if ((m_intSinkType& 0x1) && m_pFTSSink && m_pFTSDWDump)
	{
		hr = m_pFTSDWDump->Pause();
	}

	if ((m_intSinkType& 0x2) && m_pTSSink && m_pTSDWDump)
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pTSMpeg2Demux))
//			(log << "Failed to Add Output Pins to TS Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_pTSDWDump->Pause();
	}

	if ((m_intSinkType& 0x4) && m_pMPGSink && m_pMPGDWDump)
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pMPGMpeg2Demux))
//			(log << "Failed to Add Output Pins to MPG Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_pMPGDWDump->Pause();
	}
	
	if ((m_intSinkType& 0x8) &&
		(m_pVideoSink || m_pAudioSink || m_pTelexSink)
		 && m_pVideoDWDump || m_pTelexDWDump || m_pAudioDWDump)
	{
//		if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pAVMpeg2Demux))
//			(log << "Failed to Add Output Pins to AV Sink MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);

		hr = m_pVideoDWDump->Pause();
		hr = m_pTelexDWDump->Pause();
		hr = m_pAudioDWDump->Pause();
	}


	m_bPaused = FALSE;

	return hr;
}

BOOL BDADVBTSinkFile::IsRecording(void)
{
	if ((m_intSinkType & 0x1))
	{
		if (m_pFTSSink && m_pFTSDWDump)
			return m_pFTSDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x2))
	{
		if (m_pTSSink && m_pTSDWDump)
			return m_pTSDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x4))
	{
		if (m_pMPGSink && m_pMPGDWDump)
			return m_pMPGDWDump->IsRecording();
	}
	else if ((m_intSinkType & 0x8))
	{
		if ((m_pVideoSink || m_pAudioSink || m_pTelexSink) &&
			 (m_pVideoDWDump || m_pTelexDWDump || m_pAudioDWDump))
			return 	m_pVideoDWDump->IsRecording() |
					m_pTelexDWDump->IsRecording() |
					m_pAudioDWDump->IsRecording();
	}

	return m_bRecording;
}

BOOL BDADVBTSinkFile::IsPaused(void)
{
	if ((m_intSinkType & 0x1) && m_pFTSSink)
	{
		if (m_pFTSSink && m_pFTSDWDump)
			return m_pFTSDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x2) && m_pTSSink)
	{
		if (m_pTSSink && m_pTSDWDump)
			return m_pTSDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x4) && m_pMPGSink)
	{
		if (m_pMPGSink && m_pMPGDWDump)
			return m_pMPGDWDump->IsPaused();
	}
	else if ((m_intSinkType & 0x8) && (m_pVideoSink || m_pAudioSink || m_pTelexSink))
	{
		if ((m_pVideoSink || m_pAudioSink || m_pTelexSink) &&
			 (m_pVideoDWDump || m_pTelexDWDump || m_pAudioDWDump))
			return 	m_pVideoDWDump->IsPaused() |
					m_pTelexDWDump->IsPaused() |
					m_pAudioDWDump->IsPaused();
	}

	return m_bPaused;
}

HRESULT BDADVBTSinkFile::GetCurFile(LPOLESTR *ppszFileName)
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

HRESULT BDADVBTSinkFile::GetCurFileSize(__int64 *pllFileSize)
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

