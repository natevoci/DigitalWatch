/**
 *	BDADVBTSinkDSNet.h
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

#ifndef BDADVBTSINKDSNET_H
#define BDADVBTSINKDSNET_H

#include "BDADVBTSink.h"
#include "StdAfx.h"
#include <bdatif.h>
#include "BDACardCollection.h"
#include "DWGraph.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBMpeg2DataParser.h"
#include "DWDump.h"

//{A07E6137-6C07-45D9-A00C-7DE7A7E6319B}
EXTERN_GUID(CLSID_DSNetworkSender,
0xA07E6137, 0x6C07, 0x45D9, 0xA0, 0x0C, 0x7D, 0xE7, 0xA7, 0xE6, 0x31, 0x9B);

//{1CB42CC8-D32C-4f73-9267-C114DA470378}
EXTERN_GUID(IID_IMulticastConfig,
0x1CB42CC8, 0xD32C, 0x4f73, 0x92, 0x67, 0xC1, 0x14, 0xDA, 0x47, 0x03, 0x78);

class BDADVBTSink;
class BDADVBTSinkDSNet : public LogMessageCaller
{
public:
	BDADVBTSinkDSNet(BDADVBTSink *pBDADVBTSink);
	virtual ~BDADVBTSinkDSNet();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise(DWGraph *pDWGraph, int intSinkType);
	HRESULT DestroyAll();

	HRESULT AddSinkFilters(DVBTChannels_Service* pService);
	HRESULT RemoveSinkFilters();

	HRESULT SetTransportStreamPin(IPin* piPin);

	BOOL IsActive();

	BOOL SetRecordingFullTS();
	BOOL SetRecordingTSMux(long PIDs[]);
//DWS28-02-2006	BOOL StartRecording(LPWSTR filename);
	BOOL StartRecording(LPWSTR filename, LPWSTR pPath);
	BOOL PauseRecording(BOOL bPause);
	BOOL StopRecording();
	BOOL IsRecording();

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

	CComPtr <IBaseFilter> m_pInfinitePinTee;

	int m_intSinkType;

	CComPtr <IBaseFilter> m_pTelexSink;
	DWDump *m_pTelexDWDump;
	CComPtr <IBaseFilter> m_pVideoSink;
	DWDump *m_pVideoDWDump;
	CComPtr <IBaseFilter> m_pAudioSink;
	DWDump *m_pAudioDWDump;
	CComPtr <IBaseFilter> m_pAVMpeg2Demux;
	CComPtr <IBaseFilter> m_pMPGSink;
	DWDump *m_pMPGDWDump;
	CComPtr <IBaseFilter> m_pMPGMpeg2Mux;
	CComPtr <IBaseFilter> m_pMPGMpeg2Demux;
	CComPtr <IBaseFilter> m_pTSSink;
	DWDump *m_pTSDWDump;
	CComPtr <IBaseFilter> m_pTSMpeg2Demux;
	CComPtr <IBaseFilter> m_pFTSSink;
	DWDump *m_pFTSDWDump;

	LPOLESTR m_pTelexFileName;
	LPOLESTR m_pVideoFileName;
	LPOLESTR m_pAudioFileName;
	LPOLESTR m_pMPGFileName;
	LPOLESTR m_pTSFileName;
	LPOLESTR m_pFTSFileName;

	DWORD m_rotEntry;

	FilterGraphTools graphTools;
};

#endif
