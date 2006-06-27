/**
 *	BDADVBTimeShiftTuner.cpp
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


#include "BDADVBTimeShiftTuner.h"
#include "BDADVBTimeShift.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "dsnetifc.h"
#include "TSFileSinkGuids.h"
#include "Winsock.h"

//////////////////////////////////////////////////////////////////////
// BDADVBTimeShiftTuner
//////////////////////////////////////////////////////////////////////

BDADVBTimeShiftTuner::BDADVBTimeShiftTuner(BDADVBTimeShift *pBDADVBTimeShift, BDACard *pBDACard) :
	m_pBDADVBTimeShift(pBDADVBTimeShift),
	m_pBDACard(pBDACard)
{
	m_piGraphBuilder = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;
	m_bTunerRunning = FALSE;

	m_lFrequency = -1;
	m_lBandwidth = -1;

	m_pMpeg2DataParser = NULL;
	m_pMpeg2DataParser = new DVBMpeg2DataParser();
	m_pMpeg2DataParser->SetDVBTChannels(m_pBDADVBTimeShift->GetChannels());

}

BDADVBTimeShiftTuner::~BDADVBTimeShiftTuner()
{
	DestroyAll();
	if (m_pMpeg2DataParser)
	{
		m_pMpeg2DataParser->ReleaseFilter();
		delete m_pMpeg2DataParser;
		m_pMpeg2DataParser = NULL;
	}
}

void BDADVBTimeShiftTuner::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTimeShiftTuner::Initialise(IGraphBuilder *piGraphBuilder)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T Source Tuner tried to initialise a second time\n").Write(E_FAIL);

	if (!piGraphBuilder)
		return (log << "Must pass a valid DWGraph object to Initialise a tuner\n").Write(E_FAIL);

	m_piGraphBuilder = piGraphBuilder;

	//--- COM should already be initialized ---

	if FAILED(hr = m_piGraphBuilder->QueryInterface(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	//--- Initialise Tune Request ---
	if FAILED(hr = graphTools.InitDVBTTuningSpace(m_piTuningSpace))
		return (log << "Failed to initialise the Tune Request: " << hr << "\n").Write(hr);

	m_bInitialised = TRUE;
	m_bTunerRunning = FALSE;
	return S_OK;
}

HRESULT BDADVBTimeShiftTuner::DestroyAll()
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

	RemoveSourceFilters();

	m_piTuningSpace.Release();
	m_piMediaControl.Release();
//	m_piGraphBuilder.Release();

	return S_OK;
}

HRESULT BDADVBTimeShiftTuner::AddSourceFilters()
{
	HRESULT hr;

	//--- Get NetworkType CLSID ---
	CComBSTR bstrNetworkType;
	CLSID CLSIDNetworkType;
	if FAILED(hr = m_piTuningSpace->get_NetworkType(&bstrNetworkType))
		return (log << "Failed to get TuningSpace Network Type: " << hr << "\n").Write(hr);

	if FAILED(hr = CLSIDFromString(bstrNetworkType, &CLSIDNetworkType))
		return (log << "Could not convert Network Type to CLSID: " << hr << "\n").Write(hr);


    //--- Create Network Provider ---
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, CLSIDNetworkType, &m_piBDANetworkProvider, L"Network Provider"))
		return (log << "Failed to add Network Provider to the graph: " << hr << "\n").Write(hr);

	//--- Create TuneRequest ---
	CComPtr <ITuneRequest> pTuneRequest;
	m_lFrequency = -1;
	m_lBandwidth = -1;
	if FAILED(hr = graphTools.CreateDVBTTuneRequest(m_piTuningSpace, pTuneRequest, m_lFrequency, m_lBandwidth))
		return (log << "Failed to create the Tune Request: " << hr << "\n").Write(hr);

	//--- Apply TuneRequest ---
	CComQIPtr <ITuner> pTuner(m_piBDANetworkProvider);
	if (!pTuner)
		return (log << "Failed while interfacing Tuner with Network Provider\n").Write(E_FAIL);

	if FAILED(hr = pTuner->put_TuneRequest(pTuneRequest))
		return (log << "Failed to submit the Tune Tequest to the Network Provider: " << hr << "\n").Write(hr);
	pTuner.Release();

	//--- We can now add the rest of the source filters ---
	if FAILED(hr = m_pBDACard->AddFilters(m_piGraphBuilder))
	{
		DestroyAll();
		return (log << "Failed to add card filters: " << hr << "\n").Write(hr);
	}

	//Infinite Pin Tee
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.infteeclsid, &m_piInfinitePinTee, L"Infinite Pin Tee"))
	{
		DestroyAll();
		return (log << "Failed to add Infinite Pin Tee to the graph: " << hr << "\n").Write(hr);
	}

	//MPEG-2 Demultiplexer (BDA's)
	if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_piBDAMpeg2Demux, L"BDA MPEG-2 Demultiplexer"))
	{
		DestroyAll();
		return (log << "Failed to add BDA MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
	}

	// BDA MPEG2 Transport Information Filter
	if FAILED(hr = graphTools.AddFilterByName(m_piGraphBuilder, &m_piBDATIF, KSCATEGORY_BDA_TRANSPORT_INFORMATION, L"BDA MPEG2 Transport Information Filter"))
	{
		DestroyAll();
		return (log << "Cannot load TIF: " << hr << "\n").Write(hr);
	}

	// MPEG2 Sections and Tables
	if FAILED(hr = graphTools.AddFilterByName(m_piGraphBuilder, &m_piBDASecTab, KSCATEGORY_BDA_TRANSPORT_INFORMATION, L"MPEG-2 Sections and Tables"))
	{
		DestroyAll();
		return (log << "Cannot load MPEG-2 Sections and Tables: " << hr << "\n").Write(hr);
	}

	//--- Now connect up all the filters ---
	

    if FAILED(hr = m_pBDACard->Connect(m_piBDANetworkProvider))
		return (log << "Failed to connect Card Filters: " << hr << "\n").Write(hr);

	CComPtr <IPin> pCapturePin;
	m_pBDACard->GetCapturePin(&pCapturePin);

	CComPtr <IPin> pInfPinTeePin;
	if FAILED(hr = graphTools.FindPin(m_piInfinitePinTee, L"Input", &pInfPinTeePin, REQUESTED_PINDIR_INPUT))
		return (log << "Failed to get input pin on Infinite Pin Tee: " << hr << "\n").Write(hr);

	if FAILED(hr = m_piGraphBuilder->ConnectDirect(pCapturePin, pInfPinTeePin, NULL))
		return (log << "Failed to connect Capture filter to Infinite Pin Tee: " << hr << "\n").Write(hr);

	if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piInfinitePinTee, m_piBDAMpeg2Demux))
		return (log << "Failed to connect Infinite Pin Tee Filter to BDA Demux: " << hr << "\n").Write(hr);

	if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piBDAMpeg2Demux, m_piBDATIF))
		return (log << "Failed to connect to BDA Demux to TIF: " << hr << "\n").Write(hr);

	if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_piBDAMpeg2Demux, m_piBDASecTab))
		return (log << "Failed to connect BDA Demux to MPEG-2 Sections and Tables: " << hr << "\n").Write(hr);

	//Set reference clock
	if FAILED(hr = graphTools.SetReferenceClock(m_piBDAMpeg2Demux))
		return (log << "Failed to get reference clock interface on BDA demux filter: " << hr << "\n").Write(hr);

	//Clear unused pins
	if FAILED(hr = graphTools.DeleteOutputPins(m_piBDAMpeg2Demux))
		return (log << "Failed to Delete the Output Pins on the Demux filter: " << hr << "\n").Write(hr);

	m_bActive = TRUE;
	m_bTunerRunning = FALSE;
	return S_OK;
}

void BDADVBTimeShiftTuner::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}



HRESULT BDADVBTimeShiftTuner::RemoveSourceFilters()
{
	m_bActive = FALSE;
	m_bTunerRunning = FALSE;

	if (m_pMpeg2DataParser)
		m_pMpeg2DataParser->ReleaseFilter();
/*
	if (m_piBDASecTab)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDASecTab);
		m_piBDASecTab.Release();
	}

	if (m_piBDATIF)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDATIF);
		m_piBDATIF.Release();
	}

	if (m_piBDAMpeg2Demux)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDAMpeg2Demux);
		m_piBDAMpeg2Demux.Release();
	}

	if (m_piInfinitePinTee)
	{
		m_piGraphBuilder->RemoveFilter(m_piInfinitePinTee);
		m_piInfinitePinTee.Release();
	}

	m_pBDACard->RemoveFilters();

	if (m_piBDANetworkProvider)
	{
		m_piGraphBuilder->RemoveFilter(m_piBDANetworkProvider);
		m_piBDANetworkProvider.Release();
	}
*/

	DestroyFilter(m_piBDASecTab);
	DestroyFilter(m_piBDATIF);
	DestroyFilter(m_piBDAMpeg2Demux);
	DestroyFilter(m_piInfinitePinTee);
	m_pBDACard->RemoveFilters();
	DestroyFilter(m_piBDANetworkProvider);

	return S_OK;
}

