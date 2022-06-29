//////////////////////////////////////////////////////////////////////
// fractresampler.h: interface for the CFractResampler class.
//
//  This class implements a fractional resampler that can be used to
//convert between different sample rates
//
// History:
//	2010-09-15  Initial creation MSW
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
#ifndef FRACTRESAMPLER_H
#define FRACTRESAMPLER_H

#include <mutex>

#include "datatypes.h"

class CFractResampler  
{
public:
	CFractResampler();
	virtual ~CFractResampler();

	void Init(int MaxInputSize);
	//overloaded functions for processing different data types
	int Resample( int InLength, TYPEREAL Rate, TYPEREAL* pInBuf, TYPEREAL* pOutBuf);
	int Resample( int InLength, TYPEREAL Rate, TYPECPX* pInBuf, TYPECPX* pOutBuf);
	int Resample( int InLength, TYPEREAL Rate, TYPEREAL* pInBuf, TYPEMONO16* pOutBuf, TYPEREAL gain);
	int Resample( int InLength, TYPEREAL Rate, TYPECPX* pInBuf, TYPESTEREO16* pOutBuf, TYPEREAL gain);

private:
	TYPEREAL m_FloatTime;	//floating pt output time accumulator
	TYPEREAL* m_pSinc;	//ptr to sinc table
	TYPECPX* m_pInputBuf;	//internal working input sample buffer
};

#endif // FRACTRESAMPLER_H
