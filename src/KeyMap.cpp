/**
 *	KeyMap.cpp
 *	Copyright (C) 2003-2004 Nate
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

#include "KeyMap.h"
#include "ParseLine.h"
#include "GlobalFunctions.h"
#include <stdio.h>
#include "XMLDocument.h"

KeyMap::KeyMap(void) : m_filename(0)
{
}

KeyMap::~KeyMap(void)
{
	if (m_filename)
		delete[] m_filename;
}

BOOL KeyMap::GetFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR *function)
{
	std::vector<KeyMapEntry>::iterator it = keyMaps.begin();
	for ( ; it != keyMaps.end() ; it++ )
	{
		KeyMapEntry keyMap = *it;
		if ((keyMap.Keycode == keycode) &&
			(keyMap.Shift == shift) &&
			(keyMap.Ctrl == ctrl) &&
			(keyMap.Alt == alt))
		{
			strCopy(*function, keyMap.Function);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL KeyMap::LoadKeyMap(LPWSTR filename)
{
	(log << "Loading Keys file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if (file.Load(m_filename) != S_OK)
	{
		return (log << "Could not load keys file: " << m_filename << "\n").Show(FALSE);
	}

	XMLElement *element;
	XMLAttribute *attr;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		element = file.Elements.Item(item);
		if ((_wcsicmp(element->name, L"Key") == 0) || (_wcsicmp(element->name, L"Mouse") == 0))
		{
			KeyMapEntry newKeyMapEntry;
			ZeroMemory(&newKeyMapEntry, sizeof(KeyMapEntry));

			attr = element->Attributes.Item(L"code");
			if (attr == NULL)
				continue;

			if (_wcsicmp(element->name, L"Key") == 0)
			{
				if ((attr->value[0] == '\'') && (attr->value[2] == '\''))
					newKeyMapEntry.Keycode = attr->value[1];
				else
					newKeyMapEntry.Keycode = _wtoi(attr->value);
			}
			else
			{
				newKeyMapEntry.Keycode = -1 * _wtoi(attr->value);
			}

			attr = element->Attributes.Item(L"shift");
			newKeyMapEntry.Shift = (attr) && (attr->value[0] != '0');

			attr = element->Attributes.Item(L"ctrl");
			newKeyMapEntry.Ctrl = (attr) && (attr->value[0] != '0');

			attr = element->Attributes.Item(L"alt");
			newKeyMapEntry.Alt = (attr) && (attr->value[0] != '0');

			strCopy(newKeyMapEntry.Function, element->value);

			keyMaps.push_back(newKeyMapEntry);
			continue;
		}
	}

	indent.Release();
	(log << "Finished Loading Keys File\n").Write();

	return TRUE;
}
