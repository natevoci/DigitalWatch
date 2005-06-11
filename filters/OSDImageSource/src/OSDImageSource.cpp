/**
 *	OSDImageSource.cpp
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

#include <streams.h>
#include <stdio.h>

#include "OSDImageSource.h"
#include "OSDImageSourceGuid.h"
#include "resource.h"

/**********************************************
 *
 *  COSDImageSourcePin Class
 *
 **********************************************/

COSDImageSourcePin::COSDImageSourcePin(HRESULT *phr, CSource *pFilter)
	: CSourceStream(NAME("OSD Image Source"), phr, pFilter, L"Out")
{
	m_rtStartTime = 0;

	m_iFrameNumber = 0;
	m_iImageWidth = 768;
	m_iImageHeight = 576;
	m_bImageFlipped = FALSE;
	m_bImageUpdated = TRUE;

	m_bTestPattern = TRUE;
	m_pstrDisplay = L"Default text\0";

    // create a DC for the screen and create
    // a memory DC compatible to screen DC
    m_hScrDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	//hScrDC = GetDC(NULL);
    m_hMemDC = CreateCompatibleDC(m_hScrDC);

    // create a bitmap compatible with the screen DC
    m_hBitmap = CreateCompatibleBitmap(m_hScrDC, m_iImageWidth, m_iImageHeight);

    m_hOldBitmap = (HBITMAP) SelectObject(m_hMemDC, m_hBitmap);
}

COSDImageSourcePin::~COSDImageSourcePin()
{   
	DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));

    // select old bitmap back into memory DC and delete 2nd bitmap
    m_hBitmap = (HBITMAP)  SelectObject(m_hMemDC, m_hOldBitmap);
	DeleteObject(m_hBitmap);

    // clean up
    DeleteDC(m_hMemDC);
    DeleteDC(m_hScrDC);

}

// GetMediaType
//	0	32bit ARGB
//	1	32bit RGB
//	2	24bit RGB
//	3   16bit RGB555
//	>3	Invalid
HRESULT COSDImageSourcePin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CheckPointer(pmt, E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0)
        return E_INVALIDARG;

    if(iPosition >= 4)
        return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(NULL == pvi)
        return E_OUTOFMEMORY;

    ZeroMemory(pvi, sizeof(VIDEOINFO));

    switch(iPosition)
    {
    case 0:
    case 1:
        pvi->bmiHeader.biCompression = BI_RGB;
        pvi->bmiHeader.biBitCount    = 32;
        break;

    case 2:
        pvi->bmiHeader.biCompression = BI_RGB;
        pvi->bmiHeader.biBitCount    = 24;
        break;

    case 3:
		memcpy(pvi->TrueColorInfo.dwBitMasks, bits555, 3*sizeof(DWORD));
        pvi->bmiHeader.biCompression = BI_BITFIELDS;
        pvi->bmiHeader.biBitCount    = 16;
        break;
    }

    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_iImageWidth;
    pvi->bmiHeader.biHeight		= m_iImageHeight;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

	switch (iPosition)
	{
	case 0:
		pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
		break;
	default:
		const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	    pmt->SetSubtype(&SubTypeGUID);
	}
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;

} 

HRESULT COSDImageSourcePin::CheckMediaType(const CMediaType *pMediaType)
{
    CheckPointer(pMediaType,E_POINTER);

    if((*(pMediaType->Type()) != MEDIATYPE_Video) ||
        !(pMediaType->IsFixedSize()))
    {                                                  
        return E_INVALIDARG;
    }

    const GUID *SubType = pMediaType->Subtype();
	if (SubType == NULL)
		return E_INVALIDARG;
    if(	(*SubType != MEDIASUBTYPE_RGB555) &&
		(*SubType != MEDIASUBTYPE_RGB24) &&
		(*SubType != MEDIASUBTYPE_RGB32) &&
		(*SubType != MEDIASUBTYPE_ARGB32) )
    {
        return E_INVALIDARG;
    }

    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();
    if(pvi == NULL)
        return E_INVALIDARG;

	if ((pvi->bmiHeader.biBitCount == 32) || (pvi->bmiHeader.biBitCount == 24))
	{
		if (pvi->bmiHeader.biCompression != BI_RGB)
			return E_INVALIDARG;
	}
	else if (pvi->bmiHeader.biBitCount == 16)
	{
		if (pvi->bmiHeader.biCompression != BI_BITFIELDS)
			return E_INVALIDARG;
	}

	//Check that size is correct.
    if (m_iImageWidth != pvi->bmiHeader.biWidth)
    {
        return E_INVALIDARG;
    }

	if (m_iImageHeight == pvi->bmiHeader.biHeight)
	{
		m_bImageFlipped = FALSE;
	}
	else if (m_iImageHeight == -pvi->bmiHeader.biHeight)
	{
		m_bImageFlipped = TRUE;
	}
	else
	{
		return E_INVALIDARG;
	}

    return S_OK;
}

