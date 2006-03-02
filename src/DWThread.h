/**
 *	DWSource.h
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

#ifndef DWTHREAD_H
#define DWTHREAD_H

class DWThread  
{
public:
	DWThread();
	virtual ~DWThread();

	virtual void ThreadProc() = 0;
	HRESULT StartThread();
	HRESULT StopThread(DWORD dwTimeoutMilliseconds = 1000);

	BOOL ThreadIsStopping();

protected:
	void InternalThreadProc();

private:
	HANDLE m_hDoneEvent;
	HANDLE m_hStopEvent;
	HANDLE m_threadHandle;
	static void thread_function(void* p);
};

#endif