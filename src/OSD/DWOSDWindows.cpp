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
#include "DWOSDList.h"

//////////////////////////////////////////////////////////////////////
// DWOSDWindow
//////////////////////////////////////////////////////////////////////

DWOSDWindow::DWOSDWindow()
{
	m_pName = NULL;
	m_pHighlightedControl = NULL;
	m_bHideWindowsBehindThisOne = FALSE;
	m_pSelectedName = NULL;
	m_pMaskedName = NULL;
}

DWOSDWindow::~DWOSDWindow()
{
	if (m_pName)
		delete[] m_pName;

	if (m_pSelectedName)
		delete[] m_pSelectedName;

	if (m_pMaskedName)
		delete[] m_pMaskedName;

	ClearParameters();

	std::vector<DWOSDControl *>::iterator it = m_controls.begin();
	for ( ; it < m_controls.end() ; it++ )
	{
		delete *it;
	}
	m_controls.clear();
}

void DWOSDWindow::SetLogCallback(LogMessageCallback *callback)
{
	m_keyMap.SetLogCallback(callback);
}

LPWSTR DWOSDWindow::Name()
{
	return m_pName;
}

HRESULT DWOSDWindow::Render(long tickCount)
{
	CAutoLock controlsLock(&m_controlsLock);

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
	CAutoLock controlsLock(&m_controlsLock);

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
	if (m_pHighlightedControl == NULL)
		return S_FALSE;

	LPWSTR command = m_pHighlightedControl->OnUp();
	return OnKeyCommand(command);
}

HRESULT DWOSDWindow::OnDown()
{
	if (m_pHighlightedControl == NULL)
		return S_FALSE;

	LPWSTR command = m_pHighlightedControl->OnDown();
	return OnKeyCommand(command);
}

HRESULT DWOSDWindow::OnLeft()
{
	if (m_pHighlightedControl == NULL)
		return S_FALSE;

	LPWSTR command = m_pHighlightedControl->OnLeft();
	return OnKeyCommand(command);
}

HRESULT DWOSDWindow::OnRight()
{
	if (m_pHighlightedControl == NULL)
		return S_FALSE;

	LPWSTR command = m_pHighlightedControl->OnRight();
	return OnKeyCommand(command);
}

HRESULT DWOSDWindow::OnSelect()
{
	if (m_pHighlightedControl == NULL)
		return S_FALSE;

	LPWSTR command = m_pHighlightedControl->OnSelect();
	return OnKeyCommand(command);
}

HRESULT DWOSDWindow::OnSelected(LPWSTR lpName)
{
	if (lpName == NULL)
		return S_FALSE;

	m_pSelectedName = lpName;

	return S_OK;
}

HRESULT DWOSDWindow::OnMasked(LPWSTR lpName)
{
	if (lpName == NULL)
		return S_FALSE;

	m_pMaskedName = lpName;

	return S_OK;
}

HRESULT DWOSDWindow::OnKeyCommand(LPWSTR command)
{
	if (command == NULL)
		return S_FALSE;

	if (command[0] == '#')
		return SetHighlightedControl(++command);

	g_pTv->ExecuteCommandsImmediate(command);

	return S_OK;
}


BOOL DWOSDWindow::HideWindowsBehindThisOne()
{
	return m_bHideWindowsBehindThisOne;
}

void DWOSDWindow::ClearParameters()
{
	CAutoLock parametersLock(&m_parametersLock);

	std::vector<LPWSTR>::iterator it = m_parameters.begin();
	for ( ; it < m_parameters.end() ; it++ )
	{
		delete[] (*it);
	}
	m_parameters.clear();
}

void DWOSDWindow::AddParameter(LPWSTR pwcsParameter)
{
	CAutoLock parametersLock(&m_parametersLock);

	LPWSTR newArg = NULL;
	strCopy(newArg, pwcsParameter);
	m_parameters.push_back(newArg);
}

LPWSTR DWOSDWindow::GetParameter(long nIndex)
{
	CAutoLock parametersLock(&m_parametersLock);

	if ((nIndex < 0) || (nIndex >= (long)m_parameters.size()))
		return NULL;
	return m_parameters.at(nIndex);
}

