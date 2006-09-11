/**
 *	BDADVBTSinkTShift.h
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

#ifndef BDADVBTSINKSHIFT_H
#define BDADVBTSINKSHIFT_H

#include "BDADVBTSink.h"
#include "StdAfx.h"
#include <bdatif.h>
#include "BDACardCollection.h"
#include "DWGraph.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBMpeg2DataParser.h"
#include "DWDump.h"

// {0d2620cd-a57a-4458-b96f-76442b70e9c7}
EXTERN_GUID(IID_ITSFileSink,
0x0d2620cd, 0xa57a, 0x4458, 0xb9, 0x6f, 0x76, 0x44, 0x2b, 0x70, 0xe9, 0xc7);

// {5cdd5c68-80dc-43e1-9e44-c849ca8026e7}
EXTERN_GUID(CLSID_TSFileSink,
0x5cdd5c68, 0x80dc, 0x43e1, 0x9e, 0x44, 0xc8, 0x49, 0xca, 0x80, 0x26, 0xe7);

// {76635ECE-EFD6-464e-84A3-F86E37364009}
EXTERN_GUID(IID_ITSParserSink,
0x76635ece, 0xefd6, 0x464e, 0x84, 0xa3, 0xf8, 0x6e, 0x37, 0x36, 0x40, 0x9);

// {72BBD8CE-5C1B-49a5-B3A6-E046EF964421}
EXTERN_GUID(CLSID_TSParserSink,
0x72bbd8ce, 0x5c1b, 0x49a5, 0xb3, 0xa6, 0xe0, 0x46, 0xef, 0x96, 0x44, 0x21);

//{B07560B4-94BD-49F9-8F84-A738A58809B5}
//EXTERN_GUID(CLSID_TSFileSink,
//0xB07560B4, 0x94BD, 0x49F9, 0x8F, 0x84, 0xA7, 0x38, 0xA5, 0x88, 0x09, 0xB5);

class BDADVBTSink;
class BDADVBTSinkTShift : public LogMessageCaller
{
public:
	BDADVBTSinkTShift(BDADVBTSink *pBDADVBTSink);
	virtual ~BDADVBTSinkTShift();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise(IGraphBuilder *piGraphBuilder, int intSinkType);
	HRESULT DestroyAll();

	HRESULT AddSinkFilters(DVBTChannels_Service* pService);
	HRESULT RemoveSinkFilters();

	HRESULT SetTransportStreamPin(IPin* piPin);

	BOOL IsActive();

	BOOL SetRecordingFullTS();
	BOOL SetRecordingTSMux(long PIDs[]);
	BOOL StartRecording(LPWSTR filename, LPWSTR pPath);
	BOOL PauseRecording(BOOL bPause);
	BOOL StopRecording();
	BOOL IsRecording();
	HRESULT GetCurFile(LPOLESTR *ppszFileName);
	HRESULT GetCurFileSize(__int64 *pllFileSize);
	HRESULT UpdateTSFileSink(BOOL bAutoMode = FALSE);
	HRESULT SetTimeShiftInterface(IBaseFilter *pFilter, BOOL bAutoMode = FALSE);
	HRESULT ClearSinkDemuxPins();
	HRESULT GetReferenceDemux(CComPtr<IBaseFilter>&pDemux);

	BOOL SupportsRecording() { return FALSE; }

private:
	void DestroyFTSFilters();
	void DestroyTSFilters();
	void DestroyMPGFilters();
	void DestroyAVFilters();
	void DestroyFilter(CComPtr <IBaseFilter> &pFilter);

	BDADVBTSink *m_pBDADVBTSink;

	BOOL m_bInitialised;
	BOOL m_bActive;


	BOOL m_bRecording;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	CComPtr <IBaseFilter> m_pInfinitePinTee;

	int m_intSinkType;

	CComPtr <IBaseFilter> m_pAVMpeg2Demux;

	CComPtr <IBaseFilter> m_pTelexSink;
	DWDump *m_pTelexDWDump;

	CComPtr <IBaseFilter> m_pVideoQuantizer;
	CComPtr <IBaseFilter> m_pVideoSink;
	DWDump *m_pVideoDWDump;

	CComPtr <IBaseFilter> m_pAudioSink;
	DWDump *m_pAudioDWDump;

	CComPtr <IBaseFilter> m_pMPGMpeg2Demux;
	CComPtr <IBaseFilter> m_pMPGQuantizer;
	CComPtr <IBaseFilter> m_pMPGMpeg2Mux;
	CComPtr <IBaseFilter> m_pMPGSink;
	DWDump *m_pMPGDWDump;


	CComPtr <IBaseFilter> m_pTSMpeg2Demux;
	CComPtr <IBaseFilter> m_pTSSink;
	DWDump *m_pTSDWDump;

	CComPtr <IBaseFilter> m_pFTSSink;
	DWDump *m_pFTSDWDump;

	LPOLESTR m_pTelexFileName;
	LPOLESTR m_pVideoFileName;
	LPOLESTR m_pAudioFileName;
	LPOLESTR m_pMPGFileName;
	LPOLESTR m_pTSFileName;
	LPOLESTR m_pFTSFileName;

	FilterGraphTools graphTools;
};

#endif
