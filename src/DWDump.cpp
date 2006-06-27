// DWDump.cpp: implementation of the DWDump class.
//
//////////////////////////////////////////////////////////////////////

#include <math.h>
#include <crtdbg.h>
#include <streams.h>
#include "DWDump.h"
#include "BDATYPES.H"
#include "KS.H"
#include "KSMEDIA.H"
#include "BDAMedia.h"
#include "Globals.h"

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

	m_pDump->m_pPin->StopThread();

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
//Frodo code changes
    m_restBufferLen = 0;
	m_PacketErrors = 0;
//Frodo code changes

	m_writeBufferSize = 4096*32;
	m_writeBuffer =	new BYTE[m_writeBufferSize];
    m_writeBufferLen = 0;

	m_WriteBufferSize = 0;
	m_WriteThreadActive = FALSE;
}

DWDumpInputPin::~DWDumpInputPin()
{
	delete[] m_writeBuffer;
	Clear();
}

void DWDumpInputPin::Clear()
{
	StopThread(500);
	CAutoLock BufferLock(&m_BufferLock);
	std::vector<BUFFERINFO*>::iterator it = m_Array.begin();
	for ( ; it != m_Array.end() ; it++ )
	{
		BUFFERINFO *item = *it;
		delete[] item->sample;
	}
	m_Array.clear();

	m_WriteBufferSize = 0;
	m_WriteThreadActive = FALSE;
}

HRESULT DWDumpInputPin::CheckMediaType(const CMediaType *)
{
    return S_OK;
}

HRESULT DWDumpInputPin::BreakConnect()
{
	Clear();

    if (m_pDump->m_pPosition != NULL) {
        m_pDump->m_pPosition->ForceRefresh();
    }

	m_writeBufferSize = sizeof(m_writeBuffer);
    m_writeBufferLen = 0;

    return CRenderedInputPin::BreakConnect();
}

STDMETHODIMP DWDumpInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

HRESULT DWDumpInputPin::Run(REFERENCE_TIME tStart)
{
	StartThread();
	return CBaseInputPin::Run(tStart);
}

