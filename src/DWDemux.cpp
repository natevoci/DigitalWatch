/**
 *  DWDemux.cpp
 *	Copyright (C) 2005 Nate
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

#include <streams.h>
#include "bdaiface.h"
#include "ks.h"
#include "ksmedia.h"
#include "bdamedia.h"
#include "MediaFormats.h"
#include "DWDemux.h"
#include "Bdatif.h"
#include "tuner.h"
#include <commctrl.h>

#include <atlbase.h>

DWDemux::DWDemux() :

	m_FilterRefList(NAME("MyFilterList")),
	m_bAuto(TRUE),
	m_bMPEG2AudioMediaType(TRUE),
	m_bMPEG2Audio2Mode(FALSE),
	m_bNPControl(FALSE),
	m_bNPSlave(FALSE),
	m_bConnectBusyFlag(FALSE),
	m_WasPlaying(FALSE),
	m_WasPaused(FALSE),
	m_bAC3Mode(FALSE),//(TRUE),
	m_StreamH264(FALSE),
	m_StreamMpeg4(FALSE),
	m_StreamVid(FALSE),
	m_StreamAC3(FALSE),
	m_StreamMP2(FALSE),
	m_StreamAud2(FALSE),
	m_StreamAAC(FALSE),
	m_StreamDTS(FALSE),
	m_ClockMode(1),
	m_SelTelexPid(0),
	m_SelSubtitlePid(0),
	m_SelAudioPid(0), 
	m_SelVideoPid(0),
	m_bFixedAR(FALSE),
	m_bCreateTSPinOnDemux(FALSE),
	m_bCreateTxtPinOnDemux(FALSE),
	m_bCreateSubPinOnDemux(FALSE)
{
}

DWDemux::~DWDemux()
{
	//Clear the filter list;
	POSITION pos = m_FilterRefList.GetHeadPosition();
	while (pos){

		if (m_FilterRefList.Get(pos) != NULL)
			m_FilterRefList.Get(pos)->Release();

		m_FilterRefList.Remove(pos);
		pos = m_FilterRefList.GetHeadPosition();
	}
}

void DWDemux::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);
	graphTools.SetLogCallback(callback);
}

// Find all the immediate upstream or downstream peers of a filter.
HRESULT DWDemux::GetPeerFilters(
    IBaseFilter *pFilter, // Pointer to the starting filter
    PIN_DIRECTION Dir,    // Direction to search (upstream or downstream)
    CFilterList &FilterList)  // Collect the results in this list.
{
    if (!pFilter) return E_POINTER;

    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr)) return hr;
    while (S_OK == pEnum->Next(1, &pPin, 0))
    {
        // See if this pin matches the specified direction.
        PIN_DIRECTION thisPinDir;
        hr = pPin->QueryDirection(&thisPinDir);
        if (FAILED(hr))
        {
            // Something strange happened.
            hr = E_UNEXPECTED;
            pPin->Release();
            break;
        }
        if (thisPinDir == Dir)
        {
            // Check if the pin is connected to another pin.
            IPin *pPinNext = 0;
            hr = pPin->ConnectedTo(&pPinNext);
            if (SUCCEEDED(hr))
            {
                // Get the filter that owns that pin.
                PIN_INFO PinInfo;
                hr = pPinNext->QueryPinInfo(&PinInfo);
                pPinNext->Release();
                if (FAILED(hr) || (PinInfo.pFilter == NULL))
                {
                    // Something strange happened.
                    pPin->Release();
                    pEnum->Release();
                    return E_UNEXPECTED;
                }
                // Insert the filter into the list.
                AddFilterUnique(FilterList, PinInfo.pFilter);
                PinInfo.pFilter->Release();
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    return S_OK;
}

void DWDemux::AddFilterUnique(CFilterList &FilterList, IBaseFilter *pNew)
{
    if (pNew == NULL) return;

    POSITION pos = FilterList.GetHeadPosition();
    while (pos)
    {
        IBaseFilter *pF = FilterList.GetNext(pos);
        if (IsEqualObject(pF, pNew))
        {
            return;
        }
    }
    pNew->AddRef();  // The caller must release everything in the list.
    FilterList.AddTail(pNew);
}

// Get the first upstream or downstream filter
HRESULT DWDemux::GetNextFilter(
    IBaseFilter *pFilter, // Pointer to the starting filter
    PIN_DIRECTION Dir,    // Direction to search (upstream or downstream)
    IBaseFilter **ppNext) // Receives a pointer to the next filter.
{
    if (!pFilter || !ppNext) return E_POINTER;

    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr)) return hr;
    while (S_OK == pEnum->Next(1, &pPin, 0))
    {
        // See if this pin matches the specified direction.
        PIN_DIRECTION thisPinDir;
        hr = pPin->QueryDirection(&thisPinDir);
        if (FAILED(hr))
        {
            // Something strange happened.
            hr = E_UNEXPECTED;
            pPin->Release();
            break;
        }
        if (thisPinDir == Dir)
        {
            // Check if the pin is connected to another pin.
            IPin *pPinNext = 0;
            hr = pPin->ConnectedTo(&pPinNext);
            if (SUCCEEDED(hr))
            {
                // Get the filter that owns that pin.
                PIN_INFO PinInfo;
                hr = pPinNext->QueryPinInfo(&PinInfo);
                pPinNext->Release();
                pPin->Release();
                pEnum->Release();
                if (FAILED(hr) || (PinInfo.pFilter == NULL))
                {
                    // Something strange happened.
                    return E_UNEXPECTED;
                }
                // This is the filter we're looking for.
                *ppNext = PinInfo.pFilter; // Client must release.
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching filter.
    return E_FAIL;
}

HRESULT DWDemux::AOnConnect(IBaseFilter *pTSSourceFilter,
							DVBTChannels_Service* pService,
							IBaseFilter *pClockFilter)
{
	if (!pTSSourceFilter)
		return S_FALSE;

	m_pTSSourceFilter = pTSSourceFilter;

	if (!pService)
		return S_FALSE;

	m_pService = pService;

//	if (m_bConnectBusyFlag || m_pNetwork->m_ParsingLock)
//		return S_FALSE;

	// Check if Enabled
	if (!m_bAuto && !m_bNPControl && !m_bNPSlave)
		return S_FALSE;

	CAutoLock Demuxlock(&m_DemuxLock);

	m_bConnectBusyFlag = TRUE;
	m_WasPlaying = FALSE;
	m_WasPaused = FALSE;
	m_TimeOut[0] = 0;
	m_TimeOut[1] = 0;

//	if (IsStopped() != S_FALSE)
//		DoStop();

	if (IsPaused() != S_FALSE)
	{
		m_TimeOut[0] = 10000;
		if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
		m_WasPaused = TRUE;
	}
	else if (IsPlaying() == S_OK)
	{
		m_TimeOut[0] = 10000;
		m_WasPlaying = TRUE;
	}
/*
	// Parse only the existing Network Provider Filter
	// in the filter graph, we do this by looking for filters
	// that implement the ITuner interface while
	// the count is still active.
	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		IEnumFilters* EnumFilters;
		if(SUCCEEDED(Info.pGraph->EnumFilters(&EnumFilters)))
		{
			IBaseFilter* pFilter;
			ULONG Fetched(0);
			while(EnumFilters->Next(1, &pFilter, &Fetched) == S_OK)
			{
				if(pFilter != NULL)
				{
					UpdateNetworkProvider(pFilter);
	
					pFilter->Release();
					pFilter = NULL;
				}
			}
			EnumFilters->Release();
		}
		Info.pGraph->Release();
	}
*/
	// Parse only the existing Mpeg2 Demultiplexer Filter
	// in the filter graph, we do this by looking for filters
	// that implement the IMpeg2Demultiplexer interface while
	// the count is still active.
	CFilterList FList(NAME("MyList"));  // List to hold the downstream peers.
	if (SUCCEEDED(GetPeerFilters(m_pTSSourceFilter, PINDIR_OUTPUT, FList))
		&& FList.GetHeadPosition())
	{
		IBaseFilter* pFilter = NULL;
		POSITION pos = FList.GetHeadPosition();
		pFilter = FList.Get(pos);
		while (SUCCEEDED(GetPeerFilters(pFilter, PINDIR_OUTPUT, FList)) && pos)
		{
			pFilter = FList.GetNext(pos);
		}

		pos = FList.GetHeadPosition();

		while (pos)
		{
			pFilter = FList.GetNext(pos);
			if(pFilter != NULL)
			{
                AddFilterUnique(m_FilterRefList, pFilter);
				UpdateDemuxPins(pFilter);
				pFilter = NULL;
			}
		}
	}

	if (IsPlaying() == S_FALSE)
	{
		if (pClockFilter != NULL)
			SetReferenceClock(pClockFilter);
		else if (m_ClockMode)
			SetRefClock();

		//Re Start the Graph if was running and was stopped.
		if (m_WasPlaying || m_WasPaused)
			if (DoStart() == S_OK){while(IsPlaying() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}

		//Pause the Graph if was Paused and was stopped.
		if (m_WasPaused && !m_WasPlaying)
			if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
	}

	//Reload the filter list of to catch any changes to the filtergraph
	if (SUCCEEDED(GetPeerFilters(m_pTSSourceFilter, PINDIR_OUTPUT, FList))
		&& FList.GetHeadPosition())
	{
		IBaseFilter* pFilter = NULL;
		POSITION pos = FList.GetHeadPosition();
		pFilter = FList.Get(pos);
		while (SUCCEEDED(GetPeerFilters(pFilter, PINDIR_OUTPUT, FList)) && pos)
		{
			pFilter = FList.GetNext(pos);
		}
	}

	//Clear the filter list;
	POSITION pos = FList.GetHeadPosition();
	while (pos){

		if (FList.Get(pos) != NULL)
			FList.Get(pos)->Release();

		FList.Remove(pos);
		pos = FList.GetHeadPosition();
	}

	m_bConnectBusyFlag = FALSE;

	return NOERROR;
}


