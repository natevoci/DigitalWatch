/**
 *	DWRendererVMR9Bitmap.h
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

#ifndef DWRENDERERVMR9BITMAP_H
#define DWRENDERERVMR9BITMAP_H

#include "StdAfx.h"
#include "DWRenderer.h"

class DWRendererVMR9Bitmap : public DWRenderer
{
public:
	DWRendererVMR9Bitmap(DWSurface *pSurface);
	virtual ~DWRendererVMR9Bitmap();

	virtual void SetLogCallback(LogMessageCallback *callback);

	virtual HRESULT Initialise();
	virtual HRESULT Destroy();

	virtual HRESULT Clear();
	virtual HRESULT Present();

	HRESULT GetD3DDevice(IDirect3DDevice9 **ppD3DDevice)
	{
		if (!ppD3DDevice)
			return E_POINTER;
		*ppD3DDevice = m_pD3DDevice.p;
		(*ppD3DDevice)->AddRef();
		return S_OK;
	}

	HRESULT GetD3DSurface(IDirect3DSurface9 **ppD3DSurface)
	{
		if (!ppD3DSurface)
			return E_POINTER;
		*ppD3DSurface = m_pD3DSurface.p;
		(*ppD3DSurface)->AddRef();
		return S_OK;
	}

protected:
	HRESULT RefreshOffscreenSurface();

	CComPtr<IDirect3D9> m_pD3D;
	CComPtr<IDirect3DDevice9> m_pD3DDevice;

	CComPtr<IDirect3DSurface9> m_pD3DSurface;
	CComPtr<IDirect3DSurface9> m_pD3DSurfaceSysMem;

	CComPtr<IVMRMixerBitmap9> m_pMixerBitmap;

	long m_lastTickCount;
};

#endif
