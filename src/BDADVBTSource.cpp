/**
 *	BDADVBTSource.cpp
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

#include "BDADVBTSource.h"
#include "Globals.h"
#include "LogMessage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BDADVBTSource::BDADVBTSource()
{
}

BDADVBTSource::~BDADVBTSource()
{

}

BOOL BDADVBTSource::Initialise(DWGraph* pFilterGraph)
{
	wchar_t file[MAX_PATH];
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Channels.ini", g_pData->application.appPath);
	if (channels.LoadChannels((LPWSTR)&file) == FALSE)
		return FALSE;

	//Get list of BDA capture cards
	swprintf((LPWSTR)&file, L"%sBDA_DVB-T\\Cards.ini", g_pData->application.appPath);
	cardList.LoadCards((LPWSTR)&file);
	cardList.SaveCards();
	if (cardList.cards.size() == 0)
		return (g_log << "Could not find any BDA cards").Show();
	
	std::vector<BDACard *>::iterator it = cardList.cards.begin();
	for ( ; it != cardList.cards.end() ; it++ )
	{
		BDACard *tmpCard = *it;
		if (tmpCard->bActive)
		{
			m_pCurrentTuner = new BDADVBTSourceTuner(tmpCard);
			m_pCurrentTuner->Initialise(pFilterGraph);
			m_Tuners.push_back(m_pCurrentTuner);
		}
	}
	if (m_Tuners.size() == 0)
		return (g_log << "There are no active BDA cards").Show();

	m_pCurrentTuner = m_Tuners.at(0);

	return TRUE;
}

BOOL BDADVBTSource::ExecuteCommand(LPWSTR command)
{
	return FALSE;
}

HRESULT BDADVBTSource::AddFilters()
{
	return 0;
}

HRESULT BDADVBTSource::Connect()
{
	return 0;
}

HRESULT BDADVBTSource::AfterGraphBuilt()
{
	return 0;
}

HRESULT BDADVBTSource::Cleanup()
{
	return 0;
}

HRESULT BDADVBTSource::Destroy()
{
	return 0;
}

