/**
 *	BDADVBTSinkDSNet.cpp
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


#include "BDADVBTSinkDSNet.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "dsnetifc.h"
#include "Winsock.h"

#define toIPAddress(a, b, c, d) (a + (b << 8) + (c << 16) + (d << 24))

//////////////////////////////////////////////////////////////////////
// BDADVBTSinkDSNet
//////////////////////////////////////////////////////////////////////

BDADVBTSinkDSNet::BDADVBTSinkDSNet(BDADVBTSink *pBDADVBTSink) :
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
}

BDADVBTSinkDSNet::~BDADVBTSinkDSNet()
{
	DestroyAll();
}

void BDADVBTSinkDSNet::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTSinkDSNet::Initialise(IGraphBuilder *piGraphBuilder, int intSinkType)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T DSNetwork Sink tried to initialise a second time\n").Write(E_FAIL);

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

HRESULT BDADVBTSinkDSNet::DestroyAll()
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

HRESULT BDADVBTSinkDSNet::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	//--- Add & connect the DSNetworking filters ---

	if (m_intSinkType == 1)
	{
		//DSNet (Full TS DSNetworking)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.dsnetclsid, &m_pFTSSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_DSNetworkSender, &m_pFTSSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//		if FAILED(hr = graphTools.AddFilterByName(m_piGraphBuilder, &m_piFTSSink, CLSID_LegacyAmFilterCategory, L"Full TS MPEG-2 Multicast Sender (BDA Compatible)"))
		{
			(log << "Failed to add Full TS MPEG Multicast Sender to the graph: " << hr << "\n").Write(hr);
		}
		else		//Connect Multicast Sender (Full TS DSNetworking)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS MPEG-2 Multicast Sender: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
			}
			else	//Add File Name (FTS DSNetworking))
				if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pFTSFileName, pService, m_pFTSSink))
				{
					(log << "Failed to Setup the Full TS DSNetwork Multicast Sender Interface: " << hr << "\n").Write(hr);
					DestroyFTSFilters();
				}
	}
	else if (m_intSinkType == 2)
	{
		//MPEG-2 Demultiplexer (TS DSNetworking)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pTSMpeg2Demux, L"TS DSNetwork MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS DSNetwork MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else	//Multicast Sender (TS DSNetworking)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.dsnetclsid, &m_pTSSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_DSNetworkSender, &m_pTSSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//			if FAILED(hr = graphTools.AddFilterByName(m_piGraphBuilder, &m_piTSSink, CLSID_LegacyAmFilterCategory, L"MPEG-2 Multicast Sender (BDA Compatible)"))
			{
				(log << "Failed to add TS DSNetwork MPEG Multicast Sender to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else	//Connect Demux (TS DSNetworking)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS DSNetwork MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS DSNetworking)
					if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS DSNetwork MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyTSFilters();
					}
					else		//Connect Multicast Sender (TS DSNetworking)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pTSMpeg2Demux, m_pTSSink))
						{
							(log << "Failed to connect TS DSNetwork MPEG-2 Demultiplexer to TS DSNetwork MPEG Multicast Sender: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else	//Add File Name (TS DSNetworking))
							if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTSFileName, pService, m_pTSSink))
							{
								(log << "Failed to Setup the TS DSNetwork Multicast Sender Interface: " << hr << "\n").Write(hr);
								DestroyTSFilters();
							}
							else
							{
								graphTools.SetReferenceClock(m_pTSMpeg2Demux);
//								m_pBDADVBTSink->ClearDemuxPids(m_pTSMpeg2Demux);
							}
	}
	else if (m_intSinkType == 3)
	{
		//MPEG-2 Demultiplexer (MPG DSNetworking)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pMPGMpeg2Demux, L"MPG DSNetworking MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG DSNetwork MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG DSNetworking)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.mpgmuxclsid, &m_pMPGMpeg2Mux, L"MPG DSNetworking Sender MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPG TimeShift MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyMPGFilters();
			}
			else //Multicast Sender (MPG DSNetworking)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.dsnetclsid, &m_pMPGSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSID_DSNetworkSender, &m_pMPGSink, L"MPEG-2 Multicast Sender (BDA Compatible)"))
//				if FAILED(hr = graphTools.AddFilterByName(m_piGraphBuilder, &m_piMPGSink, CLSID_LegacyAmFilterCategory, L"MPEG-2 Multicast Sender (BDA Compatible)"))
				{
					(log << "Failed to add MPG DSNetwork  FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG DSNetworking)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG DSNetwork MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG DSNetworking))
						if FAILED(hr = m_pBDADVBTSink->AddDemuxPins(pService, m_pMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG DSNetwork MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
							DestroyMPGFilters();
						}
						else	//Connect Mux (MPG DSNetworking)
							if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
							{
								(log << "Failed to connect MPG DSNetwork MPEG-2 Demultiplexer Audio to MPG DSNetwork MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else	//Connect Mux (MPG DSNetworking)
								if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
								{
									(log << "Failed to connect MPG DSNetwork MPEG-2 Demultiplexer Video to MPG DSNetwork MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect Multicast Sender (MPG DSNetworking)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
									{
										(log << "Failed to connect MPG DSNetwork MPEG-2 Multiplexer to MPG DSNetwork Multicast Sender: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Add Demux Pins (MPG DSNetworking))
										if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink))
										{
											(log << "Failed to Setup the MPG DSNetwork Multicast Sender Interface: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else
											graphTools.SetReferenceClock(m_pMPGMpeg2Demux);

/*								if ((CComBSTR)GUID_NULL == (CComBSTR)g_pData->settings.filterguids.quantizerclsid)
								{
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Connect FileWriter (MPG DSNetworking's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
										{
											(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else	//Add FileName (MPG DSNetworking)
											if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 111))
											{
												(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
												DestroyMPGFilters();
											}
											else
												graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
								}
								else
								{
									if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.quantizerclsid, &m_pMPGQuantizer, L"MPG TimeShift Quantizer"))
									{
										(log << "Failed to add MPG TimeShift Quantizer to the graph: " << hr << "\n").Write(hr);
									}
									else //MPEG-2 Multiplexer (MPG DSNetworking's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGQuantizer))
										{
											(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG TimeShift Quantizer: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else
											if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGQuantizer, m_pMPGMpeg2Mux))
											{
												(log << "Failed to connect MPG Sink MPG Sink Quantizer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
												DestroyMPGFilters();
											}
											else	//Connect FileWriter (MPG DSNetworking's)
												if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
												{
													(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
													DestroyMPGFilters();
												}
												else	//Add FileName (MPG DSNetworking)
													if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink))
													{
														(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
														DestroyMPGFilters();
													}
													else
														graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
								}
*/
	}
	m_bActive = TRUE;
	return hr;
}

