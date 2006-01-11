/**
 *	DWSource.cpp
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

#include "stdafx.h"
#include "DWThread.h"
#include <process.h>

/*
void DWThreadThreadProc(void *pParam)
{
	((DWThread *)pParam)->InternalThreadProc();
}
*/

//////////////////////////////////////////////////////////////////////
// DWSource
//////////////////////////////////////////////////////////////////////

DWThread::DWThread()
{
	m_hStopEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_hDoneEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_threadHandle = INVALID_HANDLE_VALUE;
}

DWThread::~DWThread()
{
	StopThread();
	CloseHandle(m_hStopEvent);
	CloseHandle(m_hDoneEvent);
}

HRESULT DWThread::StartThread()
{
	ResetEvent(m_hStopEvent);
	unsigned long m_threadHandle = _beginthread(&DWThread::thread_function, 0, (void *) this);
	if (m_threadHandle == (unsigned long)INVALID_HANDLE_VALUE)
		return E_FAIL;

	return S_OK;
}

HRESULT DWThread::StopThread(DWORD dwTimeoutMilliseconds)
{
	HRESULT hr = S_OK;

	SetEvent(m_hStopEvent);
	DWORD result = WaitForSingleObject(m_hDoneEvent, dwTimeoutMilliseconds);

	if ((result == WAIT_TIMEOUT) && (m_threadHandle != INVALID_HANDLE_VALUE))
	{
		TerminateThread(m_threadHandle, -1);
		CloseHandle(m_threadHandle);
		hr = S_FALSE;
	}
	else if (result != WAIT_OBJECT_0)
	{
		DWORD err = GetLastError();
		return HRESULT_FROM_WIN32(err);
	}

	m_threadHandle = INVALID_HANDLE_VALUE;

	return hr;
}

BOOL DWThread::ThreadIsStopping()
{
	DWORD result = WaitForSingleObject(m_hStopEvent, 10);
	return (result != WAIT_TIMEOUT);
}

void DWThread::InternalThreadProc()
{
	ResetEvent(m_hDoneEvent);
	try
	{
		ThreadProc();
	}
	catch (LPWSTR pStr)
	{
		pStr = NULL;
	}
	SetEvent(m_hDoneEvent);
}

void DWThread::thread_function(void* p)
{
	DWThread *thread = reinterpret_cast<DWThread *>(p);
	thread->InternalThreadProc();
}