HRESULT DWDemux::UpdateDemuxPins(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	// Check if Enabled
	if (!m_bAuto)
	{
		return S_FALSE;
	}

	// Get an instance of the Demux control interface
	IMpeg2Demultiplexer* muxInterface = NULL;
	if(SUCCEEDED(pDemux->QueryInterface (&muxInterface)))
	{
		// Update Video Pin
		if (CheckVideoPin(pDemux) != S_OK){
			// change pin type if we do have a Video pid & can don't already have an video pin
			LPWSTR PinName = L"Video";
			BOOL connect = FALSE;
			ChangeDemuxPin(pDemux, &PinName, &connect);
			if (FAILED(CheckVideoPin(pDemux))){
				// if we can't change the pin type to the new video make a new pin
				muxInterface->DeleteOutputPin(PinName);
				hr = NewVideoPin(muxInterface, PinName);
			}
			if (connect){
				// If old pin was already connected
				IPin* pIPin;
				if (SUCCEEDED(pDemux->FindPin(PinName, &pIPin))){
					// Reconnect pin
					RenderFilterPin(pIPin);
					pIPin->Release();
				}
			}
		}
//		// Update Video Pin
//		if (FAILED(CheckVideoPin(pDemux))){
//				// If no Video Pin was found
//				hr = NewVideoPin(muxInterface, L"Video");
//		}
		


		// Update Audio Pin
		if (!(m_StreamAC3 | m_StreamMP2)) {

			USHORT pPid;
			pPid = get_AAC_AudioPid();
			// If we do have an AAC audio pid
			if (m_StreamAAC || pPid) {
				// Update AAC Pin
				if (FAILED(CheckAACPin(pDemux))){
					// If no AAC Pin was found
					hr = NewAACPin(muxInterface, L"Audio");
				}
			}
//			else if (FAILED(CheckAudioPin(pDemux))){
//				// If no Audio Pin was found
//				hr = NewAudioPin(muxInterface, L"Audio");
//			}

			pPid = get_DTS_AudioPid();
			// If we do have an DTS audio pid
			if (m_StreamDTS || pPid) {
				// Update DTS Pin
				if (FAILED(CheckDTSPin(pDemux))){
					// If no DTS Pin was found
					hr = NewDTSPin(muxInterface, L"Audio");
				}
			}
		}

		// If we have AC3 preference and we are not forcing MP2 Stream
		// or we are forcing an AC3 Stream
		if ((m_StreamAC3 && !m_StreamAAC && !m_StreamDTS) || (m_bAC3Mode && !m_StreamMP2 && !m_StreamAAC && !m_StreamDTS)){
			// Update AC3 Pin
			if (FAILED(CheckAC3Pin(pDemux))){
				// If no AC3 Pin found
				if (FAILED(CheckAudioPin(pDemux))){
					// If no MP1/2 Pin found
					if (FAILED(NewAC3Pin(muxInterface, L"Audio"))){
						// If unable to create AC3 pin
						NewAudioPin(muxInterface, L"Audio");
					}
				}
				else{
					// If we do have an mp1/2 audio pin
					USHORT pPid;
					pPid = get_MP2AudioPid();
					if (!pPid || m_StreamAC3){
						// If we don't have a mp1/2 audio pid or we are forcing an AC3 Stream
						pPid = get_AC3_AudioPid();
						if (pPid && FAILED(CheckAC3Pin(pDemux))){
							// change pin type if we do have a AC3 pid & can don't already have an AC3 pin
							LPWSTR PinName = L"Audio";
							BOOL connect = FALSE;
							ChangeDemuxPin(pDemux, &PinName, &connect);
							if (FAILED(CheckAC3Pin(pDemux))){
								// if we can't change the pin type to AC3 make a new pin
								muxInterface->DeleteOutputPin(PinName);
								NewAC3Pin(muxInterface, PinName);
							}
							if (connect){
								// If old pin was already connected
								IPin* pIPin;
								if (SUCCEEDED(pDemux->FindPin(PinName, &pIPin))){
									// Reconnect pin
									RenderFilterPin(pIPin);
									pIPin->Release();
								}
							}
						}
					}
				}
			}
			else{
				// If we already have a AC3 Pin
				USHORT pPid;
				pPid = get_AC3_AudioPid();
				if (!pPid){
					// If we don't have a AC3 Pid
					pPid = get_MP2AudioPid();
					if (pPid && FAILED(CheckAudioPin(pDemux))){
						// change pin type if we do have a mp1/2 pid & can don't already have an mp1/2 pin
						LPWSTR PinName = L"Audio";
						BOOL connect = FALSE;
						ChangeDemuxPin(pDemux, &PinName, &connect);
						if (FAILED(CheckAudioPin(pDemux))){
							// if we can't change the pin type to mp2 make a new pin
							muxInterface->DeleteOutputPin(PinName);
							NewAudioPin(muxInterface, PinName);
						}
						if (connect){
							// If old pin was already connected
							IPin* pIPin;
							if (SUCCEEDED(pDemux->FindPin(PinName, &pIPin))){
								// Reconnect pin
								RenderFilterPin(pIPin);
								pIPin->Release();
							}
						}
					}
				}
			}

		} //If AC3 is not prefered
		else if (FAILED(CheckAudioPin(pDemux))){
			// If no mp1/2 audio Pin found
			if (FAILED(CheckAC3Pin(pDemux))){
				// If no AC3 audio Pin found
				if (!get_MP2AudioPid() || FAILED(NewAudioPin(muxInterface, L"Audio"))){
					// If unable to create mp1/2 pin
					hr = NewAC3Pin(muxInterface, L"Audio");
				}
			}
			else{
				// If we already have a AC3 Pin
				USHORT pPid;
				pPid = get_AC3_AudioPid();
				if (!pPid || m_StreamMP2){
					// If we don't have a AC3 Pid or we are forcing an MP2 Stream
					pPid = get_MP2AudioPid();
					if (pPid && FAILED(CheckAudioPin(pDemux))){
						// change pin type if we do have a mp1/2 pid & can don't already have an mp1/2 pin
						LPWSTR PinName = L"Audio";
						BOOL connect = FALSE;
						ChangeDemuxPin(pDemux, &PinName, &connect);
						if (FAILED(CheckAudioPin(pDemux))){
							// if we can't change the pin type to mp2 make a new pin
							muxInterface->DeleteOutputPin(PinName);
							NewAudioPin(muxInterface, PinName);
						}
						if (connect){
							// If old pin was already connected
							IPin* pIPin;
							if (SUCCEEDED(pDemux->FindPin(PinName, &pIPin))){
								// Reconnect pin
								RenderFilterPin(pIPin);
								pIPin->Release();
							}
						}
					}
				}
			}
		}
		else{
				// If we do have an mp1/2 audio pin
				USHORT pPid;
				pPid = get_MP2AudioPid();
				if (!pPid){
					// If we don't have a mp1/2 Pid
					pPid = get_AC3_AudioPid();
					if (pPid && FAILED(CheckAC3Pin(pDemux))){
						// change pin type if we do have a AC3 pid & can don't already have an AC3 pin
						LPWSTR PinName = L"Audio";
						BOOL connect = FALSE;
						ChangeDemuxPin(pDemux, &PinName, &connect);
						if (FAILED(CheckAC3Pin(pDemux))){
							// if we can't change the pin type to AC3 make a new pin
							muxInterface->DeleteOutputPin(PinName);
							NewAudioPin(muxInterface, PinName);
						}
						if (connect){
							// If old pin was already connected
							IPin* pIPin;
							if (SUCCEEDED(pDemux->FindPin(PinName, &pIPin))){
								// Reconnect pin
								RenderFilterPin(pIPin);
								pIPin->Release();
							}
						}
					}
				}
		}
		// Update Transport Stream Pin
		if (FAILED(CheckTsPin(pDemux))){
			// If no Transport Stream Pin was found
			if (m_bCreateTSPinOnDemux)
			{
				//If we have the option set
				hr = NewTsPin(muxInterface, L"TS");
			}
		}

		// Update Teletext Pin
		if (FAILED(CheckTelexPin(pDemux))){
			// If no Teletext Pin was found
			if (m_bCreateTxtPinOnDemux){
				//If we have the option set
				hr = NewTelexPin(muxInterface, L"VTeletext");
			}
		}

		// Update Teletext Pin
		if (FAILED(CheckSubtitlePin(pDemux))){
			// If no Teletext Pin was found
			if (m_bCreateSubPinOnDemux){
				//If we have the option set
				hr = NewSubtitlePin(muxInterface, L"VSubtitle");
			}
		}

		muxInterface->Release();
	}
	return hr;
}

HRESULT DWDemux::CheckDemuxPin(IBaseFilter* pDemux, AM_MEDIA_TYPE pintype, IPin** pIPin)
{
	CAutoLock Demuxlock(&m_DemuxLock);
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL && *pIPin != NULL)
		return hr;

	IPin* pDPin;
	PIN_DIRECTION  direction;
	AM_MEDIA_TYPE *type;

	// Enumerate the Demux pins
	IEnumPins* pIEnumPins;
	if (SUCCEEDED(pDemux->EnumPins(&pIEnumPins))){

		ULONG pinfetch(0);
		while(pIEnumPins->Next(1, &pDPin, &pinfetch) == S_OK){

			hr = pDPin->QueryDirection(&direction);
			if(direction == PINDIR_OUTPUT){

				IEnumMediaTypes* ppEnum;
				if (SUCCEEDED(pDPin->EnumMediaTypes(&ppEnum))){

					ULONG fetched(0);
					while(ppEnum->Next(1, &type, &fetched) == S_OK)
					{

						if (type->majortype == pintype.majortype
							&& type->subtype == pintype.subtype
							&& type->formattype == pintype.formattype){

							*pIPin = pDPin;
							ppEnum->Release();
							return S_OK;
						}
						type = NULL;
					}
					ppEnum->Release();
					ppEnum = NULL;
				}
			}
			pDPin->Release();
			pDPin = NULL;
		}
		pIEnumPins->Release();
   }
	return E_FAIL;
}
/*
HRESULT DWDemux::UpdateNetworkProvider(IBaseFilter* pNetworkProvider)
{
	HRESULT hr = E_INVALIDARG;

	if(pNetworkProvider == NULL)
		return hr;

	// Check if Enabled
	if (!m_bNPControl && !m_bNPSlave)
	{
		return S_FALSE;
	}

	ITuner* pITuner;
    hr = pNetworkProvider->QueryInterface(__uuidof (ITuner), reinterpret_cast <void**> (&pITuner));
    if(SUCCEEDED (hr))
    {
		//Setup to get the tune request
		CComPtr <ITuneRequest> pNewTuneRequest;			
		CComQIPtr <IDVBTuneRequest> pDVBTTuneRequest (pNewTuneRequest);
		hr = pITuner->get_TuneRequest(&pNewTuneRequest);
		pDVBTTuneRequest = pNewTuneRequest;

		if (pDVBTTuneRequest != NULL)
		{
			//Test if we are in NP Slave mode
			if (m_bNPSlave && !m_bNPControl )
			{
				long Onid = 0;
				pDVBTTuneRequest->get_ONID(&Onid);
				long Tsid = 0;
				pDVBTTuneRequest->get_TSID(&Tsid);
	
//				if (Onid != m_pNetwork->originalNetworkId && Tsid != m_pNetwork->transportStreamId)
//					m_pNetwork->UpdateNetwork();

				long Sid = 0;
				pDVBTTuneRequest->get_SID(&Sid); //Get the SID from the NP

//				m_pNetwork->m_servicesm_ProgramSID = Sid; //Set the prefered SID
//				m_pNetwork->set_ProgramSID(); //Update the Pids
			}

			if (m_bNPControl)
			{
				//Must be in control mode to get this far then setup the NP tune request
				if (m_pNetwork->originalNetworkId == 0)
					pDVBTTuneRequest->put_ONID((long)-1);
				else
					pDVBTTuneRequest->put_ONID((long)m_pNetwork->originalNetworkId);

				if (m_pService->serviceId == 0)
					pDVBTTuneRequest->put_SID((long)-1);
				else
					pDVBTTuneRequest->put_SID((long)m_pService->serviceId);
			
				if (m_pNetwork->transportStreamId == 0)
					pDVBTTuneRequest->put_TSID((long)-1);
				else
					pDVBTTuneRequest->put_TSID((long)m_pNetwork->transportStreamId);

				//If the tune request is valid then tune.
				if (pITuner->Validate(pNewTuneRequest) == S_OK)
				{
					hr = pITuner->put_TuneRequest(pNewTuneRequest);
					Sleep(500);
				}
			}
			pDVBTTuneRequest.Release();
			pNewTuneRequest.Release();
		}
		else
		{
			pNewTuneRequest.Release();
			pNewTuneRequest = NULL;
			CComQIPtr <IATSCChannelTuneRequest> pDVBTTuneRequest (pNewTuneRequest);
			hr = pITuner->get_TuneRequest(&pNewTuneRequest);
			pDVBTTuneRequest = pNewTuneRequest;

			if (pDVBTTuneRequest != NULL)
			{
				//Test if we are in NP Slave mode
				if (m_bNPSlave && !m_bNPControl )
				{
					long Sid = 0;
					pDVBTTuneRequest->get_MinorChannel(&Sid); //Get the SID from the NP

//					m_pNetwork->m_ProgramSID = Sid; //Set the prefered SID
//					m_pNetwork->set_ProgramSID(); //Update the Pids
				}

				if (m_bNPControl)
				{
					pDVBTTuneRequest->put_Channel((long)-1);
					
					//Must be in control mode to get this far then setup the NP tune request
					if (m_pService->serviceId == 0)
						pDVBTTuneRequest->put_MinorChannel((long)-1);
					else
						pDVBTTuneRequest->put_MinorChannel((long)m_pService->serviceId);
			
					//If the tune request is valid then tune.
					if (pITuner->Validate(pNewTuneRequest) == S_OK)
					{
						hr = pITuner->put_TuneRequest(pNewTuneRequest);
						Sleep(500);
					}
				}
				pDVBTTuneRequest.Release();
				pNewTuneRequest.Release();
			}
		}
		pITuner->Release();
	}
	return hr;
}
*/
//Stop TIF Additions
HRESULT DWDemux::SetTIFState(IFilterGraph *pGraph, REFERENCE_TIME tStart)
{
	HRESULT hr = S_FALSE;

	// declare local variables
	IEnumFilters* EnumFilters;

	// Parse only the existing TIF Filter
	// in the filter graph, we do this by looking for filters
	// that implement the GuideData interface while
	// the count is still active.
	if(SUCCEEDED(pGraph->EnumFilters(&EnumFilters)))
	{
		IBaseFilter* m_pTIF;
		ULONG Fetched(0);
		while(EnumFilters->Next(1, &m_pTIF, &Fetched) == S_OK)
		{
			if(m_pTIF != NULL)
			{
				// Get the GuideData interface from the TIF filter
				IGuideData* pGuideData;
				hr = m_pTIF->QueryInterface(&pGuideData);
				if (SUCCEEDED(hr))
				{
					if (tStart == 0)
					{
						m_pTIF->Pause();
						m_pTIF->Stop();
					}
					else
						m_pTIF->Run(tStart);

					pGuideData->Release();
				}
	
				m_pTIF->Release();
				m_pTIF = NULL;
			}
		}
		EnumFilters->Release();
	}
	return hr;
}

