/**
 *	BDADVBTSinkTShift.cpp
 *	Copyright (C) 2004 Nate
 *	Copyright (C) 2006 Bear
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


#include "BDADVBTSinkTShift.h"
#include "Globals.h"
#include "LogMessage.h"

#include <ks.h>
#include <ksmedia.h>
#include <bdamedia.h>
#include "TSFileSinkGuids.h"
#include "TSFileSource/ITSFileSink.h"


//////////////////////////////////////////////////////////////////////
// BDADVBTSinkTShift
//////////////////////////////////////////////////////////////////////

BDADVBTSinkTShift::BDADVBTSinkTShift(BDADVBTSink *pBDADVBTSink) :
	m_pBDADVBTSink(pBDADVBTSink)
{
	m_piGraphBuilder = NULL;
	m_pTelexDWDump = NULL;
	m_pVideoDWDump = NULL;
	m_pAudioDWDump = NULL;
	m_pMPGDWDump = NULL;
	m_pTSDWDump = NULL;
	m_pFTSDWDump = NULL;

	m_pTelexFileName = NULL;
	m_pVideoFileName = NULL;
	m_pAudioFileName = NULL;
	m_pMPGFileName = NULL;
	m_pTSFileName = NULL;
	m_pFTSFileName = NULL;

	m_bInitialised = 0;
	m_bActive = FALSE;

	m_intSinkType = 0;
}

BDADVBTSinkTShift::~BDADVBTSinkTShift()
{
	DestroyAll();
}

void BDADVBTSinkTShift::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	graphTools.SetLogCallback(callback);
}

HRESULT BDADVBTSinkTShift::Initialise(IGraphBuilder *piGraphBuilder, int intSinkType)
{
	HRESULT hr;
	if (m_bInitialised)
		return (log << "DVB-T TimeShift Sink tried to initialise a second time\n").Write(E_FAIL);

	if (!piGraphBuilder)
		return (log << "Must pass a valid DWGraph object to Initialise a Sink\n").Write(E_FAIL);

	m_piGraphBuilder = piGraphBuilder;

	//--- COM should already be initialized ---

	if FAILED(hr = m_piGraphBuilder->QueryInterface(&m_piMediaControl))
		return (log << "Failed to get media control: " << hr << "\n").Write(hr);

	m_intSinkType = intSinkType;
	m_bInitialised = TRUE;
	return S_OK;
}

HRESULT BDADVBTSinkTShift::DestroyAll()
{
    HRESULT hr = S_OK;

	RemoveSinkFilters();

	if(m_pTelexFileName)
	{
		delete[] m_pTelexFileName;
		m_pTelexFileName = NULL;
	}

	if(m_pVideoFileName)
	{
		delete[] m_pVideoFileName;
		m_pVideoFileName = NULL;
	}

	if(m_pAudioFileName)
	{
		delete[] m_pAudioFileName;
		m_pAudioFileName = NULL;
	}

	if(m_pMPGFileName)
	{
		delete[] m_pMPGFileName;
		m_pMPGFileName = NULL;
	}

	if(m_pTSFileName)
	{
		delete[] m_pTSFileName;
		m_pTSFileName = NULL;
	}

	if(m_pFTSFileName)
	{
		delete[] m_pFTSFileName;
		m_pFTSFileName = NULL;
	}

	m_piMediaControl.Release();

	return S_OK;
}

HRESULT BDADVBTSinkTShift::AddSinkFilters(DVBTChannels_Service* pService)
{
	HRESULT hr = E_FAIL;

	//--- Add & connect the TimeShifting filters ---

	if (m_intSinkType == 1)
	{
		//FileWriter (Full TS TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pFTSSink, L"Full TS TimeShift FileWriter"))
		{
			(log << "Failed to add Full TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
		}
		else		//Connect FileWriter (Full TS TimeShifting)
			if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pFTSSink))
			{
				(log << "Failed to connect Infinite Pin Tee Filter to Full TS TimeShift FileWriter: " << hr << "\n").Write(hr);
				DestroyFTSFilters();
			}
			else	//Add Demux Pins (TS TimeShifting)
				if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pFTSFileName, pService, m_pFTSSink, 1))
				{
					(log << "Failed to Set the File Name on the Full TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
					DestroyFTSFilters();
				}
	}
	else if (m_intSinkType == 2)
	{
		//MPEG-2 Demultiplexer (TS TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pTSMpeg2Demux, L"TS TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add TS TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else		//FileWriter (TS TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pTSSink, L"TS TimeShift FileWriter"))
			{
				(log << "Failed to add TS TimeShift FileWriter to the graph: " << hr << "\n").Write(hr);
				DestroyTSFilters();
			}
			else		//Connect Demux (TS TimeShifting)
				if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pTSMpeg2Demux))
				{
					(log << "Failed to connect Infinite Pin Tee Filter to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
					DestroyTSFilters();
				}
				else	//Add Demux Pins (TS TimeShifting)
					if FAILED(hr = graphTools.AddDemuxPins(pService, m_pTSMpeg2Demux, 1))
					{
						(log << "Failed to Add Output Pins to TS TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyTSFilters();
					}
					else		//Connect FileWriter (TS TimeShifting)
						if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pTSMpeg2Demux, m_pTSSink))
						{
							(log << "Failed to connect TS TimeShift MPEG-2 Demultiplexer to TS TimeShift FileWriter: " << hr << "\n").Write(hr);
							DestroyTSFilters();
						}
						else	//Add Demux Pins (TS TimeShifting)
							if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pTSFileName, pService, m_pTSSink, 11))
							{
								(log << "Failed to Set the File Name on the TS TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
								DestroyTSFilters();
							}
							else
								graphTools.SetReferenceClock(m_pTSMpeg2Demux);
	}
	else if (m_intSinkType == 3)
	{
		//MPEG-2 Demultiplexer (MPG TimeShifting)
		if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.demuxclsid, &m_pMPGMpeg2Demux, L"MPG TimeShift MPEG-2 Demultiplexer"))
		{
			(log << "Failed to add MPG TimeShift MPEG-2 Demultiplexer to the graph: " << hr << "\n").Write(hr);
		}
		else //MPEG-2 Multiplexer (MPG TimeShifting)
			if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.mpgmuxclsid, &m_pMPGMpeg2Mux, L"MPG TimeShift MPEG-2 Multiplexer"))
			{
				(log << "Failed to add MPG TimeShift MPEG-2 Multiplexer to the graph: " << hr << "\n").Write(hr);
				DestroyMPGFilters();
			}
			else //FileWriter (MPG TimeShifting)
				if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.timeshiftclsid, &m_pMPGSink, L"MPG TimeShift FileWriter"))
				{
					(log << "Failed to add MPG TimeShift  FileWriter to the graph: " << hr << "\n").Write(hr);
					DestroyMPGFilters();
				}
				else	//Connect Demux (MPG TimeShifting)
					if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pInfinitePinTee, m_pMPGMpeg2Demux))
					{
						(log << "Failed to connect Infinite Pin Tee Filter to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
						DestroyMPGFilters();
					}
					else	//Add Demux Pins (MPG TimeShifting)
						if FAILED(hr = graphTools.AddDemuxPins(pService, m_pMPGMpeg2Demux))
						{
							(log << "Failed to Add Output Pins to MPG TimeShift MPEG-2 Demultiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
						}
						else	//Connect Mux (MPG TimeShifting)
							if (graphTools.IsPinActive(m_pMPGMpeg2Demux, L"Audio") && FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux)))
							{
								(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Audio to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
								DestroyMPGFilters();
							}
							else	//Connect Mux (MPG TimeShifting)
								if (graphTools.IsPinActive(m_pMPGMpeg2Demux, L"Video") && FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux)))
								{
									(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
									DestroyMPGFilters();
								}
								else	//Connect FileWriter (MPG TimeShifting)
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Add FileName (MPG TimeShifting)
										if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 111))
										{
											(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else
											graphTools.SetReferenceClock(m_pMPGMpeg2Demux);

/*								if ((CComBSTR)GUID_NULL == (CComBSTR)g_pData->settings.filterguids.quantizerclsid)
								{
									if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGMpeg2Mux))
									{
										(log << "Failed to connect MPG TimeShift MPEG-2 Demultiplexer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
										DestroyMPGFilters();
									}
									else	//Connect FileWriter (MPG TimeShifting's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
										{
											(log << "Failed to connect MPG TimeShift MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else	//Add FileName (MPG TimeShifting)
											if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 111))
											{
												(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
												DestroyMPGFilters();
											}
											else
												graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
								}
								else
								{
									if FAILED(hr = graphTools.AddFilter(m_piGraphBuilder, g_pData->settings.filterguids.quantizerclsid, &m_pMPGQuantizer, L"MPG TimeShift Quantizer"))
									{
										(log << "Failed to add MPG TimeShift Quantizer to the graph: " << hr << "\n").Write(hr);
									}
									else //MPEG-2 Multiplexer (MPG TimeShifting's)
										if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Demux, m_pMPGQuantizer))
										{
											(log << "Failed to connect MPG Sink MPEG-2 Demultiplexer Video to MPG TimeShift Quantizer: " << hr << "\n").Write(hr);
											DestroyMPGFilters();
										}
										else
											if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGQuantizer, m_pMPGMpeg2Mux))
											{
												(log << "Failed to connect MPG Sink MPG Sink Quantizer Video to MPG TimeShift MPEG-2 Multiplexer: " << hr << "\n").Write(hr);
												DestroyMPGFilters();
											}
											else	//Connect FileWriter (MPG TimeShifting's)
												if FAILED(hr = graphTools.ConnectFilters(m_piGraphBuilder, m_pMPGMpeg2Mux, m_pMPGSink))
												{
													(log << "Failed to connect MPG Sink MPEG-2 Multiplexer to MPG TimeShift FileWriter: " << hr << "\n").Write(hr);
													DestroyMPGFilters();
												}
												else	//Add FileName (MPG TimeShifting)
													if FAILED(hr = m_pBDADVBTSink->AddFileName(&m_pMPGFileName, pService, m_pMPGSink, 111))
													{
														(log << "Failed to Set the File Name on the MPG TimeShift FileWriter Interface: " << hr << "\n").Write(hr);
														DestroyMPGFilters();
													}
													else
														graphTools.SetReferenceClock(m_pMPGMpeg2Demux);
								}
*/
	}

	UpdateTSFileSink(FALSE);
	m_bActive = TRUE;
	return hr;
}

