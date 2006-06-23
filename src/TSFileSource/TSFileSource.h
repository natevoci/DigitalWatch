/**
 *	TSFileSource.h
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

#ifndef TSFILESOURCE_H
#define TSFILESOURCE_H

#include "DWSource.h"
#include "DVBTChannels.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DWThread.h"
#include "TSFileStreamList.h"
#include "..\FilterPropList.h"
#include <vector>
#include "DWDemux.h"

class TSFileSource : public DWSource, public DWThread
{
public:
	TSFileSource();
	virtual ~TSFileSource();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual LPWSTR GetSourceType();
	virtual DWGraph *GetFilterGraph(void);
	virtual IGraphBuilder *GetGraphBuilder(void);

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT Destroy();
	virtual HRESULT UnloadFilters();

	virtual HRESULT ExecuteCommand(ParseLine* command);

	virtual BOOL IsInitialised();
	virtual BOOL IsRecording();
	virtual HRESULT SeekTo(long percentage);
	virtual HRESULT Seek(long position);
	virtual HRESULT GetPosition(long *position);
	virtual HRESULT Skip(long seconds);

	virtual BOOL CanLoad(LPWSTR pCmdLine);
	virtual HRESULT Load(LPWSTR pCmdLine);
	virtual	HRESULT FastLoad(LPWSTR pCmdLine, DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt = NULL);
	virtual HRESULT ReLoad(LPWSTR pCmdLine);
	virtual HRESULT SetStream(long index);
	virtual HRESULT GetStreamList(void);
	virtual	HRESULT SetStreamName(LPWSTR pService, BOOL bEnable = TRUE);
	virtual	HRESULT GetFilterList(void);
	virtual	HRESULT ShowFilter(LPWSTR filterName);
	virtual	HRESULT SetRate(double dRate);
	virtual	HRESULT QueryTransportStreamPin(IPin** piPin);

	virtual void ThreadProc();


protected:
	// graph building methods
	HRESULT LoadFile(LPWSTR pFilename, DVBTChannels_Service* pService = NULL, AM_MEDIA_TYPE *pmt = NULL);
	HRESULT ReLoadFile(LPWSTR pFilename);
	HRESULT LoadResumePosition();
	HRESULT SaveResumePosition();
	HRESULT CloseDisplay();
	HRESULT OpenDisplay(BOOL bTest = FALSE);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, AM_MEDIA_TYPE *pmt, BOOL bRender = TRUE, BOOL bForceConnect = FALSE);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsVideo(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsH264(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsMpeg4(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsMp1(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsMp2(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsAC3(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsAAC(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT AddDemuxPinsTeletext(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL, BOOL bRender = TRUE);
	HRESULT SetSourceControl(IBaseFilter *pFilter, BOOL autoMode);
	HRESULT SetSourceInterface(IBaseFilter *pFilter, DVBTChannels_Service** pService);
	HRESULT TestDecoderSelection(LPWSTR pwszMediaType);
	HRESULT LoadMediaStreamType(USHORT pid, LPWSTR pwszMediaType, DVBTChannels_Stream** pStream );

	virtual void DestroyFilter(CComPtr <IBaseFilter> &pFilter);

	virtual HRESULT PlayPause();

	virtual HRESULT UpdateData();

private:
	const LPWSTR m_strSourceType;

	BOOL m_bInitialised;
	CCritSec m_listLock;

	DWGraph *m_pDWGraph;
	LPWSTR m_pFileName;
	CComPtr <IGraphBuilder> m_piGraphBuilder;

	CComPtr <IBaseFilter> m_pTSFileSource;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;

	TSFileStreamList streamList;
	FilterPropList filterList;

	FilterGraphTools graphTools;
	DWDemux m_DWDemux;
};

#endif
