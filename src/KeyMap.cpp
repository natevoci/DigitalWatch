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
#include "Globals.h"
#include "GlobalFunctions.h"

#include <fstream>
using namespace std;

#include <stdio.h>
#include "ParseLine.h"

KeyMap::KeyMap(void) : m_filename(0)
{
	log.AddCallback(&g_DWLogWriter);
}

KeyMap::~KeyMap(void)
{
	if (m_filename)
		delete[] m_filename;
}

BOOL KeyMap::GetFunction(int keycode, BOOL shift, BOOL ctrl, BOOL alt, LPWSTR &function)
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
			wcscpy(function, keyMap.Function);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL KeyMap::LoadKeyMap(LPWSTR filename)
{
	USES_CONVERSION;

	strCopy(m_filename, filename);

	ifstream file;
	file.open(W2A(filename));

	if (file.is_open() != 1)
	{
		return (log << "Could not open global keys file: " << filename << "\n").Show();
	}
	(log << "Opening keymap file: " << filename << "\n").Write();

	try
	{
		int line = 0;
		char charbuff[256];
		wchar_t buff[256];
		LPWSTR pBuff;
		LPWSTR pCurr;
		int state = 0;


		
		while (file.getline((LPSTR)&charbuff, 256), !file.eof() || (pBuff[0] != '\0'))
		{
			pBuff = (LPWSTR)&buff;

			long length = strlen((LPCSTR)&charbuff);
			mbstowcs(pBuff, (LPCSTR)&charbuff, length);
			pBuff[length] = 0;

			line++;

			pCurr = pBuff;
			skipWhitespaces(pCurr);

			if ((pCurr[0] == '\0') || (pCurr[0] == '#'))
				continue;

			/*
			if (pCurr[0] == '[')
			{
				pCurr++;
				LPWSTR pEOS = wcschr(pCurr, ']');

				if (pEOS == NULL)
					return_FALSE_SHOWMSG("Parse Error in " << filename << ": Line " << line << "\nMissing ']'");
				if (pEOS == pCurr)
					return_FALSE_SHOWMSG("Parse Error in " << filename << ": Line " << line << "\nMissing section name");
				pEOS[0] = '\0';

				//strCopy(currGroup.Name, pCurr)

				continue;
			}
			*/

			ParseLine parseLine;
			if (parseLine.Parse(pBuff) == FALSE)
				return (log << "Parse Error in " << filename << ": Line " << line << ":" << parseLine.GetErrorPosition() << "\n" << parseLine.GetErrorMessage() << "\n").Show();

			pCurr = parseLine.LHS.FunctionName;

			if (_wcsicmp(pCurr, L"Key") == 0)
			{
				if (parseLine.LHS.ParameterCount < 1)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nZero parameters found. Expecting at least a keycode\n").Show();
				if (parseLine.LHS.ParameterCount > 4)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nToo many parameters found\n").Show();

				KeyMapEntry newKeyMapEntry;
				ZeroMemory(&newKeyMapEntry, sizeof(KeyMapEntry));

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

				if ((pCurr[0] == '\'') && (pCurr[2] == '\''))
					newKeyMapEntry.Keycode = pCurr[1];
				else
					newKeyMapEntry.Keycode = _wtoi(pCurr);

				if (parseLine.LHS.ParameterCount >= 2)
				{
					pCurr = parseLine.LHS.Parameter[1];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					newKeyMapEntry.Shift = (_wtoi(pCurr) != 0);
				}
				if (parseLine.LHS.ParameterCount >= 3)
				{
					pCurr = parseLine.LHS.Parameter[2];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					newKeyMapEntry.Ctrl = (_wtoi(pCurr) != 0);
				}
				if (parseLine.LHS.ParameterCount >= 4)
				{
					pCurr = parseLine.LHS.Parameter[3];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					newKeyMapEntry.Alt = (_wtoi(pCurr) != 0);
				}

				if (!parseLine.HasRHS())
					return (log << "Parse Error in " << filename << ": Line " << line << "\nMissing right hand side of key assignment\n").Show();

				strCopy(newKeyMapEntry.Function, parseLine.RHS.Function);

				(log << "  Loaded Key(" << newKeyMapEntry.Keycode << ", " << newKeyMapEntry.Shift << ", " << newKeyMapEntry.Ctrl << ", " << newKeyMapEntry.Alt << ") = " << newKeyMapEntry.Function << "\n").Write();
				keyMaps.push_back(newKeyMapEntry);

				continue;
			}
		}
	}
	catch (LPWSTR str)
	{
		(log << str << "\n").Show();
		file.close();
		return FALSE;
	}
	file.close();
	return TRUE;
}