void BDADVBTSinkDSNet::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

void BDADVBTSinkDSNet::DestroyFTSFilters()
{
	DestroyFilter(m_pFTSSink);
	DeleteFilter(&m_pFTSDWDump);
}

void BDADVBTSinkDSNet::DestroyTSFilters()
{
	DeleteFilter(&m_pTSDWDump);
	DestroyFilter(m_pTSSink);
	DestroyFilter(m_pTSMpeg2Demux);
}

void BDADVBTSinkDSNet::DestroyMPGFilters()
{
	DeleteFilter(&m_pMPGDWDump);
	DestroyFilter(m_pMPGSink);
	DestroyFilter(m_pMPGMpeg2Mux);
	DestroyFilter(m_pMPGQuantizer);
	DestroyFilter(m_pMPGMpeg2Demux);
}

void BDADVBTSinkDSNet::DestroyAVFilters()
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

void BDADVBTSinkDSNet::DeleteFilter(DWDump **pfDWDump)
{
	if (pfDWDump)
		return;

	if (*pfDWDump)
		delete *pfDWDump;

	*pfDWDump = NULL;

}

HRESULT BDADVBTSinkDSNet::RemoveSinkFilters()
{
	m_bActive = FALSE;

	DestroyFTSFilters();
	DestroyTSFilters();
	DestroyMPGFilters();
	DestroyAVFilters();

	return S_OK;
}

HRESULT BDADVBTSinkDSNet::SetTransportStreamPin(IPin* piPin)
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

BOOL BDADVBTSinkDSNet::IsActive()
{
	return m_bActive;
}


