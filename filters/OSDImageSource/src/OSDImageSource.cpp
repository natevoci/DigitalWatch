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
        : CSourceStream(NAME("OSD Image Source"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
        m_bZeroMemory(0),
        m_iFrameNumber(0),
        m_rtFrameLength(UNITS / 25), // Capture and display desktop 5 times per second
        m_nCurrentBitDepth(32)
{
    // Get the device context of the main display
    HDC hDC;
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hDC, HORZRES) / 2;
    m_rScreen.bottom = GetDeviceCaps(hDC, VERTRES) / 2;

    // Save dimensions for later use in FillBuffer()
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;

    // Release the device context
    DeleteDC(hDC);
}

COSDImageSourcePin::~COSDImageSourcePin()
{   
	DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
}


//
// GetMediaType
//
// Prefer 5 formats - 8, 16 (*2), 24 or 32 bits per pixel
//
// Prefered types should be ordered by quality, with zero as highest quality.
// Therefore, iPosition =
//      0    Return a 32bit mediatype
//      1    Return a 24bit mediatype
//      2    Return 16bit RGB565
//      3    Return a 16bit mediatype (rgb555)
//      4    Return 8 bit palettised format
//      >4   Invalid
//
HRESULT COSDImageSourcePin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CheckPointer(pmt,E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0)
        return E_INVALIDARG;

	iPosition++;
    // Have we run off the end of types?
    if(iPosition > 5)
        return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(NULL == pvi)
        return(E_OUTOFMEMORY);

    // Initialize the VideoInfo structure before configuring its members
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    switch(iPosition)
    {
/*        case 0:
        {    
			//Alpha 32bit
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 32;
            break;
        }
*/
        case 1:
        {    
            // Return our highest quality 32bit format

            // Since we use RGB888 (the default for 32 bit), there is
            // no reason to use BI_BITFIELDS to specify the RGB
            // masks. Also, not everything supports BI_BITFIELDS
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 32;
            break;
        }

        case 2:
        {   // Return our 24bit format
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 24;
            break;
        }

        case 3:
        {       
            // 16 bit per pixel RGB565

            // Place the RGB masks as the first 3 doublewords in the palette area
            for(int i = 0; i < 3; i++)
                pvi->TrueColorInfo.dwBitMasks[i] = bits565[i];

            pvi->bmiHeader.biCompression = BI_BITFIELDS;
            pvi->bmiHeader.biBitCount    = 16;
            break;
        }

        case 4:
        {   // 16 bits per pixel RGB555

            // Place the RGB masks as the first 3 doublewords in the palette area
            for(int i = 0; i < 3; i++)
                pvi->TrueColorInfo.dwBitMasks[i] = bits555[i];

            pvi->bmiHeader.biCompression = BI_BITFIELDS;
            pvi->bmiHeader.biBitCount    = 16;
            break;
        }

        case 5:
        {   // 8 bit palettised

            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 8;
            pvi->bmiHeader.biClrUsed     = iPALETTE_COLORS;
            break;
        }
    }

    // Adjust the parameters common to all formats
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_iImageWidth;
    pvi->bmiHeader.biHeight     = m_iImageHeight;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
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

} // GetMediaType


//
// CheckMediaType
//
// We will accept 8, 16, 24 or 32 bit video formats, in any
// image size that gives room to bounce.
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT COSDImageSourcePin::CheckMediaType(const CMediaType *pMediaType)
{
    CheckPointer(pMediaType,E_POINTER);

    if((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
        !(pMediaType->IsFixedSize()))                  // in fixed size samples
    {                                                  
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (SubType == NULL)
        return E_INVALIDARG;

    if(    (*SubType != MEDIASUBTYPE_RGB8)
        && (*SubType != MEDIASUBTYPE_RGB565)
        && (*SubType != MEDIASUBTYPE_RGB555)
        && (*SubType != MEDIASUBTYPE_RGB24)
        && (*SubType != MEDIASUBTYPE_RGB32)
		&& (*SubType != MEDIASUBTYPE_ARGB32))
    {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

    if(pvi == NULL)
        return E_INVALIDARG;

    // Check if the image width & height have changed
    if(    pvi->bmiHeader.biWidth   != m_iImageWidth || 
       abs(pvi->bmiHeader.biHeight) != m_iImageHeight)
    {
        // If the image width/height is changed, fail CheckMediaType() to force
        // the renderer to resize the image.
        return E_INVALIDARG;
    }

    // Don't accept formats with negative height, which would cause the desktop
    // image to be displayed upside down.
    if (pvi->bmiHeader.biHeight < 0)
        return E_INVALIDARG;

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT COSDImageSourcePin::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory. NOTE: the function
    // can succeed (return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted.
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    // Is this allocator unsuitable?
    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
        return E_FAIL;
    }

    // Make sure that we have only 1 buffer (we erase the ball in the
    // old buffer to save having to zero a 200k+ buffer every time
    // we draw a frame)
    ASSERT(Actual.cBuffers == 1);
    return NOERROR;

} // DecideBufferSize


//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT COSDImageSourcePin::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);

    if(SUCCEEDED(hr))
    {
        VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)
            return E_UNEXPECTED;

        switch(pvi->bmiHeader.biBitCount)
        {
            case 8:     // 8-bit palettized
            case 16:    // RGB565, RGB555
            case 24:    // RGB24
            case 32:    // RGB32
                // Save the current media type and bit depth
                m_MediaType = *pMediaType;
                m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
                hr = S_OK;
                break;

            default:
                // We should never agree any other media types
                ASSERT(FALSE);
                hr = E_INVALIDARG;
                break;
        }
    } 

    return hr;

} // SetMediaType


// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT COSDImageSourcePin::FillBuffer(IMediaSample *pSample)
{
	BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);

    CAutoLock cAutoLockShared(&m_cSharedState);

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;
/*
	// Copy the DIB bits over into our filter's output buffer.
    // Since sample size may be larger than the image size, bound the copy size.
    int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD) cbData);

	HDIB hDib = CopyScreenToBitmap(&m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));
    if (hDib)
        DeleteObject(hDib);
*/

	HRESULT hr;
	hr = CopyDCToBitmap(&m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));


	//Force no transparency
	if (pVih->bmiHeader.biBitCount == 32)
	{
		BYTE *pPixel = (BYTE*)pData;
		for ( long i=0 ; i<(pVih->bmiHeader.biWidth*pVih->bmiHeader.biHeight) ; i++ )
		{
			//pPixel[0] = 0xFF;
			//pPixel[1] = 0xFF;
			//pPixel[2] = 0xFF;
			pPixel[3] = 0xFF;
			pPixel += 4;
		}
	}

	// Set the timestamps that will govern playback frame rate.
	// If this file is getting written out as an AVI,
	// then you'll also need to configure the AVI Mux filter to 
	// set the Average Time Per Frame for the AVI Header.
    // The current time is the sample's start.
    REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
    REFERENCE_TIME rtStop  = rtStart + m_rtFrameLength;

    //pSample->SetTime(&rtStart, &rtStop);
	Sleep(30);
    m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames
    pSample->SetSyncPoint(TRUE);

    return S_OK;
}

HRESULT COSDImageSourcePin::CopyDCToBitmap(LPRECT lpRect, BYTE *pData, BITMAPINFO *pHeader)
{
	HRESULT		hr;
    HDC         hScrDC, hMemDC;         // screen DC and memory DC
    HBITMAP     hBitmap, hOldBitmap;    // handles to deice-dependent bitmaps
    long        nWidth, nHeight;        // DIB width and height

	IFilterGraph*	pGraph = NULL;
	IBasicVideo*	pBasicVideo = NULL;
	FILTER_INFO		filterInfo;

    // check for an empty rectangle
    if (IsRectEmpty(lpRect))
      return NULL;

    // create a DC for the screen and create
    // a memory DC compatible to screen DC   
    //hScrDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	hScrDC = GetDC(NULL);
    hMemDC = CreateCompatibleDC(hScrDC);

	// Find FilterGraph
	hr = m_pFilter->QueryFilterInfo(&filterInfo);
	pGraph = filterInfo.pGraph;

	// Get IBasicVideo interface
	hr = pGraph->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
	pGraph->Release();
	if (pBasicVideo == NULL)
		return E_FAIL;

	// Retrieve video window size
	hr = pBasicVideo->GetVideoSize(&nWidth, &nHeight);
	pBasicVideo->Release();

    // create a bitmap compatible with the screen DC
    hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

    // select new bitmap into memory DC
    hOldBitmap = (HBITMAP) SelectObject(hMemDC, hBitmap);

//******************************************	
	static int counter = 0;
	counter ++;
	
	PTCHAR szNewText;
	int nLength;
#ifdef UNICODE
	szNewText = new wchar_t[100];
	wsprintf(szNewText, L"I'm showing number %i", counter);
	nLength = wcslen(szNewText);
#else
	szNewText = new char[100];
	sprintf(szNewText, "I'm showing number %i", counter);
	nLength = strlen(szNewText);
#endif

	HBRUSH br = CreateSolidBrush(0x00A00000);
	HBRUSH oldbrush;
	oldbrush = (HBRUSH)SelectObject(hMemDC, br);

	int slide = counter % 200;
	if (slide >= 100) slide = 200 - slide;
	RoundRect(hMemDC, nWidth*slide/300 + nWidth/10, nHeight*slide/300 + nHeight/10, (nWidth*9/10)-(nWidth*slide/300), (nHeight*9/10)-(nHeight*slide/300), 40, 40);

	br = (HBRUSH)SelectObject(hMemDC, oldbrush);
	DeleteObject(br);

	SIZE textExtent;
	GetTextExtentPoint32(hMemDC, szNewText, nLength, &textExtent);
	//SetBkColor(hMemDC, RGB(0, 0, 0));
	SetBkMode(hMemDC, TRANSPARENT);
	SetTextColor(hMemDC, RGB(0, 255, 0));
	TextOut(hMemDC, (nWidth-textExtent.cx)/2, (nHeight-textExtent.cy)/2, szNewText, nLength);

    // select old bitmap back into memory DC and get handle to
    // bitmap of the screen   
    hBitmap = (HBITMAP)  SelectObject(hMemDC, hOldBitmap);

    // Copy the bitmap data into the provided BYTE buffer
    GetDIBits(hScrDC, hBitmap, 0, nHeight, pData, pHeader, DIB_RGB_COLORS);

    // clean up
    DeleteDC(hScrDC);
    DeleteDC(hMemDC);

	DeleteObject(hBitmap);
    // return handle to the bitmap
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

/*
    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
	{
   
        return m_pPin->NonDelegatingQueryInterface(riid, ppv);
    } 
*/
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

STDMETHODIMP COSDImageSourceFilter::DoStuff()
{
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

    HRESULT hr = pUnk->QueryInterface(IID_IOSDImageSource, (void**)(&m_pImageSource));

	if (FAILED(hr))
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
                    //Refresh();
                    break ;

				case IDC_BUTTON_UPDATE:
					m_pImageSource->DoStuff();
					Refresh();
					break;
            };
            return TRUE;
        }
    }
    
    return FALSE;
}

HRESULT COSDImageSourceProp::Refresh()
{
	return S_OK;
}

