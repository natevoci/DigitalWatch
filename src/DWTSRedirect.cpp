/**
 *	DWTSRedirect.cpp
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

#include "DWTSRedirect.h"

//////////////////////////////////////////////////////////////////////
// DWTSRedirectFilter
//////////////////////////////////////////////////////////////////////

DWTSRedirectFilter::DWTSRedirectFilter(HRESULT *phr) : CBaseFilter(NAME("DWTSRedirectFilter"), GetOwner(), &m_Lock, GUID_NULL)
{
	ASSERT(phr);

	m_pPin = new DWTSRedirectInputPin(GetOwner(), this, &m_Lock, &m_ReceiveLock, phr)
	if (m_pPin == NULL)
	{
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}
}

DWTSRedirectFilter::~DWTSRedirectFilter()
{
	if (m_pPin) delete m_pPin;
	if (m_pPosition) delete m_pPosition;
}

CBasePin * DWTSRedirectFilter::GetPin(int n)
{
	if (n == 0)
		return m_pPin;
	return NULL;
}

STDMETHODIMP DWTSRedirectFilter::GetPinCount()
{
	return 1;
}

STDMETHODIMP DWTSRedirectFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_Lock);

	return CBaseFilter::Run(tStart);
}

STDMETHODIMP DWTSRedirectFilter::Pause()
{
    CAutoLock cObjectLock(m_Lock);

	return CBaseFilter::Pause();
}

STDMETHODIMP DWTSRedirectFilter::Stop()
{
	CAutoLock cObjectLock(m_Lock);

	m_bRedirecting = FALSE;

	return CBaseFilter::Stop();
}


//////////////////////////////////////////////////////////////////////
// DWDumpInputPin
//////////////////////////////////////////////////////////////////////

DWTSRedirectInputPin::DWTSRedirectInputPin(LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr)
  : CRenderedInputPin(NAME("DWTSRedirectInputPin"), pFilter, pLock, phr, L"Input"),
    m_pReceiveLock(pReceiveLock),
    m_tLast(0)
{
}

DWTSRedirectInputPin::~DWTSRedirectInputPin()
{
}

HRESULT DWTSRedirectInputPin::CheckMediaType(const CMediaType *)
{
	return S_OK;
}

HRESULT DWTSRedirectInputPin::BreakConnect()
{
	if (DWTSRedirectFilter(m_pFilter)->m_pPosition != NULL)
	{
		DWTSRedirectFilter(m_pFilter)->m_pPosition->ForceRefresh();
	}

	return CRenderedInputPin::BreakConnect();
}

STDMETHODIMP DWTSRedirectInputPin::ReceiveCanBlock()
{
	return S_FALSE;
}

STDMETHODIMP DWTSRedirectInputPin::Receive(IMediaSample *pSample)
{
	CheckPointer(pSample, E_POINTER);

	CAutoLock lock(m_pReceiveLock);
	PBYTE pbData;

	if (!DWTSRedirectFilter(m_pFilter)->m_bRedirecting)
		return NOERROR;

	REFERENCE_TIME tStart, tStop;
	pSample->GetTime(&tStart, &tStop);

	m_tLast = tStart;


	HRESULT hr = pSample->
}

STDMETHODIMP DWTSRedirectInputPin::EndOfStream()
{
	CAutoLock lock(m_pReceiveLock);
	return CRenderedInputPin::EndOfStream();
}

STDMETHODIMP DWTSRedirectInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_tLast = 0;
	return S_OK;
}