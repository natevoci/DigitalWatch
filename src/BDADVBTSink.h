/**
 *	BDADVBTSink.h
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

#ifndef BDADVBTSINK_H
#define BDADVBTSINK_H

#include "BDADVBTSourceTuner.h"
#include "BDADVBTSinkTShift.h"
#include "BDADVBTSinkDSNet.h"
#include "BDADVBTSinkFile.h"
#include "StdAfx.h"
#include <bdatif.h>
#include "BDACardCollection.h"
#include "DWGraph.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBMpeg2DataParser.h"
#include "DVBTChannels.h"

// {4DF35815-79C5-44C8-8753-847D5C9C3CF5}
EXTERN_GUID(CLSID_MPEG2Multiplexer,
0x4DF35815, 0x79C5, 0x44C8, 0x87, 0x53, 0x84, 0x7D, 0x5C, 0x9C, 0x3C, 0xF5);

//{BC650178-0DE4-47DF-AF50-BBD9C7AEF5A9}
//EXTERN_GUID(CLSID_MPEG2Multiplexer,
//0xBC650178, 0x0DE4, 0x47DF, 0xAF, 0x50, 0xBB, 0xD9, 0xC7, 0xAE, 0xF5, 0xA9);

//{6770E328-9B73-40C5-91E6-E2F321AEDE57}
//EXTERN_GUID(CLSID_MPEG2Multiplexer,
//0x6770E328, 0x9B73, 0x40C5, 0x91, 0xE6, 0xE2, 0xF3, 0x21, 0xAE, 0xDE, 0x57);

class BDADVBTSinkTShift;
class BDADVBTSinkFile;
class BDADVBTSinkDSNet;
class BDADVBTSink : public LogMessageCaller
{
public:
	BDADVBTSink();
	virtual ~BDADVBTSink();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise(DWGraph *pDWGraph);
	HRESULT DestroyAll();

	HRESULT AddSinkFilters(DVBTChannels_Service* pService);
	void DestroyFilter(CComPtr <IBaseFilter> &pFilter);
	HRESULT RemoveSinkFilters();

	HRESULT SetTransportStreamPin(IPin* piPin);

	BOOL IsActive();

	HRESULT AddFileName(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intSinkType = 0);
	HRESULT NullFileName(CComPtr<IBaseFilter>& pFilter, int intSinkType);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, int intPinType = 0);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsVideo(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsMp2(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsAC3(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsTeletext(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsTS(DVBTChannels_Service* pService, long *streamsRendered);
	HRESULT ClearDemuxPids(CComPtr<IBaseFilter>& pFilter);
	HRESULT ClearDemuxPins(IPin *pIPin);
	HRESULT StartSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd);
	HRESULT StopSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd);
	HRESULT PauseSinkChain(CComPtr<IBaseFilter>& pFilterStart, CComPtr<IBaseFilter>& pFilterEnd);
	HRESULT StartRecording(DVBTChannels_Service* pService, LPWSTR pFilename = NULL);
	HRESULT StopRecording(void);
	HRESULT PauseRecording(void);
	HRESULT UnPauseRecording(DVBTChannels_Service* pService);
	BOOL IsRecording(void);
	BOOL IsPaused(void);
	HRESULT GetCurFile(LPOLESTR *ppszFileName);

	BOOL SupportsRecording() { return FALSE; }

private:
	DWGraph *m_pDWGraph;

	BDADVBTSinkTShift *m_pCurrentTShiftSink;
	BDADVBTSinkFile	*m_pCurrentFileSink;
	BDADVBTSinkDSNet *m_pCurrentDSNetSink;

	CComPtr <IMpeg2Demultiplexer> m_piIMpeg2Demux;
	CComPtr <IFileSinkFilter> m_piFileSink;

	BOOL m_bInitialised;
	BOOL m_bActive;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	CComPtr <IBaseFilter> m_piInfinitePinTee;

	long m_intTimeShiftType;
	long m_intFileSinkType;
	long m_intDSNetworkType;

	DWORD m_rotEntry;

	FilterGraphTools graphTools;
};

#endif
