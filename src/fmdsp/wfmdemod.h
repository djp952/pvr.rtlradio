//////////////////////////////////////////////////////////////////////
// wfmdemod.h: interface for the CWFmDemod class.
//
// History:
//	2011-07-24  Initial creation MSW
//	2011-08-05  Initial release
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
#ifndef WFMDEMOD_H
#define WFMDEMOD_H
#include "datatypes.h"
#include "fir.h"
#include "iir.h"
#include "downconvert.h"
#include "rbdsconstants.h"


#define PHZBUF_SIZE 16384

#define RDS_Q_SIZE 100

class CWFmDemod
{
public:
	CWFmDemod(TYPEREAL samplerate);
	virtual ~CWFmDemod();

	TYPEREAL SetSampleRate(TYPEREAL samplerate, bool USver);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	TYPEREAL GetDemodRate(){return m_OutRate;}

	bool GetNextRdsGroupData(tRDS_GROUPS* pGroupData);
	int GetStereoLock(int* pPilotLock);

private:
	void InitDeemphasis( TYPEREAL Time, TYPEREAL SampleRate);	//create De-emphasis LP filter
	void ProcessDeemphasisFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);
	void ProcessDeemphasisFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf);
	void InitPilotPll( TYPEREAL SampleRate );
	bool ProcessPilotPll( int InLength, TYPECPX* pInData );
	void InitRds( TYPEREAL SampleRate );
	void ProcessRdsPll( int InLength, TYPECPX* pInData, TYPEREAL* pOutData );
	inline TYPEREAL arctan2(TYPEREAL y, TYPEREAL x);

	void ProcessNewRdsBit(int bit);
	quint32 CheckBlock(quint32 BlockOffset, int UseFec);

	TYPEREAL m_SampleRate;
	TYPEREAL m_OutRate;
	TYPEREAL m_RawFm[PHZBUF_SIZE];
	TYPECPX m_CpxRawFm[PHZBUF_SIZE];
	CDecimateBy2* m_pDecBy2A;
	CDecimateBy2* m_pDecBy2B;
	CDecimateBy2* m_pDecBy2C;

	TYPECPX m_D0;		//complex delay line variables
	TYPECPX m_D1;

	TYPEREAL m_DeemphasisAveRe;
	TYPEREAL m_DeemphasisAveIm;
	TYPEREAL m_DeemphasisAlpha;

	CIir m_MonoLPFilter;
	CFir m_LPFilter;
	CIir m_NotchFilter;
	CIir m_PilotBPFilter;
	CFir m_HilbertFilter;

	int m_PilotLocked;				//variables for Pilot PLL
	int m_LastPilotLocked;
	TYPEREAL m_PilotNcoPhase;
	TYPEREAL m_PilotNcoFreq;
	TYPEREAL m_PilotNcoLLimit;
	TYPEREAL m_PilotNcoHLimit;
	TYPEREAL m_PilotPllAlpha;
	TYPEREAL m_PilotPllBeta;
	TYPEREAL m_PhaseErrorMagAve;
	TYPEREAL m_PhaseErrorMagAlpha;
	TYPEREAL m_PilotPhase[PHZBUF_SIZE];
	TYPEREAL m_PilotPhaseAdjust;

	TYPEREAL m_RdsNcoPhase;		//variables for RDS PLL
	TYPEREAL m_RdsNcoFreq;
	TYPEREAL m_RdsNcoLLimit;
	TYPEREAL m_RdsNcoHLimit;
	TYPEREAL m_RdsPllAlpha;
	TYPEREAL m_RdsPllBeta;

	TYPECPX m_RdsRaw[PHZBUF_SIZE];	//variables for RDS processing
	TYPEREAL m_RdsMag[PHZBUF_SIZE];
	TYPEREAL m_RdsData[PHZBUF_SIZE];
	TYPEREAL m_RdsMatchCoef[PHZBUF_SIZE];
	TYPEREAL m_RdsLastSync;
	TYPEREAL m_RdsLastSyncSlope;
	TYPEREAL m_RdsLastData;
	int m_MatchCoefLength;
	CDownConvert m_RdsDownConvert;
	CFir m_RdsBPFilter;
	CFir m_RdsMatchedFilter;
	CIir m_RdsBitSyncFilter;
	TYPEREAL m_RdsOutputRate;
	int m_RdsLastBit;
	tRDS_GROUPS m_RdsGroupQueue[RDS_Q_SIZE];
	int m_RdsQHead;
	int m_RdsQTail;
	tRDS_GROUPS m_LastRdsGroup;
	quint32 m_InBitStream;	//input shift register for incoming raw data
	int m_CurrentBlock;
	int m_CurrentBitPosition;
	int m_DecodeState;
	int m_BGroupOffset;
	int m_BlockErrors;
	quint16 m_BlockData[4];
};

#endif // WFMDEMOD_H
