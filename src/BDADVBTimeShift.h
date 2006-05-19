/**
 *	BDADVBTimeShift.h
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

#ifndef BDADVBTIMESHIFT_H
#define BDADVBTIMESHIFT_H

#include "DWSource.h"
#include "BDADVBTimeShiftTuner.h"
#include "BDADVBTSink.h"
#include "DVBTChannels.h"
#include "BDACardCollection.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBTFrequencyList.h"
#include "DWThread.h"
#include <vector>
#include "TSFileSource/TSFileSource.h"
#include "FilterPropList.h"

class TunerSinkGraphItem
{
public:
	TunerSinkGraphItem();
	virtual ~TunerSinkGraphItem();

	BDADVBTSink * pSink;
	BDADVBTimeShiftTuner *pTuner;
	FilterPropList *pFilterList;
	CComPtr <IGraphBuilder> piGraphBuilder;
	int cardId;
	DWORD rotEntry;
	BOOL isRecording;
	long networkId;
	long serviceId;
	CComPtr <IBaseFilter> pBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> piMpeg2Demux;
};

class TSFileSource;
class BDADVBTimeShift : public DWSource, public DWThread
{

public:
	BDADVBTimeShift();
	virtual ~BDADVBTimeShift();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual LPWSTR GetSourceType();
	virtual DWGraph *GetFilterGraph(void);
	virtual IGraphBuilder *GetGraphBuilder(void);

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT Destroy();

	virtual HRESULT ExecuteCommand(ParseLine* command);
	//Keys, ControlBar, OSD, Menu, etc...

	virtual BOOL IsRecording();
	virtual HRESULT ReLoadTimeShiftFile();
	virtual HRESULT LoadRecordFile();

	virtual BOOL CanLoad(LPWSTR pCmdLine);
	virtual HRESULT Load(LPWSTR pCmdLine);
	virtual	HRESULT GetFilterList(void);
	virtual	HRESULT ShowFilter(LPWSTR filterName);
	HRESULT SetStream(long index);
	HRESULT SetStreamName(LPWSTR pService, BOOL bEnable);

	DVBTChannels *GetChannels();
	int m_cardId;

	virtual void ThreadProc();

protected:
	virtual HRESULT SetChannel(long originalNetworkId, long serviceId);
	virtual HRESULT SetChannel(long originalNetworkId, long transportStreamId, long networkId, long serviceId);
	virtual HRESULT SetFrequency(long frequency, long bandwidth = 0);
	virtual HRESULT NetworkUp();
	virtual HRESULT NetworkDown();
	virtual HRESULT ProgramUp();
	virtual HRESULT ProgramDown();

	// graph building methods
	HRESULT RenderChannel(DVBTChannels_Network* pNetwork, DVBTChannels_Service* pService);
	virtual HRESULT RenderChannel(int frequency, int bandwidth);

	void RotateFilterList(void);
	void UpdateStatusDisplay();
	HRESULT CloseBuffers();
	HRESULT CloseDisplay();
	HRESULT OpenDisplay();
	HRESULT LoadSinkGraph(int frequency, int bandwidth);
	HRESULT UnLoadSinkGraph();
	HRESULT LoadTuner();
	HRESULT LoadDemux();
	HRESULT UnloadTuner();
	HRESULT LoadSink();
	HRESULT UnloadSink();
	HRESULT LoadFileSource();
	HRESULT UnloadFileSource();

	HRESULT AddDemuxPins(DVBTChannels_Service* pService);
	HRESULT AddDemuxPins(DVBTChannels_Service* pService, DVBTChannels_Service_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsVideo(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsMp2(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsAC3(DVBTChannels_Service* pService, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsTeletext(DVBTChannels_Service* pService, long *streamsRendered = NULL);

	void UpdateData(long frequency = 0, long bandwidth = 0);

	HRESULT UpdateChannels();

	HRESULT ChangeFrequencySelectionOffset(long change);

	HRESULT MoveNetworkUp(long transportStreamId);
	HRESULT MoveNetworkDown(long transportStreamId);
//	HRESULT ToggleRecording(long mode);
//DWS28-02-2006	HRESULT ToggleRecording(long mode, LPWSTR pFilename = NULL);
	HRESULT ToggleRecording(long mode, LPWSTR pFilename = NULL, LPWSTR pPath = NULL);
	HRESULT TogglePauseRecording(long mode);
	TunerSinkGraphItem *GetCurrentTunerGraph();
	void SetCurrentTunerGraph(TunerSinkGraphItem *tuner);
	HRESULT SaveCurrentTunerItem(TunerSinkGraphItem **tuner);

private:
	const LPWSTR m_strSourceType;

	BOOL m_bInitialised;
	long m_rtTimeShiftStart;
	long m_rtTimeShiftDuration;
	long m_rtSizeMonitor;

	std::vector<TunerSinkGraphItem*> m_tuners;
	BDADVBTimeShiftTuner *m_pCurrentTuner;
	CCritSec m_tunersLock;

	BDADVBTSink *m_pCurrentSink;
	TSFileSource *m_pCurrentFileSource;
	
	BOOL m_bFileSourceActive;

	//Recorder
	DVBTChannels channels;
	DVBTChannels_Network *m_pCurrentNetwork;
	DVBTChannels_Service *m_pCurrentService;
	BDACardCollection cardList;
	//NaN

	DWGraph *m_pDWGraph;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IBaseFilter> m_pBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;
	CComPtr <IGraphBuilder> m_piSinkGraphBuilder;

	DVBTFrequencyList frequencyList;
	FilterPropList *m_pCurrentFilterList;

	FilterGraphTools graphTools;
	DWORD m_rotEntry;

};

#endif
