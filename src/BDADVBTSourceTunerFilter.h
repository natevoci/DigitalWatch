/**
 *	BDADVBTSourceTunerFilter.h
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

#ifndef BDADVBTSOURCETUNERFILTER_H
#define BDADVBTSOURCETUNERFILTER_H
/*
#include "StdAfx.h"
#include "BDADVBTSourceTuner.h"

class BDADVBTSourceTunerFilter : public CBaseFilter
{
public:
	BDADVBTSourceTunerFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual ~BDADVBTSourceTunerFilter();

    void LockFilter ()              { m_crtFilterLock.Lock () ; }
    void UnlockFilter ()            { m_crtFilterLock.Unlock () ; }

	DECLARE_IUNKNOWN;

    int GetPinCount ()              { return 1 ; }
    CBasePin *GetPin (int Index);

    CNetRecvAlloc *GetRecvAllocator()
    {
        return m_pNetRecvAlloc ;
    }

    AMOVIESETUP_FILTER *GetSetupData ()
    {
        return & g_sudRecvFilter ;
    }

    STDMETHODIMP Pause () ;
    STDMETHODIMP Stop () ;

private:
	BDADVBTSourceTuner *				m_pTuner;

    CCritSec							m_crtFilterLock ;   //  filter lock
    BDADVBTSourceTunerFilterOutputPin * m_pOutput ;         //  output pin
    IMemAllocator *						m_pIMemAllocator ;  //  mem allocator
    CBufferPool *						m_pBufferPool ;     //  buffer pool object
    CTSMediaSamplePool *				m_pMSPool ;         //  media sample pool
    CNetRecvAlloc *						m_pNetRecvAlloc ;   //  allocator
};

class BDADVBTSourceTunerFilterOutputPin : public CBaseOutputPin
{
public :
    BDADVBTSourceTunerFilterOutputPin(TCHAR *szName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *pHr, LPCWSTR pszName);
    ~BDADVBTSourceTunerFilterOutputPin();

    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT DecideBufferSize(IMemAllocator *, ALLOCATOR_PROPERTIES *);

    virtual HRESULT InitAllocator(IMemAllocator ** ppAlloc);
} ;


*/
#endif
