/**
 *	DWGraph.h
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

#ifndef DWGRAPH_H
#define DWGRAPH_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "DWMediaTypes.h"
#include "DWDecoders.h"
#include "FilterGraphTools.h"
#include "DWOnScreenDisplay.h"

class DWGraph : public LogMessageCaller
{
public:
	DWGraph();
	virtual ~DWGraph();

	virtual void SetLogCallback(LogMessageCallback *callback);

	BOOL Initialise();
	BOOL Destroy();

	HRESULT QueryGraphBuilder(IGraphBuilder** piGraphBuilder);
	HRESULT QueryMediaControl(IMediaControl** piMediaControl);
	
	HRESULT Start();
	HRESULT Stop();
	HRESULT Pause(BOOL bPause);

	HRESULT Cleanup();

	HRESULT RenderPin(IPin *piPin);

	HRESULT RefreshVideoPosition();

	HRESULT GetVolume(long &volume);
	HRESULT SetVolume(long volume);
	HRESULT Mute(BOOL bMute);

	HRESULT SetColorControls(int nBrightness, int nContrast, int nHue, int nSaturation, int nGamma);

	void GetVideoRect(RECT *rect);

	BOOL IsPlaying();
	BOOL IsPaused();

protected:
	void CalculateVideoRect(double aspectRatio = 0);
	HRESULT ApplyColorControls();

	HRESULT InitialiseVideoPosition();

private:
	CComPtr <IGraphBuilder> m_piGraphBuilder;
	CComPtr <IMediaControl> m_piMediaControl;

	BOOL m_bInitialised;
	BOOL m_bPlaying;
	BOOL m_bPaused;

	RECT m_videoRect;

	DWORD m_rotEntry;

	DWMediaTypes m_mediaTypes;
	DWDecoders m_decoders;

	FilterGraphTools graphTools;
};

#endif
