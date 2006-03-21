/**
 *	BDADVBTimeShiftTuner.h
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

#ifndef BDADVBTIMESHIFTTUNER_H
#define BDADVBTIMESHIFTTUNER_H

#include "StdAfx.h"
#include <bdatif.h>
#include "BDACardCollection.h"
#include "DWGraph.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DVBMpeg2DataParser.h"

class BDADVBTimeShift;
class BDADVBTimeShiftTuner : public LogMessageCaller
{
public:
	BDADVBTimeShiftTuner(BDADVBTimeShift *pBDADVBTimeShift, BDACard *pBDACard);
	virtual ~BDADVBTimeShiftTuner();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT Initialise(IGraphBuilder *piGraphBuilder);
	HRESULT DestroyAll();

	HRESULT AddSourceFilters();
	HRESULT DeleteOutputPins(IBaseFilter *pFilter);
	void DestroyFilter(CComPtr <IBaseFilter> &pFilter);
	HRESULT RemoveSourceFilters();

	HRESULT QueryTransportStreamPin(IPin** piPin);

	HRESULT LockChannel(long frequency, long bandwidth);
	long GetCurrentFrequency();

	HRESULT StartScanning();
	
	HRESULT GetSignalStats(BOOL &locked, long &strength, long &quality);

	BOOL IsActive();

	/*
	GetNowAndNext(...);
	*/

	/*
	BOOL SetRecordingFullTS();
	BOOL SetRecordingTSMux(long PIDs[]);
	BOOL StartRecording(LPWSTR filename);
	BOOL PauseRecording(BOOL bPause);
	BOOL StopRecording();
	BOOL IsRecording();
	*/
	BOOL SupportsRecording() { return FALSE; }

	//IBaseFilter*  m_piDWTSRedirect;
	//DWTSRedirect* m_pfDWTSRedirect;

	LPWSTR GetCardName();

private:
	BDADVBTimeShift *m_pBDADVBTimeShift;

	BDACard *m_pBDACard;

	BOOL m_bInitialised;
	BOOL m_bActive;

	long m_lFrequency;
	long m_lBandwidth;

	//BOOL m_bRecording;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	CComPtr <IBaseFilter> m_piBDANetworkProvider;
	CComPtr <IBaseFilter> m_piInfinitePinTee;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IBaseFilter> m_piBDATIF;
	CComPtr <IBaseFilter> m_piBDASecTab;

	CComPtr <ITuningSpace> m_piTuningSpace;

	/*
	CComPtr <IGuideData> m_piGuideData;
	DWGuideDataEvent* m_poGuideDataEvent;
	*/
	DVBMpeg2DataParser *m_pMpeg2DataParser;

	FilterGraphTools graphTools;
};

#endif