HRESULT BDADVBTimeShiftTuner::QueryTransportStreamPin(IPin** piPin)
{
	HRESULT hr;
	if FAILED(hr = graphTools.FindFirstFreePin(m_piInfinitePinTee, piPin, PINDIR_OUTPUT))
		return (log << "Failed to get output pin on Infinite Pin Tee: " << hr << "\n").Write(hr);
	return S_OK;
}

HRESULT BDADVBTimeShiftTuner::LockChannel(long frequency, long bandwidth)
{
	if(m_lFrequency == frequency)
		return S_OK;

	HRESULT hr;

	CComQIPtr <ITuner> pTuner(m_piBDANetworkProvider);
	if (!pTuner)
	{
		return (log << "Failed while interfacing Tuner with Network Provider\n").Write(E_FAIL);
	}

	for(m_lFrequency = -1; m_lFrequency >= -3; m_lFrequency--)
	{
		CComPtr <ITuneRequest> pTuneRequest;
		if SUCCEEDED(hr = graphTools.CreateDVBTTuneRequest(m_piTuningSpace, pTuneRequest, frequency, bandwidth))
		{
			if FAILED(hr = pTuner->put_TuneRequest(pTuneRequest))
			{
				return (log << "Failed to submit the Tune Request to the Network Provider: " << hr << "\n").Write(hr);
			}

			pTuner.Release();
			pTuneRequest.Release();
			if (frequency < 1)
				return (log << "Failed to recover the Tune Request frequency from the Network Provider: " << hr << "\n").Write(E_FAIL);

			m_lFrequency = frequency;
			return S_OK;
		}
		else
		{
			(log << "  Could not create new tune request: " << hr << "\n").Write(hr);
		}
	}
	
	pTuner.Release();

	return (log << "Error while locking channel (3 attempts failed)\n").Write(E_FAIL);
}

