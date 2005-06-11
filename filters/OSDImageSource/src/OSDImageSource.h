/**
 *	OSDTextSource.h
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

#ifndef OSDIMAGESOURCE_H
#define OSDIMAGESOURCE_H

#include <D3d9.h>
#include <Vmr9.h>
#include "IOSDImageSource.h"

/**********************************************
 *
 *  COSDImageSourcePin Class
 *
 **********************************************/

class COSDImageSourcePin : public CSourceStream
{
	friend class COSDImageSourceFilter;
public:
    COSDImageSourcePin(HRESULT *phr, CSource *pFilter);
    ~COSDImageSourcePin();

    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	HRESULT DoBufferProcessingLoop();
    HRESULT FillBuffer(IMediaSample *pSample);
    
    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // Support multiple display formats
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    HRESULT Run();


    // Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }

protected:
	HRESULT Paint();
	HRESULT CopyDCToBitmap(BYTE *pData, BITMAPINFO *pHeader);
	HRESULT GetVideoSize(long &nWidth, long &nHeight);
	HRESULT GetReferenceClock(IReferenceClock **pClock);

	REFERENCE_TIME m_rtStartTime;

    int m_iFrameNumber;
    int m_iImageHeight;                 // The current image height
    int m_iImageWidth;                  // And current image width
	BOOL m_bImageFlipped;
	BOOL m_bImageUpdated;

	//CMediaType m_MediaType;
    CCritSec m_Lock;

    HDC m_hScrDC;
	HDC m_hMemDC;
    HBITMAP m_hBitmap;
	HBITMAP m_hOldBitmap;

	BOOL m_bTestPattern;
	LPWSTR m_pstrDisplay;
};

/**********************************************
 *
 *  COSDImageSourceFilter Class
 *
 **********************************************/

class COSDImageSourceFilter :	public CSource,
								public ISpecifyPropertyPages,
								public IOSDImageSource
{
public:
	//IUnknown
	DECLARE_IUNKNOWN;
    static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);  
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	//ISpecifyPropertyPages
	STDMETHODIMP GetPages(CAUUID *pPages);

	//IOSDImageSource
	STDMETHODIMP get_TestPattern(BOOL *bTestPattern);
	STDMETHODIMP set_TestPattern(BOOL  bTestPattern);

	STDMETHODIMP GetDC(HDC* phdc);
	STDMETHODIMP ReleaseDC();
	STDMETHODIMP EraseBackground();
	STDMETHODIMP GetWindowRect(LPRECT lpRect);

private:
    COSDImageSourceFilter(IUnknown *pUnk, HRESULT *phr);
    ~COSDImageSourceFilter();

    COSDImageSourcePin *m_pPin;
    CCritSec m_Lock;
};

/**********************************************
 *
 *  COSDImageSourceProp Class
 *
 **********************************************/

class COSDImageSourceProp : public CBasePropertyPage
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT Refresh();

private:
	COSDImageSourceProp(IUnknown *pUnk);

	IOSDImageSource *m_pImageSource;
};

#endif
