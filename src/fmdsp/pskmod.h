//////////////////////////////////////////////////////////////////////
// wfmmod.h: interface for the CPskMod class.
//
// History:
//	2015-02-23  Initial creation MSW
//	2015-02-23
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
#ifndef PSKMOD_H
#define PSKMOD_H
#include "datatypes.h"


#define TX_BUF_SIZE 1024


class CPskMod
{
public:
	CPskMod();
	void GenerateData(int InLength, TYPEREAL SigAmplitude, TYPECPX* pOutData);
	void Init(TYPEREAL SampleRate, TYPEREAL SymbolRate, quint8 Mode);
	void PutTxQue(qint16 txchar);
	void ClrQue();
	int GetTXCharsRemaining();
private:
	qint16 GetTxChar();
	qint16 GetChar();
	quint8 GetNextBPSKSymbol(void);
	quint8 GetNextQPSKSymbol(void);
	quint8 GetNextTuneSymbol(void);


	bool m_AddEndingZero;
	bool m_SymbDone;
	bool m_NoSquelchTail;
	bool m_NeedCWid;
	bool m_NeedShutoff;
	bool m_TempNoSquelchTail;
	bool m_TempNeedCWid;
	bool m_TempNeedShutoff;

	int m_Tail;
	int m_Head;
	int m_TXState;
	quint8 m_Preamble[33];
	quint8 m_Postamble[33];
	int m_AmblePtr;
	int m_SavedMode;
	quint8 m_pXmitQue[TX_BUF_SIZE];

	quint16 m_TxShiftReg;
	quint16 m_TxCodeWord;
	quint8 m_Lastsymb;

	quint8 m_PSKmode;
	qint8 m_LastPhase;
	qint8 m_NextPhase;
	TYPEREAL m_SampleRate;
	TYPEREAL m_SymbTimeInc;
	TYPEREAL m_SymbTimeAcc;
	TYPEREAL m_SymbPeriod;
	TYPEREAL m_ShapingAcc;
	TYPEREAL m_ShapingInc;


};

#endif // PSKMOD_H
