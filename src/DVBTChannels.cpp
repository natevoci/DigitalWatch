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
#include "GlobalFunctions.h"

#include <fstream>
using namespace std;


//////////////////////////////////////////////////////////////////////
// DVBTChannels_Program
//////////////////////////////////////////////////////////////////////

DVBTChannels_Program::DVBTChannels_Program() :	programNumber(0),
												name(0),
												favoriteID(0),
												bDisableAutoUpdate(0)
{
}

DVBTChannels_Program::~DVBTChannels_Program()
{
	streams.clear();
}

DVBTChannels_Program_PID_Types DVBTChannels_Program::GetStreamType(int index)
{
	if ((index >= 0) && (index < streams.size()))
	{
		DVBTChannels_Program_Stream pgStream = streams.at(index);
		return pgStream.Type;
	}
	return unknown;
}

long DVBTChannels_Program::GetStreamPID(int index)
{
	if ((index >= 0) && (index < streams.size()))
	{
		DVBTChannels_Program_Stream pgStream = streams.at(index);
		return pgStream.PID;
	}
	return 0;
}

long DVBTChannels_Program::GetStreamCount()
{
	return streams.size();
}

long DVBTChannels_Program::GetStreamPID(DVBTChannels_Program_PID_Types streamtype, int index)
{
	int found = 0;
	std::vector<DVBTChannels_Program_Stream>::iterator it = streams.begin();
	for ( ; it != streams.end() ; it++ )
	{
		DVBTChannels_Program_Stream pgStream = *it;
		if ((pgStream.Type == streamtype) && pgStream.bActive)
		{
			if (found == index)
				return pgStream.PID;
			found++;
		}
	}
	return 0;
}

long DVBTChannels_Program::GetStreamCount(DVBTChannels_Program_PID_Types streamtype)
{
	int found = 0;
	std::vector<DVBTChannels_Program_Stream>::iterator it = streams.begin();
	for ( ; it != streams.end() ; it++ )
	{
		DVBTChannels_Program_Stream pgStream = *it;
		if ((pgStream.Type == streamtype) && pgStream.bActive)
		{
			found++;
		}
	}
	return found;
}


//////////////////////////////////////////////////////////////////////
// DVBTChannels_Network
//////////////////////////////////////////////////////////////////////

DVBTChannels_Network::DVBTChannels_Network() :	frequency(0),
												bandwidth(0),
												name(0)
{
}

DVBTChannels_Network::~DVBTChannels_Network()
{
	std::vector<DVBTChannels_Program *>::iterator it = programs.begin();
	for ( ; it != programs.end() ; it++ )
	{
		delete *it;
	}
	programs.clear();
}

DVBTChannels_Program* DVBTChannels_Network::Program(int programNumber)
{
	if (!IsValidProgram(programNumber))
		return NULL;
	return programs.at(programNumber-1);
}

