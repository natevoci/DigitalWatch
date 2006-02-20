/**
 *	DWSurfaceRendererVMR9Bitmap.cpp
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

#include "DWSurface.h"
#include "DWRendererVMR9Bitmap.h"
#include "DWSurfaceRendererVMR9Bitmap.h"
#include "Globals.h"
#include "GlobalFunctions.h"

//#include "DXUtil.h"
//#include "D3DEnumeration.h"
//#include "D3DSettings.h"
//#include "D3DApp.h"
//#include "D3DFont.h"
//#include "D3DUtil.h"

//////////////////////////////////////////////////////////////////////
// DWSurfaceRendererVMR9Bitmap
//////////////////////////////////////////////////////////////////////

DWSurfaceRendererVMR9Bitmap::DWSurfaceRendererVMR9Bitmap()
{
	m_hInstance = 0;
	m_nResource = 0;
	m_szBitmap = NULL;
}

DWSurfaceRendererVMR9Bitmap::~DWSurfaceRendererVMR9Bitmap()
{
	if (m_szBitmap)
		delete[] m_szBitmap;
	m_szBitmap = NULL;

	std::vector<DWSurfaceRendererVMR9BitmapFont *>::iterator it = m_fonts.begin();
	for ( ; it < m_fonts.end() ; it++ )
	{
		if ((*it)->pFont)
			delete (*it)->pFont;
		delete *it;
	}
	m_fonts.clear();
}

HRESULT DWSurfaceRendererVMR9Bitmap::CreateMainSurface()
{
	HRESULT hr = S_OK;

	// Get OSD Renderer
	DWRenderer *pOSDRenderer;
	hr = g_pOSD->GetOSDRenderer(&pOSDRenderer);
	if FAILED(hr)
		return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);

	// Cast as VMR9Bitmap renderer
	DWRendererVMR9Bitmap *pOSDRendererVMR9Bitmap = dynamic_cast<DWRendererVMR9Bitmap *>(pOSDRenderer);
	if (!pOSDRendererVMR9Bitmap)
		return (log << "Failed to cast OSD Renderer as VMR9 Bitmap OSD Renderer\n").Write(E_FAIL);

	m_Width = pOSDRenderer->GetBackBufferWidth();
	m_Height = pOSDRenderer->GetBackBufferHeight();

	hr = pOSDRendererVMR9Bitmap->GetD3DDevice(&m_pD3DDevice);
	if FAILED(hr)
		return (log << "Failed to get D3D Device: " << hr << "\n").Write(hr);

	hr = pOSDRendererVMR9Bitmap->GetD3DSurface(&m_pD3DSurface);
	if FAILED(hr)
		return (log << "Failed to get D3D Surface: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::Create(long width, long height)
{
	HRESULT hr = S_OK;

	// Get OSD Renderer
	DWRenderer *pOSDRenderer;
	hr = g_pOSD->GetOSDRenderer(&pOSDRenderer);
	if FAILED(hr)
		return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);

	// Cast as VMR9Bitmap renderer
	DWRendererVMR9Bitmap *pOSDRendererVMR9Bitmap = dynamic_cast<DWRendererVMR9Bitmap *>(pOSDRenderer);
	if (!pOSDRendererVMR9Bitmap)
		return (log << "Failed to cast OSD Renderer as VMR9 Bitmap OSD Renderer\n").Write(E_FAIL);

	m_Width = width;
	m_Height = height;

	hr = pOSDRendererVMR9Bitmap->GetD3DDevice(&m_pD3DDevice);
	if FAILED(hr)
		return (log << "Failed to get D3D Device: " << hr << "\n").Write(hr);

	hr = m_pD3DDevice->CreateRenderTarget(m_Width, m_Height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pD3DSurface, NULL);
	if FAILED(hr)
		return (log << "Failed to create render target for surface: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::LoadBitmap(HINSTANCE hInst, UINT nRes)
{
	m_hInstance = hInst;
	m_nResource = nRes;
	return LoadBitmap();
}

HRESULT DWSurfaceRendererVMR9Bitmap::LoadBitmap(LPCWSTR szBitmap)
{
	strCopy(m_szBitmap, szBitmap);
	return LoadBitmap();
}

HRESULT DWSurfaceRendererVMR9Bitmap::Destroy()
{
	HRESULT hr = S_OK;
/*
	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;
		screen->piDDSurface->Release();
		delete screen;
	}
	m_surfaces.clear();
*/
	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::Clear()
{
	HRESULT hr = m_pD3DDevice->ColorFill(m_pD3DSurface, NULL, 0x00000000);
	if FAILED(hr)
		return (log << "Failed to clear main surface: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::SetColorKey(COLORREF dwColorKey)
{
	HRESULT hr = S_OK;

	m_bColorKey = TRUE;
	m_dwColorKey = dwColorKey;

	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::Blt(DWSurfaceRenderer *targetSurface, RECT* lprcDest /*= NULL*/, RECT* lprcSrc /*= NULL*/)
{
	HRESULT hr = S_OK;

	RECT rcDest;
	RECT rcSrc;

	DWSurfaceRendererVMR9Bitmap* targetVMR9Surface = dynamic_cast<DWSurfaceRendererVMR9Bitmap*>(targetSurface);
	if (!targetVMR9Surface)
		return (log << "Failed to cast Surface Renderer to VMR9 Bitmap Surface Renderer\n").Write(E_POINTER);

	if (lprcDest)
		SetRect(&rcDest, lprcDest->left, lprcDest->top, lprcDest->right, lprcDest->bottom);
	else
		SetRect(&rcDest, 0, 0, 0, 0);

	if (lprcSrc)
		SetRect(&rcSrc, lprcSrc->left, lprcSrc->top, lprcSrc->right, lprcSrc->bottom);
	else
		SetRect(&rcSrc, 0, 0, 0, 0);

	D3DSURFACE_DESC desc;
	hr = targetVMR9Surface->m_pD3DSurface->GetDesc(&desc);

	FixRects(rcSrc, rcDest, desc.Width, desc.Height);

	//If the coordinates are off screen then return ok
	if ((rcDest.left >= (long)desc.Width) ||
		(rcDest.top >= (long)desc.Height) ||
		(rcDest.right <= 0) ||
		(rcDest.bottom <= 0))
		return S_OK;

	if ((rcSrc.left >= m_Width) ||
		(rcSrc.top >= m_Height))
		return S_OK;

	hr = m_pD3DDevice->StretchRect(m_pD3DSurface, &rcSrc, targetVMR9Surface->m_pD3DSurface, &rcDest, D3DTEXF_LINEAR);
	if FAILED(hr)
		return (log << "Failed to Blt VMR9 Bitmap surfaces: " << hr << "\n").Write(hr);

	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::DrawText(DWSurfaceText *text, int x, int y)
{
	HRESULT hr = S_OK;
#ifndef USE_CD3DFONT
	int length;
	HDC hDC;
#endif
	USES_CONVERSION;

	LPWSTR pString = text->GetText();

	if (pString == NULL)
		(log << "DWSurfaceRendererVMR9Bitmap::DrawText : No text defined\n").Write(E_FAIL);

#ifndef USE_CD3DFONT
	ID3DXFont *pFont = NULL;
#else
	CD3DFont *pFont = NULL;
#endif

	std::vector<DWSurfaceRendererVMR9BitmapFont *>::iterator it = m_fonts.begin();
	for ( ; it < m_fonts.end() ; it++ )
	{
		DWSurfaceRendererVMR9BitmapFont *font = *it;
		if (memcmp(&font->logfont, &text->font, sizeof(LOGFONT)) == 0)
		{
#ifndef USE_CD3DFONT
			pFont = font->pFont;
#else
			pFont = font->pFont;
#endif
			break;
		}
	}

	if (pFont == NULL)
	{
#ifndef USE_CD3DFONT
		HFONT hFont;

		hDC = GetDC( NULL );
		ReleaseDC( NULL, hDC );

		hFont = CreateFontIndirect(&text->font);
		if( hFont == NULL )
			return E_FAIL;

		hr = D3DXCreateFont( m_pD3DDevice, hFont, &pFont );
		if FAILED(hr)
			return (log << "Failed to create font: " << hr << "\n").Write(hr);

		DeleteObject( hFont );
#else
		pFont = new CD3DFont( text->font.lfFaceName, (DWORD)(text->font.lfHeight/1.5f));
		pFont->InitDeviceObjects(m_pD3DDevice);
		pFont->RestoreDeviceObjects();
#endif

		DWSurfaceRendererVMR9BitmapFont *font = new DWSurfaceRendererVMR9BitmapFont();
		memcpy(&font->logfont, &text->font, sizeof(LOGFONT));
		font->pFont = pFont;
		m_fonts.push_back(font);
	}

	hr = m_pD3DDevice->SetRenderTarget(0, m_pD3DSurface);
	if FAILED(hr)
		return (log << "Failed to set render target: " << hr << "\n").Write(hr);
	
	hr = m_pD3DDevice->BeginScene();

#ifndef USE_CD3DFONT
	RECT rc;
	SetRect( &rc, x, y, 0, 0 );
	hr = pFont->Begin();
	if FAILED(hr)
		return hr;

	length = pFont->DrawText(W2T(pString), -1, &rc, DT_SINGLELINE | DT_CALCRECT, 0);
	if (length <= 0)
		return E_FAIL;

    length = pFont->DrawText(W2T(pString), -1, &rc, DT_SINGLELINE, text->crTextColor);
	if (length <= 0)
		return E_FAIL;

    hr = pFont->End();
	if FAILED(hr)
		return hr;
#else
	hr = pFont->DrawText((float)x, (float)y, text->crTextColor, W2T(pString), D3DFONT_FILTERED);
#endif

	hr = m_pD3DDevice->EndScene();
	if FAILED(hr)
		return hr;

	return S_OK;
	return hr;
}

HRESULT DWSurfaceRendererVMR9Bitmap::LoadBitmap()
{
	USES_CONVERSION;

	HRESULT hr;
    HBITMAP hbm;

	if (m_szBitmap)
	{
	    hbm = (HBITMAP)LoadImage(NULL, W2T(m_szBitmap), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if (hbm == NULL)
			return E_FAIL;
	}
	else
	{
		hbm = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(m_nResource), IMAGE_BITMAP, 0, 0, 0L);
		if (hbm == NULL)
			return E_FAIL;
	}

    BITMAP bm;
    GetObject(hbm, sizeof(bm), &bm);
	DeleteObject(hbm);

	if FAILED(hr = Create(bm.bmWidth, bm.bmHeight))
		return (log << "Failed to create surface for image\n").Write(hr);

	D3DCOLOR dwColorKey = (m_bColorKey) ? m_dwColorKey : 0;

	if (m_szBitmap)
	{
		wchar_t file[MAX_PATH];
		swprintf((LPWSTR)&file, L"%s%s", g_pData->application.appPath, m_szBitmap);

		hr = D3DXLoadSurfaceFromFile(m_pD3DSurface, NULL, NULL, W2A((LPWSTR)&file), NULL, D3DX_FILTER_NONE, dwColorKey, NULL);
	}
	else
	{
		hr = D3DXLoadSurfaceFromResource(m_pD3DSurface, NULL, NULL, m_hInstance, MAKEINTRESOURCE(m_nResource), NULL, D3DX_FILTER_NONE, dwColorKey, NULL);
	}

    return S_OK;
}

void DWSurfaceRendererVMR9Bitmap::Restore()
{
/*
	std::vector<DWScreenSurface *>::iterator it = m_surfaces.begin();
	for ( ; it < m_surfaces.end() ; it++ )
	{
		DWScreenSurface* screen = *it;
		screen->piDDSurface->Restore();
	}
*/
}

void DWSurfaceRendererVMR9Bitmap::FixRects(RECT &rcSrc, RECT &rcDest, long destSurfaceWidth, long destSurfaceHeight)
{
	if (rcSrc.right == 0)
		rcSrc.right = m_Width;
	if (rcSrc.bottom == 0)
		rcSrc.bottom = m_Height;

	if (rcSrc.left + rcSrc.right > m_Width)
		rcSrc.right = m_Width - rcSrc.left;
	if (rcSrc.top + rcSrc.bottom > m_Height)
		rcSrc.bottom = m_Height - rcSrc.top;


	if (rcDest.right == 0)
		rcDest.right = rcSrc.right;
	if (rcDest.bottom == 0)
		rcDest.bottom = rcSrc.bottom;

	rcDest.right += rcDest.left;
	rcDest.bottom += rcDest.top;

	if (rcDest.right > destSurfaceWidth)
	{
		double part = (destSurfaceWidth - rcDest.left) / (double)(rcDest.right - rcDest.left);
		rcSrc.right = (long)(rcSrc.right * part);
		rcDest.right = destSurfaceWidth;
	}
	if (rcDest.left < 0)
	{
		double part = (0 - rcDest.left) / (double)(rcDest.right - rcDest.left);
		rcSrc.left  += (long)(rcSrc.right * part);
		rcSrc.right -= (long)(rcSrc.right * part);
		rcDest.left = 0;
	}
	if (rcDest.bottom > destSurfaceHeight)
	{
		double part = (destSurfaceHeight - rcDest.top) / (double)(rcDest.bottom - rcDest.top);
		rcSrc.bottom = (long)(rcSrc.bottom * part);
		rcDest.bottom = destSurfaceHeight;
	}
	if (rcDest.top < 0)
	{
		double part = (0 - rcDest.top) / (double)(rcDest.bottom - rcDest.top);
		rcSrc.top += (long)(rcSrc.bottom * part);
		rcSrc.bottom -= (long)(rcSrc.bottom * part);
		rcDest.top = 0;
	}


	
	rcSrc.right += rcSrc.left;
	rcSrc.bottom += rcSrc.top;
}

