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

//////////////////////////////////////////////////////////////////////
// DVBTChannels_Program
//////////////////////////////////////////////////////////////////////

DVBTChannels_Program::DVBTChannels_Program() :	programNumber(0),
												name(0),
												favoriteID(0),
												bManualUpdate(0)
{
}

DVBTChannels_Program::~DVBTChannels_Program()
{
	streams.clear();
}

HRESULT DVBTChannels_Program::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;
	attr = pElement->Attributes.Item(L"ProgramNumber");
	if (attr == NULL)
		return (log << "program id must be supplied in a program definition").Write(E_FAIL);
	programNumber = _wtol(attr->value);

	attr = pElement->Attributes.Item(L"Name");
	strCopy(name, (attr ? attr->value : L""));

	attr = pElement->Attributes.Item(L"FavoriteID");
	if (attr)
		favoriteID = _wtol(attr->value);

	attr = pElement->Attributes.Item(L"ManualUpdate");
	if (attr)
		bManualUpdate = (attr->value[0] != 0);
	else
		bManualUpdate = FALSE;

	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *element = pElement->Elements.Item(item);
		if (_wcsicmp(element->name, L"Stream") == 0)
		{
			DVBTChannels_Program_Stream newStream;

			attr = element->Attributes.Item(L"PID");
			if (attr == NULL)
				continue;

			newStream.PID = _wtol(attr->value);

			attr = element->Attributes.Item(L"Type");
			if (attr == NULL)
				continue;

			newStream.Type = (DVBTChannels_Program_PID_Types)_wtol(attr->value);

			attr = element->Attributes.Item(L"Active");
			if (attr == NULL)
				newStream.bActive = FALSE;
			else
				newStream.bActive = (attr->value[0] != 0);

			streams.push_back(newStream);
			continue;
		}
	}

	return S_OK;
}

HRESULT DVBTChannels_Program::SaveToXML(XMLElement *pElement)
{
	LPWSTR pValue = NULL;
	strCopy(pValue, programNumber);
	pElement->Attributes.Add(new XMLAttribute(L"ProgramNumber", pValue));
	delete pValue;

	pElement->Attributes.Add(new XMLAttribute(L"Name", name));

	if (favoriteID > 0)
	{
		pValue = NULL;
		strCopy(pValue, favoriteID);
		pElement->Attributes.Add(new XMLAttribute(L"FavoriteID", pValue));
		delete pValue;
	}

	if (bManualUpdate)
		pElement->Attributes.Add(new XMLAttribute(L"ManualUpdate", L"1"));

	std::vector<DVBTChannels_Program_Stream>::iterator it = streams.begin();
	for ( ; it != streams.end() ; it++ )
	{
		DVBTChannels_Program_Stream pgStream = *it;

		XMLElement *pStreamElement = new XMLElement(L"Stream");

		pValue = NULL;
		strCopy(pValue, pgStream.PID);
		pStreamElement->Attributes.Add(new XMLAttribute(L"PID", pValue));
		delete pValue;

		pValue = NULL;
		strCopy(pValue, pgStream.Type);
		pStreamElement->Attributes.Add(new XMLAttribute(L"Type", pValue));
		delete pValue;

		if (pgStream.bActive)
			pStreamElement->Attributes.Add(new XMLAttribute(L"Active", L"1"));

		pElement->Elements.Add(pStreamElement);
	}
	return S_OK;
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

void DVBTChannels_Network::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	std::vector<DVBTChannels_Program *>::iterator it = programs.begin();
	for ( ; it != programs.end() ; it++ )
	{
		DVBTChannels_Program *program = *it;
		program->SetLogCallback(callback);
	}
}

HRESULT DVBTChannels_Network::LoadFromXML(XMLElement *pElement)
{
	XMLAttribute *attr;
	attr = pElement->Attributes.Item(L"Frequency");
	if (attr == NULL)
		return (log << "Frequency must be supplied in a network definition").Write(E_FAIL);
	frequency = _wtol(attr->value);

	attr = pElement->Attributes.Item(L"Bandwidth");
	if (attr)
		bandwidth = _wtol(attr->value);

	attr = pElement->Attributes.Item(L"Name");
	strCopy(name, (attr ? attr->value : L""));

	int elementCount = pElement->Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *element = pElement->Elements.Item(item);
		if (_wcsicmp(element->name, L"Program") == 0)
		{
			DVBTChannels_Program *newProgram = new DVBTChannels_Program();
			if (newProgram->LoadFromXML(element) == S_OK)
				programs.push_back(newProgram);
			else
				delete newProgram;
			continue;
		}
	}

	return S_OK;
}

HRESULT DVBTChannels_Network::SaveToXML(XMLElement *pElement)
{
	LPWSTR pValue = NULL;
	strCopy(pValue, frequency);
	pElement->Attributes.Add(new XMLAttribute(L"Frequency", pValue));
	delete pValue;

	pValue = NULL;
	strCopy(pValue, bandwidth);
	pElement->Attributes.Add(new XMLAttribute(L"Bandwidth", pValue));
	delete pValue;

	pElement->Attributes.Add(new XMLAttribute(L"Name", name));

	std::vector<DVBTChannels_Program *>::iterator it = programs.begin();
	for ( ; it != programs.end() ; it++ )
	{
		DVBTChannels_Program *program = *it;
		XMLElement *pProgElement = new XMLElement(L"Program");
		if (program->SaveToXML(pProgElement) == S_OK)
			pElement->Elements.Add(pProgElement);
		else
			delete pProgElement;
	}

	return S_OK;
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

void DVBTChannels::SetLogCallback(LogMessageCallback *callback)
{
	LogMessageCaller::SetLogCallback(callback);

	std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
	for ( ; it != networks.end() ; it++ )
	{
		DVBTChannels_Network *network = *it;
		network->SetLogCallback(callback);
	}
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
	(log << "Loading DVBT Channels file: " << filename << "\n").Write();
	LogMessageIndent indent(&log);

	strCopy(m_filename, filename);

	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);
	if (file.Load(m_filename) != S_OK)
	{
		return (log << "Could not load channels file: " << m_filename << "\n").Show(FALSE);
	}

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"Bandwidth") == 0)
		{
			m_bandwidth = _wtol(element->value);
			continue;
		}

		if (_wcsicmp(element->name, L"Network") == 0)
		{
			DVBTChannels_Network *newNetwork = new DVBTChannels_Network();
			newNetwork->bandwidth = m_bandwidth;
			if (newNetwork->LoadFromXML(element) == S_OK)
				networks.push_back(newNetwork);
			else
				delete newNetwork;
			continue;
		}
	}

	if (networks.size() == 0)
		return (log << "You need to specify at least one network in your channels file\n").Show(FALSE);

	indent.Release();
	(log << "Finished Loading DVBT Channels file: " << filename << "\n").Write();

	return TRUE;
}

BOOL DVBTChannels::SaveChannels(LPWSTR filename)
{
	XMLDocument file;
	file.SetLogCallback(m_pLogCallback);

	XMLElement *pElement = new XMLElement(L"Bandwidth");
	strCopy(pElement->value, m_bandwidth);
	file.Elements.Add(pElement);

	std::vector<DVBTChannels_Network *>::iterator it = networks.begin();
	for ( ; it < networks.end() ; it++ )
	{
		pElement = new XMLElement(L"Network");
		DVBTChannels_Network *network = *it;
		network->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}

	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);

	return TRUE;
}

