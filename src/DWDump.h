/**
 *	DWDump.h
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

#ifndef DWDUMP_H
#define DWDUMP_H
#include <vector>
#include "DWThread.h"
	
typedef struct BufferInfo
{
	BYTE *sample;
	long size;

} BUFFERINFO;


class DWDump;

class DWDumpFilter : public CBaseFilter
{
public:
	DWDumpFilter(DWDump *pDump, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr);
	virtual ~DWDumpFilter();

	CBasePin * GetPin(int n);
	int GetPinCount();

	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

private:
	DWDump* const m_pDump;
};

class DWDumpInputPin : public CRenderedInputPin, public DWThread
{
    DWDump   * const m_pDump;           // Main renderer object
    CCritSec * const m_pReceiveLock;    // Sample critical section
    REFERENCE_TIME m_tLast;             // Last sample receive time

public:
    DWDumpInputPin(DWDump *pDump, LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr);
	virtual ~DWDumpInputPin();

    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

    HRESULT WriteStringInfo(IMediaSample *pSample);

    HRESULT CheckMediaType(const CMediaType *);

    HRESULT BreakConnect();
	HRESULT Run(REFERENCE_TIME tStart);

    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	virtual void ThreadProc();
	void Clear();

private:

	std::vector<BUFFERINFO*> m_Array;
	__int64 m_writeBufferSize;
	CCritSec m_BufferLock;

	HRESULT Filter(byte* rawData,long len);
	HRESULT WriteBufferSample(byte* pbData,long sampleLen);
	BYTE  m_restBuffer[4096];
	long  m_restBufferLen;
	long  m_writeBufferLen;
	BYTE*  m_writeBuffer;
	__int64 m_PacketErrors;


	long  m_WriteBufferSize;
	void PrintLongLong(LPCTSTR lstring, __int64 value);
	BOOL m_WriteThreadActive;
	int debugcount;

};

class DWDump : public CUnknown, public IFileSinkFilter
{
    friend class DWDumpFilter;
    friend class DWDumpInputPin;

    DWDumpFilter   *m_pFilter;
    DWDumpInputPin *m_pPin;

    CCritSec m_Lock;
    CCritSec m_ReceiveLock;

    CPosPassThru *m_pPosition;

    HANDLE   m_hFile;
    LPOLESTR m_pFileName;
    BOOL     m_fWriteError;
	BOOL     m_bPaused;

public:

    DECLARE_IUNKNOWN

    DWDump(HRESULT *phr);
    virtual ~DWDump();

    HRESULT Write(PBYTE pbData, LONG lDataLength);
    STDMETHODIMP SetFileName(LPCOLESTR pszFileName,const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt);

	HRESULT Record();
	HRESULT StopRecording();
	HRESULT Pause();
	BOOL IsRecording();
	BOOL IsPaused();
private:
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    HRESULT OpenFile();
    HRESULT CloseFile();
};



#endif
