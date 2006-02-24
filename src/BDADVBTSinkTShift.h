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

// {5cdd5c68-80dc-43e1-9e44-c849ca8026e7}
EXTERN_GUID(CLSID_TSFileSink,
0x5cdd5c68, 0x80dc, 0x43e1, 0x9e, 0x44, 0xc8, 0x49, 0xca, 0x80, 0x26, 0xe7);

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

	HRESULT Initialise(DWGraph *pDWGraph, int intSinkType);
	HRESULT DestroyAll();

	HRESULT AddSinkFilters(DVBTChannels_Service* pService);
	HRESULT RemoveSinkFilters();

	HRESULT SetTransportStreamPin(IPin* piPin);

	BOOL IsActive();

	BOOL SetRecordingFullTS();
	BOOL SetRecordingTSMux(long PIDs[]);
	BOOL StartRecording(LPWSTR filename);
	BOOL PauseRecording(BOOL bPause);
	BOOL StopRecording();
	BOOL IsRecording();
	HRESULT GetCurFile(LPOLESTR *ppszFileName);

	BOOL SupportsRecording() { return FALSE; }

private:
	void DestroyFTSFilters();
	void DestroyTSFilters();
	void DestroyMPGFilters();
	void DestroyAVFilters();
	void DeleteFilter(DWDump **pfDWDump);
	void DestroyFilter(CComPtr <IBaseFilter> &pFilter);

	BDADVBTSink *m_pBDADVBTSink;
	DWGraph *m_pDWGraph;

	BOOL m_bInitialised;
	BOOL m_bActive;


	BOOL m_bRecording;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	CComPtr <IBaseFilter> m_piInfinitePinTee;

	int m_intSinkType;

	CComPtr <IBaseFilter> m_piTelexSink;
	DWDump *m_piTelexDWDump;
	CComPtr <IBaseFilter> m_piVideoSink;
	DWDump *m_piVideoDWDump;
	CComPtr <IBaseFilter> m_piAudioSink;
	DWDump *m_piAudioDWDump;
	CComPtr <IBaseFilter> m_piAVMpeg2Demux;
	CComPtr <IBaseFilter> m_piMPGSink;
	DWDump *m_piMPGDWDump;
	CComPtr <IBaseFilter> m_piMPGMpeg2Mux;
	CComPtr <IBaseFilter> m_piMPGMpeg2Demux;
	CComPtr <IBaseFilter> m_piTSSink;
	DWDump *m_piTSDWDump;
	CComPtr <IBaseFilter> m_piTSMpeg2Demux;
	CComPtr <IBaseFilter> m_piFTSSink;
	DWDump *m_piFTSDWDump;

	DWORD m_rotEntry;

	FilterGraphTools graphTools;
};

#endif
