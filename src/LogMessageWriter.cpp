/**
 *	LogMessageWriter.cpp
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

#include "LogMessageWriter.h"
#include "GlobalFunctions.h"
#include "LogFileWriter.h"

//////////////////////////////////////////////////////////////////////
// LogMessageWriter
//////////////////////////////////////////////////////////////////////

LogMessageWriter::LogMessageWriter()
{
	m_logFilename = NULL;
}

LogMessageWriter::~LogMessageWriter()
{
	CAutoLock logFileLock(&m_logFileLock);

	if (m_logFilename)
		delete[] m_logFilename;
}

void LogMessageWriter::SetFilename(LPWSTR filename)
{
	CAutoLock logFileLock(&m_logFileLock);

	if ((wcslen(filename) > 2) &&
		((filename[1] == ':') ||
		 (filename[0] == '\\' && filename[1] == '\\')
		)
	   )
	{
		strCopy(m_logFilename, filename);
	}
	else
	{
		LPWSTR str = new wchar_t[MAX_PATH];
		GetCommandPath(str);
		swprintf(str, L"%s%s", str, filename);
		strCopy(m_logFilename, str);
		delete[] str;
	}
}

void LogMessageWriter::Write(LPWSTR pStr)
{
	CAutoLock logFileLock(&m_logFileLock);

	USES_CONVERSION;
	if (m_logFilename)
	{
		LogFileWriter file;
		if SUCCEEDED(file.Open(m_logFilename, TRUE))
		{
			for ( int i=0 ; i<m_indent ; i++ )
			{
				file << "  ";
			}

			//Write one line at a time so we can do dos style EOL's
			LPWSTR pCurr = pStr;
			while (pCurr[0] != '\0')
			{
				LPWSTR pEOL = wcschr(pCurr, '\n');
				if (pEOL)
				{
					pEOL[0] = '\0';
					file << pCurr << file.EOL;
					pEOL[0] = '\n';
					pCurr = pEOL + 1;
				}
				else
				{
					file << pCurr;
					break;
				}
			}

			file.Close();
		}
	}
}

void LogMessageWriter::Clear()
{
	CAutoLock logFileLock(&m_logFileLock);

	USES_CONVERSION;
	if (m_logFilename)
	{
		LogFileWriter file;
		if SUCCEEDED(file.Open(m_logFilename, FALSE))
		{
			file.Close();
		}
	}
}

