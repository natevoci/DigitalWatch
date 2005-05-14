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

enum DVBTChannels_Program_PID_Types
{
	unknown,
	video,
	mp2,
	ac3,
	teletext
};

//Stream
struct DVBTChannels_Program_Stream
{
	long PID;
	DVBTChannels_Program_PID_Types Type;	//video, mp2, ac3, data
	BOOL bActive;
};

//Program
class DVBTChannels_Program : public LogMessageCaller
{
	friend DVBTChannels;
public:
	DVBTChannels_Program();
	virtual ~DVBTChannels_Program();

	HRESULT LoadFromXML(XMLElement *pElement);
	HRESULT SaveToXML(XMLElement *pElement);

	DVBTChannels_Program_PID_Types GetStreamType(int index);
	long GetStreamPID(int index);
	long GetStreamPID(DVBTChannels_Program_PID_Types streamtype, int index);


	long GetStreamCount();
	long GetStreamCount(DVBTChannels_Program_PID_Types streamtype);

	long programNumber;
	long logicalChannelNumber;
	LPWSTR name;

protected:
	std::vector<DVBTChannels_Program_Stream> streams;
	long favoriteID;
	BOOL bManualUpdate;
};

//Network
class DVBTChannels_Network : public LogMessageCaller, public IDWOSDDataList
{
	friend DVBTChannels;
public:
	DVBTChannels_Network();
	virtual ~DVBTChannels_Network();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT LoadFromXML(XMLElement *pElement);
	HRESULT SaveToXML(XMLElement *pElement);

	long frequency;
	long bandwidth;
	LPWSTR name;

	BOOL IsValidProgram(int programNumber);
	DVBTChannels_Program* GetCurrentProgram();
	long GetCurrentProgramId();
	HRESULT SetCurrentProgramId(long nProgram);

	long GetNextProgramId();
	long GetPrevProgramId();

	//IDWOSDDataList Methods
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex);
	virtual long GetListSize();

private:
	std::vector<DVBTChannels_Program *> programs;

	long m_nCurrentProgram;

	LPWSTR m_dataListString;
};

//Channels
class DVBTChannels : public LogMessageCaller, public IDWOSDDataList
{
public:
	DVBTChannels();
	virtual ~DVBTChannels();

	virtual void SetLogCallback(LogMessageCallback *callback);

	HRESULT LoadChannels(LPWSTR filename);
	HRESULT SaveChannels(LPWSTR filename = NULL);

	BOOL IsValidNetwork(int networkNumber);
	DVBTChannels_Network* GetCurrentNetwork();
	long GetCurrentNetworkId();
	HRESULT SetCurrentNetworkId(long nNetwork);

	long GetNextNetworkId();
	long GetPrevNetworkId();

	//IDWOSDDataList Methods
	virtual LPWSTR GetListItem(LPWSTR name, long nIndex);
	virtual long GetListSize();

private:
	std::vector<DVBTChannels_Network *> networks;

	long m_bandwidth;
	LPWSTR m_filename;

	long m_nCurrentNetwork;

	LPWSTR m_dataListString;
};

#endif
