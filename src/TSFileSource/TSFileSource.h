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
#include "LogMessage.h"
#include "FilterGraphTools.h"
#include "DWThread.h"
#include "TSFileStreamList.h"
#include <vector>

class TSFileSource : public DWSource, public DWThread
{
public:
	TSFileSource();
	virtual ~TSFileSource();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual LPWSTR GetSourceType();

	virtual HRESULT Initialise(DWGraph* pFilterGraph);
	virtual HRESULT Destroy();
	virtual HRESULT UnloadFilters();

	virtual HRESULT ExecuteCommand(ParseLine* command);

	virtual HRESULT Start();

	virtual BOOL CanLoad(LPWSTR pCmdLine);
	virtual HRESULT Load(LPWSTR pCmdLine);

	virtual void ThreadProc();

protected:
	// graph building methods
	HRESULT LoadFile(LPWSTR pFilename);

	virtual HRESULT PlayPause();
	virtual HRESULT Skip(long seconds);
	virtual HRESULT SeekTo(long percentage);

	virtual HRESULT UpdateData();
	virtual HRESULT SetStream(long index);
	virtual HRESULT ShowMenu(LPWSTR szMenuName);
	virtual HRESULT ExitMenu(long nNumberOfMenusToExit = 1);
	virtual HRESULT GetStreamList(void);

private:
	const LPWSTR m_strSourceType;

	DWGraph *m_pDWGraph;
	CComPtr <IGraphBuilder> m_piGraphBuilder;

	CComPtr <IBaseFilter> m_piTSFileSource;
	CComPtr <IBaseFilter> m_piBDAMpeg2Demux;
	CComPtr <IMpeg2Demultiplexer> m_piMpeg2Demux;

	TSFileStreamList streamList;

	FilterGraphTools graphTools;
};

#endif