HRESULT DWOSDWindow::LoadFromXML(XMLElement *pElement)
{
	CAutoLock controlsLock(&m_controlsLock);

	HRESULT hr;
	XMLAttribute *attr;

	attr = pElement->Attributes.Item(L"name");
	if (attr == NULL)
		return (log << "Cannot have a window without a name\n").Write();
	if (attr->value[0] == '\0')
		return (log << "Cannot have a blank window name\n").Write();
	strCopy(m_pName, attr->value);

	DWRenderer *pDWRenderer;
	hr = g_pOSD->GetOSDRenderer(&pDWRenderer);
	if FAILED(hr)
		return (log << "Failed to get OSD Renderer: " << hr << "\n").Write(hr);

	DWSurface *pBackSurface;
	hr = pDWRenderer->GetSurface(&pBackSurface);
	if FAILED(hr)
		return (log << "Failed to get surface: " << hr << "\n").Write(hr);

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
		else if (_wcsicmp(element->name, L"hidelowerwindows") == 0)
		{
			m_bHideWindowsBehindThisOne = TRUE;
		}
		else if (_wcsicmp(element->name, L"group") == 0)
		{
			control = new DWOSDGroup(pBackSurface);
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"label") == 0)
		{
			control = new DWOSDLabel(pBackSurface);
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"button") == 0)
		{
			control = new DWOSDButton(pBackSurface);
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"list") == 0)
		{
			control = new DWOSDList(pBackSurface);
			control->SetLogCallback(m_pLogCallback);
			if FAILED(control->LoadFromXML(element))
			{
				delete control;
				control = NULL;
			}
		}
		else if (_wcsicmp(element->name, L"selection") == 0 && element->value)
		{
			strCopy(m_pSelectedName, element->value);

			if (m_pSelectedName)
				m_pHighlightedControl->SetSelect(TRUE);
		}
		else if (_wcsicmp(element->name, L"maskname") == 0 && element->value)
		{
			strCopy(m_pMaskedName, element->value);

			if (m_pMaskedName)
				m_pHighlightedControl->SetMask(TRUE);
		}

		if (control)
		{
			m_controls.push_back(control);
		}
	}

	DWOSDControl *firstSelectableControl = NULL;
	BOOL bFound = FALSE;
	std::vector<DWOSDControl *>::iterator it = m_controls.begin();

	for ( ; it < m_controls.end() ; it++ )
	{
		DWOSDControl *control = *it;
		if (control->CanSelect())
		{
			if (!bFound && m_pSelectedName && control->Name())
			{
				LPWSTR selection = g_pData->GetSelectionItem(m_pSelectedName);
				if (selection)
					bFound = (BOOL)((wcsstr(control->Name(), selection) != NULL));

				if (bFound)
				{
					m_pHighlightedControl = control;
					m_pHighlightedControl->SetSelect(TRUE);
				}
			}
			else
				control->SetSelect(FALSE);
		}
	}

	it = m_controls.begin();
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
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DWOSDWindows
//////////////////////////////////////////////////////////////////////

DWOSDWindows::DWOSDWindows() : m_filename(0)
{

}

DWOSDWindows::~DWOSDWindows()
{
	CAutoLock windowsLock(&m_windowsLock);

	if (m_filename)
		delete m_filename;

	std::vector<DWOSDWindow *>::iterator itWindows = m_windows.begin();
	for ( ; itWindows != m_windows.end() ; itWindows++ )
	{
		delete *itWindows;
	}
	m_windows.clear();

	std::vector<DWOSDImage *>::iterator itImages = m_images.begin();
	for ( ; itImages != m_images.end() ; itImages++ )
	{
		delete *itImages;
	}
	m_images.clear();
}

void DWOSDWindows::SetLogCallback(LogMessageCallback *callback)
{
	CAutoLock windowsLock(&m_windowsLock);

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
	CAutoLock windowsLock(&m_windowsLock);
	CAutoLock imagesLock(&m_imagesLock);

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
	(log << "Loaded " << (long)m_windows.size() << " windows\n").Write();
	(log << "Loaded " << (long)m_images.size() << " images\n").Write();

	indent.Release();
	(log << "Finished loading OSD file : " << hr << "\n").Write();

	return S_OK;
}

DWOSDWindow *DWOSDWindows::GetWindow(LPWSTR pName)
{
	CAutoLock windowsLock(&m_windowsLock);

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
	CAutoLock imagesLock(&m_imagesLock);

	std::vector<DWOSDImage *>::iterator it = m_images.begin();
	for ( ; it < m_images.end() ; it++ )
	{
		DWOSDImage *item = *it;
		if (_wcsicmp(item->Name(), pName) == 0)
			return item;
	}
	return NULL;
}


