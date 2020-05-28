// pskdemod.cpp: implementation of the CPskDemod class.
//
//  This class takes I/Q baseband data and performs
// PSK demodulation
// History:
//	2015-02-25  Initial creation MSW
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
#include "pskdemod.h"
#include "datatypes.h"



#define SQ_THRESHOLD 0.7	//squelch threshold(probably should make it user adjustable)
#define OUT_AUDIO_SHIFT 700.0	//audio monitor tone shift frequency in Hz


#define K_NGN (4.7)	//gain to make error in Hz
#define NLP_K (.01)	//narrow AFC freq error LP filter constant
#define ELP_K (.05)	//squelch energy LP filter constant


/////////////////////////////////////////////////////////////////////////////////
//	Construct Psk demod object
/////////////////////////////////////////////////////////////////////////////////
CPskDemod::CPskDemod()
{
}

CPskDemod::~CPskDemod()
{

}

/////////////////////////////////////////////////////////////////////////////////
/// Set Module input sample rate(8000 to 20000), Symbol rate, and mode(ignored)
/////////////////////////////////////////////////////////////////////////////////
void CPskDemod::SetPskParams(TYPEREAL InSampleRate, TYPEREAL SymbRate, int Mode)
{
	m_PskMode = Mode;
//calculate integer N to get close to 500Hz sample rate
	m_DecRate = (int)(InSampleRate/500.0 + 0.5);
	//calc actual sample rate after integer decimation
	m_SampleRate = InSampleRate/(TYPEREAL)m_DecRate;
	m_DecCnt = 0;
//create bit filter as LP filter with passband ~Symbol rate(not perfect but is close)
	m_BitFir.InitLPFilter(0, 1.0, 60.0, SymbRate/2.0, SymbRate, m_SampleRate);//initialize BIT FIR filter
//create AFC filter as LP filter with passband ~2*Symbol Rate
	m_FreqFir.InitLPFilter(0, 1.0, 30.0, SymbRate, SymbRate*2.0, m_SampleRate);//initialize LP AFC FIR filter
	m_PrevSymbol.re = 0.0;
	m_PrevSymbol.im = 0.0;
//create Hi-Q resonator at the symbol rate to recover bit sync position
	m_BitSyncFilter.InitBP(SymbRate, 150, m_SampleRate);

//create fixed digital sin/cos oscillator to shift baseband psk to soundcard audio out for monitoring
	m_NcoInc = K_2PI*OUT_AUDIO_SHIFT/(InSampleRate);
	m_OscCos = MCOS(m_NcoInc);
	m_OscSin = MSIN(m_NcoInc);
	m_Osc1.re = 1.0;	//initialize unit vector that will get rotated
	m_Osc1.im = 0.0;
//init a bunch of internal variables
	m_NcoPhase = 0.0;
	m_WFerrAve = 0.0;
	m_NFerrAve = 0.0;
	m_FreqError = 0.0;
	m_IntegralFerr = 0.0;
	m_z1.re = 0.0; m_z1.im = 0.0;
	m_z2.re = 0.0; m_z2.im = 0.0;

	m_AveMag = 0.0;
	m_AveEnergy =0.0;
	m_LastBitMag = 0.0;
	m_LastSyncSlope = 0.0;

}

/////////////////////////////////////////////////////////////////////////////////
//	Process PSK demod (STEREO audio out version)
/////////////////////////////////////////////////////////////////////////////////
int CPskDemod::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPEREAL RealAudioBuf[2048];
	//just call real version and write real audio to both channels
	ProcessData( InLength, pInData, RealAudioBuf);
	for(int i=0; i<InLength; i++)
	{
		pOutData[i].re = RealAudioBuf[i];
		pOutData[i].im = RealAudioBuf[i];
	}
	return InLength;	//length of monitor audio output samples
}