HRESULT COSDImageSourcePin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;
    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pProperties,&Actual);
    if FAILED(hr)
    {
        return hr;
    }

    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
        return E_FAIL;
    }
	ASSERT(Actual.cBuffers == 1);

    return NOERROR;

}

HRESULT COSDImageSourcePin::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();
    if(pvi == NULL)
        return E_INVALIDARG;
    return hr;
}

HRESULT COSDImageSourcePin::Run()
{
	HRESULT hr = CSourceStream::Run();
	m_rtStartTime = 0;
	//TODO: Search downstream for overlay mixer or vmr's to get color key

	return hr;
}

HRESULT COSDImageSourcePin::DoBufferProcessingLoop()
{

    Command com;

    OnThreadStartPlay();

    do
	{
		while (!CheckRequest(&com))
		{
			IMediaSample *pSample;
			Sleep(1000);

			HRESULT hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
			if FAILED(hr)
			{
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
							// or the allocator is decommited & we will be asked to
							// exit soon.
		    }

			// Virtual function user will override.
			hr = FillBuffer(pSample);

			if (hr == S_OK)
			{
				hr = Deliver(pSample);
                pSample->Release();

                // downstream filter returns S_FALSE if it wants us to
                // stop or an error if it's reporting an error.
                if(hr != S_OK)
                {
                  DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
                  return S_OK;
                }

			}
			else if (hr == S_FALSE)
			{
                // derived class wants us to stop pushing data
				pSample->Release();
				DeliverEndOfStream();
				return S_OK;
		    }
			else
			{
                // derived class encountered an error
                pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
                DeliverEndOfStream();
                m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                return hr;
			}

            // all paths release the sample
		}

        // For all commands sent to us there must be a Reply call!

		if (com == CMD_RUN || com == CMD_PAUSE)
		{
		    Reply(NOERROR);
		}
		else if (com != CMD_STOP)
		{
			Reply((DWORD) E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
    } while (com != CMD_STOP);

    return S_FALSE;
}

HRESULT COSDImageSourcePin::FillBuffer(IMediaSample *pSample)
{
	BYTE *pData;
    long cbData;
	HRESULT hr;

    CheckPointer(pSample, E_POINTER);

	if (!m_bImageUpdated)
		return S_OK;

	//DWORD dwWaitResult = WaitForSingleObject(m_hPaintEvent, 1000);

    CAutoLock cAutoLockShared(&m_Lock);

	//Need to check for State_Paused because IBasicVideo->GetVideoSize
	//will hangs when you stop the graph if the Overlay mixer is used.
	FILTER_STATE filterState;
	if FAILED(hr = m_pFilter->GetState(1000, &filterState))
		hr = S_OK;
	if (filterState == State_Paused)
		return S_OK;

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	hr = CopyDCToBitmap(pData, (BITMAPINFO *) &(pVih->bmiHeader));

	if SUCCEEDED(hr)
	{
		//Force no transparency
	    const GUID *SubType = m_mt.Subtype();
		if (*SubType == MEDIASUBTYPE_ARGB32)
		{
			BYTE *pPixel = (BYTE*)pData;
			for ( long i=0 ; i<(m_iImageWidth*m_iImageHeight) ; i++ )
			{
				if (((long*)pPixel)[0] & 0x00FFFFFF == 0x00010101)
					pPixel[3] = 0x00;
				else
					pPixel[3] = 0xFF;
				pPixel += 4;
			}
		}

		m_iFrameNumber++;

		// Set TRUE on every sample for uncompressed frames
		pSample->SetSyncPoint(TRUE);
	}
	//Sleep(30);

    return S_OK;
}

HRESULT COSDImageSourcePin::CopyDCToBitmap(BYTE *pData, BITMAPINFO *pHeader)
{
	static int counter = 0;
		if (m_bTestPattern)
		{
		counter ++;
		
		LPTSTR szNewText = new TCHAR[100];
		int nLength;
#ifdef UNICODE
		wsprintf(szNewText, L"I'm showing number %i - %s", counter, m_pstrDisplay);
		nLength = wcslen(szNewText);
#else
		sprintf(szNewText, "I'm showing number %i - %S", counter, m_pstrDisplay);
		nLength = strlen(szNewText);
#endif

		HBRUSH br = CreateSolidBrush(0x00A00000);
		HBRUSH oldbrush;
		oldbrush = (HBRUSH)SelectObject(m_hMemDC, br);

		int slide = counter % 200;
		if (slide >= 100) slide = 200 - slide;
		RECT r;
		r.left   =  (m_iImageWidth *slide/300) + (m_iImageWidth *1/10);
		r.top    =  (m_iImageHeight*slide/300) + (m_iImageHeight*1/10);
		r.right  = -(m_iImageWidth *slide/300) + (m_iImageWidth *9/10);
		r.bottom = -(m_iImageHeight*slide/300) + (m_iImageHeight*9/10);
		RoundRect(m_hMemDC, r.left, r.top, r.right, r.bottom, 40, 40);

		br = (HBRUSH)SelectObject(m_hMemDC, oldbrush);
		DeleteObject(br);

		SIZE textExtent;
		GetTextExtentPoint32(m_hMemDC, szNewText, nLength, &textExtent);
		//SetBkColor(hMemDC, RGB(0, 0, 0));
		SetBkMode(m_hMemDC, TRANSPARENT);
		SetTextColor(m_hMemDC, RGB(0, 255, 0));
		TextOut(m_hMemDC, (m_iImageWidth-textExtent.cx)/2, (m_iImageHeight-textExtent.cy)/2, szNewText, nLength);
		delete[] szNewText;
	}

	if (!m_bImageFlipped)
		pHeader->bmiHeader.biHeight = m_iImageHeight;
	else
		pHeader->bmiHeader.biHeight = -m_iImageHeight;

	// Copy the bitmap data into the provided BYTE buffer
	int copied = GetDIBits(m_hMemDC, m_hBitmap, 0, m_iImageHeight, pData, pHeader, DIB_RGB_COLORS);
	m_bImageUpdated = FALSE;

    return S_OK;
}

HRESULT COSDImageSourcePin::GetVideoSize(long &nWidth, long &nHeight)
{
	HRESULT hr;
	IFilterGraph*	pGraph = NULL;
	IBasicVideo*	pBasicVideo = NULL;

	// Find FilterGraph
	pGraph = m_pFilter->GetFilterGraph();	//this does not do AddRef() so don't Release()
	if (pGraph == NULL)
		return E_POINTER;

	// Get IBasicVideo interface
	if FAILED(hr = pGraph->QueryInterface(IID_IBasicVideo, reinterpret_cast<void**>(&pBasicVideo)))
		return hr;
	if (pBasicVideo == NULL)
		return E_FAIL;

	// Retrieve video window size
	if FAILED(hr = pBasicVideo->GetVideoSize(&nWidth, &nHeight))
		return hr;
	pBasicVideo->Release();
	return S_OK;
}

HRESULT COSDImageSourcePin::GetReferenceClock(IReferenceClock **pClock)
{
	HRESULT hr;

	// Find FilterGraph
	IFilterGraph* pGraph = NULL;
	pGraph = m_pFilter->GetFilterGraph();	//this does not do AddRef() so don't Release()
	if (pGraph == NULL)
		return E_POINTER;

	// Get IMediaFilter interface
	IMediaFilter* pMediaFilter = NULL;
	hr = pGraph->QueryInterface(IID_IMediaFilter, reinterpret_cast<void**>(&pMediaFilter));

	if (pMediaFilter)
	{
		// Get IReferenceClock interface
		hr = pMediaFilter->GetSyncSource(pClock);
		pMediaFilter->Release();
		return S_OK;
	}
	return E_FAIL;
}

HRESULT COSDImageSourcePin::Paint()
{
	m_bImageUpdated = TRUE;
	return S_OK;
}


/**********************************************
 *
 *  COSDImageSourceFilter Class
 *
 **********************************************/

COSDImageSourceFilter::COSDImageSourceFilter(IUnknown *pUnk, HRESULT *phr)
           : CSource(NAME("OSD Image Source"), pUnk, CLSID_OSDImageSource)
{
    // The pin magically adds itself to our pin array.
    m_pPin = new COSDImageSourcePin(phr, this);

	if (phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}  
}


COSDImageSourceFilter::~COSDImageSourceFilter()
{
    delete m_pPin;
}


CUnknown * WINAPI COSDImageSourceFilter::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    COSDImageSourceFilter *pNewFilter = new COSDImageSourceFilter(pUnk, phr );

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;
}

STDMETHODIMP COSDImageSourceFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    // Do we have this interface
	if (riid == IID_IOSDImageSource)
	{
        return GetInterface((IOSDImageSource*)this, ppv);
    }

	if (riid == IID_ISpecifyPropertyPages)
    {
        return GetInterface((ISpecifyPropertyPages*)this, ppv);
    }

    return CSource::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP COSDImageSourceFilter::GetPages(CAUUID *pPages)
{
	if (pPages == NULL)
		return E_POINTER;
	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;

	pPages->pElems[0] = CLSID_OSDImageSourcePropertyPage;
	return S_OK;
}


STDMETHODIMP COSDImageSourceFilter::get_TestPattern(BOOL *bTestPattern)
{
	if (bTestPattern == NULL)
		return E_POINTER;
	CAutoLock lock(&m_pPin->m_Lock);
	*bTestPattern = m_pPin->m_bTestPattern;
	return S_OK;
}

STDMETHODIMP COSDImageSourceFilter::set_TestPattern(BOOL  bTestPattern)
{
	CAutoLock lock(&m_pPin->m_Lock);
	m_pPin->m_bTestPattern = bTestPattern;
	return S_OK;
}

STDMETHODIMP COSDImageSourceFilter::GetDC(HDC* phdc)
{
	if (phdc == NULL)
		return E_POINTER;
	CAutoLock lock(&m_pPin->m_Lock);
	*phdc = m_pPin->m_hMemDC;
	return S_OK;
}

STDMETHODIMP COSDImageSourceFilter::ReleaseDC()
{
	CAutoLock lock(&m_pPin->m_Lock);
	m_pPin->Paint();
	return S_OK;
}

STDMETHODIMP COSDImageSourceFilter::EraseBackground()
{
	CAutoLock lock(&m_pPin->m_Lock);
	RECT r;
	SetRect(&r, 0, 0, m_pPin->m_iImageWidth, m_pPin->m_iImageHeight);
	HBRUSH br = CreateSolidBrush(0x00010101);
	FillRect(m_pPin->m_hMemDC, &r, br);
	m_pPin->Paint();
	return S_OK;
}

STDMETHODIMP COSDImageSourceFilter::GetWindowRect(LPRECT lpRect)
{
	if (lpRect == NULL)
		return E_POINTER;
	CAutoLock lock(&m_pPin->m_Lock);
	SetRect(lpRect, 0, 0, m_pPin->m_iImageWidth, m_pPin->m_iImageHeight);
	return S_OK;
}

/**********************************************
 *
 *  COSDImageSourceProp Class
 *
 **********************************************/

COSDImageSourceProp::COSDImageSourceProp(IUnknown *pUnk) : 
	CBasePropertyPage(NAME("OSDImageSourceProp"), pUnk, IDD_PROPPAGE, IDS_PROPPAGE_TITLE),
	m_pImageSource(0)
{
}

CUnknown * WINAPI COSDImageSourceProp::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
    COSDImageSourceProp *pNewObject = new COSDImageSourceProp(pUnk);
    if (pNewObject == NULL) 
    {
        *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
} 

HRESULT COSDImageSourceProp::OnConnect(IUnknown *pUnk)
{
    if (pUnk == NULL)
        return E_POINTER;
    ASSERT(m_pImageSource == NULL);

    HRESULT hr = pUnk->QueryInterface(IID_IOSDImageSource, reinterpret_cast<void**>((&m_pImageSource));

	if FAILED(hr)
        return hr;
    ASSERT(m_pImageSource);

    return NOERROR;
}

HRESULT COSDImageSourceProp::OnDisconnect(void)
{
    if (m_pImageSource)
    {
        m_pImageSource->Release();
        m_pImageSource = NULL;
    }
    return S_OK;
}

HRESULT COSDImageSourceProp::OnActivate()
{
    ASSERT(m_pImageSource != NULL);
	Refresh();
    return S_OK;
}

BOOL COSDImageSourceProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            //m_hwndDialog = hwnd;
            Refresh();
            return TRUE;
        }

        case WM_DESTROY:
        {
            DestroyWindow(m_hwnd);
            return TRUE;
        }

        case WM_COMMAND:
        {
            
            switch (LOWORD (wParam))
			{
                case IDCANCEL :
					{
	                    //Refresh();
					}
	                break ;
				case IDC_BUTTON_CLEAR:
					{
						m_pImageSource->EraseBackground();
						Refresh();
					}
					break;
				case IDC_BUTTON_UPDATE:
					{
						HDC dc;
						m_pImageSource->GetDC(&dc);
						LPTSTR szText = new TCHAR[1024];
						long szTextLen = 0;
						GetDlgItemText(m_hwnd, IDC_EDIT_DISPLAYTEXT, szText, 1024);
#if UNICODE
						szTextLen = wcslen(szText);
#else
						szTextLen = strlen(szText);
#endif
						TextOut(dc, 100, 100, szText, szTextLen);
						m_pImageSource->ReleaseDC();
						Refresh();
					}
					break;
				case IDC_CHECK_TESTPATTERN:
					{
						BOOL bChecked = IsDlgButtonChecked(m_hwnd, IDC_CHECK_TESTPATTERN);
						m_pImageSource->set_TestPattern(bChecked);
						Refresh();
					}
					break;
            };
            return TRUE;
        }
    }
    
    return FALSE;
}

HRESULT COSDImageSourceProp::Refresh()
{
	BOOL bChecked;
	m_pImageSource->get_TestPattern(&bChecked);
	CheckDlgButton(m_hwnd, IDC_CHECK_TESTPATTERN, bChecked);

	return S_OK;
}

