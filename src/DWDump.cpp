// DWDump.cpp: implementation of the DWDump class.
//
//////////////////////////////////////////////////////////////////////

#include <streams.h>
#include "DWDump.h"

//////////////////////////////////////////////////////////////////////
// DWDumpFilter
//////////////////////////////////////////////////////////////////////
DWDumpFilter::DWDumpFilter(DWDump *pDump, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr)
  : CBaseFilter(NAME("DWDumpFilter"), pUnk, pLock, GUID_NULL),
	m_pDump(pDump)
{
}

DWDumpFilter::~DWDumpFilter()
{
}

CBasePin * DWDumpFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pDump->m_pPin;
    } else {
        return NULL;
    }
}

int DWDumpFilter::GetPinCount()
{
    return 1;
}

STDMETHODIMP DWDumpFilter::Stop()
{
    CAutoLock cObjectLock(m_pLock);

    if (m_pDump)
        m_pDump->CloseFile();
    
    return CBaseFilter::Stop();
}

STDMETHODIMP DWDumpFilter::Pause()
{
    CAutoLock cObjectLock(m_pLock);

    //if ((m_pDump) && (!m_pDump->m_fWriteError))
    //{
	//	m_pDump->OpenFile();
    //}

    return CBaseFilter::Pause();
}

STDMETHODIMP DWDumpFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);

    //m_pDump->m_fWriteError = FALSE;

    //if (m_pDump)
    //	m_pDump->OpenFile();

    return CBaseFilter::Run(tStart);
}


//////////////////////////////////////////////////////////////////////
// DWDumpInputPin
//////////////////////////////////////////////////////////////////////
DWDumpInputPin::DWDumpInputPin(DWDump *pDump, LPUNKNOWN pUnk, CBaseFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr)
  : CRenderedInputPin(NAME("CDumpInputPin"), pFilter, pLock, phr, L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pDump(pDump),
    m_tLast(0)
{
}

DWDumpInputPin::~DWDumpInputPin()
{
}

HRESULT DWDumpInputPin::CheckMediaType(const CMediaType *)
{
    return S_OK;
}

HRESULT DWDumpInputPin::BreakConnect()
{
    if (m_pDump->m_pPosition != NULL) {
        m_pDump->m_pPosition->ForceRefresh();
    }

    return CRenderedInputPin::BreakConnect();
}

STDMETHODIMP DWDumpInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

STDMETHODIMP DWDumpInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);

    CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;

    // Has the filter been stopped yet?
    if (m_pDump->m_hFile == INVALID_HANDLE_VALUE)
	{
        return NOERROR;
    }

    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);

    DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tStop),
           (LONG)((tStart - m_tLast) / 10000),
           pSample->GetActualDataLength()));

    m_tLast = tStart;

    // Copy the data to the file

    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) {
        return hr;
    }

    return m_pDump->Write(pbData, pSample->GetActualDataLength());
}

STDMETHODIMP DWDumpInputPin::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

}

STDMETHODIMP DWDumpInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    m_tLast = 0;
    return S_OK;

}

//////////////////////////////////////////////////////////////////////
// DWDump
//////////////////////////////////////////////////////////////////////
DWDump::DWDump(HRESULT *phr) :
    CUnknown(NAME("DWDump"), NULL),
    m_pFilter(NULL),
    m_pPin(NULL),
    m_pPosition(NULL),
    m_hFile(INVALID_HANDLE_VALUE),
    m_pFileName(0),
    m_fWriteError(0),
	m_bPaused(FALSE)
{
    ASSERT(phr);
    
    m_pFilter = new DWDumpFilter(this, GetOwner(), &m_Lock, phr);
    if (m_pFilter == NULL)
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }

    m_pPin = new DWDumpInputPin(this,GetOwner(), m_pFilter, &m_Lock, &m_ReceiveLock, phr);
    if (m_pPin == NULL)
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }
}

DWDump::~DWDump()
{
	CloseFile();

	if (m_pPin) delete m_pPin;
	if (m_pFilter) delete m_pFilter;
	if (m_pPosition) delete m_pPosition;
	if (m_pFileName) delete m_pFileName;
}

