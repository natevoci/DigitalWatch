/**
 *	DWTSRedirect.h
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

#ifndef DWTSREDIRECT_H
#define DWTSREDIRECT_H

#include <streams.h>

class DWTSRedirectInputPin;

class DWTSRedirectFilter : public CBaseFilter
{
public:
	DWTSRedirectFilter(HRESULT *phr);
	virtual ~DWTSRedirectFilter();

	DECLARE_IUNKNOWN;

	CBasePin * GetPin(int n);
	int GetPinCount();

	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	BOOL m_bRedirecting;

private:
	DWTSRedirectInputPin *m_pPin;

	CCritSec m_Lock;
	CCritSec m_ReceiveLock;

	CPosPassThru *m_pPosition;

};

class DWTSRedirectInputPin : public CRenderedInputPin
{
public:
    DWTSRedirectInputPin(LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr);
	virtual ~DWTSRedirectInputPin();

    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

    HRESULT WriteStringInfo(IMediaSample *pSample);

    HRESULT CheckMediaType(const CMediaType *);

    HRESULT BreakConnect();

    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

private:
	REFERENCE_TIME m_tLast;
	CCritSec * const m_pReceiveLock;
};

#endif
