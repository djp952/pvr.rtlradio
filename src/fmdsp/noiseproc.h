//////////////////////////////////////////////////////////////////////
// noiseproc.h: interface for the CNoiseProc class.
//
//  This class implements various noise reduction and blanker functions.
//
// History:
//	2011-01-06  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
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
#ifndef NOISEPROC_H
#define NOISEPROC_H

#include "datatypes.h"

#include <mutex>

typedef struct _snproc
{
	bool NBOn;
	int NBThreshold;
	int NBWidth;
}tNoiseProcdInfo;

class CNoiseProc
{
public:
	CNoiseProc();
	virtual ~CNoiseProc();

	void SetupBlanker( bool On, TYPEREAL Threshold, TYPEREAL Width, TYPEREAL SampleRate);
	void ProcessBlanker(int InLength, TYPECPX* pInData, TYPECPX* pOutData);

private:
	bool m_On;
	TYPEREAL m_Threshold;
	TYPEREAL m_Width;
	TYPEREAL m_SampleRate;

	TYPECPX* m_DelayBuf;
	TYPEREAL* m_MagBuf;
	TYPECPX* m_TestBenchDataBuf;
	int m_Dptr;
	int m_Mptr;
	int m_BlankCounter;
	int m_DelaySamples;
	int m_MagSamples;
	int m_WidthSamples;
	TYPEREAL m_Ratio;
	TYPEREAL m_MagAveSum;

	std::mutex m_Mutex;		//for keeping threads from stomping on each other

};
#endif //  NOISEPROC_H
