//////////////////////////////////////////////////////////////////////
// wfmmod.h: interface for the CWFmMod class.
//
// History:
//	2011-08-18  Initial creation MSW
//	2011-08-18  Initial release
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
#ifndef WFMMOD_H
#define WFMMOD_H
#include "datatypes.h"
#include "fir.h"
#include "iir.h"

#define RDSBUF_SIZE 16384
#define MAX_RDS_DATA 2000	//max number of blocks that can be stored


class CWFmMod
{
public:
	CWFmMod();
	void GenerateData(int InLength,TYPEREAL Amplitude, TYPECPX* pOutData);
	void SetSampleRate(TYPEREAL SampleRate);
	void SetSweep(TYPEREAL SweepFreqNorm, TYPEREAL SweepFrequency, TYPEREAL SweepStopFrequency, TYPEREAL SweepRateInc);

private:
	void InitRDS();
	void CreateRdsGroup(quint16 Blk1, quint16 Blk2, quint16 Blk3, quint16 Blk4);
	quint32 CreateBlockWithCheckword(quint16 Data, quint32 BlockOffset);
	void CreateRdsSamples(int InLength , TYPEREAL* pBuf);
	TYPEREAL CreateNextRdsBit();

	TYPEREAL m_DeviationRate;
	TYPEREAL m_ModAcc;
	TYPEREAL m_PilotAcc;
	TYPEREAL m_PilotInc;
	TYPEREAL m_LeftAcc;
	TYPEREAL m_LeftInc;
	TYPEREAL m_RightAcc;
	TYPEREAL m_RightInc;
	TYPEREAL m_LeftAmp;
	TYPEREAL m_RightAmp;

	TYPEREAL m_SweepFrequency;
	TYPEREAL m_SweepFreqNorm;
	TYPEREAL m_SweepStopFrequency;
	TYPEREAL m_SweepRateInc;



	TYPEREAL m_SampleRate;
	int StateTimer;
	int TimerPeriod;
	int ModState;

	TYPEREAL m_RdsPulseCoef[RDSBUF_SIZE];
	TYPEREAL m_RdsOut[RDSBUF_SIZE];
	TYPEREAL m_RdsPulseLength;
	TYPEREAL m_RdsTime;
	TYPEREAL m_RdsSamplePeriod;
	int m_RdsD1;
	int m_RdsD2;
	int m_RdsBufPos;
	int m_RdsBufLength;
	int m_RdsLastBit;
	quint32 m_RdsBitPtr;
	quint32 m_RdsDataBuf[MAX_RDS_DATA];

};

#endif // WFMMOD_H
