/**
 *	BDADVBTSourceTuner.cpp
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


#include "BDADVBTSourceTuner.h"
#include "Globals.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "dsnetifc.h"

#define toIPAddress(a, b, c, d) (a + (b << 8) + (c << 16) + (d << 24))


//////////////////////////////////////////////////////////////////////
// BDADVBTSourceTuner
//////////////////////////////////////////////////////////////////////


BDADVBTSourceTuner::BDADVBTSourceTuner(BDACard *pBDACard)
{
	m_pBDACard = pBDACard;
	m_pDWGraph = NULL;

	activationRank = 0;
	lockedAsActiveTuner = 0;

	m_bInitialised = 0;
	m_lFrequency = -1;
	m_lBandwidth = -1;

	m_piGraphBuilder = NULL;
	m_piMediaControl = NULL;

	m_piBDANetworkProvider = NULL;
	m_piBDATuner = NULL;
	m_piBDACapture = NULL;
	m_piBDAMpeg2Demux = NULL;
	m_piBDATIF = NULL;
	m_piBDASecTab = NULL;
	m_piInfinitePinTee = NULL;
	m_piDSNetworkSink = NULL;

	m_piTuningSpace = NULL;
}

BDADVBTSourceTuner::~BDADVBTSourceTuner()
{
	DestroyAll();
}

BOOL BDADVBTSourceTuner::Initialise(DWGraph *pDWGraph)
{
	HRESULT hr;
	if (m_bInitialised)
		return (g_log << "DVB-T Source Tuner tried to initialise a second time").Write();

	if (!pBDACard)
		return (g_log << "Must pass a valid BDACard object to Initialise a tuner").Write();

	if (!pDWGraph)
		return (g_log << "Must pass a valid DWGraph object to Initialise a tuner").Write();

	m_pDWGraph = pDWGraph;

	//--- COM should already be initialized ---

	//--- Create Graph ---
/*	if (FAILED(hr = m_piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER)))
		return (g_log << "Failed Creating DVB-T Graph Builder").Write();

	//--- Add To Running Object Table --- (for graphmgr.exe)
	if (g_pData->settings.application.addToROT)
	{
		if (FAILED(hr = AddToRot(m_piGraphBuilder, &m_rotEntry)))
		{
			//TODO: release graphbuilder
			return (g_log << "Failed adding DVB-T graph to ROT").Write();
		}
	}

	//--- Get InterfacesInFilters ---
	if (FAILED(hr = m_piGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_piMediaControl.p)))
		return (g_log << "Failed to get Media Control interface").Write();


	//--- Initialise Tune Request ---
//	if (FAILED(hr = InitTuningSpace()))
	if (FAILED(hr = InitDVBTTuningSpace(m_piTuningSpace)))
		return (g_log << "Failed to initialise the Tune Request").Write();

	//--- Get NetworkType CLSID ---
	CComBSTR bstrNetworkType;
	CLSID CLSIDNetworkType;
	if (FAILED(hr = m_piTuningSpace->get_NetworkType(&bstrNetworkType)))
		return (g_log << "Failed to get TuningSpace Network Type").Write();

	if (FAILED(hr = CLSIDFromString(bstrNetworkType, &CLSIDNetworkType)))
		return (g_log << "Could not convert Network Type to CLSID").Write();


    //--- Create Network Provider ---
	if (FAILED(hr = AddFilter(m_piGraphBuilder.p, CLSIDNetworkType, m_piBDANetworkProvider.p, L"Network Provider")))
		return (g_log << "Failed to add Network Provider to the graph").Write();

	//--- Create TuneRequest ---
	CComPtr <ITuneRequest> pTuneRequest;
	m_lFrequency = -1;
	m_lBandwidth = -1;
//	if (FAILED(hr = this->submitTuneRequest(pTuneRequest.p)))
	if (FAILED(hr = SubmitDVBTTuneRequest(m_piTuningSpace, pTuneRequest, m_lFrequency, m_lBandwidth)))
		return (g_log << "Failed to create the Tune Request.").Write();

	//--- Apply TuneRequest ---
	CComQIPtr <ITuner> pTuner(m_piBDANetworkProvider);
	if (!pTuner)
		return (g_log << "Failed while interfacing Tuner with Network Provider").Write();

	if (FAILED(hr = pTuner->put_TuneRequest(pTuneRequest)))
		return (g_log << "Failed to submit the Tune Tequest to the Network Provider").Write();
	pTuner.Release();

	//--- We can now add the rest of the source filters ---
	
	// THDTV DVB-t BDA Tuner Filter
	//if (FAILED(hr = AddFilterByName(m_piGraphBuilder, m_piBDATuner.p, KSCATEGORY_BDA_NETWORK_TUNER, L"THDTV DVB-t BDA Tuner Filter")))
	if (FAILED(hr = AddFilterByDisplayName(m_piGraphBuilder, m_piBDATuner.p, m_pBDACard->tunerDevice.strDevicePath, m_pBDACard->tunerDevice.strFriendlyName)))
	{
		DestroyAll();
		return (g_log << "Cannot load Tuner Device").Write();
	}
    
	// THDTV DVB-t BDA Capture Filter
	//if (FAILED(hr = AddFilterByName(m_piGraphBuilder, m_piBDACapture.p, KSCATEGORY_BDA_RECEIVER_COMPONENT, L"THDTV DVB-t BDA Capture Filter")))
	if (FAILED(hr = AddFilterByDisplayName(m_piGraphBuilder, m_piBDACapture.p, m_pBDACard->captureDevice.strDevicePath, m_pBDACard->captureDevice.strFriendlyName)))
	{
		DestroyAll();
		return (g_log << "Cannot load Capture Device").Write();
	}

	//Infinite Pin Tee
	if (FAILED(hr = AddFilter(m_piGraphBuilder, CLSID_InfTee, m_piInfinitePinTee.p, L"Infinite Pin Tee")))
	{
		DestroyAll();
		return (g_log << "Failed to add Infinite Pin Tee to the graph").Write();
	}

	//MPEG-2 Demultiplexer (BDA's)
	if (FAILED(hr = AddFilter(m_piGraphBuilder, CLSID_MPEG2Demultiplexer, m_piBDAMpeg2Demux.p, L"BDA MPEG-2 Demultiplexer")))
	{
		DestroyAll();
		return (g_log << "Failed to add BDA MPEG-2 Demultiplexer to the graph").Write();
	}

	// BDA MPEG2 Transport Information Filter
	if (FAILED(hr = AddFilterByName(m_piGraphBuilder, m_piBDATIF.p, KSCATEGORY_BDA_TRANSPORT_INFORMATION, L"BDA MPEG2 Transport Information Filter")))
	{
		DestroyAll();
		return (g_log << "Cannot load TIF").Write();
	}

	// MPEG2 Sections and Tables
	if (FAILED(hr = AddFilterByName(m_piGraphBuilder, m_piBDASecTab.p, KSCATEGORY_BDA_TRANSPORT_INFORMATION, L"MPEG-2 Sections and Tables")))
	{
		DestroyAll();
		return (g_log << "Cannot load MPEG-2 Sections and Tables").Write();
	}

	if (FAILED(hr = AddFilterByName(m_piGraphBuilder, m_piDSNetworkSink.p, CLSID_LegacyAmFilterCategory, L"MPEG-2 Multicast Sender (DigitalWatch)")))
	{
		DestroyAll();
		return (g_log << "Cannot load MPEG-2 Multicast Sender").Write();
	}

	//--- Now connect up all the filters ---
    if (FAILED(hr = ConnectFilters(m_piBDANetworkProvider, m_piBDATuner)))
		return (g_log << "Failed to connect Network Provider to Tuner Filter").Write();

	if (FAILED(hr = ConnectFilters(m_piBDATuner, m_piBDACapture)))
		return (g_log << "Failed to connect Tuner Filter to Capture Filter").Write();

	if (FAILED(hr = ConnectFilters(m_piBDACapture, m_piInfinitePinTee)))
		return (g_log << "Failed to connect Capture Filter to Infinite Pin Tee").Write();

	if (FAILED(hr = ConnectFilters(m_piInfinitePinTee, m_piBDAMpeg2Demux)))
		return (g_log << "Failed to connect Infinite Pin Tee Filter to BDA Demux").Write();

	if (FAILED(hr = ConnectFilters(m_piBDAMpeg2Demux, m_piBDATIF)))
		return (g_log << "Failed to connect to BDA Demux to TIF").Write();

	if (FAILED(hr = ConnectFilters(m_piBDAMpeg2Demux, m_piBDASecTab)))
		return (g_log << "Failed to connect BDA Demux to MPEG-2 Sections and Tables").Write();

	if (FAILED(hr = ConnectFilters(m_piInfinitePinTee, m_piDSNetworkSink)))
		return (g_log << "Failed to connect Infinite Pin Tee Filter to DSNetwork Sender filter").Write();
	
	//Setup dsnet sender
	IMulticastConfig *piMulticastConfig = NULL;
	if (FAILED(hr = m_piDSNetworkSink->QueryInterface(IID_IMulticastConfig, (void**)&piMulticastConfig)))
		return (g_log << "Failed to query Sink filter for IMulticastConfig").Write();
	if (FAILED(hr = piMulticastConfig->SetNetworkInterface(0))) //0 == INADDR_ANY
		return (g_log << "Failed to set network interface for Sink filter").Write();

    ULONG ulIP = toIPAddress(224,0,0,1);
	if (FAILED(hr = piMulticastConfig->SetMulticastGroup(ulIP, htons(1234))))
		return (g_log << "Failed to set multicast group for Sink filter").Write();
	piMulticastConfig->Release();
*/
	m_bInitialised = TRUE;
	return TRUE;
}

