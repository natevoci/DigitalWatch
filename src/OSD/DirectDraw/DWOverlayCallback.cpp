/**
 *	DWOverlayCallback.cpp
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

#include "DWOverlayCallback.h"
#include "Globals.h"


//////////////////////////////////////////////////////////////////////
// DWOverlayCallback
//////////////////////////////////////////////////////////////////////

DWOverlayCallback::DWOverlayCallback(HRESULT *phr, DWDirectDraw *pDirectDraw) :
    CUnknown("Overlay Callback Object", NULL, phr)
{
    AddRef();
	m_pDirectDraw = pDirectDraw;
}


DWOverlayCallback::~DWOverlayCallback()
{
}

HRESULT DWOverlayCallback::OnUpdateOverlay(BOOL bBefore, DWORD dwFlags, BOOL bOldVisible, const RECT *prcSrcOld, const RECT *prcDestOld, BOOL bNewVisible, const RECT *prcSrcNew, const RECT *prcDestNew)
{
    if ((g_pOSD == NULL) || (m_pDirectDraw == NULL))
    {
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: NULL DDraw object pointer was specified"))) ;
        return E_POINTER ;
    }

    if (bBefore)  // overlay is going to be updated
    {
        DbgLog((LOG_TRACE, 5, TEXT("Just turn off color keying and return"))) ;
        m_pDirectDraw->SetOverlayEnabled(FALSE);
        //g_pOSD->Render(GetTickCount());  // render the surface so that video doesn't show anymore
        return S_OK ;
    }

    //
    // After overlay has been updated. Turn on/off overlay color keying based on 
    // flags and use new source/dest position etc.
    //
    if (dwFlags & (AM_OVERLAY_NOTIFY_VISIBLE_CHANGE |   // it's a valid flag
                   AM_OVERLAY_NOTIFY_SOURCE_CHANGE  |
                   AM_OVERLAY_NOTIFY_DEST_CHANGE))
    {               
        m_pDirectDraw->SetOverlayEnabled(bNewVisible) ;  // paint/don't paint color key based on param
    }        

    if (dwFlags & AM_OVERLAY_NOTIFY_DEST_CHANGE)     // overlay destination rect change
    {
        ASSERT(prcDestOld);
        ASSERT(prcDestNew);
        m_pDirectDraw->SetOverlayPosition(prcDestNew);
    }

    return S_OK ;
}


HRESULT DWOverlayCallback::OnUpdateColorKey(COLORKEY const *pKey, DWORD dwColor)
{
	m_pDirectDraw->SetOverlayColor(dwColor);
    return S_OK ;
}


HRESULT DWOverlayCallback::OnUpdateSize(DWORD dwWidth, DWORD dwHeight, DWORD dwARWidth, DWORD dwARHeight)
{
    //PostMessage(g_pData->hWnd, WM_SIZE_CHANGE, 0, 0) ;
    return S_OK ;
}