STDMETHODIMP DWDump::SetFileName(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(pszFileName,E_POINTER);

    // Is the file already opened
    if (IsRecording())
        CloseFile();

    if(wcslen(pszFileName) > MAX_PATH)
        return ERROR_FILENAME_EXCED_RANGE;

	if (m_pFileName)
		delete[] m_pFileName;

    m_pFileName = new WCHAR[1+lstrlenW(pszFileName)];
    if (m_pFileName == 0)
        return E_OUTOFMEMORY;

    lstrcpyW(m_pFileName,pszFileName);
    m_fWriteError = FALSE;
    HRESULT hr = OpenFile();
    CloseFile();

    return hr;
}

STDMETHODIMP DWDump::GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;

    if (m_pFileName != NULL) 
    {
		//QzTask = CoTask
        *ppszFileName = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * (1+lstrlenW(m_pFileName)));

        if (*ppszFileName != NULL)
        {
            lstrcpyW(*ppszFileName, m_pFileName);
        }
    }

    if(pmt)
    {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->majortype = MEDIATYPE_NULL;
        pmt->subtype = MEDIASUBTYPE_NULL;
    }

    return S_OK;

}

HRESULT DWDump::Record()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
	{
		m_bPaused = FALSE;
		return S_OK;
	}
	return OpenFile();
}

HRESULT DWDump::StopRecording()
{
	return CloseFile();
}

HRESULT DWDump::Pause()
{
	m_bPaused = !m_bPaused;
	return S_OK;
}

BOOL DWDump::IsRecording()
{
	return (m_hFile != INVALID_HANDLE_VALUE);
}

BOOL DWDump::IsPaused()
{
	return m_bPaused;
}

HRESULT DWDump::OpenFile()
{
    TCHAR *pFileName = NULL;

    // Is the file already opened
    if (IsRecording())
	{
        return NOERROR;
    }

    // Has a filename been set yet
    if (m_pFileName == NULL)
	{
        return ERROR_INVALID_NAME;
    }

    // Convert the UNICODE filename if necessary

#if defined(WIN32) && !defined(UNICODE)
    char convert[MAX_PATH];

    if(!WideCharToMultiByte(CP_ACP,0,m_pFileName,-1,convert,MAX_PATH,0,0))
        return ERROR_INVALID_NAME;

    pFileName = convert;
#else
    pFileName = m_pFileName;
#endif

    // Try to open the file

    m_hFile = CreateFile((LPCTSTR) pFileName,   // The filename
                         GENERIC_WRITE,         // File access
                         FILE_SHARE_READ,       // Share access
                         NULL,                  // Security
                         CREATE_ALWAYS,         // Open flags
                         (DWORD) 0,             // More flags
                         NULL);                 // Template

    if (m_hFile == INVALID_HANDLE_VALUE) 
    {
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }

    return S_OK;
}

HRESULT DWDump::CloseFile()
{
    // Must lock this section to prevent problems related to
    // closing the file while still receiving data in Receive()
    CAutoLock lock(&m_Lock);

    if (m_hFile == INVALID_HANDLE_VALUE) {
        return NOERROR;
    }

    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file 

    return NOERROR;
}

HRESULT DWDump::Write(PBYTE pbData, LONG lDataLength)
{
    DWORD dwWritten;

    // If the file has already been closed, don't continue
    if (m_hFile == INVALID_HANDLE_VALUE)
	{
        return S_FALSE;
    }

	// Recording is paused
	if (m_bPaused)
	{
		return S_OK;
	}

    if (!WriteFile(m_hFile, (PVOID)pbData, (DWORD)lDataLength, &dwWritten, NULL)) 
    {
		DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }

    return S_OK;
}





STDMETHODIMP DWDump::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    if (riid == IID_IFileSinkFilter)
	{
        return GetInterface((IFileSinkFilter *) this, ppv);
    } 
    else if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist)
	{
        return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
    } 
    else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
	{
        if (m_pPosition == NULL) 
        {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("Dump Pass Through"), (IUnknown *) GetOwner(), (HRESULT *) &hr, m_pPin);
            if (m_pPosition == NULL) 
                return E_OUTOFMEMORY;

            if (FAILED(hr)) 
            {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }

        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } 

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);

}