/////////////////////////////////////////////////////////////////////////////////
//	Process PSK demod (MONO audio out version)
/////////////////////////////////////////////////////////////////////////////////
int CPskDemod::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
int length = 0;
	for(int i=0; i<InLength; i++)
	{
		//shift to baseband by AFC error frequency
		TYPECPX tmp = pInData[i];
		TYPEREAL Sin = MSIN(m_NcoPhase);
		TYPEREAL Cos = MCOS(m_NcoPhase);
		pInData[i].re = ((tmp.re * Cos) - (tmp.im * Sin));
		pInData[i].im = ((tmp.re * Sin) + (tmp.im * Cos));
		tmp = pInData[i];
		//update NCO phase with freqeuncy error offset
		m_NcoPhase += m_FreqError;
		m_NcoPhase = MFMOD(m_NcoPhase, K_2PI);	//keep radian counter bounded
		//now create shifted frequency data for audio out
		//use digital oscillator since is fixed freq
		TYPEREAL OscGn;
		TYPECPX Osc;
		Osc.re = m_Osc1.re * m_OscCos - m_Osc1.im * m_OscSin;
		Osc.im = m_Osc1.im * m_OscCos + m_Osc1.re * m_OscSin;
		OscGn = 1.95 - (m_Osc1.re*m_Osc1.re + m_Osc1.im*m_Osc1.im);
		m_Osc1.re = OscGn * Osc.re;
		m_Osc1.im = OscGn * Osc.im;
		//Cpx multiply by audio output shift frequency take only real
		pOutData[i] = ((tmp.re * Osc.re) - (tmp.im * Osc.im));

		//perform decimate by m_DecRate and normalize input data to about +/- 1.0
		// !! input has to be BW limited by main filter ~ <200Hz so just take every m_DecRate samples !!
		// in cutesdr input values are AGC'd but not normalized to 1.0 so do it here after decimation
		if( ++m_DecCnt >= m_DecRate )
		{
			m_DecCnt = 0;
			TYPEREAL p = sqrt( (tmp.re*tmp.re)+(tmp.im*tmp.im));
			m_AveMag = (1.0-0.01)*m_AveMag + (0.01)*p;
			if(m_AveMag>0.0)
			{
				pInData[length].re = pInData[i].re / m_AveMag;
				pInData[length].im = pInData[i].im / m_AveMag;
			}
			length++;
		}
	}
	//perform AFC on decimated I/Q data
	CalcAfc(length, pInData);
	//perform narrow bit filtering
	m_BitFir.ProcessFilter(length, pInData, pInData);

	//Generate bit magnitude array for getting bit sinc position
	for(int i=0; i<length; i++)
		m_BitMag[i] = fabs( pInData[i].re ) + fabs( pInData[i].im );
	//run Hi-Q resonator filter on mag data that creates a sin wave that will lock to BitRate clock
	m_BitSyncFilter.ProcessFilter(length, m_BitMag, m_BitMag);

	//search through sync filter output looking for positive peak of sine wave position
	for(int i=0; i<length; i++)
	{
		//the best bit sync position is at the positive peak of the m_BitMag waveform
		TYPEREAL CurrentSlope = m_BitMag[i] - m_LastBitMag;	//current slope
		//see if at the top peak of the sync waveform(slope changes from pos to neg)
		if( (CurrentSlope < 0.0) && (m_LastSyncSlope >= 0.0) )
		{	//are at sample time so use previous sample value as we are one sample behind in sync position
			ManageSquelch( DecodeSymb(m_PrevSample) );
		}
		m_LastBitMag = m_BitMag[i];		//save previous states
		m_LastSyncSlope = CurrentSlope;
		m_PrevSample = pInData[i];
	}
	return InLength;	//length of monitor audio output samples
}