long BDADVBTimeShiftTuner::GetCurrentFrequency()
{
	return m_lFrequency;
}

HRESULT BDADVBTimeShiftTuner::StartScanning()
{
	if (m_pMpeg2DataParser)
	{
		m_pMpeg2DataParser->SetFrequency(m_lFrequency);
		m_pMpeg2DataParser->SetFilter(m_piBDASecTab);
		return m_pMpeg2DataParser->StartScan();
	}
	return S_FALSE;
}

HRESULT BDADVBTimeShiftTuner::StopScanning()
{
	if (m_pMpeg2DataParser)
		return m_pMpeg2DataParser->EndScan();

	return S_FALSE;
}

HRESULT BDADVBTimeShiftTuner::StopTIF()
{
	if (m_piBDATIF)
		return m_piBDATIF->Stop();

	return S_FALSE;
}

HRESULT BDADVBTimeShiftTuner::GetSignalStats(BOOL &locked, long &strength, long &quality)
{
	HRESULT hr = E_FAIL;
	BOOL present;
	locked = FALSE;
	present = FALSE;
	strength = 0;
	quality = 0;

	//make sure that we have data flowing to the tif before we stop it or get card stats.
	if (IsTunerRunning())
	{
		if FAILED(hr = m_pBDACard->GetSignalStatistics(locked, present, strength, quality))
			return hr;
	}

	return S_OK;
}

BOOL BDADVBTimeShiftTuner::IsTunerRunning()
{
	//return if already tested and was running
	if (m_bTunerRunning)
		return TRUE;

	//Setup to get the tune request
	CComQIPtr <ITuner> pTuner(m_piBDANetworkProvider);
	if (!pTuner)
	{
		(log << "Failed while interfacing Tuner with Network Provider\n").Write();
		return m_bTunerRunning;
	}

	CComPtr <ITuneRequest> pNewTuneRequest;			
	CComQIPtr <IDVBTuneRequest> pDVBTTuneRequest (pNewTuneRequest);
	pTuner->get_TuneRequest(&pNewTuneRequest);
	pDVBTTuneRequest = pNewTuneRequest;

	long Sid = -1;
	if (pDVBTTuneRequest != NULL)
	{
		long Onid = 0;
		pDVBTTuneRequest->get_ONID(&Onid);
		long Tsid = 0;
		pDVBTTuneRequest->get_TSID(&Tsid);

		pDVBTTuneRequest->get_SID(&Sid); 
	}

	if (Sid != -1)
		m_bTunerRunning = TRUE;
	else
		(log << "Tuner Not Running.\n").Write();

	return m_bTunerRunning;
}


BOOL BDADVBTimeShiftTuner::IsActive()
{
	return m_bActive;
}

LPWSTR BDADVBTimeShiftTuner::GetCardName()
{
	return m_pBDACard->tunerDevice.strFriendlyName;
}