//Frodo code changes
HRESULT DWDumpInputPin::Filter(byte* pbData,long sampleLen)
{
	HRESULT hr;
	int packet = 0;
	BOOL bProg = FALSE;
	_AMMediaType *mtype = &m_mt;
	if (mtype->majortype == MEDIATYPE_Stream &&
		((mtype->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT) | (mtype->subtype == KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT)))
	{
		//filter transport Packets
		packet = 188;
	}
	else if (mtype->majortype == MEDIATYPE_Stream &&
		mtype->subtype == MEDIASUBTYPE_MPEG2_PROGRAM)
	{
		//filter mpeg2 es Packets
		bProg = TRUE;
		packet = 2048;
	}
	else
	{
		//Write raw data method
		return WriteBufferSample(pbData, sampleLen);
	}

	int off=-1;

	// did last media sample we received contain a incomplete transport packet at the end? 
	if (m_restBufferLen>0) 
	{ 
       //yep then we copy the remaining bytes of the packet first 
		int len=packet-m_restBufferLen;

		//remaining bytes of packet  
		if (len>0 && len < packet)  
		{         
			if (m_restBufferLen>=0 && m_restBufferLen+len < packet+2)    
			{      
				memcpy(&m_restBuffer[m_restBufferLen], pbData, len);

				if(!bProg)
				{
					//check if this is indeed a transport packet  
					if(m_restBuffer[0]==0x47)   
					{     
						if FAILED(hr = WriteBufferSample(m_restBuffer,packet))
							return hr;
					}

					//set offset ...   
					if (pbData[len]==0x47 && pbData[len+packet]==0x47 && pbData[len+2*packet]==0x47)    
					{    
						off=len;   
					}      
					else             
					{                 
						m_restBufferLen=0;      
					}  
				}
				else
				{
					if (((0xFF&pbData[0])<<24
					| (0xFF&pbData[1])<<16
					| (0xFF&pbData[2])<<8
					| (0xFF&pbData[3])) == 0x1BA)
					{
						if FAILED(hr = WriteBufferSample(m_restBuffer,packet))   
							return hr;
					}

					//set offset ...   
					if (((0xFF&pbData[len])<<24
						| (0xFF&pbData[len+1])<<16
						| (0xFF&pbData[len+2])<<8
						| (0xFF&pbData[len+3])) == 0x1BA &&
						((0xFF&pbData[len+packet])<<24
						| (0xFF&pbData[len+packet+1])<<16
						| (0xFF&pbData[len+packet+2])<<8
						| (0xFF&pbData[len+packet+3])) == 0x1BA &&
						((0xFF&pbData[len+2*packet])<<24
						| (0xFF&pbData[len+2*packet+1])<<16
						| (0xFF&pbData[len+2*packet+2])<<8
						| (0xFF&pbData[len+2*packet+3])) == 0x1BA)    
					{    
						off=len;   
					}      
					else             
					{                 
						m_restBufferLen=0;      
					}  
				}
			}       
			else    
			{       
				m_restBufferLen=0;   
			}   
		}      
		else     
		{      
			m_restBufferLen=0;   
		}  
	}

	// is offset set ?   
	if (off==-1)  
	{     
		//no then find first 3 transport packets in mediasample  
		for (int i=0; i < sampleLen-2*packet;++i)  
		{         
			if(!bProg)
			{
				if (pbData[i]==0x47 && pbData[i+packet]==0x47 && pbData[i+2*packet]==0x47) 
				{     
					//found first 3 ts packets 
					//set the offset     
					off=i;    
					break;     
				} 
			}
			else
			{
				if (((0xFF&pbData[i])<<24
					| (0xFF&pbData[i+1])<<16
					| (0xFF&pbData[i+2])<<8
					| (0xFF&pbData[i+3])) == 0x1BA &&
					((0xFF&pbData[i+packet])<<24
					| (0xFF&pbData[i+packet+1])<<16
					| (0xFF&pbData[i+packet+2])<<8
					| (0xFF&pbData[i+packet+3])) == 0x1BA &&
					((0xFF&pbData[i+2*packet])<<24
					| (0xFF&pbData[i+2*packet+1])<<16
					| (0xFF&pbData[i+2*packet+2])<<8
					| (0xFF&pbData[i+2*packet+3])) == 0x1BA)    
				{     
					//found first 3 mpeg2 es packets
					//set the offset     
					off=i;    
					break;     
				} 
			}
		}   
	} 
	
	if (off<0)   
	{       
		off=0; 
    }

	DWORD t;
	PBYTE pData = new BYTE[sampleLen];
	DWORD pos = 0;
    //loop through all transport packets in the media sample   
	for(t=off;t<(DWORD)sampleLen;t+=packet)   
	{
        //sanity check 
		if (t+packet > sampleLen)
			break;
		
		if(!bProg)
		{
			//is this a transport packet   
			if(pbData[t]==0x47)     
			{     
				memcpy(&pData[pos], &pbData[t], packet);
				pos += packet;
			} 
			else
				m_PacketErrors++;
		}
		else
		{
			//is this a mpeg2 es packet   
			if (((0xFF&pbData[t])<<24
				| (0xFF&pbData[t+1])<<16
				| (0xFF&pbData[t+2])<<8
				| (0xFF&pbData[t+3])) == 0x1BA)    
			{     
				memcpy(&pData[pos], &pbData[t], packet);
				pos += packet;
			}
			else
				m_PacketErrors++;

		}
	};

	if (pos)
		if FAILED(hr = WriteBufferSample(&pData[0], pos))
		{
			delete [] pData;
			return hr;
		}

	delete [] pData;

    //calculate if there's a incomplete transport packet at end of media sample   
	m_restBufferLen=(sampleLen-off); 
	if (m_restBufferLen>0) 
	{       
		m_restBufferLen/=packet;    
		m_restBufferLen *=packet;   
		m_restBufferLen=(sampleLen-off)-m_restBufferLen;   
		if (m_restBufferLen>0 && m_restBufferLen < packet)  
		{      
			//copy the incomplete packet in the rest buffer      
			memcpy(m_restBuffer,&pbData[sampleLen-m_restBufferLen],m_restBufferLen); 
		}  
	}
	return S_OK;
}//Frodo code changes

void DWDumpInputPin::ThreadProc()
{
	m_WriteThreadActive = TRUE;

	BoostThread Boost;
	
	while (!ThreadIsStopping(0))
	{
		BYTE *item = NULL;
		long sampleLen = 0;
		{
			CAutoLock BufferLock(&m_BufferLock);
			if (m_Array.size())
			{
				std::vector<BUFFERINFO*>::iterator it = m_Array.begin();
				BUFFERINFO *bufferInfo = (*it);
				item = bufferInfo->sample;
				sampleLen = bufferInfo->size;
				m_Array.erase(it);
				m_WriteBufferSize -= sampleLen;
			}
			else
				Sleep(1);
		}
		if (item)
		{
			HRESULT hr = m_pDump->Write(item, sampleLen);
			delete[] item;
			if (FAILED(hr))
			{
				CAutoLock BufferLock(&m_BufferLock);
				::OutputDebugString(TEXT("DWDumpInputPin::ThreadProc:Write Fail."));
				std::vector<BUFFERINFO*>::iterator it = m_Array.begin();
				for ( ; it != m_Array.end() ; it++ )
				{
					BUFFERINFO *item = *it;
					delete[] item->sample;
				}
				m_Array.clear();
				m_WriteBufferSize = 0;
			}
		}
//		Sleep(1);
	}
	Clear();
	return;
}

