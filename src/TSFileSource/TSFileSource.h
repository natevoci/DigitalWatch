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

	virtual BOOL IsRecording();
	virtual HRESULT SeekTo(long percentage);
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

	virtual void ThreadProc();


protected:
	// graph building methods
	HRESULT LoadFile(LPWSTR pFilename, DVBTChannels_Service* pService = NULL, AM_MEDIA_TYPE *pmt = NULL);
	HRESULT ReLoadFile(LPWSTR pFilename);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, CComPtr<IBaseFilter>& pFilter, AM_MEDIA_TYPE *pmt, int intPinType = 0);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL);
	HRESULT VetDemuxPin(IPin* pIPin, ULONG pid);
	HRESULT AddDemuxPinsVideo(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsMp2(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsAC3(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsTeletext(DVBTChannels_Service* pService, AM_MEDIA_TYPE *pmt, long *streamsRendered = NULL);
	HRESULT GetAC3Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetMP2Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetMP1Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetAACMedia(AM_MEDIA_TYPE *pintype);
	HRESULT GetVideoMedia(AM_MEDIA_TYPE *pintype);
	HRESULT GetH264Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetMpeg4Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetTIFMedia(AM_MEDIA_TYPE *pintype);
	HRESULT GetTelexMedia(AM_MEDIA_TYPE *pintype);
	HRESULT GetTSMedia(AM_MEDIA_TYPE *pintype);
	HRESULT SetSourceInterface(IBaseFilter *pFilter);

	virtual void DestroyFilter(CComPtr <IBaseFilter> &pFilter);

	virtual HRESULT PlayPause();

	virtual HRESULT UpdateData();

private:
	const LPWSTR m_strSourceType;

	BOOL m_bInitialised;

	DWGraph *m_pDWGraph;
	CComPtr <IGraphBuilder> m_piGraphBuilder;

	CComPtr <IBaseFilter> m_pTSFileSource;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;

	TSFileStreamList streamList;
	FilterPropList filterList;

	FilterGraphTools graphTools;
};

#endif
