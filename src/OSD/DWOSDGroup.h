/**
 *	DWOSDGroup.h
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

#ifndef DWOSDGROUP_H
#define DWOSDGROUP_H

#include "StdAfx.h"
#include "LogMessage.h"
#include "XMLDocument.h"
#include "DWOSDControl.h"
#include <vector>

class DWOSDGroup : public DWOSDControl
{
public:
	DWOSDGroup(DWSurface* pSurface);
	virtual ~DWOSDGroup();

	HRESULT LoadFromXML(XMLElement *pElement);

protected:
	virtual HRESULT Draw(long tickCount);

	std::vector<DWOSDControl *> m_controls;
	CCritSec m_controlsLock;
};

#endif
