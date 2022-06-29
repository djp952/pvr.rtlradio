// fmdemod.cpp: implementation of the CFmDemod class.
//
//  This class takes I/Q baseband data and performs
// FM demodulation and squelch
//
// History:
//	2011-01-17  Initial creation MSW
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
#include "fmdemod.h"
#include "datatypes.h"


#define FMPLL_RANGE 15000.0	//maximum deviation limit of PLL
#define VOICE_BANDWIDTH 2500.0 //3000.0

#define FMPLL_BW VOICE_BANDWIDTH	//natural frequency ~loop bandwidth
#define FMPLL_ZETA .707				//PLL Loop damping factor

#define FMDC_ALPHA 0.001	//time constant for DC removal filter

#define MAX_FMOUT 100000.0

#define SQUELCH_MAX 8000.0		//roughly the maximum noise average with no signal
#define SQUELCHAVE_TIMECONST .02
#define SQUELCH_HYSTERESIS 50.0

#define DEMPHASIS_TIME 75e-6 //80e-6

/////////////////////////////////////////////////////////////////////////////////
//	Construct FM demod object
/////////////////////////////////////////////////////////////////////////////////
CFmDemod::CFmDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
	m_FreqErrorDC = 0.0;
	m_NcoPhase = 0.0;
	m_NcoFreq = 0.0;

	SetSampleRate(m_SampleRate);
}


/////////////////////////////////////////////////////////////////////////////////
// Sets sample rate and adjusts any parameters that are affected.
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::SetSampleRate(TYPEREAL samplerate)
{
	m_SampleRate = samplerate;

	TYPEREAL norm = K_2PI/m_SampleRate;	//to normalize Hz to radians

	//initialize the PLL
	m_NcoLLimit = -FMPLL_RANGE * norm;		//clamp FM PLL NCO
	m_NcoHLimit = FMPLL_RANGE * norm;
	m_PllAlpha = 2.0*FMPLL_ZETA*FMPLL_BW * norm;
	m_PllBeta = (m_PllAlpha * m_PllAlpha)/(4.0*FMPLL_ZETA*FMPLL_ZETA);

	m_OutGain = MAX_FMOUT/m_NcoHLimit;	//audio output level gain value

	//DC removal filter time constant
	m_DcAlpha = (1.0 - MEXP(-1.0/(m_SampleRate*FMDC_ALPHA)) );

	//initialize some noise squelch items
	m_SquelchHPFreq = VOICE_BANDWIDTH;
	m_SquelchAve = 0.0;
	m_SquelchState = true;
	m_SquelchAlpha = (1.0-MEXP(-1.0/(m_SampleRate*SQUELCHAVE_TIMECONST)) );

	m_DeemphasisAlpha = (1.0-MEXP(-1.0/(m_SampleRate*DEMPHASIS_TIME)) );
	m_DeemphasisAve = 0.0;

	//m_LpFir.InitLPFilter(0, 1.0, 50.0, VOICE_BANDWIDTH, 1.6 * VOICE_BANDWIDTH, m_SampleRate);
	m_LpFir.InitLPFilter(0, 1.0, 50.0, VOICE_BANDWIDTH, 2.0 * VOICE_BANDWIDTH, m_SampleRate);

	InitNoiseSquelch();
}


/////////////////////////////////////////////////////////////////////////////////
// Sets squelch threshold based on 'Value' which goes from -160 to 0.
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::SetSquelch(int Value)
{
	m_SquelchThreshold = (SQUELCH_MAX*(TYPEREAL)Value)/-160.0;
}


/////////////////////////////////////////////////////////////////////////////////
// Sets up Highpass noise filter parameters based on input filter BW
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::InitNoiseSquelch()
{
	//m_HpFir.InitHPFilter(0, 1.0, 50.0, m_SquelchHPFreq*.8, m_SquelchHPFreq*.65, m_SampleRate);
	m_HpFir.InitHPFilter(0, 1.0, 50.0, VOICE_BANDWIDTH*2.0, VOICE_BANDWIDTH, m_SampleRate);
}


