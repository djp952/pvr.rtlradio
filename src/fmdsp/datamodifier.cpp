#include "datamodifier.h"
#include <cstdlib>


CDataModifier::CDataModifier()
{
	m_SampleRate = 1;
	m_SweepRate = 1;
	m_SweepStartFrequency = 1;
	m_SweepStopFrequency = 1;

}

void CDataModifier::Init(TYPEREAL SampleRate)
{
	m_SampleRate = SampleRate;
	//initialize sweep generator values
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepFreqNorm = K_2PI/m_SampleRate;
	m_SweepAcc = 0.0;
	m_SweepRateInc = m_SweepRate/m_SampleRate;
	m_SweepDirUp = true;
	if(m_SweepStartFrequency>=m_SweepStopFrequency)
		m_SweepDirUp = true;
	else
		m_SweepDirUp = false;
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
void CDataModifier::SetSweepStart(int start)
{
	m_SweepStartFrequency = (TYPEREAL)start;
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepAcc = 0.0;
	if(m_SweepStartFrequency>=m_SweepStopFrequency)
		m_SweepDirUp = true;
	else
		m_SweepDirUp = false;

}

void CDataModifier::SetSweepStop(int stop)
{
	m_SweepStopFrequency = (TYPEREAL)stop;
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepAcc = 0.0;
	if(m_SweepStartFrequency>=m_SweepStopFrequency)
		m_SweepDirUp = true;
	else
		m_SweepDirUp = false;
}

void CDataModifier::SetSweepRate(TYPEREAL rate)
{
	m_SweepRate = rate; // Hz/sec
	m_SweepAcc = 0.0;
	m_SweepRateInc = m_SweepRate/m_SampleRate;
}

void CDataModifier::SetSignalPower( TYPEREAL PwrdB)
{
	m_SignalAmplitude = 1.0*MPOW(10.0, PwrdB/20.0);
}

void CDataModifier::SetNoisePower( TYPEREAL NoisedB)
{
	m_NoisePower = NoisedB;
	m_NoiseAmplitude = 1.0*MPOW(10.0, NoisedB/20.0);
}


void CDataModifier::ProcessBlock(TYPECPX* pBuf, int NumSamples)
{
	TYPECPX dtmp;
	TYPECPX Osc;
	TYPEREAL r;
	TYPEREAL rad;
	TYPEREAL u1;
	TYPEREAL u2;
	for(int i=0; i<NumSamples; i++)
	{
		//create complex sin/cos modulation signal
		Osc.re = m_SignalAmplitude * MCOS(m_SweepAcc);
		Osc.im = m_SignalAmplitude * MSIN(m_SweepAcc);
		//inc phase accummulator with normalized freqeuency step
		m_SweepAcc += ( m_SweepFrequency*m_SweepFreqNorm );
		if(	m_SweepDirUp )
		{
			m_SweepFrequency += m_SweepRateInc;	//inc sweep frequency
			if(m_SweepFrequency >= m_SweepStopFrequency)	//reached end of sweep?
				m_SweepDirUp = false;
		}
		else
		{
			m_SweepFrequency -= m_SweepRateInc;	//dec sweep frequency
			if(m_SweepFrequency <= m_SweepStartFrequency)	//reached start of sweep?
				m_SweepDirUp = true;
		}
		//Cpx multiply by shift frequency
		dtmp = pBuf[i];
		pBuf[i].re = ((dtmp.re * Osc.re) - (dtmp.im * Osc.im));
		pBuf[i].im = ((dtmp.re * Osc.im) + (dtmp.im * Osc.re));

		//////////////////  Gaussian Noise generator
		// Generate two uniform random numbers between -1 and +1
		// that are inside the unit circle
		if(m_NoisePower > -160.0)
		{	//create and add noise samples to signal
			do {
				u1 = 1.0 - 2.0 * (TYPEREAL)rand()/(TYPEREAL)RAND_MAX ;
				u2 = 1.0 - 2.0 * (TYPEREAL)rand()/(TYPEREAL)RAND_MAX ;
				r = u1*u1 + u2*u2;
			} while(r >= 1.0 || r == 0.0);
			rad = MSQRT(-2.0*MLOG(r)/r);
			//add noise samples to generator output
			pBuf[i].re += (m_NoiseAmplitude*u1*rad);
			pBuf[i].im += (m_NoiseAmplitude*u2*rad);
		}
	}
	m_SweepAcc = (TYPEREAL)MFMOD((TYPEREAL)m_SweepAcc, K_2PI);	//keep radian counter bounded
}
