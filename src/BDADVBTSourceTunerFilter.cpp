// BDADVBTSourceTunerFilter.cpp: implementation of the BDADVBTSourceTunerFilter class.
//
//////////////////////////////////////////////////////////////////////
/*
#include "BDADVBTSourceTunerFilter.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BDADVBTSourceTunerFilter::BDADVBTSourceTunerFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
	  : CBaseFilter(tszName, punk, & m_crtFilterLock, CLSID_DSNetSend),
        m_pOutput           (NULL),
        m_pIMemAllocator    (NULL),
        m_pBufferPool       (NULL),
        m_pMSPool           (NULL),
        m_pNetRecvAlloc     (NULL)
{
	m_pTuner = NULL;

    //  instantiate the output pin
    m_pOutput = new BDADVBTSourceTunerOutputPin(NAME("CNetOutputPin"), this, & m_crtFilterLock, phr, L"MPEG-2 Transport");
    if (m_pOutput == NULL || FAILED (* phr))
	{
        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        return;
    }

    //  the buffer pool
    m_pBufferPool = new CBufferPool (POOL_SIZE, MAX_IOBUFFER_LENGTH, phr) ;
    if (m_pBufferPool == NULL || FAILED (*phr))
	{
        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        return;
    }

    //  the media sample pool
    m_pMSPool = new CTSMediaSamplePool (MEDIA_SAMPLE_POOL_SIZE, this, phr) ;
    if (m_pMSPool == NULL || FAILED (*phr))
	{
        (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
        return;
    }

    //  IMemAllocator
    m_pNetRecvAlloc = new CNetRecvAlloc (m_pBufferPool, m_pMSPool, this) ;
    if (m_pNetRecvAlloc == NULL)
	{
        (* phr) = E_OUTOFMEMORY ;
        return;
    }
}

BDADVBTSourceTunerFilter::~BDADVBTSourceTunerFilter()
{
    delete m_pOutput ;
    RELEASE_AND_CLEAR (m_pIMemAllocator) ;
    delete m_pBufferPool ;
    delete m_pMSPool ;
    delete m_pNetRecvAlloc ;
}


CBasePin *BDADVBTSourceTunerFilter::GetPin(int Index)
{
    CBasePin *pPin = NULL;
    LockFilter();
    if (Index == 0)
	{
        pPin = m_pOutput;
    }
    UnlockFilter();
    return pPin;
}

STDMETHODIMP BDADVBTSourceTunerFilter::Pause()
{
    HRESULT hr = S_OK;

    LockFilter();

    if (m_State == State_Stopped)
	{
        //  --------------------------------------------------------------------
        //  stopped -> pause transition; get the filter up and running

        //  activate the receiver; joins the multicast group and starts the
        //  thread
        //hr = m_pNetReceiver -> Activate (m_ulIP, m_usPort, m_ulNIC) ;
		hr = m_pTuner->m_pfDWTSRedirect->StartCapturing(this);

        if (SUCCEEDED (hr))
		{
            m_State = State_Paused;

            if (m_pOutput && m_pOutput->IsConnected())
			{
                m_pOutput->Active () ;
            }
        }
    }
    else
	{
        //  --------------------------------------------------------------------
        //  run -> pause transition; do nothing but set the state
        m_State = State_Paused ;
        hr = S_OK ;
    }

    UnlockFilter () ;
    return hr ;
}

STDMETHODIMP BDADVBTSourceTunerFilter::Stop ()
{
    LockFilter () ;

    //m_pNetReceiver -> Stop();
	m_pTuner->m_pfDWTSRedirect->StopCapturing();

    //  if we have an output pin (we should) stop it
    if (m_pOutput)
	{
        m_pOutput->Inactive();
    }

    m_State = State_Stopped ;
    UnlockFilter () ;
    return S_OK ;
}



//////////////////////////////////////////////////////////////////////
// BDADVBTSourceTunerFilterOutputPin
//////////////////////////////////////////////////////////////////////

BDADVBTSourceTunerFilterOutputPin::BDADVBTSourceTunerFilterOutputPin (
	IN  TCHAR *         szName,
    IN  CBaseFilter *   pFilter,
    IN  CCritSec *      pLock,
    OUT HRESULT *       pHr,
    IN  LPCWSTR         pszName
    ) : CBaseOutputPin  (szName, pFilter, pLock, pHr, pszName)
{
}

BDADVBTSourceTunerFilterOutputPin::~BDADVBTSourceTunerFilterOutputPin ()
{
}

HRESULT BDADVBTSourceTunerFilterOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition == 0)
	{
        ASSERT (pmt) ;
        pmt -> InitMediaType () ;

        pmt -> SetType      (& MEDIATYPE_Stream) ;
        pmt -> SetSubtype   (& MEDIASUBTYPE_MPEG2_TRANSPORT) ;

        return S_OK ;
    }

    return VFW_S_NO_MORE_ITEMS;
}

HRESULT BDADVBTSourceTunerFilterOutputPin::CheckMediaType(const CMediaType * pmt)
{
    ASSERT (pmt) ;
    if (pmt -> majortype    == MEDIATYPE_Stream &&
        pmt -> subtype      == MEDIASUBTYPE_MPEG2_TRANSPORT)
	{
        return S_OK;
    }
    return S_FALSE;
}

HRESULT
BDADVBTSourceTunerFilterOutputPin::InitAllocator(IMemAllocator ** ppAlloc)
{
    (* ppAlloc) = BDADVBTSourceTuner(m_pFilter)->GetRecvAllocator () ;
    (* ppAlloc) -> AddRef () ;

    return S_OK ;
}

HRESULT BDADVBTSourceTunerFilterOutputPin::DecideBufferSize (IMemAllocator *pIMemAllocator, ALLOCATOR_PROPERTIES *pProp)
{
    HRESULT hr ;

    if (pIMemAllocator == NET_RECV (m_pFilter)->GetRecvAllocator())
	{
        //  we're the allocator; get the properties and succeed
        hr = BDADVBTSourceTunerFilter(m_pFilter) -> GetRecvAllocator () -> GetProperties (pProp) ;
    }
    else {
        //  this is not our allocator; fail the call
        hr = E_FAIL ;
    }
    return hr ;
}
*/
