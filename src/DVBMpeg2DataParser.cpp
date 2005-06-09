/**
 *	DVBMpeg2DataParser.cpp
 *	Copyright (C) 2004, 2005 Nate
 *  Copyright (C) 2004 JoeyBloggs
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

#include "stdafx.h"
#include "DVBMpeg2DataParser.h"

#include <process.h>
#include <math.h>

void ParseMpeg2DataThread(void *pParam)
{
	DVBMpeg2DataParser *scanner;
	scanner = (DVBMpeg2DataParser *)pParam;
	scanner->StartScanThread();
}

//////////////////////////////////////////////////////////////////////
// DVBSection
//////////////////////////////////////////////////////////////////////
DVBSection::DVBSection()
{
	segmented = 0;
	fd = -1;
	pid = -1;
	table_id = -1;
	table_id_ext = -1;
	section_version_number = -1;
	memset (section_done, 0, 32 * sizeof(__int8));
	sectionfilter_done = 0;
	buf = NULL;

	timeout = 0;
	start_time = 0;
	running_time = 0;
	next_segment = NULL;
}

DVBSection::~DVBSection()
{
	if (buf)
		delete[] buf;
	if (next_segment)
		delete next_segment;
}

void DVBSection::Setup(long pid, long table_id, long segmented, int timeout)
{
	this->pid = pid;
	this->table_id = table_id;
	this->segmented = segmented;
	this->timeout = timeout;
}

//////////////////////////////////////////////////////////////////////
// DVBMpeg2DataParser
//////////////////////////////////////////////////////////////////////

DVBMpeg2DataParser::DVBMpeg2DataParser()
{
	m_hScanningDoneEvent = CreateEvent(NULL, TRUE, FALSE, "ScanningDone");
	m_piMpeg2Data = NULL;
	m_pDVBTChannels = NULL;
	m_bThreadStarted = FALSE;
}

DVBMpeg2DataParser::~DVBMpeg2DataParser()
{
}

void DVBMpeg2DataParser::SetDVBTChannels(DVBTChannels *pChannels)
{
	m_pDVBTChannels = pChannels;
}

void DVBMpeg2DataParser::SetFilter(CComPtr <IBaseFilter> pBDASecTab)
{
	m_piMpeg2Data.Release();

	if (pBDASecTab != NULL)
	{
		HRESULT hr = pBDASecTab->QueryInterface(__uuidof(IMpeg2Data), reinterpret_cast<void**>(&m_piMpeg2Data));
	}
}

void DVBMpeg2DataParser::ReleaseFilter()
{
	m_piMpeg2Data.Release();
}

//////////////////////////////////////////////////////////////////////
// Method just to kick off a separate thread
//////////////////////////////////////////////////////////////////////
HRESULT DVBMpeg2DataParser::StartScan()
{
	//Start a new thread to do the scanning because the 
	//IGuideDataEvent::ServiceChanged method isn't allowed to block.

	//Make sure we only start scanning the first time the ServiceChanged event calls us.
	if (m_bThreadStarted)
		return S_FALSE;
	m_bThreadStarted = TRUE;

	unsigned long result = _beginthread(ParseMpeg2DataThread, 0, (void *) this);
	if (result == -1L)
	{
		m_bThreadStarted = FALSE;
		return E_FAIL;
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// The real work starts here
//////////////////////////////////////////////////////////////////////
void DVBMpeg2DataParser::StartScanThread()
{
	try
	{
		if (m_piMpeg2Data)
		{
			DVBSection *sect;
			sect = new DVBSection();
			sect->Setup(0x00, 0x00, 0, 5);	// PAT
			waiting_filters.push_back(sect);

			sect = new DVBSection();
			sect->Setup(0x10, 0x40, 0, 15);	// NIT
			waiting_filters.push_back(sect);

			sect = new DVBSection();
			sect->Setup(0x11, 0x42, 0, 5);	// SDT
			waiting_filters.push_back(sect);

			while (waiting_filters.size())
			{
				sect = waiting_filters.front();
			}
		}
	}
	catch (...)
	{
		(log << "Unhandled exception in DVBMpeg2DataParser::StartScanThread()\n");
	}
	m_bThreadStarted = FALSE;
	SetEvent(m_hScanningDoneEvent);

}