//////////////////////////////////////////////////////////////////////
//  Manage AFC logic
//////////////////////////////////////////////////////////////////////
void CPskDemod::CalcAfc(int InLength, TYPECPX* pInData)
{
#define K_WGN (38.5)			//gain to make error in Hz
#define WLP_K (.002)
#define K_GNP 1.0
#define K_GNI 0.1
TYPEREAL ferror;
	//filter input about twice the BW of the psk signal for AFC calculation
	m_FreqFir.ProcessFilter(InLength, pInData, m_FreqErrBuf);
	for(int i=0; i<InLength; i++)
	{
		//FM demodulate using differentiator and LP filter to get overall frequency error
		ferror = K_WGN*((m_FreqErrBuf[i].im - m_z2.im) * m_z1.re - (m_FreqErrBuf[i].re - m_z2.re) * m_z1.im);
		m_z2 = m_z1;
		m_z1 = m_FreqErrBuf[i];
		// error is ~Hz error
		if( ferror > 16.0 )		//clamp range
			ferror = 16.0;
		if( ferror < -16.0 )
			ferror = -16.0;
		m_WFerrAve = (1.0-WLP_K)*m_WFerrAve + (WLP_K)*ferror;
	}
	if( fabs(m_WFerrAve) > 2.0 )
	{	//use wide freq error for large errors
		m_IntegralFerr += (K_GNI*m_WFerrAve);
		m_FreqError = K_GNP*m_WFerrAve + m_IntegralFerr;
	}
	else
	{	//use cross product freq error for small errors
		m_IntegralFerr += (K_GNI*m_NFerrAve);
		m_FreqError = K_GNP*m_NFerrAve + m_IntegralFerr;
	}
	//clamp integrator and frequency error terms
	if(m_IntegralFerr > 20.0)
		m_IntegralFerr = 20.0;
	else if(m_IntegralFerr < -20.0)
		m_IntegralFerr = -20.0;

	if(m_FreqError > 20.0)
		m_FreqError = 20.0;
	else if(m_FreqError < -20.0)
		m_FreqError = -20.0;

	//scale correction error to NCO phase increment units
	m_FreqError = -(K_2PI*m_FreqError)/(m_SampleRate*m_DecRate);
}

//////////////////////////////////////////////////////////////////////
//  Manage Squelch
//////////////////////////////////////////////////////////////////////
void CPskDemod::ManageSquelch(quint8 ch)
{
	if(0x0000 == m_BitAcc)	//if idle state then force sq on
		m_AveEnergy = 5.0;
	else if(0xFFFF == m_BitAcc)	//if ones force off
		m_AveEnergy = 0.0;
//qDebug()<<m_AveEnergy;
	if(m_AveEnergy<SQ_THRESHOLD)
		ch = 0;
	//if(ch != 0)
	//	emit g_pChatDialog->SendChatData(ch);
}

//////////////////////////////////////////////////////////////////////
//  Decode the new symbol
//////////////////////////////////////////////////////////////////////
quint8 CPskDemod::DecodeSymb(TYPECPX newsymb)
{
quint8 ch = 0;
quint8 bit;
	//calc dot product of BPSK symbol with previous symbol
	TYPEREAL DotProd = m_PrevSymbol.re * newsymb.re +  m_PrevSymbol.im * newsymb.im;
	//bpsk data bit is just sign of dot product
	if(DotProd < 0.0)
	{
		bit = 0;				//phase change
		DotProd = -DotProd;		//create abs of dot product as signal energy measure
	}
	else
		bit = 1;				//no phase change

	//filter dot product as rough signal energy indicator for squelch function
	m_AveEnergy = (1.0-ELP_K)*m_AveEnergy + (ELP_K)*DotProd;

	//calc cross product of BPSK symbol with previous symbol
	//cross product is proportional to frequency error after correcting with decoded bit
	TYPEREAL ferror = K_NGN*(m_PrevSymbol.re * newsymb.im - m_PrevSymbol.im * newsymb.re);
	//use decoded bit to remove sign ambiguity in error
	if(!bit)
		ferror = -ferror;
	// error is ~Hz error
	if( ferror > 3.0 )		//clamp error range
		ferror = 3.0;
	if( ferror < -3.0 )
		ferror = -3.0;
	//LP filter error
	m_NFerrAve = (1.0-NLP_K)*m_NFerrAve + (NLP_K)*ferror;
	//put new bit in veroicode shift register
	m_VericodeAcc <<= 1;
	m_VericodeAcc |= bit;
	if( 0 == (m_VericodeAcc & 0x0003) )	//if last 2 bits are zeros, character delimiter
	{
		if(m_VericodeAcc != 0 )
		{
			ch = VARICODE_DEC_TABLE[(m_VericodeAcc>>3) & 0x07FF];
			m_VericodeAcc = 0;
		}
	}
	m_BitAcc <<= 1;		//create bit shifter that doesnt get cleared for fast squelch use
	m_BitAcc |= bit;
	m_PrevSymbol = newsymb;
	return ch;
}





