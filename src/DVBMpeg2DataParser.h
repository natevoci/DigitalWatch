/**
 *	DVBMpeg2DataParser.h
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

#ifndef DVBMPEG2DATAPARSER_H
#define DVBMPEG2DATAPARSER_H

#include "LogMessage.h"
#include "DVBTChannels.h"
#include <mpeg2data.h>
#include <mpeg2bits.h>
#include <vector>

enum running_mode
{
	RM_NOT_RUNNING = 0x01,
	RM_STARTS_SOON = 0x02,
	RM_PAUSING     = 0x03,
	RM_RUNNING     = 0x04
};

class DVBSection
{
public:
	DVBSection();
	virtual ~DVBSection();
	void Setup(long pid, long table_id, long segmented, int timeout);

	//unsigned long run_once : 1;
	unsigned long segmented : 1;
	long fd;
	long pid;
	long table_id;
	long table_id_ext;
	long section_version_number;
	__int8 section_done[32];
	long sectionfilter_done;
	LPWSTR buf;

	time_t timeout;
	time_t start_time;
	time_t running_time;
	DVBSection *next_segment;
};

class DVBServiceAudio
{
public:
	long pid;
	char land[4];
};

class DVBService
{
public:
	long transport_stream_id;
	long service_id;
	LPWSTR provider_name;
	LPWSTR service_name;
	long pmt_pid;
	long pcr_pid;
	long video_pid;
	std::vector<DVBServiceAudio> audio_pids;
	std::vector<long> ca_id;
	long teletext_pid;
	long subtitling_pid;
	unsigned char type;
	unsigned char scrambled : 1;
	enum running_mode running;
	DVBSection *section; //priv
	long channel_num;
};

class DVBTransponder
{
public:
	std::vector<DVBService *> services;
	long network_id;
	long transport_stream_id;
	long original_network_id;
	// only for DVB-S
	//enum fe_type type;
	//enum polarisation polarisation;
	//long orbital_pos;
	//unsigned long we_flag : 1;
	
	//unsigned char scan_done : 1;
	unsigned char last_tuning_failed : 1;
	unsigned char other_frequency_flag : 1;
	std::vector<long> other_frequencys;
	LPWSTR network_name;
	long frequency;
	long bandwidth;
};

class DVBMpeg2DataParser : public LogMessageCaller
{
public:
	DVBMpeg2DataParser();
	virtual ~DVBMpeg2DataParser();

	void SetDVBTChannels(DVBTChannels *pChannels);
	void SetFilter(CComPtr <IBaseFilter> pBDASecTab);
	void ReleaseFilter();

	HRESULT StartScan();
	void StartScanThread();

private:

private:

	DVBTChannels *m_pDVBTChannels;
	CComPtr <IMpeg2Data> m_piMpeg2Data;
	HANDLE m_hScanningDoneEvent;

	BOOL m_bThreadStarted;

	vector<DVBSection *> waiting_filters;

};

#endif
