/**
 *	DWOSDWindows.cpp
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

#include "DWOSDWindows.h"
#include "Globals.h"
#include "GlobalFunctions.h"
#include "DWOSDGroup.h"
#include "DWOSDLabel.h"
#include "DWOSDButton.h"

//////////////////////////////////////////////////////////////////////
// DWOSDWindow
//////////////////////////////////////////////////////////////////////

DWOSDWindow::DWOSDWindow()
{
	m_pName = NULL;
	m_pHighlightedControl = NULL;
}

DWOSDWindow::~DWOSDWindow()
{
	if (m_pName)
		delete[] m_pName;
}

LPWSTR DWOSDWindow::Name()
{
	return m_pName;
}

HRESULT DWOSDWindow::Render(long tickCount)
{
	std::vector<DWOSDControl *>::iterator it = m_controls.begin();
	for ( ; it < m_controls.end() ; it++ )
	{
		DWOSDControl *control = *it;
		control->Render(tickCount);
	}
	return S_OK;
}

DWOSDControl* DWOSDWindow::GetControl(LPWSTR pName)
{
	std::vector<DWOSDControl *>::iterator it = m_controls.begin();
	for ( ; it < m_controls.end() ; it++ )
	{
		DWOSDControl *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}

HRESULT DWOSDWindow::GetKeyFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function)
{
	return m_keyMap.GetFunction(keycode, shift, ctrl, alt, function);
}

HRESULT DWOSDWindow::OnUp()
{
	if (m_pHighlightedControl)
	{
		LPWSTR wszNextControl = m_pHighlightedControl->OnUp();
		SetHighlightedControl(wszNextControl);
	}
	return S_FALSE;
}

HRESULT DWOSDWindow::OnDown()
{
	if (m_pHighlightedControl)
	{
		LPWSTR wszNextControl = m_pHighlightedControl->OnDown();
		SetHighlightedControl(wszNextControl);
	}
	return S_FALSE;
}

HRESULT DWOSDWindow::OnLeft()
{
	if (m_pHighlightedControl)
	{
		LPWSTR wszNextControl = m_pHighlightedControl->OnLeft();
		SetHighlightedControl(wszNextControl);
	}
	return S_FALSE;
}

HRESULT DWOSDWindow::OnRight()
{
	if (m_pHighlightedControl)
	{
		LPWSTR wszNextControl = m_pHighlightedControl->OnRight();
		SetHighlightedControl(wszNextControl);
	}
	return S_FALSE;
}

HRESULT DWOSDWindow::OnSelect()
{
	if (m_pHighlightedControl)
	{
		LPWSTR command = m_pHighlightedControl->OnSelect();
		if (command)
		{
			HRESULT hr = g_pTv->ExecuteCommands(command);
			if FAILED(hr)
				(log << "An error occured executing the command " << command << " on control " << m_pHighlightedControl->Name() << "\n").Write();
			return S_OK;
		}
	}
	return S_FALSE;
}

HRESULT DWOSDWindow::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;

	attr = pElement->Attributes.Item(L"name");
	if (attr == NULL)
		return (log << "Cannot have a window without a name\n").Write();
	if (attr->value[0] == '\0')
		return (log << "Cannot have a blank window name\n").Write();
	strCopy(m_pName, attr->value);

	XMLElement *element = NULL;

	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = pElement->Elements.Item(item);
		DWOSDControl* control = NULL;

		if (_wcsicmp(element->name, L"keys") == 0)
		{
			m_keyMap.LoadFromXML(&element->Elements);
		}
		else if (_wcsicmp(element->name, L"group") == 0)
		{
			control = new DWOSDGroup();
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"label") == 0)
		{
			control = new DWOSDLabel();
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"button") == 0)
		{
			control = new DWOSDButton();
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}

		if (control)
		{
			attr = element->Attributes.Item(L"name");
			if (attr)
				control->SetName(attr->value);
			m_controls.push_back(control);
		}
	}

	DWOSDControl *firstSelectableControl = NULL;
	BOOL bFound = FALSE;
	std::vector<DWOSDControl *>::iterator it = m_controls.begin();
	for ( ; it < m_controls.end() ; it++ )
	{
		DWOSDControl *control = *it;
		if (control->CanHighlight())
		{
			if (!bFound)
			{
				bFound = control->IsHighlighted();
				if (bFound)
					m_pHighlightedControl = control;
			}
			else
				control->SetHighlight(FALSE);

			if (!firstSelectableControl)
				firstSelectableControl = control;
		}		
	}
	if (!bFound && firstSelectableControl)
	{
		m_pHighlightedControl = firstSelectableControl;
		firstSelectableControl->SetHighlight(TRUE);
	}

	return S_OK;
}

HRESULT DWOSDWindow::SetHighlightedControl(LPWSTR wszNextControl)
{
	if (wszNextControl)
	{
		DWOSDControl* nextControl = GetControl(wszNextControl);
		if (nextControl)
		{
			if (m_pHighlightedControl)
				m_pHighlightedControl->SetHighlight(FALSE);
			m_pHighlightedControl = nextControl;
			m_pHighlightedControl->SetHighlight(TRUE);
			return S_OK;
		}
	}
	return S_FALSE;
}


//////////////////////////////////////////////////////////////////////
// DWOSDWindows
//////////////////////////////////////////////////////////////////////

DWOSDWindows::DWOSDWindows() : m_filename(0)
{

}

DWOSDWindows::~DWOSDWindows()
{
	if (m_filename)
		delete m_filename;

	std::vector<DWOSDWindow *>::iterator it = m_windows.begin();
	for ( ; it != m_windows.end() ; it++ )
	{
		delete *it;
	}
	m_windows.clear();
}

void DWOSDWindows::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	std::vector<DWOSDWindow *>::iterator it = m_windows.begin();
	for ( ; it != m_windows.end() ; it++ )
	{
		DWOSDWindow *item = *it;
		item->SetLogCallback(callback);
	}
}

HRESULT DWOSDWindows::Load(LPWSTR filename)
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
			DWOSDWindow *window = new DWOSDWindow();
			window->SetLogCallback(m_pLogCallback);

			if (window->LoadFromXML(element) == S_OK)
				m_windows.push_back(window);
			else
				delete window;
		}
		else if (_wcsicmp(element->name, L"image") == 0)
		{
			DWOSDImage *image = new DWOSDImage();
			image->SetLogCallback(m_pLogCallback);

			if (image->LoadFromXML(element) == S_OK)
				m_images.push_back(image);
			else
				delete image;
		}
	}

	return S_OK;
}

DWOSDWindow *DWOSDWindows::GetWindow(LPWSTR pName)
{
	std::vector<DWOSDWindow *>::iterator it = m_windows.begin();
	for ( ; it < m_windows.end() ; it++ )
	{
		DWOSDWindow *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}

DWOSDImage *DWOSDWindows::GetImage(LPWSTR pName)
{
	std::vector<DWOSDImage *>::iterator it = m_images.begin();
	for ( ; it < m_images.end() ; it++ )
	{
		DWOSDImage *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}