//TIF Additions
HRESULT DWDemux::CheckTIFPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetTIFMedia(&pintype);

	IPin* pOPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pOPin)))
	{

		IPin* pIPin = NULL;
		if (SUCCEEDED(pOPin->ConnectedTo(&pIPin)))
		{

			PIN_INFO pinInfo;
			if (SUCCEEDED(pIPin->QueryPinInfo(&pinInfo)))
			{
		
				IBaseFilter* pTIF;
				pTIF = pinInfo.pFilter;

				// Get the GuideData interface from the TIF filter
				IGuideData* pGuideData;
				hr = pTIF->QueryInterface(&pGuideData);
				if (SUCCEEDED(hr))
				{
					// Get the TuneRequestinfo interface from the TIF filter
					CComPtr <ITuneRequestInfo> pTuneRequestInfo;
					hr = pTIF->QueryInterface(&pTuneRequestInfo);
					if (SUCCEEDED(hr))
					{

						// Get a list of services from the GuideData interface
						IEnumTuneRequests*  piEnumTuneRequests; 
						hr = pGuideData->GetServices(&piEnumTuneRequests); 
						if (SUCCEEDED(hr))
						{
							bool foundSID = false;
							while (!foundSID)
							{

							unsigned long ulRetrieved = 1;
							ITuneRequest *pTuneRequest = NULL; 

							while (SUCCEEDED(piEnumTuneRequests->Next(1, &pTuneRequest, &ulRetrieved)) && ulRetrieved > 0)
							{

								// Fill in the Components lists for the tune request
								hr = pTuneRequestInfo->CreateComponentList(pTuneRequest);
								if(SUCCEEDED(hr))
								{

									CComPtr <IComponents> pConponents;
									hr = pTuneRequest->get_Components(&pConponents);
									if(SUCCEEDED(hr))
									{

									    CComPtr<IEnumComponents> pEnum;
										hr = pConponents->EnumComponents(&pEnum);

										if (SUCCEEDED(hr))
										{
											CComPtr <IComponent> pComponent;
											ULONG cFetched = 1;

											while (SUCCEEDED(pEnum->Next(1, &pComponent, &cFetched)) && cFetched > 0)
											{

												CComPtr <IMPEG2Component> mpegComponent;
												hr = pComponent.QueryInterface(&mpegComponent);

												long progSID = -1;
												mpegComponent->get_ProgramNumber(&progSID);
												if (progSID == m_pService->serviceId)
													foundSID = true;
												mpegComponent.Release();
											}
											pComponent.Release();
										}
										pEnum.Release();
										pConponents.Release();
									}
									pTuneRequest->Release();
								}
							}
							piEnumTuneRequests->Release();
							}
						}
						pTuneRequestInfo.Release();
					}
					pGuideData->Release();
				}
				pTIF->Release();
			}
			pIPin->Release();
		}
		pOPin->Release();
		return S_OK;
	}
	return hr;
}

HRESULT DWDemux::CheckVideoPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	IPin* pIPin = NULL;

//	USHORT pPid;
//	pPid = get_VideoPid();
//	if (pPid)
//	{
		graphTools.GetVideoMedia(&pintype);
		if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

			USHORT pPid = get_VideoPid();
			if (pPid && SUCCEEDED(LoadVideoPin(pIPin, pPid))){
				pIPin->Release();
				return S_OK;
			}
			pIPin->Release();
			return S_FALSE;
		}
//	}

//	pPid = get_H264Pid();
//	if (pPid)
//	{
		graphTools.GetH264Media(&pintype);
		if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

			USHORT pPid = get_H264Pid();
			if (pPid && SUCCEEDED(LoadVideoPin(pIPin, pPid))){
				pIPin->Release();
				return S_OK;
			}
			pIPin->Release();
			return S_FALSE;
		}
//	}

//	pPid = get_Mpeg4Pid();
//	if (pPid)
//	{
		graphTools.GetMpeg4Media(&pintype);
		if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

			USHORT pPid = get_Mpeg4Pid();
			if (pPid && SUCCEEDED(LoadVideoPin(pIPin, pPid))){
				pIPin->Release();
				return S_OK;
			}
			pIPin->Release();
			return S_FALSE;
		}
//	}

	return E_FAIL;
}

HRESULT DWDemux::CheckAudioPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetMP1Media(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_MP2AudioPid();
		if (SUCCEEDED(LoadAudioPin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		};
	}
	else
	{
		graphTools.GetMP2Media(&pintype);
		if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

			USHORT pPid;
			pPid = get_MP2AudioPid();
			if (SUCCEEDED(LoadAudioPin(pIPin, pPid))){
				pIPin->Release();
				return S_OK;
			};
		};
	};
	return hr;
}

HRESULT DWDemux::CheckAACPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetAACMedia(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_AAC_AudioPid();
		if (SUCCEEDED(LoadAudioPin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		};
	}
	return hr;
}

HRESULT DWDemux::CheckDTSPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetDTSMedia(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_DTS_AudioPid();
		if (SUCCEEDED(LoadAudioPin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		};
	}
	return hr;
}

