/**
 *	DVBTChannels.cpp
 *	Copyright (C) 2004 Nate
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


#include "DVBTChannels.h"
#include "ParseLine.h"
#include "Globals.h"
#include "LogMessage.h"
#include "GlobalFunctions.h"

#if (_MSC_VER == 1200)
	#include <fstream.h>
#else
	#include <fstream>
	using namespace std;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DVBTChannels_Program::DVBTChannels_Program() :	programNumber(0),
												name(0),
												favoriteID(0),
												bDisableAutoUpdate(0)
{
}

DVBTChannels_Program::~DVBTChannels_Program()
{
}


DVBTChannels_Network::DVBTChannels_Network() :	frequency(0),
												bandwidth(0),
												name(0)
{
}

DVBTChannels_Network::~DVBTChannels_Network()
{
}

DVBTChannels::DVBTChannels() :	m_bandwidth(7),
								m_filename(0)
{
}

DVBTChannels::~DVBTChannels()
{
	if (m_filename)
		delete m_filename;
}

BOOL DVBTChannels::LoadChannels(LPWSTR filename)
{
	USES_CONVERSION;

	strCopy(m_filename, filename);

	ifstream file;
	file.open(W2A(filename));

	if (file.is_open() != 1)
		return (g_log << "Could not open channels file: " << filename).Show();

	try
	{
		int line = 0;
		char charbuff[256];
		wchar_t buff[256];
		LPWSTR pBuff;
		LPWSTR pCurr;
		int state = 0;
		DVBTChannels_Network *currNetwork;
		DVBTChannels_Program *currProgram;

		//file.getline(pBuff, 256);
		while (file.getline((LPSTR)&charbuff, 256), !file.eof() || (pBuff[0] != '\0'))
		{
			pBuff = (LPWSTR)&buff;

			//strCopyA2W(pBuff, (LPCSTR)&charbuff);
			long length = strlen((LPCSTR)&charbuff);
			mbstowcs(pBuff, (LPCSTR)&charbuff, length);
			pBuff[length] = 0;

			line++;

			pCurr = pBuff;
			skipWhitespaces(pCurr);

			if ((pCurr[0] == '\0') || (pCurr[0] == '#'))
				continue;

			if (pCurr[0] == '[')
			{
				pCurr++;
				LPWSTR pEOS = wcschr(pCurr, ']');
				if (pEOS == NULL)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nMissing ']'").Show();
				if (pEOS == pCurr)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nMissing section name").Show();
				pEOS[0] = '\0';

				if (state == 0)
				{
					if (_wcsicmp(pCurr, L"Networks") != 0)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nThe first section must be [Networks]\n[" << pCurr << "] was found instead").Show();
					state = 1;
					continue;
				}
				else if (state == 1)
				{
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nNo Network() definitions found in [Networks] section.").Show();
				}
				else if ((state == 2) || (state == 4))
				{
					long frequency = _wtol(pCurr);

					std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
					currNetwork = NULL;
					for ( ; it != networks.end() ; it++ )
					{
						DVBTChannels_Network *nw = *it;
						if (nw->frequency == frequency)
						{
							currNetwork = *it;
							break;
						}
					}
					if (currNetwork == NULL)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nFrequency section found without matching Network function").Show();

					state = 3;
				}
				else
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nSecond consecutive frequency section found").Show();

				continue;
			}

			ParseLine parseLine;
			if (parseLine.Parse(pBuff) == FALSE)
				return (g_log << "Parse Error in " << filename << ": Line " << line << ":" << parseLine.GetErrorPosition() << "\n" << parseLine.GetErrorMessage()).Show();


			if (parseLine.HasRHS())
				return (g_log << "Parse Error in " << filename << ": Line " << line << "\nEquals not valid for this command").Show();

			pCurr = parseLine.LHS.FunctionName;

			if (_wcsicmp(pCurr, L"Bandwidth") == 0)
			{
				if (parseLine.LHS.ParameterCount < 1)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a bandwidth parameter").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();

				m_bandwidth = _wtol(pCurr);
				continue;
			}

			if (_wcsicmp(pCurr, L"Network") == 0)
			{
				if ((state != 1) && (state != 2))
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nthe Network function is only valid in the [Networks] section.").Show();

				if (parseLine.LHS.ParameterCount < 1)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a frequency parameter").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();

				long frequency = _wtol(pCurr);
				
				std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
				for ( ; it != networks.end() ; it++ )
				{
					DVBTChannels_Network *nw = *it;
					if (nw->frequency == frequency)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate frequency found").Show();
				}

				//Add Network
				currNetwork = new DVBTChannels_Network();
				currNetwork->frequency = frequency;
				currNetwork->bandwidth = m_bandwidth;
				networks.push_back(currNetwork);

				state = 2;
				continue;
			}

			if (_wcsicmp(pCurr, L"Program") == 0)
			{
				if (state < 3)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nthe Program function is only valid in the a specific frequency section.").Show();

				if (parseLine.LHS.ParameterCount < 1)
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a program number parameter").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();

				long programNumber = _wtol(pCurr);

				std::vector<DVBTChannels_Program *>::iterator it = currNetwork->programs.begin();
				for ( ; it != currNetwork->programs.end() ; it++ )
				{
					DVBTChannels_Program *pg = *it;
					if (pg->programNumber == programNumber)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate program number found").Show();
				}

				//Add Program
				currProgram = new DVBTChannels_Program();
				currProgram->programNumber = programNumber;
				currNetwork->programs.push_back(currProgram);

				state = 4;
				continue;
			}

			if (_wcsicmp(pCurr, L"Name") == 0)
			{
				if (state == 2)
				{
					if (parseLine.LHS.ParameterCount < 1)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a name parameter").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr)
						return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();

					strCopy(currNetwork->name, pCurr);
				}
				else if (state == 4)
				{
					if (parseLine.LHS.ParameterCount < 1)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a name parameter").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr)
						return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();

					strCopy(currProgram->name, pCurr);
				}
				else
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nName function is only valid after a Network or Program function").Show();

				continue;
			}

			if (_wcsicmp(pCurr, L"Stream") == 0)
			{
				if (state == 4)
				{
					if (parseLine.LHS.ParameterCount < 3)
						return (g_log << "Parse Error in " << filename << ": Line " << line << "\nInsufficient number of parameters. Stream(PID, Type, Active)").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr) return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();
					long PID = _wtol(pCurr);

					pCurr = parseLine.LHS.Parameter[1];
					if (!pCurr) return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();
					long Type = _wtol(pCurr);

					pCurr = parseLine.LHS.Parameter[2];
					if (!pCurr) return (g_log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception").Write();
					long Active = _wtol(pCurr);


					std::vector<DVBTChannels_Program_Stream>::iterator it = currProgram->streams.begin();
					for ( ; it != currProgram->streams.end() ; it++ )
					{
						DVBTChannels_Program_Stream pgStream = *it;
						if (pgStream.PID == PID)
							return (g_log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate program number found").Show();
					}

					//Add Stream
					DVBTChannels_Program_Stream pgStream;
					pgStream.PID = PID;
					pgStream.Type = (DVBTChannels_Program_PID_Types)Type;
					pgStream.bActive = (Active != 0);
					currProgram->streams.push_back(pgStream);
				}
				else
					return (g_log << "Parse Error in " << filename << ": Line " << line << "\nStream function is only valid after a Program function").Show();

				continue;
			}
		}
	}
	catch (LPWSTR str)
	{
		(g_log << str).Show();
		file.close();
		return FALSE;
	}
	file.close();
	if (networks.size() == 0)
		return (g_log << "You need to specify at least one network in your channels file").Show();
	return TRUE;
}

BOOL DVBTChannels::SaveChannels(LPWSTR filename)
{
	USES_CONVERSION;

	ofstream file;
	if (filename)
		file.open(W2A(filename));
	else
		file.open(W2A(m_filename));

	if (file.is_open() != 1)
		return (g_log << "Could not open channels file for writing: " << filename).Show();

	try
	{
		file << "# DigitalWatch - Channels File" << endl;
		file << "#" << endl;
		file << endl;
		file << "Bandwidth(" << m_bandwidth << ")" << endl;
		file << endl;
		file << "[Networks]" << endl;

		DVBTChannels_Network *currNetwork;
		DVBTChannels_Program *currProgram;

		std::vector<DVBTChannels_Network *>::iterator nw_it = networks.begin();
		for ( ; nw_it != networks.end() ; nw_it++ )
		{
			currNetwork = *nw_it;
			file << "Network(" << currNetwork->frequency << ")" << endl;
			if (currNetwork->name)
				file << "    Name(\"" << W2A(currNetwork->name) << "\")" << endl;
		}

		nw_it = networks.begin();
		for ( ; nw_it != networks.end() ; nw_it++ )
		{
			currNetwork = *nw_it;

			file << endl;
			file << "[" << currNetwork->frequency << "]" << endl;

			std::vector<DVBTChannels_Program *>::iterator pg_it = currNetwork->programs.begin();
			for ( ; pg_it != currNetwork->programs.end() ; pg_it++ )
			{
				currProgram = *pg_it;
				file << "Program(" << currProgram->programNumber << ")" << endl;

				if (currProgram->name)
					file << "    Name(\"" << W2A(currProgram->name) << "\")" << endl;

				std::vector<DVBTChannels_Program_Stream>::iterator st_it = currProgram->streams.begin();
				for ( ; st_it != currProgram->streams.end() ; st_it++ )
				{
					DVBTChannels_Program_Stream pgStream = *st_it;
					char line[256];
					sprintf((LPSTR)&line, "    Stream(%4i, %i, %i)", pgStream.PID, pgStream.Type, (pgStream.bActive ? 1 : 0));
					file << (LPSTR)&line << endl;
				}
			}
		}

	}
	catch (LPWSTR str)
	{
		(g_log << str).Show();
		file.close();
		return FALSE;
	}
	file.close();
	return TRUE;
}

