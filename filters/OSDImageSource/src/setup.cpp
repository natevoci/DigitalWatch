/**
 *	setup.cpp
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
#include <initguid.h>

#include "OSDImageSourceGuid.h"
#include "OSDImageSource.h"

// Filter name strings
#define OSDIMAGESOURCENAME			L"OSD Image Source Filter"
#define OSDIMAGESOURCEPROPERTIES	L"OSD Image Source Properties"

// Filter setup data
const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOSDImageSourcePin = 
{
	L"Output",      // Obsolete, not used.
	FALSE,          // Is this pin rendered?
	TRUE,           // Is it an output pin?
	FALSE,          // Can the filter create zero instances?
	FALSE,          // Does the filter create multiple instances?
	&CLSID_NULL,    // Obsolete.
	NULL,           // Obsolete.
	1,              // Number of media types.
	&sudOpPinTypes  // Pointer to media types.
};

const AMOVIESETUP_FILTER sudOSDImageSourceFilter =
{
	&CLSID_OSDImageSource,	// Filter CLSID
	OSDIMAGESOURCENAME,		// String name
	MERIT_DO_NOT_USE,		// Filter merit
	1,						// Number pins
	&sudOSDImageSourcePin	// Pin details
};

CFactoryTemplate g_Templates[] = 
{
	{ 
		OSDIMAGESOURCENAME,						// Name
		&CLSID_OSDImageSource,					// CLSID
		COSDImageSourceFilter::CreateInstance,	// Method to create an instance of MyComponent
		NULL,									// Initialization function
		&sudOSDImageSourceFilter				// Set-up information (for filters)
	},
	{
		OSDIMAGESOURCEPROPERTIES,
		&CLSID_OSDImageSourcePropertyPage,
		COSDImageSourceProp::CreateInstance,
		NULL,
		NULL
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    



////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