HRESULT DWDemux::CheckAC3Pin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetAC3Media(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_AC3_AudioPid();
		if (SUCCEEDED(LoadAudioPin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::CheckTelexPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetTelexMedia(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_TeletextPid();
		if (SUCCEEDED(LoadTelexPin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::CheckSubtitlePin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetSubtitleMedia(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		USHORT pPid;
		pPid = get_SubtitlePid();
		if (SUCCEEDED(LoadSubtitlePin(pIPin, pPid))){
			pIPin->Release();
			return S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::CheckTsPin(IBaseFilter* pDemux)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL)
		return hr;

	AM_MEDIA_TYPE pintype;
	graphTools.GetTSMedia(&pintype);

	IPin* pIPin = NULL;
	if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

		if (SUCCEEDED(LoadTsPin(pIPin))){
			ReconnectFilterPin(pIPin);
			pIPin->Release();
			return S_OK;
		}
	}
	return hr;
}
/*
HRESULT DWDemux::NewTsPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL)
		return hr;

	// Create out new pin  
	AM_MEDIA_TYPE type;
	graphTools.GetTSMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
//			hr = muxInterface->SetOutputPinMediaType(pinName, &type);
//			if FAILED(hr)
//			{
//				pIPin->Release();
//				return hr;
//			}
		}

		hr = LoadTsPin(pIPin);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewVideoPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	AM_MEDIA_TYPE type;
	ZeroMemory(&type, sizeof(AM_MEDIA_TYPE));

	if (get_Mpeg4Pid())
	{
		graphTools.GetMpeg4Media(&type);
		pPid = get_Mpeg4Pid();
	}
	else if (get_H264Pid())
	{
		graphTools.GetH264Media(&type);
		pPid = get_H264Pid();
	}
	else
	{
		graphTools.GetVideoMedia(&type);
		pPid = get_VideoPid();
	}

	HRESULT hr = E_INVALIDARG;
	//Test if no interface 
	if(muxInterface == NULL) //|| !pPid)
		return hr;

	// Create out new pin 
	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
//			hr = muxInterface->SetOutputPinMediaType(pinName, &type);
//			if FAILED(hr)
//			{
//				pIPin->Release();
//				return hr;
//			}
		}

		hr = LoadVideoPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAudioPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_MP2AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL)
		return hr;

	if(pPid == 0 && get_PcrPid())
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	ZeroMemory(&type, sizeof(AM_MEDIA_TYPE));
	if (m_bMPEG2AudioMediaType)
		graphTools.GetMP2Media(&type);
	else
		graphTools.GetMP1Media(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
//			hr = muxInterface->SetOutputPinMediaType(pinName, &type);
//			if FAILED(hr)
//			{
//				pIPin->Release();
//				return hr;
//			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAC3Pin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_AC3_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetAC3Media(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
//			hr = muxInterface->SetOutputPinMediaType(pinName, &type);
//			if FAILED(hr)
//			{
//				pIPin->Release();
//				return hr;
//			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAACPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_AAC_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetAACMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
//			hr = muxInterface->SetOutputPinMediaType(pinName, &type);
//			if FAILED(hr)
//			{
//				pIPin->Release();
//				return hr;
//			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewDTSPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_DTS_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetDTSMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewTelexPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_TeletextPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetTelexMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
		}

		hr = LoadTelexPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewSubtitlePin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_SubtitlePid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetSubtitleMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);
				pinInfo.pFilter->Release();
				pInpPin->Release();

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
						pInpPin->Release();
					}
							
					if (connect == TRUE){

						pInpPin->Disconnect();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (connect)
						RenderFilterPin(pIPin);

				}
			}
		}

		hr = LoadSubtitlePin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}
*/
HRESULT DWDemux::NewTsPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL)
		return hr;

	// Create out new pin  
	AM_MEDIA_TYPE type;
	graphTools.GetTSMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2) pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();
						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadTsPin(pIPin);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewVideoPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	AM_MEDIA_TYPE type;
	ZeroMemory(&type, sizeof(AM_MEDIA_TYPE));

	if (get_Mpeg4Pid())
	{
		graphTools.GetMpeg4Media(&type);
		pPid = get_Mpeg4Pid();
	}
	else if (get_H264Pid())
	{
		graphTools.GetH264Media(&type);
		pPid = get_H264Pid();
	}
	else
	{
		graphTools.GetVideoMedia(&type);
		pPid = get_VideoPid();
	}

	HRESULT hr = E_INVALIDARG;
	//Test if no interface 
	if(muxInterface == NULL) //|| !pPid)
		return hr;

	// Create out new pin 
	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadVideoPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAudioPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_MP2AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL)
		return hr;

	if(pPid == 0 && get_PcrPid())
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	ZeroMemory(&type, sizeof(AM_MEDIA_TYPE));
	if (m_bMPEG2AudioMediaType)
		graphTools.GetMP2Media(&type);
	else
		graphTools.GetMP1Media(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;

			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAC3Pin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_AC3_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetAC3Media(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}


		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewAACPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_AAC_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetAACMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewDTSPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_DTS_AudioPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetDTSMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadAudioPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewTelexPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_TeletextPid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetTelexMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadTelexPin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::NewSubtitlePin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName)
{
	USHORT pPid;
	pPid = get_SubtitlePid();

	HRESULT hr = E_INVALIDARG;

	if(muxInterface == NULL || pPid == 0)
		return hr;

	// Create out new pin 
	AM_MEDIA_TYPE type;
	graphTools.GetSubtitleMedia(&type);

	IPin* pIPin = NULL;
	hr = muxInterface->CreateOutputPin(&type, pinName ,&pIPin);
	if(SUCCEEDED(hr) || hr == VFW_E_DUPLICATE_NAME)
	{
		//the following should not happen if it is a MS Demux
		if(pIPin == NULL)
		{
			IBaseFilter *pIBaseFilter = NULL;
			hr = muxInterface->QueryInterface(IID_IBaseFilter, (void **)&pIBaseFilter);
			if (FAILED(hr) || pIBaseFilter == NULL)
				return hr;

			pIBaseFilter->FindPin(pinName ,&pIPin);
			pIBaseFilter->Release();
			if(pIPin == NULL)
				return hr;
			
			IPin* pInpPin = NULL;
			if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

				IPin* pInpPin2 = NULL;
				PIN_INFO pinInfo;
				pInpPin->QueryPinInfo(&pinInfo);

				if (m_WasPlaying) {
					
					if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
				}

				if (SUCCEEDED(pIPin->Disconnect())){

					BOOL connect = FALSE;

					muxInterface->SetOutputPinMediaType(pinName, &type);
					if (FAILED(pInpPin->QueryAccept(&type)))
						connect = TRUE;
					else if (FAILED(ReconnectFilterPin(pInpPin)))
						connect = TRUE;
					else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
						connect = TRUE;
					else {

						pInpPin->BeginFlush();
						pInpPin->EndFlush();
					}
							
					if (pInpPin2)
						pInpPin2->Release();

					if (connect == TRUE){

						pInpPin->Disconnect();
						if (pInpPin)
							pInpPin->Release();

						RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
					}

					if (pInpPin)
						pInpPin->Release();

					if (connect)
						RenderFilterPin(pIPin);
					else
						pinInfo.pFilter->Release();
				}
				else
				{
					pinInfo.pFilter->Release();
					pIPin->Release();
					return hr;
				}
			}
			else
			{
				hr = muxInterface->SetOutputPinMediaType(pinName, &type);
				if FAILED(hr)
				{
					pIPin->Release();
					return hr;
				}
			}
		}

		hr = LoadSubtitlePin(pIPin, (ULONG)pPid);
		pIPin->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::LoadTsPin(IPin* pIPin)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	ClearDemuxPin(pIPin);


	// Get the Pid Map interface of the pin
	// and map the pids we want.
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
	{
		long count = m_pService->GetStreamCount();
		for ( long currentStream=0 ; currentStream<count ; currentStream++ )
		{
			ULONG Pid = m_pService->GetStreamPID(currentStream);
			if FAILED(hr = muxMapPid->MapPID(1, &Pid, MEDIA_TRANSPORT_PACKET))
			{
//				(log << "Failed to map demux " << pPinName << " pin : " << hr << "\n").Write();
				continue;	//it's safe to not piPidMap.Release() because it'll go out of scope
			}
//			renderedStreams++;
		}

		ULONG Pidarray[6] = {0x00, 0x10, 0x11, 0x12, 0x13, 0x14};
		if FAILED(hr = muxMapPid->MapPID(6, &Pidarray[0], MEDIA_TRANSPORT_PACKET))
		{
//			(log << "Failed to map demux " << pPinName << " pin Fixed Pids : " << hr << "\n").Write();
		}
//		muxMapPid->MapPID(m_pNetwork->pids.TsArray[0] + 1, &m_pNetwork->pids.TsArray[1], MEDIA_TRANSPORT_PACKET);
		muxMapPid->Release();
		hr = S_OK;
	}
	return hr;
}

HRESULT DWDemux::LoadVideoPin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	VetDemuxPin(pIPin, pid);

	// Get the Pid Map interface of the pin
	// and map the pids we want.
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
	{
		if (pid)
		{
			muxMapPid->MapPID(1, &pid , MEDIA_ELEMENTARY_STREAM);
			m_SelVideoPid = pid;
		}

		muxMapPid->Release();
		hr = S_OK;
	}
	else {

		IMPEG2StreamIdMap* muxMapPid;
		if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
		{
			if (pid)
			{
				muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0);
				m_SelVideoPid = pid;
			}

			muxMapPid->Release();
			hr = S_OK;
		}
	}

	return hr;
}

HRESULT DWDemux::LoadAudioPin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	VetDemuxPin(pIPin, pid);

	// Get the Pid Map interface of the pin
	// and map the pids we want.
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
	{
		if (pid)
		{
			muxMapPid->MapPID(1, &pid , MEDIA_ELEMENTARY_STREAM);
			m_SelAudioPid = pid;
		}

		muxMapPid->Release();
		hr = S_OK;
	}
	else {

		IMPEG2StreamIdMap* muxMapPid;
		if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
		{
			if (pid)
			{
				if (pid == get_MP2AudioPid())
					muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0x00, 0x00);

				if (pid == get_AC3_AudioPid())
					muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0x80, 0x04);

				if (pid == get_AAC_AudioPid())
					muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0x00, 0x00);

				if (pid == get_DTS_AudioPid())
					muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0x00, 0x00);

				m_SelAudioPid = pid;
			}

			muxMapPid->Release();
			hr = S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::LoadTelexPin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	VetDemuxPin(pIPin, pid);

	// Get the Pid Map interface of the pin
	// and map the pids we want.
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
	{
		if (pid){
			muxMapPid->MapPID(1, &pid , MEDIA_TRANSPORT_PACKET); 
			m_SelTelexPid = pid;
		}
		muxMapPid->Release();
		hr = S_OK;
	}
	else {

		IMPEG2StreamIdMap* muxMapPid;
		if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
		{
			if (pid)
			{
				muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0);
				m_SelTelexPid = pid;
			}
			muxMapPid->Release();
			hr = S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::LoadSubtitlePin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	VetDemuxPin(pIPin, pid);

	// Get the Pid Map interface of the pin
	// and map the pids we want.
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
	{
		if (pid){
			muxMapPid->MapPID(1, &pid , MEDIA_ELEMENTARY_STREAM); 
			m_SelTelexPid = pid;
		}
		muxMapPid->Release();
		hr = S_OK;
	}
	else {

		IMPEG2StreamIdMap* muxMapPid;
		if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid)))
		{
			if (pid)
			{
				muxMapPid->MapStreamId(pid, MPEG2_PROGRAM_ELEMENTARY_STREAM, 0, 0);
				m_SelSubtitlePid = pid;
			}
			muxMapPid->Release();
			hr = S_OK;
		}
	}
	return hr;
}

HRESULT DWDemux::VetDemuxPin(IPin* pIPin, ULONG pid)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	ULONG pids = 0;
	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid))){

		IEnumPIDMap *pIEnumPIDMap;
		if (SUCCEEDED(muxMapPid->EnumPIDMap(&pIEnumPIDMap))){
			ULONG pNumb = 0;
			PID_MAP pPidMap;
			while(pIEnumPIDMap->Next(1, &pPidMap, &pNumb) == S_OK){
				if (pid != pPidMap.ulPID)
				{
					pids = pPidMap.ulPID;
					hr = muxMapPid->UnmapPID(1, &pids);
				}
			}
		}
		muxMapPid->Release();
	}
	else {

		IMPEG2StreamIdMap* muxStreamMap;
		if(SUCCEEDED(pIPin->QueryInterface (&muxStreamMap))){

			IEnumStreamIdMap *pIEnumStreamMap;
			if (SUCCEEDED(muxStreamMap->EnumStreamIdMap(&pIEnumStreamMap))){
				ULONG pNumb = 0;
				STREAM_ID_MAP pStreamIdMap;
				while(pIEnumStreamMap->Next(1, &pStreamIdMap, &pNumb) == S_OK){
					if (pid != pStreamIdMap.stream_id)
					{
						pids = pStreamIdMap.stream_id;
						hr = muxStreamMap->UnmapStreamId(1, &pids);
					}
				}
			}
			muxStreamMap->Release();
		}
	}
	return S_OK;
}

