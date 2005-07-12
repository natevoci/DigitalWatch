/**
 *	DVBTChannels.h
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


#ifndef DVBTCHANNELS_H
#define DVBTCHANNELS_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include "IDWOSDDataList.h"
#include <vector>

class DVBTChannels;
class DVBTChannels_Network;

enum DVBTChannels_Service_PID_Types
{
	unknown,
	video,
	mp2,
	ac3,
	teletext,
	DVBTChannels_Service_PID_Types_Count
};

static const LPWSTR DVBTChannels_Service_PID_Types_String[] =
{
	L"Unknown",
	L"Video",
	L"MPEG-2 Audio",
	L"AC3 Audio",
	L"Teletext"
};

//Stream
class DVBTChannels_Stream : public LogMessageCaller
{
public:
	DVBTChannels_Stream();
	virtual ~DVBTChannels_Stream();

	void UpdateStream(DVBTChannels_Stream *pNewStream);
	void PrintStreamDetails();

	long PID;
	DVBTChannels_Service_PID_Types Type;
	//char Lang[4];
	LPWSTR Language;
	BOOL bActive;
	BOOL bDetected;
};

//Service
class DVBTChannels_Service : public LogMessageCaller
{
	friend DVBTChannels;
public:
	DVBTChannels_Service();
	virtual ~DVBTChannels_Service();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT LoadFromXML(XMLElement *pElement);
	HRESULT SaveToXML(XMLElement *pElement);

	DVBTChannels_Service_PID_Types GetStreamType(int index);
	long GetStreamPID(int index);
	long GetStreamPID(DVBTChannels_Service_PID_Types streamtype, int index);

	long GetStreamCount();
	long GetStreamCount(DVBTChannels_Service_PID_Types streamtype);

	BOOL UpdateService(DVBTChannels_Service *pNewService);
	BOOL UpdateStreams(DVBTChannels_Service *pNewService);
	void PrintServiceDetails();

	DVBTChannels_Stream *FindStreamByPID(long PID);

public:
	long serviceId;
	long logicalChannelNumber;
	LPWSTR serviceName;

protected:
	std::vector<DVBTChannels_Stream *> m_streams;
	CCritSec  m_streamsLock;
	long favoriteID;
	BOOL bManualUpdate;
};

//Network
class DVBTChannels_Network : public LogMessageCaller, public IDWOSDDataList
{
	friend DVBTChannels;
public:
	DVBTChannels_Network(DVBTChannels *pChannels);
	virtual ~DVBTChannels_Network();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT LoadFromXML(XMLElement *pElement);
	HRESULT SaveToXML(XMLElement *pElement);

	DVBTChannels_Service *FindDefaultService();
	DVBTChannels_Service *FindServiceByServiceId(long serviceId);
	DVBTChannels_Service *FindNextServiceByServiceId(long serviceId);
	DVBTChannels_Service *FindPrevServiceByServiceId(long serviceId);

	BOOL UpdateNetwork(DVBTChannels_Network *pNewNetwork);
	void PrintNetworkDetails();

	//IDWOSDDataList Methods
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex);
	virtual long GetListSize();

public:
	long originalNetworkId;
	long transportStreamId;
	long networkId;
	long frequency;
	long frequencyInStream;
	long bandwidth;
	LPWSTR networkName;

protected:
	DVBTChannels *m_pChannels;

	std::vector<DVBTChannels_Service *> m_services;
	CCritSec  m_servicesLock;
	LPWSTR m_dataListString;
};

//Channels
class DVBTChannels : public LogMessageCaller, public IDWOSDDataList
{
public:
	DVBTChannels();
	virtual ~DVBTChannels();

	virtual void SetLogCallback(LogMessageCallback *callback);
	HRESULT Destroy();

	HRESULT LoadChannels(LPWSTR filename);
	HRESULT SaveChannels(LPWSTR filename = NULL);

	long get_DefaultBandwidth();

	DVBTChannels_Network *FindDefaultNetwork();
	DVBTChannels_Network *FindNetwork(long originalNetworkId, long transportStreamId, long networkId);
	DVBTChannels_Network *FindNetworkByONID(long originalNetworkId);
	DVBTChannels_Network *FindNetworkByFrequency(long frequency);
	DVBTChannels_Network *FindNextNetworkByOriginalNetworkId(long oldOriginalNetworkId);
	DVBTChannels_Network *FindPrevNetworkByOriginalNetworkId(long oldOriginalNetworkId);
	DVBTChannels_Network *FindNextNetworkByFrequency(long oldFrequency);
	DVBTChannels_Network *FindPrevNetworkByFrequency(long oldFrequency);

	//Update Methods
	BOOL UpdateNetwork(DVBTChannels_Network *pNewNetwork);

	HRESULT MoveNetworkUp(long originalNetworkId);
	HRESULT MoveNetworkDown(long originalNetworkId);

	//IDWOSDDataList Methods
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex);
	virtual long GetListSize();

protected:
	std::vector<DVBTChannels_Network *> m_networks;
	CCritSec  m_networksLock;
	long m_bandwidth;
	LPWSTR m_filename;
	LPWSTR m_dataListString;
};

#endif
