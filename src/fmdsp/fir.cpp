//////////////////////////////////////////////////////////////////////
// fir.cpp: implementation of the CFir class.
//
//  This class implements a FIR  filter using a dual flat coefficient
//array to eliminate testing for buffer wrap around.
//
//Filter coefficients can be from a fixed table or this class will create
// a lowpass or highpass filter from frequency and attenuation specifications
// using a Kaiser-Bessel windowed sinc algorithm
//
// History:
//	2011-01-29  Initial creation MSW
//	2011-03-27  Initial release
//	2011-08-07  Modified FIR filter initialization to force fixed size
//	2013-07-28  Added single/double precision math macros
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
//==========================================================================================
#include "fir.h"

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////
#define MAX_HALF_BAND_BUFSIZE 8192


/////////////////////////////////////////////////////////////////////////////////
//	Construct CFir object
/////////////////////////////////////////////////////////////////////////////////
CFir::CFir()
{
	m_NumTaps = 1;
	m_State = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//  Note the Coefficient array is twice the length and has a duplicated set
// in order to eliminate testing for buffer wrap in the inner loop
//  ex: if 3 tap FIR with coefficients{21,-43,15} is made into a array of 6 entries
//   {21, -43, 15, 21, -43, 15 }
//REAL version
/////////////////////////////////////////////////////////////////////////////////
void CFir::ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf)
{
TYPEREAL acc;
TYPEREAL* Zptr;
const TYPEREAL* Hptr;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(int i=0; i<InLength; i++)
	{
		m_rZBuf[m_State] = InBuf[i];
		Hptr = &m_Coef[m_NumTaps - m_State];
		Zptr = m_rZBuf;
		acc = (*Hptr++ * *Zptr++);	//do the 1st MAC
		for(int j=1; j<m_NumTaps; j++)
			acc += (*Hptr++ * *Zptr++);	//do the remaining MACs
		if(--m_State < 0)
			m_State += m_NumTaps;
		OutBuf[i] = acc;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//  Note the Coefficient array is twice the length and has a duplicated set
// in order to eliminate testing for buffer wrap in the inner loop
//  ex: if 3 tap FIR with coefficients{21,-43,15} is mae into a array of 6 entries
//   {21, -43, 15, 21, -43, 15 }
//COMPLEX version
/////////////////////////////////////////////////////////////////////////////////
void CFir::ProcessFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf)
{
TYPECPX acc;
TYPECPX* Zptr;
TYPEREAL* HIptr;
TYPEREAL* HQptr;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(int i=0; i<InLength; i++)
	{
		m_cZBuf[m_State] = InBuf[i];
		HIptr = m_ICoef + m_NumTaps - m_State;
		HQptr = m_QCoef + m_NumTaps - m_State;
		Zptr = m_cZBuf;
		acc.re = (*HIptr++ * (*Zptr).re);		//do the first MAC
		acc.im = (*HQptr++ * (*Zptr++).im);
		for(int j=1; j<m_NumTaps; j++)
		{
			acc.re += (*HIptr++ * (*Zptr).re);		//do the remaining MACs
			acc.im += (*HQptr++ * (*Zptr++).im);
		}
		if(--m_State < 0)
			m_State += m_NumTaps;
		OutBuf[i] = acc;
	}
}


/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//  Note the Coefficient array is twice the length and has a duplicated set
// in order to eliminate testing for buffer wrap in the inner loop
//  ex: if 3 tap FIR with coefficients{21,-43,15} is mae into a array of 6 entries
//   {21, -43, 15, 21, -43, 15 }
//REAL in COMPLEX out version (for Hilbert filter pair)
/////////////////////////////////////////////////////////////////////////////////
void CFir::ProcessFilter(int InLength, TYPEREAL* InBuf, TYPECPX* OutBuf)
{
TYPECPX acc;
TYPECPX* Zptr;
TYPEREAL* HIptr;
TYPEREAL* HQptr;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(int i=0; i<InLength; i++)
	{
		m_cZBuf[m_State].re = InBuf[i];
		m_cZBuf[m_State].im = InBuf[i];
		HIptr = m_ICoef + m_NumTaps - m_State;
		HQptr = m_QCoef + m_NumTaps - m_State;
		Zptr = m_cZBuf;
		acc.re = (*HIptr++ * (*Zptr).re);		//do the first MAC
		acc.im = (*HQptr++ * (*Zptr++).im);
		for(int j=1; j<m_NumTaps; j++)
		{
			acc.re += (*HIptr++ * (*Zptr).re);		//do the remaining MACs
			acc.im += (*HQptr++ * (*Zptr++).im);
		}
		if(--m_State < 0)
			m_State += m_NumTaps;
		OutBuf[i] = acc;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//  Initializes a pre-designed FIR filter with fixed coefficients
//	Iniitalize FIR variables and clear out buffers.
/////////////////////////////////////////////////////////////////////////////////
void CFir::InitConstFir( int NumTaps, const TYPEREAL* pCoef, TYPEREAL Fsamprate)
{
#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_SampleRate = Fsamprate;
	if(NumTaps>MAX_NUMCOEF)
		m_NumTaps = MAX_NUMCOEF;
	else
		m_NumTaps = NumTaps;
	for(int i=0; i<m_NumTaps; i++)
	{
		m_Coef[i] = pCoef[i];
		m_Coef[m_NumTaps+i] = pCoef[i];	//create duplicate for calculation efficiency
	}
	for(int i=0; i<m_NumTaps; i++)
	{	//zero input buffers
		m_rZBuf[i] = 0.0;
		m_cZBuf[i].re = 0.0;
		m_cZBuf[i].im = 0.0;
	}
	m_State = 0;	//zero filter state variable
}

/////////////////////////////////////////////////////////////////////////////////
//  Initializes a pre-designed complex FIR filter with fixed coefficients
//	Iniitalize FIR variables and clear out buffers.
/////////////////////////////////////////////////////////////////////////////////
void CFir::InitConstFir( int NumTaps, const TYPEREAL* pICoef, const TYPEREAL* pQCoef, TYPEREAL Fsamprate)
{
#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_SampleRate = Fsamprate;
	if(NumTaps>MAX_NUMCOEF)
		m_NumTaps = MAX_NUMCOEF;
	else
		m_NumTaps = NumTaps;
	for(int i=0; i<m_NumTaps; i++)
	{
		m_ICoef[i] = pICoef[i];
		m_ICoef[m_NumTaps+i] = pICoef[i];	//create duplicate for calculation efficiency
		m_QCoef[i] = pQCoef[i];
		m_QCoef[m_NumTaps+i] = pQCoef[i];	//create duplicate for calculation efficiency
	}
	for(int i=0; i<m_NumTaps; i++)
	{	//zero input buffers
		m_rZBuf[i] = 0.0;
		m_cZBuf[i].re = 0.0;
		m_cZBuf[i].im = 0.0;
	}
	m_State = 0;	//zero filter state variable
}

////////////////////////////////////////////////////////////////////
// Create a FIR Low Pass filter with scaled amplitude 'Scale'
// NumTaps if non-zero, forces filter design to be this number of taps
// Scale is linear amplitude scale factor.
// Astop = Stopband Atenuation in dB (ie 40dB is 40dB stopband attenuation)
// Fpass = Lowpass passband frequency in Hz
// Fstop = Lowpass stopband frequency in Hz
// Fsamprate = Sample Rate in Hz
//
//           -------------
//                        |
//                         |
//                          |
//                           |
//    Astop                   ---------------
//                    Fpass   Fstop
//
////////////////////////////////////////////////////////////////////
int CFir::InitLPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate)
{
int n;
TYPEREAL Beta;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_SampleRate = Fsamprate;
	//create normalized frequency parameters
	TYPEREAL normFpass = Fpass/Fsamprate;
	TYPEREAL normFstop = Fstop/Fsamprate;
	TYPEREAL normFcut = (normFstop + normFpass)/2.0;	//low pass filter 6dB cutoff

	//calculate Kaiser-Bessel window shape factor, Beta, from stopband attenuation
	if(Astop < 20.96)
		Beta = 0;
	else if(Astop >= 50.0)
		Beta = .1102 * (Astop - 8.71);
	else
		Beta = .5842 * MPOW( (Astop-20.96), 0.4) + .07886 * (Astop - 20.96);

	//Now Estimate number of filter taps required based on filter specs
	m_NumTaps = static_cast<int>((Astop - 8.0) / (2.285*K_2PI*(normFstop - normFpass) ) + 1);

	//clamp range of filter taps
	if(m_NumTaps > MAX_NUMCOEF )
		m_NumTaps = MAX_NUMCOEF;
	if(m_NumTaps < 3)
		m_NumTaps = 3;

	if(NumTaps)	//if need to force to to a number of taps
		m_NumTaps = NumTaps;

	TYPEREAL fCenter = .5*(TYPEREAL)(m_NumTaps-1);
	TYPEREAL izb = Izero(Beta);		//precalculate denominator since is same for all points
	for( n=0; n < m_NumTaps; n++)
	{
		TYPEREAL x = (TYPEREAL)n - fCenter;
		TYPEREAL c;
		// create ideal Sinc() LP filter with normFcut
		if( (TYPEREAL)n == fCenter )	//deal with odd size filter singularity where sin(0)/0==1
			c = 2.0 * normFcut;
		else
			c = MSIN(K_2PI*x*normFcut)/(K_PI*x);
		//calculate Kaiser window and multiply to get coefficient
		x = ((TYPEREAL)n - ((TYPEREAL)m_NumTaps-1.0)/2.0 ) / (((TYPEREAL)m_NumTaps-1.0)/2.0);
		m_Coef[n] = Scale * c * Izero( Beta * MSQRT(1 - (x*x) ) )  / izb;
	}

	//make a 2x length array for FIR flat calculation efficiency
	for (n = 0; n < m_NumTaps; n++)
		m_Coef[n+m_NumTaps] = m_Coef[n];

	//copy into complex coef buffers
	for (n = 0; n < m_NumTaps*2; n++)
	{
		m_ICoef[n] = m_Coef[n];
		m_QCoef[n] = m_Coef[n];
	}

	//Initialize the FIR buffers and state
	for(int i=0; i<m_NumTaps; i++)
	{
		m_rZBuf[i] = 0.0;
		m_cZBuf[i].re = 0.0;
		m_cZBuf[i].im = 0.0;
	}
	m_State = 0;

	return m_NumTaps;

}

////////////////////////////////////////////////////////////////////
// Create a FIR high Pass filter with scaled amplitude 'Scale'
// NumTaps if non-zero, forces filter design to be this number of taps
// Astop = Stopband Atenuation in dB (ie 40dB is 40dB stopband attenuation)
// Fpass = Highpass passband frequency in Hz
// Fstop = Highpass stopband frequency in Hz
// Fsamprate = Sample Rate in Hz
//
//  Apass              -------------
//                    /
//                   /
//                  /
//                 /
//  Astop ---------
//            Fstop   Fpass
////////////////////////////////////////////////////////////////////
int CFir::InitHPFilter(int NumTaps, TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate)
{
int n;
TYPEREAL Beta;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_SampleRate = Fsamprate;
	//create normalized frequency parameters
	TYPEREAL normFpass = Fpass/Fsamprate;
	TYPEREAL normFstop = Fstop/Fsamprate;
	TYPEREAL normFcut = (normFstop + normFpass)/2.0;	//high pass filter 6dB cutoff

	//calculate Kaiser-Bessel window shape factor, Beta, from stopband attenuation
	if(Astop < 20.96)
		Beta = 0;
	else if(Astop >= 50.0)
		Beta = .1102 * (Astop - 8.71);
	else
		Beta = .5842 * MPOW( (Astop-20.96), 0.4) + .07886 * (Astop - 20.96);

	//Now Estimate number of filter taps required based on filter specs
	m_NumTaps = static_cast<int>((Astop - 8.0) / (2.285*K_2PI*(normFpass - normFstop ) ) + 1);

	//clamp range of filter taps
	if(m_NumTaps>(MAX_NUMCOEF-1) )
		m_NumTaps = MAX_NUMCOEF-1;
	if(m_NumTaps < 3)
		m_NumTaps = 3;

	m_NumTaps |= 1;		//force to next odd number

	if(NumTaps)	//if need to force to to a number of taps
		m_NumTaps = NumTaps;

	TYPEREAL izb = Izero(Beta);		//precalculate denominator since is same for all points
	TYPEREAL fCenter = .5*(TYPEREAL)(m_NumTaps-1);
	for( n=0; n < m_NumTaps; n++)
	{
		TYPEREAL x = (TYPEREAL)n - (TYPEREAL)(m_NumTaps-1)/2.0;
		TYPEREAL c;
		// create ideal Sinc() HP filter with normFcut
		if( (TYPEREAL)n == fCenter )	//deal with odd size filter singularity where sin(0)/0==1
			c = 1.0 - 2.0 * normFcut;
		else
			c = MSIN(K_PI*x)/(K_PI*x) - MSIN(K_2PI*x*normFcut)/(K_PI*x);

		//calculate Kaiser window and multiply to get coefficient
		x = ((TYPEREAL)n - ((TYPEREAL)m_NumTaps-1.0)/2.0 ) / (((TYPEREAL)m_NumTaps-1.0)/2.0);
		m_Coef[n] = Scale * c * Izero( Beta * MSQRT(1 - (x*x) ) )  / izb;
	}

	//make a 2x length array for FIR flat calculation efficiency
	for (n = 0; n < m_NumTaps; n++)
		m_Coef[n+m_NumTaps] = m_Coef[n];

	//copy into complex coef buffers
	for (n = 0; n < m_NumTaps*2; n++)
	{
		m_ICoef[n] = m_Coef[n];
		m_QCoef[n] = m_Coef[n];
	}

	//Initialize the FIR buffers and state
	for(int i=0; i<m_NumTaps; i++)
	{
		m_rZBuf[i] = 0.0;
		m_cZBuf[i].re = 0.0;
		m_cZBuf[i].im = 0.0;
	}
	m_State = 0;

	return m_NumTaps;
}

///////////////////////////////////////////////////////////////////////////
// function to convert LP filter coefficients into complex hilbert bandpass
// filter coefficients.
// Hbpreal(n)= 2*Hlp(n)*cos( 2PI*FreqOffset*(n-(N-1)/2)/samplerate );
// Hbpimaj(n)= 2*Hlp(n)*sin( 2PI*FreqOffset*(n-(N-1)/2)/samplerate );
void CFir::GenerateHBFilter( TYPEREAL FreqOffset)
{
int n;
	for(n=0; n<m_NumTaps; n++)
	{
		// apply complex frequency shift transform to low pass filter coefficients
		m_ICoef[n] = 2.0 * m_Coef[n] * MCOS( (K_2PI*FreqOffset/m_SampleRate)*((TYPEREAL)n - ( (TYPEREAL)(m_NumTaps-1)/2.0 ) ) );
		m_QCoef[n] = 2.0 * m_Coef[n] * MSIN( (K_2PI*FreqOffset/m_SampleRate)*((TYPEREAL)n - ( (TYPEREAL)(m_NumTaps-1)/2.0 ) ) );
	}
	//make a 2x length array for FIR flat calculation efficiency
	for (n = 0; n < m_NumTaps; n++)
	{
		m_ICoef[n+m_NumTaps] = m_ICoef[n];
		m_QCoef[n+m_NumTaps] = m_QCoef[n];
	}
}

///////////////////////////////////////////////////////////////////////////
// private helper function to Compute Modified Bessel function I0(x)
//     using a series approximation.
// I0(x) = 1.0 + { sum from k=1 to infinity ---->  [(x/2)^k / k!]^2 }
///////////////////////////////////////////////////////////////////////////
TYPEREAL CFir::Izero(TYPEREAL x)
{
TYPEREAL x2 = x/2.0;
TYPEREAL sum = 1.0;
TYPEREAL ds = 1.0;
TYPEREAL di = 1.0;
TYPEREAL errorlimit = 1e-9;
TYPEREAL tmp;
	do
	{
		tmp = x2/di;
		tmp *= tmp;
		ds *= tmp;
		sum += ds;
		di += 1.0;
	}while(ds >= errorlimit*sum);

	return(sum);
}


// *&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*
//						Class to do decimation by 2
// *&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*

//////////////////////////////////////////////////////////////////////
//Decimate by 2 Halfband filter class implementation
//////////////////////////////////////////////////////////////////////
CDecimateBy2::CDecimateBy2(int len,const TYPEREAL* pCoef )
	: m_FirLength(len), m_pCoef(pCoef)
{
	//create buffer for FIR implementation
	m_pHBFirRBuf = new TYPEREAL[MAX_HALF_BAND_BUFSIZE];
	m_pHBFirCBuf = new TYPECPX[MAX_HALF_BAND_BUFSIZE];
	TYPECPX CPXZERO = {0.0,0.0};
	for(int i=0; i<MAX_HALF_BAND_BUFSIZE ;i++)
	{
		m_pHBFirRBuf[i] = 0.0;
		m_pHBFirCBuf[i] = CPXZERO;
	}
}

//////////////////////////////////////////////////////////////////////
// Half band filter and decimate by 2 function.
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps
// InLength must be an even number
//Mono version
//////////////////////////////////////////////////////////////////////
int CDecimateBy2::DecBy2(int InLength, TYPEREAL* pInData, TYPEREAL* pOutData)
{
int i;
int j;
int numoutsamples = 0;
	if(InLength<m_FirLength)	//safety net to make sure InLength is large enough to process
	{
		return InLength/2;
	}
	//copy input samples into buffer starting at position m_FirLength-1
	for(i=0,j = m_FirLength - 1; i<InLength; i++)
		m_pHBFirRBuf[j++] = pInData[i];
	//perform decimation FIR filter on even samples
	for(i=0; i<InLength; i+=2)
	{
		TYPEREAL acc;
		acc = ( m_pHBFirRBuf[i] * m_pCoef[0] );
		for(j=2; j<m_FirLength; j+=2)	//only use even coefficients since odd are zero(except center point)
			acc += ( m_pHBFirRBuf[i+j] * m_pCoef[j] );
		//now multiply the center coefficient
		acc += ( m_pHBFirRBuf[i+(m_FirLength-1)/2] * m_pCoef[(m_FirLength-1)/2] );
		pOutData[numoutsamples++] = acc;	//put output buffer
	}
	//need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
	// for FIR wrap around management
	for(i=0,j = InLength-m_FirLength+1; i<m_FirLength - 1; i++)
		m_pHBFirRBuf[i] = pInData[j++];
	return numoutsamples;
}

//////////////////////////////////////////////////////////////////////
// Half band filter and decimate by 2 function.
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps
// InLength must be an even number  ~37nS
// complex or stereo version
//////////////////////////////////////////////////////////////////////
int CDecimateBy2::DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int i;
int j;
int numoutsamples = 0;
	if( InLength<m_FirLength)	//safety net to make sure InLength is large enough to process
	{
		return InLength/2;
	}
	//copy input samples into buffer starting at position m_FirLength-1
	for(i=0,j = m_FirLength - 1; i<InLength; i++)
		m_pHBFirCBuf[j++] = pInData[i];
	//perform decimation FIR filter on even samples
	for(i=0; i<InLength; i+=2)
	{
		TYPECPX acc;
		acc.re = ( m_pHBFirCBuf[i].re * m_pCoef[0] );
		acc.im = ( m_pHBFirCBuf[i].im * m_pCoef[0] );
		for(j=2; j<m_FirLength; j+=2)	//only use even coefficients since odd are zero(except center point)
		{
			acc.re += ( m_pHBFirCBuf[i+j].re * m_pCoef[j] );
			acc.im += ( m_pHBFirCBuf[i+j].im * m_pCoef[j] );
		}
		//now multiply the center coefficient
		acc.re += ( m_pHBFirCBuf[i+(m_FirLength-1)/2].re * m_pCoef[(m_FirLength-1)/2] );
		acc.im += ( m_pHBFirCBuf[i+(m_FirLength-1)/2].im * m_pCoef[(m_FirLength-1)/2] );
		pOutData[numoutsamples++] = acc;	//put output buffer

	}
	//need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
	// for FIR wrap around management
	for(i=0,j = InLength-m_FirLength+1; i<m_FirLength - 1; i++)
		m_pHBFirCBuf[i] = pInData[j++];
	return numoutsamples;
}