void BDADVBTSinkTShift::DestroyFilter(CComPtr <IBaseFilter> &pFilter)
{
	if (pFilter)
	{
		m_piGraphBuilder->RemoveFilter(pFilter);
		pFilter.Release();
	}
}

void BDADVBTSinkTShift::DestroyFTSFilters()
{
	DestroyFilter(m_pFTSSink);
	m_pFTSDWDump = NULL;
}

void BDADVBTSinkTShift::DestroyTSFilters()
{
	DestroyFilter(m_pTSSink);
	m_pTSDWDump = NULL;
	DestroyFilter(m_pTSMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyMPGFilters()
{
	DestroyFilter(m_pMPGSink);
	m_pMPGDWDump = NULL;
	DestroyFilter(m_pMPGMpeg2Mux);
	DestroyFilter(m_pMPGQuantizer);
	DestroyFilter(m_pMPGMpeg2Demux);
}

void BDADVBTSinkTShift::DestroyAVFilters()
{
	DestroyFilter(m_pVideoSink);
	m_pVideoDWDump = NULL;
	DestroyFilter(m_pTelexSink);
	m_pTelexDWDump = NULL;
	DestroyFilter(m_pAudioSink);
	m_pAudioDWDump = NULL;
	DestroyFilter(m_pVideoQuantizer);
	DestroyFilter(m_pAVMpeg2Demux);
}

HRESULT BDADVBTSinkTShift::RemoveSinkFilters()
{
	m_bActive = FALSE;

	DestroyFTSFilters();
	DestroyTSFilters();
	DestroyMPGFilters();
	DestroyAVFilters();

	return S_OK;
}

HRESULT BDADVBTSinkTShift::SetTransportStreamPin(IPin* piPin)
{
	if (!piPin)
		return E_FAIL;

	HRESULT hr;
	PIN_INFO pinInfo;
	if FAILED(hr = piPin->QueryPinInfo(&pinInfo))
		return E_FAIL;

	m_pInfinitePinTee = pinInfo.pFilter;

	return S_OK;
}

BOOL BDADVBTSinkTShift::IsActive()
{
	return m_bActive;
}

HRESULT BDADVBTSinkTShift::GetCurFile(LPOLESTR *ppszFileName)
{
	if (!ppszFileName)
		return E_INVALIDARG;

	if(*ppszFileName)
		delete[] *ppszFileName;

	if (m_intSinkType == 1 && m_pFTSFileName)
		*ppszFileName = m_pFTSFileName;
	else if (m_intSinkType == 2 && m_pTSSink && m_pTSFileName)
		*ppszFileName = m_pTSFileName;
	else if (m_intSinkType == 3 && m_pMPGSink && m_pMPGFileName)
		*ppszFileName = m_pMPGFileName;
	else if (m_intSinkType == 4 && m_pVideoSink && m_pVideoFileName)
		*ppszFileName = m_pVideoFileName;
	else if (m_intSinkType == 4 && m_pAudioSink && m_pAudioFileName)
		*ppszFileName = m_pAudioFileName;
	else if (m_intSinkType == 4 && m_pTelexSink && m_pTelexFileName)
		*ppszFileName = m_pTelexFileName;

	if (*ppszFileName)
		return S_OK;
	else
		return E_FAIL;
}

HRESULT BDADVBTSinkTShift::GetCurFileSize(__int64 *pllFileSize)
{
	USES_CONVERSION;

	if (!pllFileSize)
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	if (m_intSinkType == 1 && m_pFTSSink)
		return m_pBDADVBTSink->GetSinkSize(m_pFTSFileName, pllFileSize);
	else if (m_intSinkType == 2 && m_pTSSink)
		return m_pBDADVBTSink->GetSinkSize(m_pTSFileName, pllFileSize);
	else if (m_intSinkType == 3 && m_pMPGSink)
		return m_pBDADVBTSink->GetSinkSize(m_pMPGFileName, pllFileSize);
	else if (m_intSinkType == 4 && m_pVideoSink)
		return m_pBDADVBTSink->GetSinkSize(m_pVideoFileName, pllFileSize);
	else if (m_intSinkType == 4 && m_pAudioSink)
		return m_pBDADVBTSink->GetSinkSize(m_pAudioFileName, pllFileSize);
	else if (m_intSinkType == 4 && m_pTelexSink)
		return m_pBDADVBTSink->GetSinkSize(m_pTelexFileName, pllFileSize);

	return hr;
}

HRESULT BDADVBTSinkTShift::UpdateTSFileSink(BOOL bAutoMode)
{

	if (m_intSinkType == 1 && m_pFTSSink)
		return SetTimeShiftInterface(m_pFTSSink, bAutoMode);
	else if (m_intSinkType == 2 && m_pTSSink)
		return SetTimeShiftInterface(m_pTSSink, bAutoMode);
	else if (m_intSinkType == 3 && m_pMPGSink)
		return SetTimeShiftInterface(m_pMPGSink, bAutoMode);
	else if (m_intSinkType == 4 && m_pVideoSink)
		return SetTimeShiftInterface(m_pVideoSink, bAutoMode);
	else if (m_intSinkType == 4 && m_pAudioSink)
		return SetTimeShiftInterface(m_pAudioSink, bAutoMode);
	else if (m_intSinkType == 4 && m_pTelexSink)
		return SetTimeShiftInterface(m_pTelexSink, bAutoMode);

	return E_FAIL;
}

HRESULT BDADVBTSinkTShift::SetTimeShiftInterface(IBaseFilter *pFilter, BOOL bAutoMode)
{
	if (!pFilter)
		return E_INVALIDARG;

	HRESULT hr = E_NOINTERFACE;
	CComQIPtr <ITSFileSink, &IID_ITSFileSink> piTSFileSink(pFilter);
	if(!piTSFileSink)
		return (log << "Failed to get ITSFileSink interface from IBaseFilter filter: " << hr << "\n").Write(hr);

	//Do auto set of buffer files.
	if (bAutoMode)
	{
		__int64 filebuffersize = 1;
		piTSFileSink->GetFileBufferSize(&filebuffersize);
		filebuffersize /= (__int64)10485760;		//Round down to 10mb blocks
		filebuffersize++;							//Allow for remainder

		int numbfiles = 1;
		while((__int64)(filebuffersize / (__int64)2) >= 4)
		{
			filebuffersize /= (__int64)2;
			numbfiles *= 2; 
		}
		numbfiles +=(__int64)2;

		filebuffersize *= (__int64)10485760;
		piTSFileSink->SetChunkReserve(filebuffersize);
		piTSFileSink->SetMaxTSFileSize(filebuffersize);
		piTSFileSink->SetMinTSFiles(numbfiles);

		return S_OK;
	}

	int	maxNumbFiles = max(g_pData->values.timeshift.numbfilesrecycled + 1, g_pData->values.timeshift.maxnumbfiles);	
	maxNumbFiles = min(100, maxNumbFiles);
	g_pData->values.timeshift.maxnumbfiles = maxNumbFiles;
	piTSFileSink->SetMaxTSFiles((USHORT)maxNumbFiles);

	__int64 bufferFileSize = (__int64)max(10, g_pData->values.timeshift.bufferfilesize);	
	bufferFileSize = (__int64)min(500, bufferFileSize);
	bufferFileSize *= (__int64)1048576;
	g_pData->values.timeshift.bufferfilesize = bufferFileSize/1048576;
	piTSFileSink->SetChunkReserve(bufferFileSize);
	piTSFileSink->SetMaxTSFileSize(bufferFileSize);

	int	numbFilesRecycled = max(2, g_pData->values.timeshift.numbfilesrecycled);	
	numbFilesRecycled = min(maxNumbFiles, numbFilesRecycled);
	g_pData->values.timeshift.numbfilesrecycled = numbFilesRecycled;
	piTSFileSink->SetMinTSFiles((USHORT)numbFilesRecycled);

	return S_OK;
}

HRESULT BDADVBTSinkTShift::ClearSinkDemuxPins()
{
    HRESULT hr = S_OK;

	if (m_intSinkType == 1 && m_pFTSSink)
	{
		return S_OK;
	}
	else if (m_intSinkType == 2 && m_pTSMpeg2Demux)
	{
		graphTools.ClearDemuxPids(m_pTSMpeg2Demux);
		return S_OK;
	}
	else if (m_intSinkType == 3 && m_pMPGMpeg2Demux)
	{
		graphTools.ClearDemuxPids(m_pMPGMpeg2Demux);
		return S_OK;
	}

	return S_FALSE;
}

HRESULT BDADVBTSinkTShift::GetReferenceDemux(CComPtr<IBaseFilter>&pDemux)
{
	if (pDemux != NULL)
		return E_INVALIDARG;

    HRESULT hr = S_OK;

	if (m_intSinkType == 2 && m_pTSMpeg2Demux)
		CComQIPtr<IBaseFilter>pDemux(m_pTSMpeg2Demux);  
	else if (m_intSinkType == 3 && m_pMPGMpeg2Demux)
		CComQIPtr<IBaseFilter>pDemux(m_pTSMpeg2Demux);  

	if (pDemux)
		return S_OK;

	return S_FALSE;
}