HRESULT DWDemux::ClearDemuxPin(IPin* pIPin)
{
	HRESULT hr = E_INVALIDARG;

	if(pIPin == NULL)
		return hr;

	IMPEG2PIDMap* muxMapPid;
	if(SUCCEEDED(pIPin->QueryInterface (&muxMapPid))){

		IEnumPIDMap *pIEnumPIDMap;
		if (SUCCEEDED(muxMapPid->EnumPIDMap(&pIEnumPIDMap))){
			ULONG pNumb = 0;
			PID_MAP pPidMap;
			while(pIEnumPIDMap->Next(1, &pPidMap, &pNumb) == S_OK){
				ULONG pid = pPidMap.ulPID;
				hr = muxMapPid->UnmapPID(1, &pid);
			}
		}
		muxMapPid->Release();
	}
	else {

		IMPEG2StreamIdMap* muxStreamMap;
		if(SUCCEEDED(pIPin->QueryInterface (&muxStreamMap))){

			IEnumStreamIdMap *pIEnumStreamMap;
			if (SUCCEEDED(muxStreamMap->EnumStreamIdMap(&pIEnumStreamMap))){
				ULONG pNumb = 0;
				STREAM_ID_MAP pStreamIdMap;
				while(pIEnumStreamMap->Next(1, &pStreamIdMap, &pNumb) == S_OK){
					ULONG pid = pStreamIdMap.stream_id;
					hr = muxStreamMap->UnmapStreamId(1, &pid);
				}
			}
			muxStreamMap->Release();
		}
	}
	return S_OK;
}

