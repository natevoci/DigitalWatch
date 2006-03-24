/**
*  IMP2Requant.h
* 
*  Copyright (C) 2004-2005 bear
*  Copyright 2004-2005 © All rights reserved Dmitry Vergheles
*
*  This file is part of DVBQtoMPG, a Dos command line converter
*  program that uses directshow filters to convert files from
*  one format type to MPG with compression.
*
*  DVBQtoMPG is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  DVBQtoMPG is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with DVBQtoMPG; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*  bear can be reached on the forums at
*    http://forums.dvbowners.com/
*  Dmitry Vergheles can be reached at 
*    www.solveigmm.com
*/
// IMP2Requant.h: interface for the Quantizer Filter.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_IMP2Requant_H__DC97569C_F779_4B65_9705_6395A9B4433E__INCLUDED_)
#define AFX_IMP2Requant_H__DC97569C_F779_4B65_9705_6395A9B4433E__INCLUDED_

//"{8FF0EB9B-8812-4f27-A26F-A6975562EF38}"
DEFINE_GUID(IID_IMP2Requant,
0x8FF0EB9B,0x8812,0x4f27,0xA2,0x6F,0xA6,0x97,0x55,0x62,0xEF,0x38);

DECLARE_INTERFACE_(IMP2Requant, IUnknown)
{
STDMETHOD(get_CompRation) (THIS_ int* pnRatio) PURE;
STDMETHOD(put_CompRation) (THIS_ int nRatio) PURE;

STDMETHOD(get_AdvSettings) (THIS_ int nFrameType,THIS_ int* pnQuantDiv,THIS_ int* pnCoefNum ) PURE;
STDMETHOD(put_AdvSettings) (THIS_ int nFrameType,THIS_ int nQuantDiv, THIS_ int nCoefNum ) PURE;

STDMETHOD(get_NumPassedFrames) (THIS_ int* pnFrames, double *pdFrameRate ) PURE;
};
#endif // !defined(AFX_GLOBAL_H__DC97569C_F779_4B65_9705_6395A9B4433E__INCLUDED_)
/*

Today, 08:15 PM Post #45  
Participant
Posts: 28
User # 2,111
Card: None


 the routines below have been realized

HRESULT put_AdvSettings (int nFrameType,int nQuantRatio, int nCoefNum ) ;
HRESULT get_AdvSettings (int nFrameType,int* pnQuantRatio, int* pnCoefNum );

nFrameType - an parameter to specify a frame type (Intra, Predicted or Bi-directional) for whom nQuantRatio and nCoefNum parameters have to be applied.

Also it is possible to specify type of block (Intra - non Intra) for those parameters.

#define REQUANT_PARAM_BLOCK_INTRA 1
#define REQUANT_PARAM_BLOCK_NONINTRA 2
#define REQUANT_PARAM_FRAME_I 4
#define REQUANT_PARAM_FRAME_P 8
#define REQUANT_PARAM_FRAME_B 16

nQuantRatio - a percentage factor an original quantizer has to be decreased to.
100 - without recompression, 200 decrease in two times etc.

nCoefNum - amount of last of 64 DCT coefficients (0 - 64), that have to be discarded.

for example:

nFrameType = REQUANT_PARAM_BLOCK_INTRA | REQUANT_PARAM_FRAME_P;
nQuantRatio = 180;
nCoefNum = 2;
put_AdvSettings (nFrameType , nQuantRatio, nCoefNum );

it means that intrablock in P frames have to be decreased on 80 percentages and last 3 non zero coefficients in blocks have to be discarded.

Regards,
Dmirty
 
*/
