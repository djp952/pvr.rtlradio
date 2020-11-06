//////////////////////////////////////////////////////////////////////
// demodulator.h: interface for the CDemodulator class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
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
#ifndef DEMODULATOR_H
#define DEMODULATOR_H

#include "downconvert.h"
#include "wfmdemod.h"

#include <string>
#include <mutex>

#define DEMOD_WFM 7

#define MAX_INBUFSIZE 250000	//maximum size of demod input buffer
								//pick so that worst case decimation leaves
								//reasonable number of samples to process
#define MAX_MAGBUFSIZE 32000

typedef struct _sdmd
{
	int HiCut;
	int HiCutmin;	//not saved in settings
	int HiCutmax;	//not saved in settings
	int LowCut;
	int LowCutmin;	//not saved in settings
	int LowCutmax;	//not saved in settings
	int DefFreqClickResolution;//not saved in settings
	int FreqClickResolution;
	int FilterClickResolution;//not saved in settings
	int Offset;
	int SquelchValue;
	int AgcSlope;
	int AgcThresh;
	int AgcManualGain;
	int AgcDecay;
	bool AgcOn;
	bool AgcHangOn;
	bool Symetric;	//not saved in settings
	std::string txt;	//not saved in settings
}tDemodInfo;

class CDemodulator
{
public:
	CDemodulator();
	virtual ~CDemodulator();

	void SetInputSampleRate(TYPEREAL InputRate);
	TYPEREAL GetOutputRate(){return m_DemodOutputRate;}

	void SetDemod(int Mode, tDemodInfo CurrentDemodInfo);
	void SetDemodFreq(TYPEREAL Freq){m_DownConvert.SetCwOffset(m_CW_Offset);
										m_DownConvert.SetFrequency(Freq);}

	//overloaded functions to perform demod mono or stereo
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);

	void SetUSFmVersion(bool USFm){m_USFm = USFm;}
	bool GetUSFmVersion(){return m_USFm;}

	// expose the input buffer limit
	int GetInputBufferLimit(void) const
	{
		return m_InBufLimit;
	}

	//access to WFM mode status
	int GetStereoLock(int* pPilotLock){ if(m_pWFmDemod) return m_pWFmDemod->GetStereoLock(pPilotLock); else return false;}
	bool GetNextRdsGroupData(tRDS_GROUPS* pGroupData)
	{
		if(m_pWFmDemod) return m_pWFmDemod->GetNextRdsGroupData(pGroupData); else return false;
	}

private:
	void DeleteAllDemods();
	CDownConvert m_DownConvert;
#ifdef FMDSP_THREAD_SAFE
	mutable std::mutex m_Mutex;		//for keeping threads from stomping on each other
#endif
	tDemodInfo m_DemodInfo;
	TYPEREAL m_InputRate;
	TYPEREAL m_DownConverterOutputRate;
	TYPEREAL m_DemodOutputRate;
	TYPEREAL m_DesiredMaxOutputBandwidth;
	TYPECPX* m_pDemodInBuf;
	TYPECPX* m_pDemodTmpBuf;
	TYPEREAL m_CW_Offset;
	TYPEREAL m_PskRate;
	bool m_USFm;
	int m_DemodMode;
	int m_InBufPos;
	int m_InBufLimit;
	//pointers to all the various implemented demodulator classes
	CWFmDemod* m_pWFmDemod;
};

#endif // DEMODULATOR_H