BOOL BDADVBTSourceTuner::DestroyAll()
{
    HRESULT hr = S_OK;
    CComPtr <IBaseFilter> pFilter;
    CComPtr <IEnumFilters> pFilterEnum;

	m_piTuningSpace.Release();

    hr = m_piGraphBuilder->EnumFilters(&pFilterEnum);
	switch (hr)
	{
	case S_OK:
		break;
	case E_OUTOFMEMORY:
		return (g_log << "Insufficient memory to create the enumerator.").Write();
	case E_POINTER:
		return (g_log << "Null pointer argument.").Write();
	default:
		return (g_log << "Unknown Error enumerating graph: " << hr).Write();
	}

    if (FAILED(hr = pFilterEnum->Reset()))
		return (g_log << "Failed to reset graph enumerator").Write();

	while (pFilterEnum->Next(1, &pFilter, 0) == S_OK) // addrefs filter
	{
		if (FAILED(hr = m_piGraphBuilder->RemoveFilter(pFilter)))
		{
			FILTER_INFO info;
			pFilter->QueryFilterInfo(&info);
			if (info.pGraph)
				info.pGraph->Release();
            return (g_log << "Failed to remove filter: " << info.achName).Write();
		}
		pFilter.Release();
	}
	pFilterEnum.Release();

	return TRUE;
}

BOOL BDADVBTSourceTuner::LockChannel(long frequency, long bandwidth)
{
	return TRUE;
}

long BDADVBTSourceTuner::GetCurrentFrequency()
{
	return m_lFrequency;
}

BOOL BDADVBTSourceTuner::GetSignalStats(BOOL &locked, long &strength, long &quality)
{
	return FALSE;
}