BOOL DVBTChannels_Network::IsValidProgram(int programNumber)
{
	if (programNumber < 1)
		return FALSE;
	if (programNumber > programs.size())
		return FALSE;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////
// DVBTChannels
//////////////////////////////////////////////////////////////////////

DVBTChannels::DVBTChannels() :	m_bandwidth(7),
								m_filename(0)
{
}

DVBTChannels::~DVBTChannels()
{
	if (m_filename)
		delete m_filename;

	std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
	for ( ; it != networks.end() ; it++ )
	{
		delete *it;
	}
	networks.clear();
}

DVBTChannels_Network* DVBTChannels::Network(int networkNumber)
{
	if (!IsValidNetwork(networkNumber))
		return NULL;
	return networks.at(networkNumber-1);
}

BOOL DVBTChannels::IsValidNetwork(int networkNumber)
{
	if (networkNumber < 1)
		return FALSE;
	if (networkNumber > networks.size())
		return FALSE;
	return TRUE;
}

BOOL DVBTChannels::LoadChannels(LPWSTR filename)
{
	USES_CONVERSION;

	strCopy(m_filename, filename);

	ifstream file;
	file.open(W2A(filename));

	if (file.is_open() != 1)
		return (log << "Could not open channels file: " << filename << "\n").Show();

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
					return (log << "Parse Error in " << filename << ": Line " << line << "\nMissing ']'\n").Show();
				if (pEOS == pCurr)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nMissing section name\n").Show();
				pEOS[0] = '\0';

				if (state == 0)
				{
					if (_wcsicmp(pCurr, L"Networks") != 0)
						return (log << "Parse Error in " << filename << ": Line " << line << "\nThe first section must be [Networks]\n[" << pCurr << "] was found instead\n").Show();
					state = 1;
					continue;
				}
				else if (state == 1)
				{
					return (log << "Parse Error in " << filename << ": Line " << line << "\nNo Network() definitions found in [Networks] section.\n").Show();
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
						return (log << "Parse Error in " << filename << ": Line " << line << "\nFrequency section found without matching Network function\n").Show();

					state = 3;
				}
				else
					return (log << "Parse Error in " << filename << ": Line " << line << "\nSecond consecutive frequency section found\n").Show();

				continue;
			}

			ParseLine parseLine;
			if (parseLine.Parse(pBuff) == FALSE)
				return (log << "Parse Error in " << filename << ": Line " << line << ":" << parseLine.GetErrorPosition() << "\n" << parseLine.GetErrorMessage() << "\n").Show();


			if (parseLine.HasRHS())
				return (log << "Parse Error in " << filename << ": Line " << line << "\nEquals not valid for this command\n").Show();

			pCurr = parseLine.LHS.FunctionName;

			if (_wcsicmp(pCurr, L"Bandwidth") == 0)
			{
				if (parseLine.LHS.ParameterCount < 1)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a bandwidth parameter\n").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

				m_bandwidth = _wtol(pCurr);
				continue;
			}

			if (_wcsicmp(pCurr, L"Network") == 0)
			{
				if ((state != 1) && (state != 2))
					return (log << "Parse Error in " << filename << ": Line " << line << "\nthe Network function is only valid in the [Networks] section.\n").Show();

				if (parseLine.LHS.ParameterCount < 1)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a frequency parameter\n").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

				long frequency = _wtol(pCurr);
				
				std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
				for ( ; it != networks.end() ; it++ )
				{
					DVBTChannels_Network *nw = *it;
					if (nw->frequency == frequency)
						return (log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate frequency found\n").Show();
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
					return (log << "Parse Error in " << filename << ": Line " << line << "\nthe Program function is only valid in the a specific frequency section.\n").Show();

				if (parseLine.LHS.ParameterCount < 1)
					return (log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a program number parameter\n").Show();

				pCurr = parseLine.LHS.Parameter[0];
				if (!pCurr)
					return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

				long programNumber = _wtol(pCurr);

				std::vector<DVBTChannels_Program *>::iterator it = currNetwork->programs.begin();
				for ( ; it != currNetwork->programs.end() ; it++ )
				{
					DVBTChannels_Program *pg = *it;
					if (pg->programNumber == programNumber)
						return (log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate program number found\n").Show();
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
						return (log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a name parameter\n").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr)
						return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

					strCopy(currNetwork->name, pCurr);
				}
				else if (state == 4)
				{
					if (parseLine.LHS.ParameterCount < 1)
						return (log << "Parse Error in " << filename << ": Line " << line << "\nExpecting a name parameter\n").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr)
						return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();

					strCopy(currProgram->name, pCurr);
				}
				else
					return (log << "Parse Error in " << filename << ": Line " << line << "\nName function is only valid after a Network or Program function\n").Show();

				continue;
			}

			if (_wcsicmp(pCurr, L"Stream") == 0)
			{
				if (state == 4)
				{
					if (parseLine.LHS.ParameterCount < 3)
						return (log << "Parse Error in " << filename << ": Line " << line << "\nInsufficient number of parameters. Stream(PID, Type, Active)\n").Show();

					pCurr = parseLine.LHS.Parameter[0];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					long PID = _wtol(pCurr);

					pCurr = parseLine.LHS.Parameter[1];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					long Type = _wtol(pCurr);

					pCurr = parseLine.LHS.Parameter[2];
					if (!pCurr) return (log << "Internal Error in " << filename << ": Line " << line << "\nnull pointer exception\n").Write();
					long Active = _wtol(pCurr);


					std::vector<DVBTChannels_Program_Stream>::iterator it = currProgram->streams.begin();
					for ( ; it != currProgram->streams.end() ; it++ )
					{
						DVBTChannels_Program_Stream pgStream = *it;
						if (pgStream.PID == PID)
							return (log << "Parse Error in " << filename << ": Line " << line << "\nDuplicate program number found\n").Show();
					}

					//Add Stream
					DVBTChannels_Program_Stream pgStream;
					pgStream.PID = PID;
					pgStream.Type = (DVBTChannels_Program_PID_Types)Type;
					pgStream.bActive = (Active != 0);
					currProgram->streams.push_back(pgStream);
				}
				else
					return (log << "Parse Error in " << filename << ": Line " << line << "\nStream function is only valid after a Program function\n").Show();

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
	if (networks.size() == 0)
		return (log << "You need to specify at least one network in your channels file\n").Show();
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
		return (log << "Could not open channels file for writing: " << filename << "\n").Show();

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
		(log << str << "\n").Show();
		file.close();
		return FALSE;
	}
	file.close();
	return TRUE;
}

