/**
 *	BDADVBTSource.h
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

#ifndef BDADVBTSOURCE_H
#define BDADVBTSOURCE_H

#include "DWSource.h"
#include "BDADVBTSourceTuner.h"
#include "BDADVBTSink.h"
#include "DVBTChannels.h"
#include "BDACardCollection.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBTFrequencyList.h"
#include "DWThread.h"
#include <vector>

class BDADVBTSource : public DWSource, public DWThread
{

typedef struct TunerInfo
{
	BDADVBTSourceTuner *tuners;
	BDADVBTSink *sinks;

} TUNERINFO;

public:
	BDADVBTSource();
	virtual ~BDADVBTSource();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual LPWSTR GetSourceType();
	virtual DWGraph *GetFilterGraph(void);

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT Destroy();

	virtual HRESULT ExecuteCommand(ParseLine* command);
	//Keys, ControlBar, OSD, Menu, etc...

	virtual HRESULT Start();
	virtual BOOL IsRecording();

	virtual BOOL CanLoad(LPWSTR pCmdLine);
	virtual HRESULT Load(LPWSTR pCmdLine);

	DVBTChannels *GetChannels();

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

	HRESULT LoadTuner();
	HRESULT UnloadTuner();
	HRESULT LoadSink();
	HRESULT UnloadSink();

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

private:
	const LPWSTR m_strSourceType;

	BOOL m_bInitialised;

	BDADVBTSourceTuner *m_pCurrentTuner;
	std::vector<BDADVBTSourceTuner *> m_tuners;
	CCritSec m_tunersLock;

	BDADVBTSink *m_pCurrentSink;

	//Recorder
	DVBTChannels channels;
	DVBTChannels_Network *m_pCurrentNetwork;
	DVBTChannels_Service *m_pCurrentService;
	BDACardCollection cardList;
	//NaN

	DWGraph *m_pDWGraph;
	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;

	DVBTFrequencyList frequencyList;

	FilterGraphTools graphTools;
};

#endif