HRESULT DWDemux::CheckDemuxPids(void)
{
	HRESULT hr = S_OK;

	if ((m_SelTelexPid != get_TeletextPid()) && m_bCreateTxtPinOnDemux)
		return S_FALSE;

	if ((m_SelSubtitlePid != get_SubtitlePid()) && m_bCreateSubPinOnDemux)
		return S_FALSE;

	if (get_AudioPid() | get_AC3Pid() | get_AACPid() | get_DTSPid())
		if (((m_SelAudioPid != get_AudioPid())
				&& (m_SelAudioPid != get_Audio2Pid())
				&& (m_SelAudioPid != get_AC3Pid())
				&& (m_SelAudioPid != get_AC3_2Pid())
				&& (m_SelAudioPid != get_AACPid())
				&& (m_SelAudioPid != get_AAC2Pid())
				&& (m_SelAudioPid != get_DTSPid())
				&& (m_SelAudioPid != get_DTS2Pid()))
				|| !m_SelAudioPid)
			return S_FALSE;

	if (get_VideoPid() | get_H264Pid() | get_Mpeg4Pid())
		if (((m_SelVideoPid != get_VideoPid())
				&& (m_SelVideoPid != get_H264Pid())
				&& (m_SelVideoPid != get_Mpeg4Pid()))
				|| !m_SelVideoPid)
			return S_FALSE;

	return hr;

}
/*
HRESULT DWDemux::ChangeDemuxPin(IBaseFilter* pDemux, LPWSTR* pPinName, BOOL* pConnect)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL || pConnect == NULL || pPinName == NULL)
		return hr;

	// Get an instance of the Demux control interface
	IMpeg2Demultiplexer* muxInterface = NULL;
	if(SUCCEEDED(pDemux->QueryInterface (&muxInterface)))
	{
		char audiocheck[128] ="";
		char videocheck[128] ="";
		char telexcheck[128] ="";
		char tscheck[128] ="";
		char pinname[128] ="";
		wcscpy((wchar_t*)pinname, *pPinName);
		wcscpy((wchar_t*)audiocheck, L"Audio");
		wcscpy((wchar_t*)videocheck, L"Video");
		wcscpy((wchar_t*)telexcheck, L"Teletext");
		wcscpy((wchar_t*)tscheck, L"TS");

		if (strcmp(audiocheck, pinname) == 0){

			AM_MEDIA_TYPE pintype;
			IPin* pIPin = NULL;

			graphTools.GetAC3Media(&pintype);
			if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);

				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);
					pinInfo.pFilter->Release();
					pInpPin->Release();

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						*pConnect = FALSE;
						if (m_bMPEG2AudioMediaType)
							graphTools.GetMP2Media(&pintype);
						else
							graphTools.GetMP1Media(&pintype);

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
							pInpPin->Release();
						}
								
						if (*pConnect == TRUE){

							pInpPin->Disconnect();
							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}

//						if (pinInfo.pFilter) pinInfo.pFilter->Release();
//						if (pInpPin) pInpPin->Release();
//						if (pInpPin2) pInpPin2->Release();
					}
					muxInterface->Release();
					if (pIPin) pIPin->Release();
					return S_FALSE;
				}
				else {

					if (m_bMPEG2AudioMediaType)
						graphTools.GetMP2Media(&pintype);
					else
						graphTools.GetMP1Media(&pintype);

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				USHORT pPid;
				pPid = get_MP2AudioPid();
				LoadAudioPin(pIPin, pPid);
				pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
			else {

				graphTools.GetMP2Media(&pintype);
				if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

					pIPin->QueryId(pPinName);
					IPin* pInpPin;

					if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

						IPin* pInpPin2 = NULL;
						PIN_INFO pinInfo;
						pInpPin->QueryPinInfo(&pinInfo);
						pinInfo.pFilter->Release();
						pInpPin->Release();

						if (m_WasPlaying) {
					
							if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
							if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						}

						if (SUCCEEDED(pIPin->Disconnect())){

							*pConnect = FALSE;
							graphTools.GetAC3Media(&pintype);
							muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
							if (FAILED(pInpPin->QueryAccept(&pintype)))
								*pConnect = TRUE;
							else if (FAILED(ReconnectFilterPin(pInpPin)))
								*pConnect = TRUE;
							else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
								*pConnect = TRUE;
							else {

								pInpPin->BeginFlush();
								pInpPin->EndFlush();
								pInpPin->Release();
							}
							
							if (*pConnect == TRUE){

								pInpPin->Disconnect();
								RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
							}

//							if (pinInfo.pFilter) pinInfo.pFilter->Release();
//							if (pInpPin) pInpPin->Release();
//							if (pInpPin2) pInpPin2->Release();
						}
						muxInterface->Release();
						if (pIPin) pIPin->Release();
						return S_FALSE;
					}
					else {

						graphTools.GetAC3Media(&pintype);
						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
					}

					USHORT pPid;
					pPid = get_AC3_AudioPid();
					LoadAudioPin(pIPin, pPid);
					pIPin->Release();
					muxInterface->Release();
					return S_OK;
				}  
				else {

					graphTools.GetMP1Media(&pintype);
					if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){
						ClearDemuxPin(pIPin);
						pIPin->QueryId(pPinName);

						IPin* pInpPin;
						if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

							IPin* pInpPin2 = NULL;
							*pConnect = TRUE;
							PIN_INFO pinInfo;
							pInpPin->QueryPinInfo(&pinInfo);
							pinInfo.pFilter->Release();
							pInpPin->Release();

							if (m_WasPlaying) {

								if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
								if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
							}

							if (SUCCEEDED(pIPin->Disconnect())){

								*pConnect = FALSE;
								graphTools.GetAC3Media(&pintype);
								muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
								if (FAILED(pInpPin->QueryAccept(&pintype)))
									*pConnect = TRUE;
								else if (FAILED(ReconnectFilterPin(pInpPin)))
									*pConnect = TRUE;
								else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
									*pConnect = TRUE;
								else {

									pInpPin->BeginFlush();
									pInpPin->EndFlush();
									pInpPin->Release();
								}
								
								if (*pConnect == TRUE){

									pInpPin->Disconnect();
									RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
								}

//								if (pinInfo.pFilter) pinInfo.pFilter->Release();
//								if (pInpPin) pInpPin->Release();
//								if (pInpPin2) pInpPin2->Release();
							}
							muxInterface->Release();
							if (pIPin) pIPin->Release();
							return S_FALSE;
						}
						else {

							graphTools.GetAC3Media(&pintype);
							muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						}

						USHORT pPid;
						pPid = get_AC3_AudioPid();
						LoadAudioPin(pIPin, pPid);
						if (pIPin) pIPin->Release();
						muxInterface->Release();
						return S_OK;
					} 
				} 
			} 
			muxInterface->Release();
			return hr;
		}
		else if (strcmp(videocheck, pinname) == 0) {

			IPin* pIPin = NULL;
			AM_MEDIA_TYPE pintype;

			//Check if we have a video pin type
			graphTools.GetVideoMedia(&pintype);
			if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				graphTools.GetH264Media(&pintype);
				if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

					graphTools.GetMpeg4Media(&pintype);
					if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

						muxInterface->DeleteOutputPin(*pPinName);
						muxInterface->Release();
						return S_FALSE;
					}
				}
			}

			//parse video type to set media type & pid value
			USHORT pPid;
			pPid = get_VideoPid();
			if (pPid)
				graphTools.GetVideoMedia(&pintype);
			else {

				pPid = get_H264Pid();
				if (pPid)
					graphTools.GetH264Media(&pintype);
				else {

					pPid = get_Mpeg4Pid();
					if (pPid)
						graphTools.GetMpeg4Media(&pintype);
					else
						graphTools.GetVideoMedia(&pintype);
				}
			}

			//Change the media type only if we have a video pin
			if (pIPin) {

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						*pConnect = FALSE;
						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
							pInpPin->Release();
						}
								
						if (*pConnect == TRUE){
							pInpPin->Disconnect();
							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}
					}

					if (pIPin) pIPin->Release();
					muxInterface->Release();
					pinInfo.pFilter->Release();
					return S_FALSE;
				}
				else
				{
					muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				LoadVideoPin(pIPin, pPid);
				if (pIPin) pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
		}
		else if (strcmp(telexcheck, pinname) == 0){

			AM_MEDIA_TYPE pintype;
			graphTools.GetTelexMedia(&pintype);

			IPin* pIPin = NULL;
			if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){
				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);

				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);
					pinInfo.pFilter->Release();
					pInpPin->Release();

					if (m_WasPlaying) {
						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){
						*pConnect = TRUE;
						RemoveFilterChain(pinInfo.pFilter, NULL);
					}
				}

				pIPin->Disconnect();
				muxInterface->DeleteOutputPin(*pPinName);
				pIPin->Release();
				hr = S_OK;
			}
		}
		else if (strcmp(tscheck, pinname) == 0){

			AM_MEDIA_TYPE pintype;
			graphTools.GetTSMedia(&pintype);

			IPin* pIPin = NULL;
			if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){
				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);

				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					*pConnect = TRUE;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);
					pinInfo.pFilter->Release();
					pInpPin->Release();

					if (m_WasPlaying) {
						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){
						*pConnect = TRUE;
						RemoveFilterChain(pinInfo.pFilter, NULL);
					}
				}
				pIPin->Disconnect();
				muxInterface->DeleteOutputPin(*pPinName);
				pIPin->Release();
				hr = S_OK;
			}
		}
		muxInterface->Release();
	}
		return hr;
}
*/
HRESULT DWDemux::ChangeDemuxPin(IBaseFilter* pDemux, LPWSTR* pPinName, BOOL* pConnect)
{
	HRESULT hr = E_INVALIDARG;

	if(pDemux == NULL || pConnect == NULL || pPinName == NULL)
		return hr;

	// Get an instance of the Demux control interface
	IMpeg2Demultiplexer* muxInterface = NULL;
	if(SUCCEEDED(pDemux->QueryInterface (&muxInterface)))
	{
		char audiocheck[128] ="";
		char videocheck[128] ="";
		char telexcheck[128] ="";
		char subtitlecheck[128] ="";
		char tscheck[128] ="";
		char pinname[128] ="";
		wcscpy((wchar_t*)pinname, *pPinName);
		wcscpy((wchar_t*)audiocheck, L"Audio");
		wcscpy((wchar_t*)videocheck, L"Video");
		wcscpy((wchar_t*)telexcheck, L"Teletext");
		wcscpy((wchar_t*)subtitlecheck, L"Subtitle");
		wcscpy((wchar_t*)tscheck, L"TS");

		if (strcmp(audiocheck, pinname) == 0){

			AM_MEDIA_TYPE pintype;
			IPin* pIPin = NULL;

			graphTools.GetAC3Media(&pintype);
			if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				*pConnect = FALSE;
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						if (m_bMPEG2AudioMediaType && get_MP2AudioPid())
							graphTools.GetMP2Media(&pintype);
						else if (get_MP2AudioPid())
							graphTools.GetMP1Media(&pintype);
						else if (get_AAC_AudioPid())
							graphTools.GetAACMedia(&pintype);
						else if (get_DTS_AudioPid())
							graphTools.GetDTSMedia(&pintype);
						else if (m_bMPEG2AudioMediaType)
							graphTools.GetMP2Media(&pintype);
						else
							graphTools.GetMP1Media(&pintype);

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
						}
								
						if (pInpPin2)
							pInpPin2->Release();

						if (*pConnect == TRUE){

							pInpPin->Disconnect();
							if (pInpPin)
								pInpPin->Release();

							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}

						if (pInpPin)
							pInpPin->Release();
					}

					if (!*pConnect)
						pinInfo.pFilter->Release();

					muxInterface->Release();
					if (pIPin) pIPin->Release();
					return S_FALSE;
				}
				else {

					if (m_bMPEG2AudioMediaType && get_MP2AudioPid())
						graphTools.GetMP2Media(&pintype);
					else if (get_MP2AudioPid())
						graphTools.GetMP1Media(&pintype);
					else if (get_AAC_AudioPid())
						graphTools.GetAACMedia(&pintype);
					else if (get_DTS_AudioPid())
						graphTools.GetDTSMedia(&pintype);
					else if (m_bMPEG2AudioMediaType)
						graphTools.GetMP2Media(&pintype);
					else
						graphTools.GetMP1Media(&pintype);

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				USHORT pPid;
				if (get_MP2AudioPid())
					pPid = get_MP2AudioPid();
				else if (get_AAC_AudioPid())
					pPid = get_AAC_AudioPid();
				else if (get_DTS_AudioPid())
					pPid = get_DTS_AudioPid();
				else
					pPid = get_MP2AudioPid();

				LoadAudioPin(pIPin, pPid);
				pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
			else {

				graphTools.GetMP2Media(&pintype);
				if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

					ClearDemuxPin(pIPin);
					pIPin->QueryId(pPinName);
					*pConnect = FALSE;
					IPin* pInpPin;

					if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

						IPin* pInpPin2 = NULL;
						PIN_INFO pinInfo;
						pInpPin->QueryPinInfo(&pinInfo);

						if (m_WasPlaying) {
					
							if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
							if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						}

						if (SUCCEEDED(pIPin->Disconnect())){

							if (get_AC3_AudioPid())
								graphTools.GetAC3Media(&pintype);
							else if (get_AAC_AudioPid())
								graphTools.GetAACMedia(&pintype);
							else if (get_DTS_AudioPid())
								graphTools.GetDTSMedia(&pintype);
							else
								graphTools.GetAC3Media(&pintype);

							muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
							if (FAILED(pInpPin->QueryAccept(&pintype)))
								*pConnect = TRUE;
							else if (FAILED(ReconnectFilterPin(pInpPin)))
								*pConnect = TRUE;
							else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
								*pConnect = TRUE;
							else {

								pInpPin->BeginFlush();
								pInpPin->EndFlush();
							}
							
							if (pInpPin2)
								pInpPin2->Release();

							if (*pConnect == TRUE){

								pInpPin->Disconnect();
								if (pInpPin)
									pInpPin->Release();

								RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
							}

							if (pInpPin)
								pInpPin->Release();
						}
						if (!*pConnect)
							pinInfo.pFilter->Release();

						muxInterface->Release();
						if (pIPin) pIPin->Release();
						return S_FALSE;
					}
					else {

						if (get_AC3_AudioPid())
							graphTools.GetAC3Media(&pintype);
						else if (get_AAC_AudioPid())
							graphTools.GetAACMedia(&pintype);
						else if (get_DTS_AudioPid())
							graphTools.GetDTSMedia(&pintype);
						else
							graphTools.GetAC3Media(&pintype);

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
					}

					USHORT pPid;
					if (get_AC3_AudioPid())
						pPid = get_AC3_AudioPid();
					else if (get_AAC_AudioPid())
						pPid = get_AAC_AudioPid();
					else if (get_DTS_AudioPid())
						pPid = get_DTS_AudioPid();
					else
						pPid = get_AC3_AudioPid();

					LoadAudioPin(pIPin, pPid);
					pIPin->Release();
					muxInterface->Release();
					return S_OK;
				}  
				else {

					graphTools.GetMP1Media(&pintype);
					if (SUCCEEDED(CheckDemuxPin(pDemux, pintype, &pIPin))){

						ClearDemuxPin(pIPin);
						pIPin->QueryId(pPinName);
						*pConnect = FALSE;
						IPin* pInpPin;
						if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

							IPin* pInpPin2 = NULL;
							PIN_INFO pinInfo;
							pInpPin->QueryPinInfo(&pinInfo);

							if (m_WasPlaying) {

								if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
								if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
							}

							if (SUCCEEDED(pIPin->Disconnect())){

								*pConnect = FALSE;

								if (get_AC3_AudioPid())
									graphTools.GetAC3Media(&pintype);
								else if (get_AAC_AudioPid())
									graphTools.GetAACMedia(&pintype);
								else if (get_DTS_AudioPid())
									graphTools.GetDTSMedia(&pintype);
								else
									graphTools.GetAC3Media(&pintype);

								muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
								if (FAILED(pInpPin->QueryAccept(&pintype)))
									*pConnect = TRUE;
								else if (FAILED(ReconnectFilterPin(pInpPin)))
									*pConnect = TRUE;
								else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
									*pConnect = TRUE;
								else {

									pInpPin->BeginFlush();
									pInpPin->EndFlush();
								}
								
								if (pInpPin2)
									pInpPin2->Release();

								if (*pConnect == TRUE){

									pInpPin->Disconnect();
									if (pInpPin)
										pInpPin->Release();

									RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
								}

								if (pInpPin)
									pInpPin->Release();
							}

							if (!*pConnect)
								pinInfo.pFilter->Release();

							muxInterface->Release();
							if (pIPin) pIPin->Release();
							return S_FALSE;
						}
						else {

							if (get_AC3_AudioPid())
								graphTools.GetAC3Media(&pintype);
							else if (get_AAC_AudioPid())
								graphTools.GetAACMedia(&pintype);
							else if (get_DTS_AudioPid())
								graphTools.GetDTSMedia(&pintype);
							else
								graphTools.GetAC3Media(&pintype);

							muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						}

						USHORT pPid;
						if (get_AC3_AudioPid())
							pPid = get_AC3_AudioPid();
						else if (get_AAC_AudioPid())
							pPid = get_AAC_AudioPid();
						else if (get_DTS_AudioPid())
							pPid = get_DTS_AudioPid();
						else
							pPid = get_AC3_AudioPid();

						LoadAudioPin(pIPin, pPid);
						if (pIPin) pIPin->Release();
						muxInterface->Release();
						return S_OK;
					} 
				} 
			} 
			if (pIPin) pIPin->Release();
			muxInterface->Release();
			return hr;
		}
		else if (strcmp(videocheck, pinname) == 0) {

			IPin* pIPin = NULL;
			AM_MEDIA_TYPE pintype;

			//Check if we have a video pin type
			graphTools.GetVideoMedia(&pintype);
			if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				graphTools.GetH264Media(&pintype);
				if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

					graphTools.GetMpeg4Media(&pintype);
					if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

						muxInterface->DeleteOutputPin(*pPinName);
						muxInterface->Release();
						return S_FALSE;
					}
				}
			}

			//parse video type to set media type & pid value
			USHORT pPid;
			pPid = get_VideoPid();
			if (pPid)
				graphTools.GetVideoMedia(&pintype);
			else {

				pPid = get_H264Pid();
				if (pPid)
					graphTools.GetH264Media(&pintype);
				else {

					pPid = get_Mpeg4Pid();
					if (pPid)
						graphTools.GetMpeg4Media(&pintype);
					else
						graphTools.GetVideoMedia(&pintype);
				}
			}

			//Change the media type only if we have a video pin
			if (pIPin) {

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				*pConnect = FALSE;
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
						}
								
						if (pInpPin2)
							pInpPin2->Release();

						if (*pConnect == TRUE){
							pInpPin->Disconnect();
							if (pInpPin)
								pInpPin->Release();

							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}

						if (pInpPin)
							pInpPin->Release();
					}

					if (!*pConnect)
						pinInfo.pFilter->Release();

					if (pIPin) pIPin->Release();
					muxInterface->Release();
					return S_FALSE;
				}
				else
				{
					muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				LoadVideoPin(pIPin, pPid);
				if (pIPin) pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
		}
		else if (strcmp(telexcheck, pinname) == 0){

			IPin* pIPin = NULL;
			AM_MEDIA_TYPE pintype;

			//Check if we have a teletext pin type
			graphTools.GetTelexMedia(&pintype);
			if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				muxInterface->DeleteOutputPin(*pPinName);
				muxInterface->Release();
				return S_FALSE;
			}

			//parse teletext type to set media type & pid value
			USHORT pPid;
			pPid = get_TeletextPid();

			if (pIPin) {

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				*pConnect = FALSE;
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
						}
								
						if (pInpPin2)
							pInpPin2->Release();

						if (*pConnect == TRUE){
							pInpPin->Disconnect();
							if (pInpPin)
								pInpPin->Release();

							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}

						if (pInpPin)
							pInpPin->Release();
					}

					if (!*pConnect)
						pinInfo.pFilter->Release();

					if (pIPin) pIPin->Release();
					muxInterface->Release();
					return S_FALSE;
				}
				else
				{
					muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				LoadTelexPin(pIPin, pPid);
				if (pIPin) pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
		}
		else if (strcmp(subtitlecheck, pinname) == 0){

			IPin* pIPin = NULL;
			AM_MEDIA_TYPE pintype;

			//Check if we have a subtitle pin type
			graphTools.GetSubtitleMedia(&pintype);
			if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				muxInterface->DeleteOutputPin(*pPinName);
				muxInterface->Release();
				return S_FALSE;
			}

			//parse teletext type to set media type & pid value
			USHORT pPid;
			pPid = get_SubtitlePid();

			if (pIPin) {

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				*pConnect = FALSE;
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
						}
								
						if (pInpPin2)
							pInpPin2->Release();

						if (*pConnect == TRUE){
							pInpPin->Disconnect();
							if (pInpPin)
								pInpPin->Release();

							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}

						if (pInpPin)
							pInpPin->Release();
					}

					if (!*pConnect)
						pinInfo.pFilter->Release();

					if (pIPin) pIPin->Release();
					muxInterface->Release();
					return S_FALSE;
				}
				else
				{
					muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				LoadSubtitlePin(pIPin, pPid);
				if (pIPin) pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
		}
		else if (strcmp(tscheck, pinname) == 0){

			IPin* pIPin = NULL;
			AM_MEDIA_TYPE pintype;

			//Check if we have a TS pin type
			graphTools.GetTSMedia(&pintype);
			if (FAILED(CheckDemuxPin(pDemux, pintype, &pIPin))){

				muxInterface->DeleteOutputPin(*pPinName);
				muxInterface->Release();
				return S_FALSE;
			}

			if (pIPin) {

				ClearDemuxPin(pIPin);
				pIPin->QueryId(pPinName);
				*pConnect = FALSE;
				IPin* pInpPin;
				if (SUCCEEDED(pIPin->ConnectedTo(&pInpPin))){

					IPin* pInpPin2 = NULL;
					PIN_INFO pinInfo;
					pInpPin->QueryPinInfo(&pinInfo);

					if (m_WasPlaying) {

						if (DoPause() == S_OK){while(IsPaused() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
						if (DoStop() == S_OK){while(IsStopped() == S_FALSE){if (Sleeps(100,m_TimeOut) != S_OK) break;}}
					}

					if (SUCCEEDED(pIPin->Disconnect())){

						muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
						if (FAILED(pInpPin->QueryAccept(&pintype)))
							*pConnect = TRUE;
						else if (FAILED(ReconnectFilterPin(pInpPin)))
							*pConnect = TRUE;
						else if (FAILED(pIPin->ConnectedTo(&pInpPin2)))
							*pConnect = TRUE;
						else {

							pInpPin->BeginFlush();
							pInpPin->EndFlush();
						}
								
						if (pInpPin2)
							pInpPin2->Release();

						if (*pConnect == TRUE){
							pInpPin->Disconnect();

							if (pInpPin)
								pInpPin->Release();

							RemoveFilterChain(pinInfo.pFilter, pinInfo.pFilter);
						}
						if (pInpPin)
							pInpPin->Release();
					}

					if (!*pConnect)
						pinInfo.pFilter->Release();

					if (pIPin) pIPin->Release();
					muxInterface->Release();
					return S_FALSE;
				}
				else
				{
					muxInterface->SetOutputPinMediaType(*pPinName, &pintype);
				}

				LoadTsPin(pIPin);
				if (pIPin) pIPin->Release();
				muxInterface->Release();
				return S_OK;
			}
		}
		muxInterface->Release();
	}
		return hr;
}


