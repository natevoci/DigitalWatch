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
#include <vector>

#define PID_TYPE_VIDEO 1

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

struct DVBTChannels_Program_Stream
{
	long PID;
	DVBTChannels_Program_PID_Types Type;	//video, mp2, ac3, data
	BOOL bActive;
};

class DVBTChannels_Program
{
	friend DVBTChannels;
public:
	DVBTChannels_Program();
	virtual ~DVBTChannels_Program();

	DVBTChannels_Program_PID_Types GetStreamType(int index);
	long GetStreamPID(int index);
	long GetStreamPID(DVBTChannels_Program_PID_Types streamtype, int index);


	long GetStreamCount();
	long GetStreamCount(DVBTChannels_Program_PID_Types streamtype);


protected:
	long programNumber;
	LPWSTR name;
	std::vector<DVBTChannels_Program_Stream> streams;
	long favoriteID;
	BOOL bDisableAutoUpdate;
};

class DVBTChannels_Network
{
	friend DVBTChannels;
public:
	DVBTChannels_Network();
	virtual ~DVBTChannels_Network();

	long frequency;
	long bandwidth;
	LPWSTR name;

	DVBTChannels_Program* Program(int programNumber);
	BOOL IsValidProgram(int programNumber);

private:
	std::vector<DVBTChannels_Program *> programs;
};

class DVBTChannels
{
public:
	DVBTChannels();
	virtual ~DVBTChannels();

	BOOL LoadChannels(LPWSTR filename);
	BOOL SaveChannels(LPWSTR filename = NULL);

	DVBTChannels_Network* Network(int networkNumber);
	BOOL IsValidNetwork(int networkNumber);

private:
	std::vector<DVBTChannels_Network *> networks;

	long m_bandwidth;
	LPWSTR m_filename;

	LogMessage log;
};

#endif
