/**
 *	IOSDImageSource.h
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

#ifndef IOSDIMAGESOURCE_H
#define IOSDIMAGESOURCE_H


// {28d85399-7459-4038-b2b0-fc4f113a4023}
DEFINE_GUID(IID_IOSDImageSource,
0x28d85399, 0x7459, 0x4038, 0xb2, 0xb0, 0xfc, 0x4f, 0x11, 0x3a, 0x40, 0x23);

interface IOSDImageSource : public IUnknown
{
	//HRESULT __stdcall GetDC(void *phdc) = 0;
	virtual STDMETHODIMP DoStuff() = 0;
};

#endif
