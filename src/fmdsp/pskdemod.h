//////////////////////////////////////////////////////////////////////
// pskdemod.h: interface for the CPskDemod class.
//
// History:
//	2015-02-19  Initial creation MSW
/////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//=============================================================================
#ifndef PSKDEMOD_H
#define PSKDEMOD_H

#include "datatypes.h"
#include "psktables.h"
#include "fir.h"
#include "iir.h"

class CPskDemod
{
public:
	explicit CPskDemod();
	~CPskDemod();
	void SetPskParams(TYPEREAL InSampleRate, TYPEREAL SymbRate, int Mode);
	//overloaded functions for mono and stereo audio out
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);


private:
	void CalcAfc(int InLength, TYPECPX* pInData);
	void ManageSquelch(quint8 ch);
	quint8 DecodeSymb(TYPECPX newsymb);

	quint16 m_BitAcc;
	quint16 m_VericodeAcc;
	int m_PskMode;
	int m_DecRate;
	int m_DecCnt;
	TYPEREAL m_WFerrAve;
	TYPEREAL m_NFerrAve;
	TYPEREAL m_IntegralFerr;
	TYPEREAL m_FreqError;
	TYPEREAL m_NcoPhase;
	TYPEREAL m_NcoInc;
	TYPEREAL m_OscCos;
	TYPEREAL m_OscSin;
	TYPEREAL m_AveMag;
	TYPEREAL m_AveEnergy;
	TYPEREAL m_LastBitMag;
	TYPEREAL m_LastSyncSlope;
	TYPEREAL m_SampleRate;
	TYPEREAL m_BitMag[1024];

	TYPECPX m_PrevSymbol;
	TYPECPX m_z1;
	TYPECPX m_z2;
	TYPECPX m_Osc1;
	TYPECPX m_PrevSample;
	TYPECPX m_FreqErrBuf[128];

	CFir m_BitFir;
	CFir m_FreqFir;
	CIir m_BitSyncFilter;
};

#endif // PSKDEMOD_H
