/**
 *	DWOnScreenDisplayWindows.cpp
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

#include "DWOnScreenDisplayWindows.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// DWOnScreenDisplayWindow
//////////////////////////////////////////////////////////////////////

DWOnScreenDisplayWindow::DWOnScreenDisplayWindow()
{
}

DWOnScreenDisplayWindow::~DWOnScreenDisplayWindow()
{
}

LPWSTR DWOnScreenDisplayWindow::Name()
{
	XMLAttribute *attr = m_pElement->Attributes.Item(L"name");
	if (attr)
	{
		return attr->value;
	}
	return L"";
}

//////////////////////////////////////////////////////////////////////
// DWOnScreenDisplayWindows
//////////////////////////////////////////////////////////////////////

DWOnScreenDisplayWindows::DWOnScreenDisplayWindows() : m_filename(0)
{

}

DWOnScreenDisplayWindows::~DWOnScreenDisplayWindows()
{
	if (m_filename)
		delete m_filename;

	std::vector<DWOnScreenDisplayWindow *>::iterator it = m_windows.begin();
	for ( ; it != m_windows.end() ; it++ )
	{
		delete *it;
	}
	m_windows.clear();
}

void DWOnScreenDisplayWindows::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	std::vector<DWOnScreenDisplayWindow *>::iterator it = m_windows.begin();
	for ( ; it != m_windows.end() ; it++ )
	{
		DWOnScreenDisplayWindow *window = *it;
		window->SetLogCallback(callback);
	}
}

HRESULT DWOnScreenDisplayWindows::Load(LPWSTR filename)
{
	(log << "Loading OSD file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	strCopy(m_filename, filename);

	HRESULT hr;
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if FAILED(hr = file.Load(m_filename))
	{
		return (log << "Could not load OSD file: " << m_filename << "\n").Show(hr);
	}

	XMLElement *element = NULL;
	XMLElement *subelement = NULL;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"window") == 0)
		{
			DWOnScreenDisplayWindow *window = new DWOnScreenDisplayWindow();
			window->SetLogCallback(m_pLogCallback);
			window->m_pElement = element;
			window->m_pElement->AddRef();
			m_windows.push_back(window);
		}
	}

	return S_OK;
}

DWOnScreenDisplayWindow *DWOnScreenDisplayWindows::Item(LPWSTR pName)
{
	std::vector<DWOnScreenDisplayWindow *>::iterator it = m_windows.begin();
	for ( ; it < m_windows.end() ; it++ )
	{
		DWOnScreenDisplayWindow *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}
