/**
 *	LogMessage.cpp
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

#include "LogMessage.h"
#include "Globals.h"

#if (_MSC_VER == 1200)
	#include <fstream.h>
#else
	#include <fstream>
	using namespace std;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LogMessage::LogMessage()
{
	str[0] = 0;
}

LogMessage::~LogMessage()
{
	if (str[0] != 0)
		WriteLogMessage();
}

void LogMessage::WriteLogMessage()
{
	USES_CONVERSION;

	if (g_pData->settings.application.logFilename)
	{
		ofstream file;
		file.open(W2A(g_pData->settings.application.logFilename), ios::app);

		if (file.is_open() == 1)
		{
			file << str << endl;
			file.close();
		}
	}
	
	str[0] = 0;
}

int LogMessage::Write()
{
	return Write(FALSE);
}

int LogMessage::Write(int returnValue)
{
	if (str[0] != 0)
		WriteLogMessage();

	return returnValue;
}

int LogMessage::Show()
{
	return Show(FALSE);
}

int LogMessage::Show(int returnValue)
{
	USES_CONVERSION;

	//TODO: check for OSD to display message. if no OSD then show messagebox
	MessageBox(g_pData->hWnd, A2T(str), "DigitalWatch", MB_OK);

	if (str[0] != 0)
		WriteLogMessage();

	return returnValue;
}

void LogMessage::ClearFile()
{
	USES_CONVERSION;

	if (g_pData->settings.application.logFilename)
	{
		ofstream file;
		file.open(W2A(g_pData->settings.application.logFilename));

		if (file.is_open() == 1)
		{
			//file << str << endl;
			file.close();
		}
	}
}

//Numbers
LogMessage& LogMessage::operator<< (const int& val)
{
	sprintf((char*)str, "%s%i", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const double& val)
{
	sprintf((char*)str, "%s%f", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const __int64& val)
{
	sprintf((char*)str, "%s%i", str, val);
	return *this;
}

//Characters
LogMessage& LogMessage::operator<< (const char& val)
{
	sprintf((char*)str, "%s%c", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const wchar_t& val)
{
	sprintf((char*)str, "%s%C", str, val);
	return *this;
}

//Strings
LogMessage& LogMessage::operator<< (const LPSTR& val)
{
	sprintf((char*)str, "%s%s", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const LPWSTR& val)
{
	sprintf((char*)str, "%s%S", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const LPCSTR& val)
{
	sprintf((char*)str, "%s%s", str, val);
	return *this;
}
LogMessage& LogMessage::operator<< (const LPCWSTR& val)
{
	sprintf((char*)str, "%s%S", str, val);
	return *this;
}