HRESULT DWDumpInputPin::WriteBufferSample(byte* pbData,long sampleLen)
{
	HRESULT hr;
	long bufferLen = 32768/2;
	//
	//Only start buffering if the buffer thread is active
	//
	if (!m_WriteThreadActive)
	{
		BoostThread Boost;
		hr = m_pDump->Write(pbData, sampleLen);
		return hr;
	}

	//
	//If buffer thread is active and the buffer is not full
	//
	if(m_WriteThreadActive && m_WriteBufferSize + sampleLen < 64000000)
	{
		//use the sample packet size for the buffer
		if(sampleLen <= bufferLen)
		{
			BUFFERINFO *newItem = new BUFFERINFO;
			newItem->sample = new BYTE[sampleLen];
			//Return if we are out of memory
			if (!newItem->sample)
			{
				::OutputDebugString(TEXT("DWDumpInputPin::WriteBufferSample:Out of Memory."));
				return S_OK;
			}
			//store the sample in the temp buffer
			memcpy((void*)newItem->sample, &pbData[0], sampleLen);
			newItem->size = sampleLen;
			CAutoLock BufferLock(&m_BufferLock);
			m_Array.push_back(newItem);
			m_WriteBufferSize += sampleLen;
		}
		else
		{
			long pos = 0;
			//break up the sample into smaller packets
			for (long i = sampleLen; i > 0; i -= bufferLen)
			{
				long size = ((i/bufferLen) != 0)*bufferLen + ((i/bufferLen) == 0)*i;
				BUFFERINFO *newItem = new BUFFERINFO;
				newItem->sample = new BYTE[size];
				//Return if we are out of memory
				if (!newItem->sample)
				{
					::OutputDebugString(TEXT("DWDumpInputPin::WriteBufferSample:Out of Memory."));
					return S_OK;
				}
				//store the sample in the temp buffer
				memcpy((void*)newItem->sample, &pbData[pos], size);
				newItem->size = size;
				CAutoLock BufferLock(&m_BufferLock);
				m_Array.push_back(newItem);
				m_WriteBufferSize += size;
				pos += size;
			}
		}
		return S_OK;
	}
	//else clear the buffer
	::OutputDebugString(TEXT("DWDumpInputPin::WriteBufferSample:Buffer Full error."));
	CAutoLock BufferLock(&m_BufferLock);
	::OutputDebugString(TEXT("DWDumpInputPin::ThreadProc:Write Fail."));
	std::vector<BUFFERINFO*>::iterator it = m_Array.begin();
	for ( ; it != m_Array.end() ; it++ )
	{
		BUFFERINFO *item = *it;
		delete[] item->sample;
	}
	m_Array.clear();
	m_WriteBufferSize = 0;
	return S_OK;
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

	return WriteBufferSample(pbData,pSample->GetActualDataLength()); 

//Frodo code changes
//	return m_pDump->Write(pbData, pSample->GetActualDataLength());

//	return Filter(pbData,pSample->GetActualDataLength()); 
	
//Frodo code changes
}

STDMETHODIMP DWDumpInputPin::EndOfStream(void)
{
	Clear();
    CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

}

STDMETHODIMP DWDumpInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	Clear();
	m_PacketErrors = 0;
	m_restBufferLen=0;
	m_writeBufferSize = sizeof(m_writeBuffer);
    m_writeBufferLen = 0;
	m_tLast = 0;
    return S_OK;
}

void DWDumpInputPin::PrintLongLong(LPCTSTR lstring, __int64 value)
{
	TCHAR sz[100];
	double dVal = value;
	double len = log10(dVal);
	int pos = len;
	sz[pos+1] = '\0';
	while (pos >= 0)
	{
		int val = value % 10;
		sz[pos] = '0' + val;
		value /= 10;
		pos--;
	}
	TCHAR szout[100];
	wsprintf(szout, TEXT("%05i - %s %s\n"), debugcount, lstring, sz);
	::OutputDebugString(szout);
	debugcount++;
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

    m_pFileName = new WCHAR[1+wcslen(pszFileName)];
    if (m_pFileName == 0)
        return E_OUTOFMEMORY;

    wcscpy(m_pFileName,pszFileName);
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
        *ppszFileName = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * (1+wcslen(m_pFileName)));

        if (*ppszFileName != NULL)
        {
            wcscpy(*ppszFileName, m_pFileName);
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
//                         (DWORD) FILE_FLAG_WRITE_THROUGH,             // More flags
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

