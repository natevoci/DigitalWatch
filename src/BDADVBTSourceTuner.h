/**
 *	BDADVBTSourceTuner.h
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

#ifndef BDADVBTSOURCETUNER_H
#define BDADVBTSOURCETUNER_H

#include "StdAfx.h"
#include <streams.h>
#include <bdatif.h>
#include "BDACardCollection.h"

class BDADVBTSourceTuner
{
public:
	BDADVBTSourceTuner(BDACard *pBDACard);
	virtual ~BDADVBTSourceTuner();

	BOOL Initialise(DWGraph *pDWGraph);
	BOOL DestroyAll();

	BOOL LockChannel(long frequency, long bandwidth);
	long GetCurrentFrequency();

	BOOL GetSignalStats(BOOL &locked, long &strength, long &quality);

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

	long activationRank;
	BOOL lockedAsActiveTuner;

	IBaseFilter*  m_piDWTSRedirect;
	//DWTSRedirect* m_pfDWTSRedirect;

private:
//	BOOL InitTuningSpace();
//	BOOL submitTuneRequest(ITuneRequest* &pExTuneRequest);

	BDACard *m_pBDACard;
	DWGraph *m_pDWGraph;

	BOOL m_bInitialised;
	long m_lFrequency;
	long m_lBandwidth;

	//BOOL m_bRecording;

	VARIANT m_pTuningSpaceIndex;

	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	CComPtr <IBaseFilter> m_piBDANetworkProvider;
	CComPtr <IBaseFilter> m_piBDATuner;
	CComPtr <IBaseFilter> m_piBDACapture;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IBaseFilter> m_piBDATIF;
	CComPtr <IBaseFilter> m_piBDASecTab;
	CComPtr <IBaseFilter> m_piInfinitePinTee;
	CComPtr <IBaseFilter> m_piDSNetworkSink;

	CComPtr <ITuningSpace> m_piTuningSpace;

	/*
	CComPtr <IGuideData> m_piGuideData;
	DWGuideDataEvent* m_poGuideDataEvent;
	*/

	DWORD m_rotEntry;
};

#endif