/*
HRESULT DWDemux::GetAC3Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_DOLBY_AC3;
	pintype->cbFormat = sizeof(MPEG1AudioFormat);//sizeof(AC3AudioFormat); //
	pintype->pbFormat = MPEG1AudioFormat;//AC3AudioFormat; //
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->formattype = FORMAT_WaveFormatEx;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT DWDemux::GetMP2Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_MPEG2_AUDIO; 
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(MPEG2AudioFormat);
	pintype->pbFormat = MPEG2AudioFormat; 
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT DWDemux::GetMP1Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_MPEG1Payload;
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(MPEG1AudioFormat);
	pintype->pbFormat = MPEG1AudioFormat;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT DWDemux::GetAACMedia(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Audio;
	pintype->subtype = MEDIASUBTYPE_AAC;
	pintype->formattype = FORMAT_WaveFormatEx; 
	pintype->cbFormat = sizeof(AACAudioFormat);
	pintype->pbFormat = AACAudioFormat;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = 0;
	pintype->lSampleSize = 1;
	pintype->pUnk = NULL;

	return S_OK;
}

HRESULT DWDemux::GetVideoMedia(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_VIDEO;
	pintype->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
	pintype->bFixedSizeSamples = TRUE;
	pintype->bTemporalCompression = FALSE;
	pintype->lSampleSize = 1;
	pintype->formattype = FORMAT_MPEG2Video;
	pintype->pUnk = NULL;
	if(m_bFixedAR) //change format if fixed aspect ratio
	{
		pintype->cbFormat = sizeof(Mpeg2ProgramVideo);
		pintype->pbFormat = Mpeg2ProgramVideo;
	}
	else
	{
		pintype->cbFormat = sizeof(g_Mpeg2ProgramVideo);
		pintype->pbFormat = g_Mpeg2ProgramVideo;
	}

	return S_OK;
}
static GUID H264_SubType = {0x8D2D71CB, 0x243F, 0x45E3, {0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B}};
//static GUID MEDIASUBTYPE_H264 = {0x8D2D71CB, 0x243F, 0x45E3, {0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B}};


HRESULT DWDemux::GetH264Media(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Video;
//	pintype->subtype = FOURCCMap(MAKEFOURCC('h','2','6','4'));
	pintype->subtype = H264_SubType;
	pintype->bFixedSizeSamples = FALSE;
	pintype->bTemporalCompression = TRUE;
	pintype->lSampleSize = 1;

	pintype->formattype = FORMAT_VideoInfo;
	pintype->pUnk = NULL;
	pintype->cbFormat = sizeof(H264VideoFormat);
	pintype->pbFormat = H264VideoFormat;

	return S_OK;
}

HRESULT DWDemux::GetMpeg4Media(AM_MEDIA_TYPE *pintype)
{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Video;
	pintype->subtype = FOURCCMap(MAKEFOURCC('h','2','6','4'));
	pintype->bFixedSizeSamples = FALSE;
	pintype->bTemporalCompression = TRUE;
	pintype->lSampleSize = 1;

	pintype->formattype = FORMAT_VideoInfo;
	pintype->pUnk = NULL;
	pintype->cbFormat = sizeof(H264VideoFormat);
	pintype->pbFormat = H264VideoFormat;

	return S_OK;
}

HRESULT DWDemux::GetTIFMedia(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	pintype->subtype = MEDIASUBTYPE_DVB_SI; 
	pintype->formattype = KSDATAFORMAT_SPECIFIER_NONE;

	return S_OK;
}

HRESULT DWDemux::GetTelexMedia(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	pintype->subtype = KSDATAFORMAT_SUBTYPE_NONE; 
	pintype->formattype = KSDATAFORMAT_SPECIFIER_NONE; 

	return S_OK;
}

HRESULT DWDemux::GetTSMedia(AM_MEDIA_TYPE *pintype)

{
	HRESULT hr = E_INVALIDARG;

	if(pintype == NULL)
		return hr;

	ZeroMemory(pintype, sizeof(AM_MEDIA_TYPE));
	pintype->majortype = MEDIATYPE_Stream;
	pintype->subtype = KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT; 
	pintype->formattype = FORMAT_None; 

	return S_OK;
}
*/
HRESULT DWDemux::Sleeps(ULONG Duration, long TimeOut[])
{
	HRESULT hr = S_OK;

	Sleep(Duration);
	TimeOut[0] = TimeOut[0] - Duration;
	if (TimeOut[0] <= 0)
	{
		hr = S_FALSE;
	}
	return hr;
}

HRESULT DWDemux::IsStopped()
{
	HRESULT hr = S_FALSE;

	FILTER_STATE state = State_Stopped;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
//			return m_pTSSourceFilter->GetState(200, &state);
			hr = m_pTSSourceFilter->GetState(200, &state);
			if (state == State_Stopped)
			{
				if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
					return S_OK;
			}
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->GetState(200, (OAFilterState*)&state);
			pMediaControl->Release();
		}

		Info.pGraph->Release();
	
//		if (hr == S_OK && state == State_Stopped)
//			return S_OK;

//		if (hr == VFW_S_STATE_INTERMEDIATE)
//			return S_OK;

		if (state == State_Stopped)
		{
			if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
				return S_OK;
		}
	} 
	return S_FALSE;
}

HRESULT DWDemux::IsPlaying()
{
	HRESULT hr = S_FALSE;

	FILTER_STATE state = State_Stopped;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
			hr =  m_pTSSourceFilter->GetState(200, &state);
			if (state == State_Running)
			{
				if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
					return S_OK;
			}

			return S_FALSE;
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl = NULL;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->GetState(200, (OAFilterState*)&state);
			pMediaControl->Release();
		}

		Info.pGraph->Release();
	
//		if (hr == S_OK && state == State_Running)
//			return S_OK;

//		if (hr == VFW_S_STATE_INTERMEDIATE)
//			return S_OK;

		if (state == State_Running)
		{
			if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
				return S_OK;
		}
	}
	return S_FALSE;
}

HRESULT DWDemux::IsPaused()
{
	HRESULT hr = S_FALSE;

	FILTER_STATE state = State_Stopped;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
//			return m_pTSSourceFilter->GetState(200, &state);
			hr = m_pTSSourceFilter->GetState(200, &state);
			if (state == State_Paused)
			{
				if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
					return S_OK;
			}
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->GetState(200, (OAFilterState*)&state);
			pMediaControl->Release();
		}
		Info.pGraph->Release();

//		if (hr == S_OK && state == State_Paused)
//			return S_OK;

//		if (hr == VFW_S_STATE_INTERMEDIATE)
//			return S_OK;

//		if (hr == VFW_S_CANT_CUE)
//			return hr;
		hr = pMediaControl->GetState(200, (OAFilterState*)&state);
		if (state == State_Paused)
		{
			if (hr == S_OK || hr == VFW_S_STATE_INTERMEDIATE || VFW_S_CANT_CUE)
				return S_OK;
		}
	}
	return S_FALSE;
}

HRESULT DWDemux::DoStop()
{
	HRESULT hr = S_OK;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
			return m_pTSSourceFilter->Stop();
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->Stop(); 
			pMediaControl->Release();
		}

		Info.pGraph->Release();

		if (FAILED(hr))
			return S_OK;
	}
	return S_OK;
}

HRESULT DWDemux::DoStart()
{
	HRESULT hr = S_OK;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
			return m_pTSSourceFilter->Run(NULL);
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->Run();
			pMediaControl->Release();
		}

		Info.pGraph->Release();

		if (FAILED(hr))
			return S_OK;
	}
	return S_OK;
}

HRESULT DWDemux::DoPause()
{
	HRESULT hr = S_OK;

	FILTER_INFO Info;;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (!pMediaFilter)
		{
			Info.pGraph->Release();
			return m_pTSSourceFilter->Pause();
		}
		else
			pMediaFilter->Release();

		IMediaControl *pMediaControl;
		if (SUCCEEDED(Info.pGraph->QueryInterface(IID_IMediaControl, (void **) &pMediaControl)))
		{
			hr = pMediaControl->Pause();
			pMediaControl->Release();
		}

		Info.pGraph->Release();

		if (FAILED(hr))
			return S_OK;
	}

	return S_OK;
}

