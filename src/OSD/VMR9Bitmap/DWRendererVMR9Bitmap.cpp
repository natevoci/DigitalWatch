/**
 *	DWRendererVMR9Bitmap.cpp
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

#include "DWRendererVMR9Bitmap.h"
#include "Globals.h"
#include <d3dx9tex.h>

DWRendererVMR9Bitmap::DWRendererVMR9Bitmap(DWSurface *pSurface) : DWRenderer(pSurface)
{
	m_lastTickCount = 0;
}

DWRendererVMR9Bitmap::~DWRendererVMR9Bitmap()
{
}

void DWRendererVMR9Bitmap::SetLogCallback(LogMessageCallback *callback)
{
	DWRenderer::SetLogCallback(callback);
}

HRESULT DWRendererVMR9Bitmap::Initialise()
{
	HRESULT hr;

    m_pD3D.Release();
    m_pD3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (m_pD3D == NULL)
        return (log << "Failed to create Direct3D object\n").Write(E_FAIL);

    D3DDISPLAYMODE d3ddm;
    hr = m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	if FAILED(hr)
		return (log << "Failed to get D3D Adapter display mode: " << hr << "\n").Write(hr);

    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.hDeviceWindow    = g_pData->hWnd;

    hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice);
	if FAILED(hr)
		return (log << "Failed to create Direct3D device: " << hr << "\n").Write(hr);

    long lWidth = 768;
	long lHeight = 576;

	hr = m_pD3DDevice->CreateRenderTarget(lWidth, lHeight, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pD3DSurface, NULL);
	if FAILED(hr)
		return (log << "Failed to create render target: " << hr << "\n").Write(hr);

	hr = m_pD3DDevice->SetRenderTarget(0, m_pD3DSurface);
	if FAILED(hr)
		return (log << "Failed to set render target: " << hr << "\n").Write(hr);

	hr = m_pD3DDevice->ColorFill(m_pD3DSurface, NULL, 0x00000000);
	if FAILED(hr)
		return (log << "Failed to clear main surface: " << hr << "\n").Write(hr);

	hr = m_pSurface->CreateMainSurface();
	if FAILED(hr)
		return (log << "Failed to create DWSurface from directdraw back surface : " << hr << "\n").Write(hr);

	m_bInitialised = TRUE;

	return hr;
}

HRESULT DWRendererVMR9Bitmap::Destroy()
{
	HRESULT hr;

	hr = m_pSurface->Destroy();
	if FAILED(hr)
		(log << "Failed to destroy main directdraw surface: " << hr << "\n").Write();

	m_bInitialised = FALSE;

	return S_OK;
}

HRESULT DWRendererVMR9Bitmap::Clear()
{
	HRESULT hr = m_pD3DDevice->ColorFill(m_pD3DSurface, NULL, 0x00000000);
	if FAILED(hr)
		return (log << "Failed to clear main surface: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWRendererVMR9Bitmap::Present()
{
	if (!m_bInitialised)
		return S_OK;

	if (m_lTickCount < m_lastTickCount + 100)
	{
		return S_OK;
	}
	m_lastTickCount = m_lTickCount;

	HRESULT hr = S_OK;

#ifdef DEBUG
	//Display FPS
	DWSurfaceText text;

	wchar_t buffer[30];
	swprintf((LPWSTR)&buffer, L"FPS - %f", GetFPS());
	text.SetText(buffer);

	hr = m_pSurface->DrawText(&text, 0, 560);
	hr = m_pSurface->DrawText(&text, 700, 560);
#endif

	hr = RefreshOffscreenSurface();

	DWGraph *pDWGraph;
	hr = g_pTv->GetFilterGraph(&pDWGraph);
	if FAILED(hr)
		return (log << "Failed to get main filter graph: " << hr << "\n").Write(hr);

	CComPtr<IGraphBuilder> piGraphBuilder;
	hr = pDWGraph->QueryGraphBuilder(&piGraphBuilder);
	if FAILED(hr)
		return (log << "Failed to get graph builder: " << hr << "\n").Write(hr);

	FilterGraphTools graphTools;
	CComPtr<IBaseFilter> piVMR9;
	hr = graphTools.FindFilterByCLSID(piGraphBuilder, CLSID_VideoMixingRenderer9, &piVMR9);
	if FAILED(hr)
	{
		(log << "Failed to find VMR9 filter: " << hr << "\n").Write();
		return S_FALSE;
	}

	CComPtr<IVMRMixerBitmap9> pMixerBitmap;
	piVMR9.QueryInterface(&pMixerBitmap);

	VMR9AlphaBitmap alphaBitmap;
    ZeroMemory( &alphaBitmap, sizeof VMR9AlphaBitmap);

    alphaBitmap.dwFlags   = VMR9AlphaBitmap_EntireDDS;
    alphaBitmap.hdc       = NULL;
    alphaBitmap.pDDS      = m_pD3DSurfaceSysMem;

    alphaBitmap.rDest.top    = 0.0f;
    alphaBitmap.rDest.left   = 0.0f;
    alphaBitmap.rDest.bottom = 1.0f;
    alphaBitmap.rDest.right  = 1.0f;
    alphaBitmap.fAlpha       = 1.0f;

	hr = pMixerBitmap->SetAlphaBitmap(&alphaBitmap);
	if FAILED(hr)
		(log << "Failed to set VMR9 Bitmap: " << hr << "\n").Write(hr);

	return S_OK;
}

HRESULT DWRendererVMR9Bitmap::RefreshOffscreenSurface()
{
	HRESULT hr;

	if (!m_pD3DSurface)
		return E_POINTER;

	D3DSURFACE_DESC mainDesc;
	D3DSURFACE_DESC sysmemDesc;

	hr = m_pD3DSurface->GetDesc(&mainDesc);

	if (m_pD3DSurfaceSysMem)
	{
		hr = m_pD3DSurfaceSysMem->GetDesc(&sysmemDesc);
	}

	if (!m_pD3DSurfaceSysMem || (mainDesc.Width != sysmemDesc.Width) || (mainDesc.Height != sysmemDesc.Height))
	{
		m_pD3DSurfaceSysMem.Release();

		hr = m_pD3DDevice->CreateOffscreenPlainSurface(mainDesc.Width, mainDesc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_pD3DSurfaceSysMem, NULL);
		if FAILED(hr)
			return (log << "Failed to create offscreen direct3D system memory surface: " << hr << "\n").Write(hr);

		/*
		D3DLOCKED_RECT lockedRect;
		hr = m_pD3DSurfaceSysMem->LockRect(&lockedRect, NULL, D3DLOCK_DISCARD);
		if FAILED(hr)
			return (log << "Failed to lock rect: " << hr << "\n").Write(hr);

		DWORD *pSrc;
		for (int y=0 ; y<lHeight ; y++ )
		{
			pSrc = (DWORD *)((BYTE *)(lockedRect.pBits) + y * lockedRect.Pitch);
			for (int x=0 ; x<lWidth ; x++ )
			{
				*pSrc = 0x60000000;
				pSrc++;
			}
		}
		m_pD3DSurfaceSysMem->UnlockRect();
		*/

	}

	hr = m_pD3DDevice->GetRenderTargetData(m_pD3DSurface, m_pD3DSurfaceSysMem);
	if FAILED(hr)
		return (log << "Failed to get render target data: " << hr << "\n").Write(hr);

	return S_OK;
}