/**
 *	Globals.h
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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "AppData.h"
#include "TVControl.h"
#include "DWOnScreenDisplay.h"
#include "LogMessageWriter.h"

#define reminderStringize( L )		#L
#define reminderMakeString( M, L )	M(L)
#define reminder(A)					message(__FILE__ "(" reminderMakeString( reminderStringize, __LINE__ ) ") : Reminder: " A)

extern AppData* g_pData;
extern TVControl* g_pTv;
extern DWOnScreenDisplay* g_pOSD;
extern LogMessageWriter g_DWLogWriter;


#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif

#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif

#ifndef FILE_START_POS_TS
#define FILE_START_POS_TS 500000 //Minimum file position to begin parsing for TS Mode
#endif

#ifndef RT_FILE_START_TS
#define RT_FILE_START_TS 1000000 //0.1sec stream position to begin playing in TS Mode
#endif

#ifndef FILE_START_POS_PS
#define FILE_START_POS_PS 0 //Minimum file position to begin parsing for PS Mode
#endif

#ifndef RT_FILE_START_PS
#define RT_FILE_START_PS 0 //0sec stream position to begin playing in PS Mode
#endif

#ifndef MIN_FILE_SIZE
#define MIN_FILE_SIZE 2000000 //Minimum filesize to parse
#endif

#ifndef RT_SECOND
#define RT_SECOND 10000000 //1sec
#endif

#ifndef RT_2_SECOND
#define RT_2_SECOND 20000000 //2 sec
#endif

/**********************************************
 *
 *  CBufferInfo Class
 *
 **********************************************/

class CBufferInfo
{
public:
	CBufferInfo()
	{
		sample = NULL;
		long size = 0;
	};

	virtual ~CBufferInfo()
	{
		if(sample)
			delete[] sample;
	};

	BYTE *sample;
	long size;
};

class BoostThread
{
public:
	BoostThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL - 1);
		   }
		#endif
   }

   ~BoostThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
     
   int m_nPriority;
};

class LowBoostThread
{
public:
   LowBoostThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL - 2);
		   }
		#endif
   }

   ~LowBoostThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
     
   int m_nPriority;
};

class HighestThread
{
public:
   HighestThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		   }
		#endif
   }

   ~HighestThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
      
   int m_nPriority;
};

class AbnormalThread
{
public:
   AbnormalThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		   }
		#endif
   }

   ~AbnormalThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
      
   int m_nPriority;
};

class NormalThread
{
public:
   NormalThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		   }
		#endif
   }

   ~NormalThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
      
   int m_nPriority;
};

class BrakeThread
{
public:
   BrakeThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				m_nPriority = GetThreadPriority(GetCurrentThread());
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
		   }
		#endif
   }

   ~BrakeThread()
   {
		#ifndef DEBUG
		   if ((int)GetPriorityClass(GetCurrentProcess()) != IDLE_PRIORITY_CLASS)
		   {
				SetThreadPriority(GetCurrentThread(), m_nPriority);
		   }
		#endif
   }
     
   int m_nPriority;
};

class BoostProcess
{
public:
   BoostProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		#endif
   }

   ~BoostProcess()
   {
		#ifndef DEBUG
			SetPriorityClass(GetCurrentProcess(), m_nPriority);
		#endif
   }
     
   DWORD m_nPriority;
};

class HighProcess
{
public:
   HighProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		#endif
   }

   ~HighProcess()
   {
		#ifndef DEBUG
			SetPriorityClass(GetCurrentProcess(), m_nPriority);
		#endif
   }
      
   DWORD m_nPriority;
};

class AbnormalProcess
{
public:
   AbnormalProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
		#endif
   }

   ~AbnormalProcess()
   {
		#ifndef DEBUG
			SetThreadPriority(GetCurrentThread(), m_nPriority);
		#endif
   }
      
   DWORD m_nPriority;
};

class NormalProcess
{
public:
   NormalProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		#endif
   }

   ~NormalProcess()
   {
		#ifndef DEBUG
			SetPriorityClass(GetCurrentProcess(), m_nPriority);
		#endif
   }
      
   DWORD m_nPriority;
};

class BelowNormalProcess
{
public:
   BelowNormalProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		#endif
   }

   ~BelowNormalProcess()
   {
		#ifndef DEBUG
			SetPriorityClass(GetCurrentProcess(), m_nPriority);
		#endif
   }
      
   DWORD m_nPriority;
};

class IdleProcess
{
public:
   IdleProcess()
   {
		#ifndef DEBUG
			m_nPriority = GetPriorityClass(GetCurrentProcess());
			SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
		#endif
   }

   ~IdleProcess()
   {
		#ifndef DEBUG
			SetPriorityClass(GetCurrentProcess(), m_nPriority);
		#endif
   }
     
   DWORD m_nPriority;
};

#endif

