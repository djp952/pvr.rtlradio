// downconvert.h: interface for the CDownConvert class.
//
//  This class takes I/Q baseband data and performs tuning
//(Frequency shifting of the baseband signal) as well as
// decimation in powers of 2 after the shifting.
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
#ifndef DOWNCONVERT_H
#define DOWNCONVERT_H

#include "datatypes.h"
#include <mutex>


#define MAX_DECSTAGES 10	//one more than max to make sure is a null at end of list

enum class DownsampleQuality
{
	Low = 0,			// 11 tap
	Medium = 1,			// 27 tap
	High = 2,			// 51 tap
};

//////////////////////////////////////////////////////////////////////////////////
// Main Downconverter Class
//////////////////////////////////////////////////////////////////////////////////
class CDownConvert  
{
public:
	CDownConvert();
	virtual ~CDownConvert();
	void SetFrequency(TYPEREAL NcoFreq);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	TYPEREAL SetDataRate(TYPEREAL InRate, TYPEREAL MaxBW);
	TYPEREAL SetWfmDataRate(TYPEREAL InRate, TYPEREAL MaxBW);
	void SetQuality(enum DownsampleQuality Quality) { m_Quality = Quality; }

private:
	////////////
	//pure abstract base class for all the different types of decimate by 2 stages
	//DecBy2 function is defined in derived classes
	////////////
	class CDec2
	{
	public:
		CDec2(){}
		virtual ~CDec2(){}
		virtual int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData) = 0;
	};

	////////////
	//private class for the Half Band decimate by 2 stages
	////////////
	class CHalfBandDecimateBy2 : public CDec2
	{
	public:
		CHalfBandDecimateBy2(int len,const TYPEREAL* pCoef);
		~CHalfBandDecimateBy2(){if(m_pHBFirBuf) delete m_pHBFirBuf;}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPECPX* m_pHBFirBuf;
		int m_FirLength;
		const TYPEREAL* m_pCoef;
	};


	////////////
	//private class for the fixed 11 tap Half Band decimate by 2 stages
	////////////
	class CHalfBand11TapDecimateBy2 : public CDec2
	{
	public:
		CHalfBand11TapDecimateBy2();
		~CHalfBand11TapDecimateBy2(){}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPEREAL H0;	//unwrapped coeeficients
		TYPEREAL H2;
		TYPEREAL H4;
		TYPEREAL H5;
		TYPEREAL H6;
		TYPEREAL H8;
		TYPEREAL H10;
		TYPECPX d0;		//unwrapped delay buffer
		TYPECPX d1;
		TYPECPX d2;
		TYPECPX d3;
		TYPECPX d4;
		TYPECPX d5;
		TYPECPX d6;
		TYPECPX d7;
		TYPECPX d8;
		TYPECPX d9;
	};

	////////////
	//private class for the N=3 CIC decimate by 2 stages
	////////////
	class CCicN3DecimateBy2 : public CDec2
	{
	public:
		CCicN3DecimateBy2();
		~CCicN3DecimateBy2(){}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPECPX m_Xodd;
		TYPECPX m_Xeven;
	};

private:
	//private helper functions
	void DeleteFilters();

	enum DownsampleQuality m_Quality = DownsampleQuality::High;

	TYPEREAL m_OutputRate;
	TYPEREAL m_NcoFreq;
	TYPEREAL m_NcoInc;
	TYPEREAL m_NcoTime;
	TYPEREAL m_InRate;
	TYPEREAL m_MaxBW;
	TYPECPX m_Osc1;
	TYPEREAL m_OscCos;
	TYPEREAL m_OscSin;
#ifdef FMDSP_THREAD_SAFE
	mutable std::mutex m_Mutex;		//for keeping threads from stomping on each other
#endif
	//array of pointers for performing decimate by 2 stages
	CDec2* m_pDecimatorPtrs[MAX_DECSTAGES];

};

#endif // DOWNCONVERT_H
