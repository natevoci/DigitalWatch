/**
 *  DWDemux.h
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
 *
*/

#ifndef DWDEMUX_H
#define DWDEMUX_H

// Define a typedef for a list of filters.
typedef CGenericList<IBaseFilter> CFilterList;

#include "FilterGraphTools.h"
#include "Control.h"
#include "LogMessage.h"
#include "DVBTChannels.h"
#include "Globals.h"

class DWDemux : public LogMessageCaller
{
public:

	DWDemux();

	virtual ~DWDemux();
	virtual void SetLogCallback(LogMessageCallback *callback);

	STDMETHODIMP AOnConnect(IBaseFilter *pTSSourceFilter,
							DVBTChannels_Service* pService = NULL,
							IBaseFilter *pClockFilter = NULL);
	HRESULT SetTIFState(IFilterGraph *pGraph, REFERENCE_TIME tStart);

	BOOL get_Auto();
	BOOL get_NPControl();
	BOOL get_NPSlave();
	BOOL get_AC3Mode();
	BOOL get_FixedAspectRatio();
	BOOL get_CreateTSPinOnDemux();
	BOOL get_CreateTxtPinOnDemux();
	BOOL get_CreateSubPinOnDemux();
	BOOL get_MPEG2AudioMediaType();
	BOOL get_MPEG2Audio2Mode();
	void set_MPEG2AudioMediaType(BOOL bMPEG2AudioMediaType);
	void set_FixedAspectRatio(BOOL bFixedAR);
	void set_CreateTSPinOnDemux(BOOL bCreateTSPinOnDemux);
	void set_CreateTxtPinOnDemux(BOOL bCreateTxtPinOnDemux);
	void set_CreateSubPinOnDemux(BOOL bCreateSubPinOnDemux);
	void set_AC3Mode(BOOL bAC3Mode);
	void set_NPSlave(BOOL bNPSlave);
	void set_NPControl(BOOL bNPControl);
	void set_Auto(BOOL bAuto);
	void set_MPEG2Audio2Mode(BOOL bMPEG2Audio2Mode);
	void set_ClockMode(int clockMode);
	void SetRefClock();
	int  get_MP2AudioPid();
	int  get_AAC_AudioPid();
	int	 get_DTS_AudioPid();
	int  get_AC3_AudioPid();
	int get_ClockMode();
/*	HRESULT	GetAC3Media(AM_MEDIA_TYPE *pintype);
	HRESULT	GetMP2Media(AM_MEDIA_TYPE *pintype);
	HRESULT	GetMP1Media(AM_MEDIA_TYPE *pintype);
	HRESULT	GetAACMedia(AM_MEDIA_TYPE *pintype);
	HRESULT	GetVideoMedia(AM_MEDIA_TYPE *pintype);
	HRESULT GetH264Media(AM_MEDIA_TYPE *pintype);
	HRESULT GetMpeg4Media(AM_MEDIA_TYPE *pintype);
	HRESULT	GetTelexMedia(AM_MEDIA_TYPE *pintype);
	HRESULT	GetTSMedia(AM_MEDIA_TYPE *pintype);*/
	static	HRESULT GetPeerFilters(IBaseFilter *pFilter, PIN_DIRECTION Dir, CFilterList &FilterList);  
	static  HRESULT GetNextFilter(IBaseFilter *pFilter, PIN_DIRECTION Dir, IBaseFilter **ppNext);
	static  void AddFilterUnique(CFilterList &FilterList, IBaseFilter *pNew);
	static  HRESULT GetReferenceClock(IBaseFilter *pFilter, IReferenceClock **ppClock);
	HRESULT CheckDemuxPids(void);
	HRESULT	Sleeps(ULONG Duration, long TimeOut[]);
	HRESULT	IsStopped();
	HRESULT	IsPlaying();
	HRESULT	IsPaused();
	HRESULT	DoStop();
	HRESULT	DoStart();
	HRESULT	DoPause();

protected:
	HRESULT UpdateDemuxPins(IBaseFilter* pDemux);
	HRESULT CheckDemuxPin(IBaseFilter* pDemux, AM_MEDIA_TYPE pintype, IPin** pIPin);
	HRESULT CheckVideoPin(IBaseFilter* pDemux);
	HRESULT CheckAudioPin(IBaseFilter* pDemux);
	HRESULT CheckAACPin(IBaseFilter* pDemux);
	HRESULT CheckDTSPin(IBaseFilter* pDemux);
	HRESULT CheckAC3Pin(IBaseFilter* pDemux);
	HRESULT CheckTelexPin(IBaseFilter* pDemux);
	HRESULT CheckSubtitlePin(IBaseFilter* pDemux);
	HRESULT CheckTsPin(IBaseFilter* pDemux);
	HRESULT	NewTsPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewVideoPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewAudioPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewAACPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewAC3Pin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewDTSPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewTelexPin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	NewSubtitlePin(IMpeg2Demultiplexer* muxInterface, LPWSTR pinName);
	HRESULT	LoadTsPin(IPin* pIPin);
	HRESULT	LoadAudioPin(IPin* pIPin, ULONG pid);
	HRESULT	LoadVideoPin(IPin* pIPin, ULONG pid);
	HRESULT	LoadTelexPin(IPin* pIPin, ULONG pid);
	HRESULT	LoadSubtitlePin(IPin* pIPin, ULONG pid);
	HRESULT VetDemuxPin(IPin* pIPin, ULONG pid);
	HRESULT	ClearDemuxPin(IPin* pIPin);
	HRESULT	ChangeDemuxPin(IBaseFilter* pDemux, LPWSTR* pPinName, BOOL* pConnect);
//	HRESULT UpdateNetworkProvider(IBaseFilter* pNetworkProvider);
	HRESULT CheckTIFPin(IBaseFilter* pDemux);
//	HRESULT GetTIFMedia(AM_MEDIA_TYPE *pintype);
	HRESULT RemoveFilterChain(IBaseFilter *pStartFilter, IBaseFilter *pEndFilter);
	HRESULT RenderFilterPin(IPin *pIPin);
	HRESULT ReconnectFilterPin(IPin *pIPin);
	HRESULT SetReferenceClock(IBaseFilter *pFilter);
	long get_PcrPid();
	long get_VideoPid();
	long get_H264Pid();
	long get_Mpeg4Pid();
	long get_AudioPid();
	long get_Audio2Pid();
	long get_AC3Pid();
	long get_AC3_2Pid();
	long get_TeletextPid();
	long get_SubtitlePid();
	long get_AACPid();
	long get_AAC2Pid();
	long get_DTSPid();
	long get_DTS2Pid();