BOOL DWDemux::get_Auto()
{
	return m_bAuto;
}

void DWDemux::set_Auto(BOOL bAuto)
{
	m_bAuto = bAuto;
}

BOOL DWDemux::get_NPControl()
{
	return m_bNPControl;
}

void DWDemux::set_NPControl(BOOL bNPControl)
{
	m_bNPControl = bNPControl;
}

BOOL DWDemux::get_NPSlave()
{
	return m_bNPSlave;
}

void DWDemux::set_NPSlave(BOOL bNPSlave)
{
	m_bNPSlave = bNPSlave;
}

BOOL DWDemux::get_AC3Mode()
{
	return m_bAC3Mode;
}

void DWDemux::set_AC3Mode(BOOL bAC3Mode)
{
	m_bAC3Mode = bAC3Mode;
}

BOOL DWDemux::get_FixedAspectRatio()
{
	return m_bFixedAR;
}

BOOL DWDemux::get_CreateTSPinOnDemux()
{
	return m_bCreateTSPinOnDemux;
}

void DWDemux::set_FixedAspectRatio(BOOL bFixedAR)
{
	m_bFixedAR = bFixedAR;
}

void DWDemux::set_CreateTSPinOnDemux(BOOL bCreateTSPinOnDemux)
{
	m_bCreateTSPinOnDemux = bCreateTSPinOnDemux;
}

BOOL DWDemux::get_CreateTxtPinOnDemux()
{
	return m_bCreateTxtPinOnDemux;
}

void DWDemux::set_CreateTxtPinOnDemux(BOOL bCreateTxtPinOnDemux)
{
	m_bCreateTxtPinOnDemux = bCreateTxtPinOnDemux;
}

BOOL DWDemux::get_CreateSubPinOnDemux()
{
	return m_bCreateSubPinOnDemux;
}

void DWDemux::set_CreateSubPinOnDemux(BOOL bCreateSubPinOnDemux)
{
	m_bCreateSubPinOnDemux = bCreateSubPinOnDemux;
}

BOOL DWDemux::get_MPEG2AudioMediaType()
{
	return m_bMPEG2AudioMediaType;
}

void DWDemux::set_MPEG2AudioMediaType(BOOL bMPEG2AudioMediaType)
{
	m_bMPEG2AudioMediaType = bMPEG2AudioMediaType;
}

BOOL DWDemux::get_MPEG2Audio2Mode()
{
	return m_bMPEG2Audio2Mode;
}

void DWDemux::set_MPEG2Audio2Mode(BOOL bMPEG2Audio2Mode)
{
	m_bMPEG2Audio2Mode = bMPEG2Audio2Mode;
	return;
}

int DWDemux::get_MP2AudioPid()
{
	if ((m_bMPEG2Audio2Mode && get_Audio2Pid()) || (m_StreamAud2 && get_Audio2Pid()))
		return get_Audio2Pid();
	else
		return get_AudioPid();
}

int DWDemux::get_AAC_AudioPid()
{
	if ((m_bMPEG2Audio2Mode && get_AAC2Pid()) || (m_StreamAud2 && get_AAC2Pid()))
		return get_AAC2Pid();
	else
		return get_AACPid();
}

int DWDemux::get_DTS_AudioPid()
{
	if ((m_bMPEG2Audio2Mode && get_DTS2Pid()) || (m_StreamAud2 && get_DTS2Pid()))
		return get_DTS2Pid();
	else
		return get_DTSPid();
}

int DWDemux::get_AC3_AudioPid()
{
	if ((m_bMPEG2Audio2Mode && get_AC3_2Pid()) || (m_StreamAud2 && get_AC3_2Pid()))
		return get_AC3_2Pid();
	else
		return get_AC3Pid();
}

int DWDemux::get_ClockMode()
{
	return m_ClockMode;
}

void DWDemux::set_ClockMode(int clockMode)
{
	m_ClockMode = clockMode;
}

HRESULT DWDemux::RemoveFilterChain(IBaseFilter *pStartFilter, IBaseFilter *pEndFilter)
{
	HRESULT hr = E_FAIL;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		IFilterChain *pFilterChain;
		if(SUCCEEDED(Info.pGraph->QueryInterface(IID_IFilterChain, (void **) &pFilterChain)))
		{
			hr = pFilterChain->RemoveChain(pStartFilter, pEndFilter);
			pFilterChain->Release();
		}

		Info.pGraph->Release();
	}
	return hr;
}

HRESULT DWDemux::RenderFilterPin(IPin *pIPin)
{
	HRESULT hr = E_FAIL;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		IGraphBuilder *pGraphBuilder;
		if(SUCCEEDED(Info.pGraph->QueryInterface(IID_IGraphBuilder, (void **) &pGraphBuilder)))
		{
			hr = pGraphBuilder->Render(pIPin);
			pGraphBuilder->Release();
		}

		Info.pGraph->Release();
	}
	return hr;
}

HRESULT DWDemux::ReconnectFilterPin(IPin *pIPin)
{
	HRESULT hr = E_FAIL;

	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		hr = Info.pGraph->Reconnect(pIPin);
		Info.pGraph->Release();
	}
	return hr;
}

HRESULT DWDemux::GetReferenceClock(IBaseFilter *pFilter, IReferenceClock **ppClock)
{
	HRESULT hr;

	FILTER_INFO Info;
	if (SUCCEEDED(pFilter->QueryFilterInfo(&Info)))
	{
		// Get IMediaFilter interface
		IMediaFilter* pMediaFilter = NULL;
		hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		Info.pGraph->Release();
		if (pMediaFilter)
		{
			// Get IReferenceClock interface
			hr = pMediaFilter->GetSyncSource(ppClock);
			pMediaFilter->Release();
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT DWDemux::SetReferenceClock(IBaseFilter *pFilter)
{
	HRESULT hr;
	FILTER_INFO Info;
	if (SUCCEEDED(m_pTSSourceFilter->QueryFilterInfo(&Info)))
	{
		if (pFilter != NULL)
		{
			IReferenceClock *pClock = NULL;
			if (SUCCEEDED(pFilter->QueryInterface(IID_IReferenceClock, (void**)&pClock)) && pClock != NULL)
			{
				// Get IMediaFilter interface
				IMediaFilter* pMediaFilter = NULL;
				hr = Info.pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
				Info.pGraph->Release();
				if (pMediaFilter != NULL)
				{
					// Get IReferenceClock interface
					hr = pMediaFilter->SetSyncSource(pClock);
					pClock->Release();
					pMediaFilter->Release();
					return S_OK;
				}
				pClock->Release();
			}
		}
		else // If Null pFilter set default clock
		{
			Info.pGraph->SetDefaultSyncSource();
			Info.pGraph->Release();
			return S_FALSE;
		}

	}
	return E_FAIL;
}


void DWDemux::SetRefClock()
{
	if (!m_pTSSourceFilter)
		return;

	if (m_ClockMode == 0)
	{
		//Let the filter graph choose the best clock
		SetReferenceClock(NULL);
		return;
	}

	if (m_ClockMode == 1)
	{
		//Set the reference Clock to this TSFileSource filter
		SetReferenceClock(m_pTSSourceFilter);
		return;
	}

	// Parse only the existing Mpeg2 Demultiplexer Filter
	// in the filter graph, we do this by looking for filters
	// that implement the IMpeg2Demultiplexer interface while
	// the count is still active.
	CFilterList FList(NAME("MyList"));  // List to hold the downstream peers.
	if (SUCCEEDED(GetPeerFilters(m_pTSSourceFilter, PINDIR_OUTPUT, FList)) && FList.GetHeadPosition())
	{
		IBaseFilter* pFilter = NULL;
		POSITION pos = FList.GetHeadPosition();
		pFilter = FList.Get(pos);
		while (SUCCEEDED(GetPeerFilters(pFilter, PINDIR_OUTPUT, FList)) && pos)
		{
			pFilter = FList.GetNext(pos);
		}

		pos = FList.GetHeadPosition();
		bool haveClock = false;
		while (pos)
		{
			pFilter = FList.GetNext(pos);
			if(pFilter != NULL)
			{
                AddFilterUnique(m_FilterRefList, pFilter);
				if (!haveClock)
				{
					CComPtr<IReferenceClock> pClock;
					if (SUCCEEDED(pFilter->QueryInterface(IID_IReferenceClock, (void**)&pClock)) && pClock != NULL)
					{
						bool bClock = false;
						
						if (m_ClockMode == 2)
						{
							//Set the reference Clock to the first Demux
							bClock = true;
						}
						else if (m_ClockMode == 3)
						{
							//Set the reference Clock to the first audio renderer
							CComPtr<IBasicAudio> pBasicAudio;
							if (SUCCEEDED(pFilter->QueryInterface(IID_IBasicAudio, (void**)&pBasicAudio)) && pBasicAudio != NULL)
								bClock = true;
						}

						if (bClock)
						{
							if (SUCCEEDED(SetReferenceClock(pFilter)))
								haveClock = true;
						}

					}
				}
				pFilter = NULL;
			}
		}
	}
	//Clear the filter list;
	POSITION pos = FList.GetHeadPosition();
	while (pos){

		if (FList.Get(pos) != NULL)
				FList.Get(pos)->Release();

		FList.Remove(pos);
		pos = FList.GetHeadPosition();
	}
}

long DWDemux::get_PcrPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(pcr, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_VideoPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(video, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_H264Pid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(h264, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_Mpeg4Pid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(mpeg4, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_AudioPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(mp2, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_Audio2Pid()
{
	long pid1 = 0;
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(mp2, currentStream);
		if (!pid1 && pid)
			pid = pid1;
		else if (pid)
			return pid;
	}
	return pid1;
}

long DWDemux::get_AC3Pid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(ac3, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_AC3_2Pid()
{
	long pid1 = 0;
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(ac3, currentStream);
		if (!pid1 && pid)
			pid = pid1;
		else if (pid)
			return pid;
	}
	return pid1;
}

long DWDemux::get_TeletextPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(teletext, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_SubtitlePid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(subtitle, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_AACPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(aac, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_AAC2Pid()
{
	long pid1 = 0;
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(aac, currentStream);
		if (!pid1 && pid)
			pid = pid1;
		else if (pid)
			return pid;
	}
	return pid1;
}

long DWDemux::get_DTSPid()
{
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(dts, currentStream);
		if (pid)
			return pid;
	}
	return 0;
}

long DWDemux::get_DTS2Pid()
{
	long pid1 = 0;
	long count = m_pService->GetStreamCount();
	for ( long currentStream=0 ; currentStream<count ; currentStream++ )
	{
		long pid = m_pService->GetStreamPID(dts, currentStream);
		if (!pid1 && pid)
			pid = pid1;
		else if (pid)
			return pid;
	}
	return pid1;
}

//TCHAR sz[128];
//sprintf(sz, "%u", pClock);
//MessageBox(NULL, sz,"test", NULL);

