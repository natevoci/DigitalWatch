/**
 *	BDACardCollection.cpp
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

//this is a file from DigitalWatch 2 that i've hacked up to work here.

#include "BDACardCollection.h"
#include "StdAfx.h"
#include "ParseLine.h"

#include "GlobalFunctions.h"
#include "FilterGraphTools.h"
#include <dshow.h>
#include <ks.h> // Must be included before ksmedia.h
#include <ksmedia.h> // Must be included before bdamedia.h
#include <bdatypes.h> // Must be included before bdamedia.h
#include <bdamedia.h>
#include "XMLDocument.h"

BDACardCollection::BDACardCollection()
{
	m_filename = NULL;
}

BDACardCollection::~BDACardCollection()
{
	std::vector<BDACard *>::iterator it = cards.begin();
	for ( ; it != cards.end() ; it++ )
	{
		delete *it;
	}
	cards.clear();
}

LogMessage BDACardCollection::get_Logger()
{
	return log;
}

BOOL BDACardCollection::LoadCards()
{
	if (m_filename)
		delete m_filename;
	m_filename = NULL;

	return LoadCardsFromHardware();
}

BOOL BDACardCollection::LoadCards(LPWSTR filename)
{
	strCopy(m_filename, filename);

	LoadCardsFromFile();

	return LoadCardsFromHardware();
}

BOOL BDACardCollection::LoadCardsFromFile()
{
	XMLDocument file;
	if (file.Load(m_filename) != S_OK)
	{
		return (log << "Could not load cards file: " << m_filename << "\n").Write(false);
	}

	(log << "Loading BDA DVB-T Cards file: " << m_filename << "\n").Write();

	BDACard* currCard = NULL;

	int elementCount = file.Elements.Count();
	for ( int item=0 ; item<elementCount ; item++ )
	{
		XMLElement *element = file.Elements.Item(item);
		if (_wcsicmp(element->name, L"card") != 0)
			continue;

		currCard = new BDACard();
		if (currCard->LoadFromXML(element) == S_OK)
			AddCardToList(currCard);
		else
			delete currCard;
	}

	return (cards.size() > 0);
}

void BDACardCollection::AddCardToList(BDACard* currCard)
{
	//check if currCard is present and check that all it's values got set before pushing it into the cards vector
	if (currCard)
	{
		if ((currCard->tunerDevice.strDevicePath == NULL) ||
			(currCard->tunerDevice.strFriendlyName == NULL))
		{
			(log << "Error in " << m_filename << ": Cannot add device without setting tuner name and device path\n").Write();
			return;
		}
		cards.push_back(currCard);
	}
}

BOOL BDACardCollection::SaveCards(LPWSTR filename)
{
	XMLDocument file;

	std::vector<BDACard *>::iterator it = cards.begin();
	for ( ; it < cards.end() ; it++ )
	{
		XMLElement *pElement = new XMLElement(L"Card");
		BDACard *pCard = *it;
		pCard->SaveToXML(pElement);
		file.Elements.Add(pElement);
	}
	
	if (filename)
		file.Save(filename);
	else
		file.Save(m_filename);
		
	return TRUE;
}

BOOL BDACardCollection::LoadCardsFromHardware()
{
	HRESULT hr = S_OK;

	(log << "Checking for new BDA DVB-T Cards\n").Write();

	DirectShowSystemDevice* pTunerDevice;
	DirectShowSystemDeviceEnumerator enumerator(KSCATEGORY_BDA_NETWORK_TUNER);

	while (enumerator.Next(&pTunerDevice) == S_OK)
	{
		BDACard *bdaCard = NULL;

		std::vector<BDACard *>::iterator it = cards.begin();
		for ( ; it != cards.end() ; it++ )
		{
			bdaCard = *it;
			if (_wcsicmp(pTunerDevice->strDevicePath, bdaCard->tunerDevice.strDevicePath) == 0)
				break;
			bdaCard = NULL;
		}

		if (bdaCard)
		{
			bdaCard->bDetected = TRUE;
			//TODO: maybe?? verify tuner can connect to the capture filter.
		}
		else
		{
			DirectShowSystemDevice* pDemodDevice;
			DirectShowSystemDevice* pCaptureDevice;

			if (FindCaptureDevice(pTunerDevice, &pDemodDevice, &pCaptureDevice))
			{
				bdaCard = new BDACard();
				bdaCard->bActive = TRUE;
				bdaCard->bNew = TRUE;
				bdaCard->bDetected = TRUE;

				bdaCard->tunerDevice = *pTunerDevice;
				if (pDemodDevice)
					bdaCard->demodDevice = *pDemodDevice;
				if (pCaptureDevice)
					bdaCard->captureDevice = *pCaptureDevice;

				cards.push_back(bdaCard);
			}
		}

		delete pTunerDevice;
		pTunerDevice = NULL;
	}

	if (cards.size() == 0)
		//return (log << "No cards found\n").Show();
		return FALSE;

	return TRUE;
}

BOOL BDACardCollection::FindCaptureDevice(DirectShowSystemDevice* pTunerDevice, DirectShowSystemDevice** ppDemodDevice, DirectShowSystemDevice** ppCaptureDevice)
{
	HRESULT hr;
	CComPtr <IGraphBuilder> piGraphBuilder;
	CComPtr <IBaseFilter> piBDANetworkProvider;
	CComPtr <ITuningSpace> piTuningSpace;
	CComPtr <ITuneRequest> pTuneRequest;
	CComPtr <IBaseFilter> piBDATuner;
	CComPtr <IBaseFilter> piBDADemod;
	CComPtr <IBaseFilter> piBDACapture;

	BOOL bRemoveNP = FALSE;
	BOOL bRemoveTuner = FALSE;
	BOOL bFoundDemod = FALSE;
	BOOL bFoundCapture = FALSE;

	do
	{
		//--- Create Graph ---
		if FAILED(hr = piGraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER))
		{
			//(log << "Failed Creating DVB-T Graph Builder\n").Write();
			break;
		}

		//--- Initialise Tune Request ---
		if FAILED(hr = InitDVBTTuningSpace(piTuningSpace))
		{
			//(log << "Failed to initialise the Tune Request\n").Write();
			break;
		}

		//--- Get NetworkType CLSID ---
		CComBSTR bstrNetworkType;
		CLSID CLSIDNetworkType;
		if FAILED(hr = piTuningSpace->get_NetworkType(&bstrNetworkType))
		{
			//(log << "Failed to get TuningSpace Network Type\n").Write();
			break;
		}

		if FAILED(hr = CLSIDFromString(bstrNetworkType, &CLSIDNetworkType))
		{
			//(log << "Could not convert Network Type to CLSID\n").Write();
			break;
		}

		//--- Create Network Provider ---
		if FAILED(hr = AddFilter(piGraphBuilder, CLSIDNetworkType, &piBDANetworkProvider, L"Network Provider"))
		{
			//(log << "Failed to add Network Provider to the graph\n").Write();
			break;
		}
		bRemoveNP = TRUE;

		//--- Create TuneRequest ---
		if FAILED(hr = CreateDVBTTuneRequest(piTuningSpace, pTuneRequest, -1, -1))
		{
			//(log << "Failed to create the Tune Request.\n").Write();
			break;
		}

		//--- Apply TuneRequest ---
		CComQIPtr <ITuner> pTuner(piBDANetworkProvider);
		if (!pTuner)
		{
			//(log << "Failed while interfacing Tuner with Network Provider\n").Write();
			break;
		}

		if FAILED(hr = pTuner->put_TuneRequest(pTuneRequest))
		{
			//(log << "Failed to submit the Tune Tequest to the Network Provider\n").Write();
			break;
		}

		//--- We can now add and connect the tuner filter ---
		
		if FAILED(hr = AddFilterByDevicePath(piGraphBuilder, &piBDATuner, pTunerDevice->strDevicePath, pTunerDevice->strFriendlyName))
		{
			//(log << "Cannot load Tuner Device\n").Write();
			break;
		}
		bRemoveTuner = TRUE;
    
		if FAILED(hr = ConnectFilters(piGraphBuilder, piBDANetworkProvider, piBDATuner))
		{
			//(log << "Failed to connect Network Provider to Tuner Filter\n").Write();
			break;
		}

		DirectShowSystemDeviceEnumerator enumerator(KSCATEGORY_BDA_RECEIVER_COMPONENT);
		*ppDemodDevice = NULL;
		while (hr = enumerator.Next(ppDemodDevice) == S_OK)
		{
			if FAILED(hr = AddFilterByDevicePath(piGraphBuilder, &piBDADemod, (*ppDemodDevice)->strDevicePath, (*ppDemodDevice)->strFriendlyName))
			{
				break;
			}

			if SUCCEEDED(hr = ConnectFilters(piGraphBuilder, piBDATuner, piBDADemod))
			{
				bFoundDemod = TRUE;
				break;
			}

			//(log << "Connection failed, trying next card\n").Write();
			piBDADemod.Release();
			delete *ppDemodDevice;
			*ppDemodDevice = NULL;
		}

		if (bFoundDemod)
		{
			*ppCaptureDevice = NULL;
			while (hr = enumerator.Next(ppCaptureDevice) == S_OK)
			{
				if FAILED(hr = AddFilterByDevicePath(piGraphBuilder, &piBDACapture, (*ppCaptureDevice)->strDevicePath, (*ppCaptureDevice)->strFriendlyName))
				{
					break;
				}

				if SUCCEEDED(hr = ConnectFilters(piGraphBuilder, piBDADemod, piBDACapture))
				{
					bFoundCapture = TRUE;
					break;
				}

				//(log << "Connection failed, trying next card\n").Write();
				piBDACapture.Release();
				delete *ppCaptureDevice;
				*ppCaptureDevice = NULL;
			}
		}

		//if (hr != S_OK)
			//(log << "No Cards Left\n").Write();
	} while (FALSE);

	DisconnectAllPins(piGraphBuilder);
	if (bFoundCapture)
		piGraphBuilder->RemoveFilter(piBDACapture);
	if (bFoundDemod)
		piGraphBuilder->RemoveFilter(piBDADemod);
	if (bRemoveTuner)
		piGraphBuilder->RemoveFilter(piBDATuner);
	if (bRemoveNP)
		piGraphBuilder->RemoveFilter(piBDANetworkProvider);

	piBDACapture.Release();
	piBDADemod.Release();
	piBDATuner.Release();
	piBDANetworkProvider.Release();

	pTuneRequest.Release();
	piTuningSpace.Release();
	piGraphBuilder.Release();

	if (!bFoundCapture)
	{
		*ppCaptureDevice = *ppDemodDevice;
		*ppDemodDevice = NULL;
	}

	return (bFoundDemod);
}

