/**
 *	DWOSDGroup.cpp
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

#include "DWOSDGroup.h"
#include "GlobalFunctions.h"
#include "DWOSDLabel.h"
#include "DWOSDButton.h"

//////////////////////////////////////////////////////////////////////
// DWOSDGroup
//////////////////////////////////////////////////////////////////////

DWOSDGroup::DWOSDGroup()
{
	m_bVisible = FALSE;
}

DWOSDGroup::~DWOSDGroup()
{
}

HRESULT DWOSDGroup::LoadFromXML(XMLElement *pElement)
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

		if (_wcsicmp(element->name, L"label") == 0)
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

	return S_OK;
}

HRESULT DWOSDGroup::Draw(long tickCount)
{
	std::vector<DWOSDControl *>::iterator it = m_controls.begin();
	for ( ; it < m_controls.end() ; it++ )
	{
		DWOSDControl *control = *it;
		control->Render(tickCount);
	}
	return S_OK;
}

