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
#include "DVBTChannels.h"
#include "BDACardCollection.h"
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include <vector>

class BDADVBTSource : public DWSource
{
public:
	BDADVBTSource();
	virtual ~BDADVBTSource();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual LPWSTR GetSourceType();

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT Destroy();

	virtual HRESULT ExecuteCommand(ParseLine* command);
	//Keys, ControlBar, OSD, Menu, etc...

	virtual HRESULT Play();

protected:
	HRESULT LoadTuner();
	HRESULT UnloadTuner();

	HRESULT AddDemuxPins(DVBTChannels_Program* program);

	HRESULT AddDemuxPins(DVBTChannels_Program* program, DVBTChannels_Program_PID_Types streamType, LPWSTR pPinName, AM_MEDIA_TYPE *pMediaType, long *streamsRendered = NULL);
		
	HRESULT AddDemuxPinsVideo(DVBTChannels_Program* program, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsMp2(DVBTChannels_Program* program, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsAC3(DVBTChannels_Program* program, long *streamsRendered = NULL);
	HRESULT AddDemuxPinsTeletext(DVBTChannels_Program* program, long *streamsRendered = NULL);


	virtual HRESULT SetChannel(int network, int program);
	virtual HRESULT NetworkUp();
	virtual HRESULT NetworkDown();
	virtual HRESULT ProgramUp();
	virtual HRESULT ProgramDown();

private:
	const LPWSTR m_strSourceType;

	BDADVBTSourceTuner *m_pCurrentTuner;
	std::vector<BDADVBTSourceTuner *> m_Tuners;
	//Recorder
	DVBTChannels channels;
	BDACardCollection cardList;
	//NaN

	DWGraph *m_pDWGraph;
	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;

	FilterGraphTools graphTools;
};

#endif
