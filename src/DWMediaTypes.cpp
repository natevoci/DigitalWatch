/**
 *	DWMediaTypes.cpp
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

#include "DWMediaTypes.h"
#include "ParseLine.h"
#include "GlobalFunctions.h"
//#include "FileWriter.h"
//#include "FileReader.h"
#include "XMLDocument.h"

DWMediaTypes::DWMediaTypes() : m_filename(0)
{

}

DWMediaTypes::~DWMediaTypes()
{
	if (m_filename)
		delete m_filename;

	std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
	for ( ; it != m_mediaTypes.end() ; it++ )
	{
		delete *it;
	}
	m_mediaTypes.clear();
}

HRESULT DWMediaTypes::Load(LPWSTR filename)
{
	HRESULT hr;

	strCopy(m_filename, filename);

	//FileReader file;
	XMLDocument file;
	if (FAILED(hr = file.Load(m_filename)))
	{
		return (log << "Could not open media types file: " << m_filename << "\n").Show(hr);
	}

	(log << "Loading media types file: " << m_filename << "\n").Write();

	try
	{
/*
		DWMediaType *currMediaType = NULL;

		while (file.ReadLine(pBuff) == S_OK)
		{
			line++;

			pCurr = pBuff;
			skipWhitespaces(pCurr);

			if ((pCurr[0] == '\0') || (pCurr[0] == '#'))
				continue;

			ParseLine parseLine;
			if (parseLine.Parse(pBuff) == FALSE)
				return (log << "Parse Error in " << m_filename << ": Line " << line << ":" << parseLine.GetErrorPosition() << "\n" << parseLine.GetErrorMessage() << "\n").Show(E_FAIL);

			if (parseLine.HasRHS())
				return (log << "Parse Error in " << m_filename << ": Line " << line << "\nEquals not valid for this command\n").Show(E_FAIL);

			pCurr = parseLine.LHS.FunctionName;

			if (_wcsicmp(pCurr, L"MediaType") == 0)
			{
				std::vector<DWMediaType *>::iterator it = m_mediaTypes.begin();
				currMediaType = NULL;
				for ( ; it != m_mediaTypes.end() ; it++ )
				{
					if (_wcsicmp((*it)->name, pCurr) == 0)
					{
						currMediaType = *it;
					}
				}
			}
		}
*/
	}
	catch (LPWSTR str)
	{
		(log << "Error caught reading media types file: " << str << "\n").Show();
		//file.Close();
		return E_FAIL;
	}
	//file.Close();
	return S_OK;
}

HRESULT DWMediaTypes::Save(LPWSTR filename)
{
	return S_OK;
}

