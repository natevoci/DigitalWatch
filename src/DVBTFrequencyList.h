/**
 *	DVBTFrequencyList.h
 *	Copyright (C) 2005 Nate
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

#ifndef DVBTFREQUENCYLIST_H
#define DVBTFREQUENCYLIST_H

#include "IDWOSDDataList.h"
#include "LogMessage.h"
#include <vector>

class DVBTFrequencyListItem
{
public:
	DVBTFrequencyListItem();
	virtual ~DVBTFrequencyListItem();

	LPWSTR frequencyLow;
	LPWSTR frequencyCentre;
	LPWSTR frequencyHigh;
	LPWSTR bandwidth;
};

class DVBTFrequencyList : public LogMessageCaller, public IDWOSDDataList
{
public:
	DVBTFrequencyList();
	virtual ~DVBTFrequencyList();

	virtual HRESULT LoadFrequencyList(LPWSTR filename);
	virtual HRESULT ChangeOffset(long change);

	virtual LPWSTR GetListItem(LPWSTR name, long nIndex);
	virtual long GetListSize();
private:
	std::vector<DVBTFrequencyListItem *> m_list;
	long m_offset;
	LPWSTR m_tmpString;
};

#endif