	FilterGraphTools graphTools;
	CFilterList	m_FilterRefList;

	IBaseFilter *m_pTSSourceFilter;
	DVBTChannels_Service* m_pService;

	CCritSec m_DemuxLock;

	BOOL m_bAuto;
	BOOL m_bNPControl;
	BOOL m_bNPSlave;
	BOOL m_bAC3Mode;
	BOOL m_bFixedAR;
	BOOL m_bCreateTSPinOnDemux;
	BOOL m_bCreateTxtPinOnDemux;
	BOOL m_bCreateSubPinOnDemux;
	BOOL m_bMPEG2AudioMediaType;
	BOOL m_bMPEG2Audio2Mode;
	BOOL m_WasPlaying;
	BOOL m_WasPaused;
	int  m_ClockMode;
	LONG m_TimeOut[2];

public:

	BOOL m_bConnectBusyFlag;
	BOOL m_StreamH264;
	BOOL m_StreamMpeg4;
	BOOL m_StreamVid;
	BOOL m_StreamAC3;
	BOOL m_StreamMP2;
	BOOL m_StreamAud2;
	BOOL m_StreamAAC;
	BOOL m_StreamDTS;
	BOOL m_StreamSub;
	int  m_SelAudioPid;
	int  m_SelVideoPid;
	int  m_SelTelexPid;
	int  m_SelSubtitlePid;
};

#endif
