/**
 *	LogMessage.h
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



#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include "StdAfx.h"

class LogMessage
{
public:

	LogMessage();
	virtual ~LogMessage();

	int Show();
	int Show(int returnValue);

	int Write();
	int Write(int returnValue);

	void ClearFile();

	LogMessage& operator<< (const int& val);
	LogMessage& operator<< (const double& val);
	LogMessage& operator<< (const __int64& val);

	LogMessage& operator<< (const char& val);
	LogMessage& operator<< (const wchar_t& val);

	LogMessage& operator<< (const LPSTR& val);
	LogMessage& operator<< (const LPWSTR& val);

	LogMessage& operator<< (const LPCSTR& val);
	LogMessage& operator<< (const LPCWSTR& val);
	
private:
	void WriteLogMessage();

	char str[8192];
};

extern LogMessage g_log;


/*#define LOGMSG(a)		\
{						\
	LogMessage em;		\
	em << ##a;			\
}
#define SHOWMSG(a)		\
{						\
	LogMessage em;		\
	em << ##a;			\
	em.ShowMessage();	\
}
#define return_FALSE_LOGMSG(a)	\
{								\
	{							\
		LogMessage em;			\
		em << ##a;				\
	}							\
	return FALSE;				\
}
#define return_FALSE_SHOWMSG(a)	\
{								\
	{							\
		LogMessage em;			\
		em << ##a;				\
		em.ShowMessage();		\
	}							\
	return FALSE;				\
}
#define LOGERR(a)											\
{															\
	LogMessage em;											\
	char *file = strrchr(__FILE__, '\\');					\
	if (file)												\
		em << file+1 << "(" << __LINE__ << ") : " << ##a;	\
	else													\
		em << ##a;											\
}
#define return_FALSE_LOGERR(a)									\
{																\
	{															\
		LogMessage em;											\
		char *file = strrchr(__FILE__, '\\');					\
		if (file)												\
			em << file+1 << "(" << __LINE__ << ") : " << ##a;	\
		else													\
			em << ##a;											\
	}															\
	return FALSE;												\
}
*/
#endif
