//////////////////////////////////////////////////////////////////////
// fmdemod.h: interface for the CFmDemod class.
//
// History:
//	2011-01-17  Initial creation MSW
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
#ifndef FMDEMOD_H
#define FMDEMOD_H
#include "datatypes.h"
#include "fir.h"
#include "iir.h"

#define MAX_SQBUF_SIZE 16384

class CFmDemod
{
public:
	CFmDemod(TYPEREAL samplerate);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPECPX* pOutData);
	int ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPEREAL* pOutData);

	void SetSampleRate(TYPEREAL samplerate);
	void SetSquelch(int Value);		//call with range of -160 to 0 to set squelch threshold

private:
	
	void PerformNoiseSquelch(int InLength, TYPEREAL* pOutData);
	void InitNoiseSquelch();
	void ProcessDeemphasisFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);

	bool m_SquelchState;
	TYPEREAL m_SampleRate;
	TYPEREAL m_SquelchHPFreq;
	TYPEREAL m_OutGain;
	TYPEREAL m_FreqErrorDC;
	TYPEREAL m_DcAlpha;
	TYPEREAL m_NcoPhase;
	TYPEREAL m_NcoFreq;
	TYPEREAL m_NcoLLimit;
	TYPEREAL m_NcoHLimit;
	TYPEREAL m_PllAlpha;
	TYPEREAL m_PllBeta;

	TYPEREAL m_SquelchThreshold;
	TYPEREAL m_SquelchAve;
	TYPEREAL m_SquelchAlpha;

	TYPEREAL m_OutBuf[MAX_SQBUF_SIZE];

	TYPEREAL m_DeemphasisAve;
	TYPEREAL m_DeemphasisAlpha;

	CFir m_HpFir;
	CFir m_LpFir;

};

#endif // FMDEMOD_H