/////////////////////////////////////////////////////////////////////////////////
// Performs noise squelch by reading the noise power above the voice frequencies
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::PerformNoiseSquelch(int InLength, TYPEREAL* pOutData)
{
	if(InLength>MAX_SQBUF_SIZE)
		return;
	TYPEREAL sqbuf[MAX_SQBUF_SIZE];
	//high pass filter to get the high frequency noise above the voice
	m_HpFir.ProcessFilter(InLength, pOutData, sqbuf);
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL mag = MFABS( sqbuf[i] );	//get magnitude of High pass filtered data
		// exponential filter squelch magnitude
		m_SquelchAve = (1.0-m_SquelchAlpha)*m_SquelchAve + m_SquelchAlpha*mag;
	}
	//perform squelch compare to threshold using some Hysteresis
	if(0==m_SquelchThreshold)
	{	//force squelch if zero(strong signal threshold)
		m_SquelchState = true;
	}
	else if(m_SquelchState)	//if in squelched state
	{
		if(m_SquelchAve < (m_SquelchThreshold-SQUELCH_HYSTERESIS))
			m_SquelchState = false;
	}
	else
	{
		if(m_SquelchAve >= (m_SquelchThreshold+SQUELCH_HYSTERESIS))
			m_SquelchState = true;
	}
//m_SquelchState = false;
	if(m_SquelchState)
	{	//zero output if squelched
		for(int i=0; i<InLength; i++)
			pOutData[i] = 0.0;
	}
	else
	{	//low pass filter audio if squelch is open
//		ProcessDeemphasisFilter(InLength, pOutData, pOutData);
		m_LpFir.ProcessFilter(InLength, pOutData, pOutData);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Process FM demod MONO version
/////////////////////////////////////////////////////////////////////////////////
int CFmDemod::ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPEREAL* pOutData)
{
TYPECPX tmp;
	if(m_SquelchHPFreq != FmBW)
	{	//update squelch HP filter cutoff from main filter BW
		m_SquelchHPFreq = FmBW;
		InitNoiseSquelch();
	}
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = MSIN(m_NcoPhase);
		TYPEREAL Cos = MCOS(m_NcoPhase);
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -MATAN2(tmp.im, tmp.re);
		//create new NCO frequency term
		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't get out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//LP filter the NCO frequency term to get DC offset value
		m_FreqErrorDC = (1.0-m_DcAlpha)*m_FreqErrorDC + m_DcAlpha*m_NcoFreq;
		//subtract out DC term to get FM audio
		pOutData[i] = (m_NcoFreq-m_FreqErrorDC)*m_OutGain;
	}
	m_NcoPhase = MFMOD(m_NcoPhase, K_2PI);	//keep radian counter bounded
	PerformNoiseSquelch(InLength, pOutData);	//calculate squelch
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process FM demod STEREO version
/////////////////////////////////////////////////////////////////////////////////
int CFmDemod::ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPECPX tmp;
	if(m_SquelchHPFreq != FmBW)
	{	//update Squelch HP filter cutoff from main filter BW
		m_SquelchHPFreq = FmBW;
		InitNoiseSquelch();
	}
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = MSIN(m_NcoPhase);
		TYPEREAL Cos = MCOS(m_NcoPhase);
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -MATAN2(tmp.im, tmp.re);

		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't drift out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//LP filter the NCO frequency term to get DC offset value
		m_FreqErrorDC = (1.0-m_DcAlpha)*m_FreqErrorDC + m_DcAlpha*m_NcoFreq;
		//subtract out DC term to get FM audio
		m_OutBuf[i] = (m_NcoFreq-m_FreqErrorDC)*m_OutGain;
	}
	m_NcoPhase = MFMOD(m_NcoPhase, K_2PI);	//keep radian counter bounded
	PerformNoiseSquelch(InLength, m_OutBuf);
	for(int i=0; i<InLength; i++)
	{	//copy audio stream into both output channels for stereo version
		pOutData[i].re = m_OutBuf[i];
		pOutData[i].im = m_OutBuf[i];
	}
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process InLength InBuf[] samples and place in OutBuf[]
//MONO version
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::ProcessDeemphasisFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf)
{
	for(int i=0; i<InLength; i++)
	{
		m_DeemphasisAve = (1.0-m_DeemphasisAlpha)*m_DeemphasisAve + m_DeemphasisAlpha*InBuf[i];
		OutBuf[i] = m_DeemphasisAve;
	}
}

