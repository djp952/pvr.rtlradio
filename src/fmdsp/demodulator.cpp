/////////////////////////////////////////////////////////////////////
// demodulator.cpp: implementation of the Cdemodulator class.
//
//	This class implements the demodulation DSP functionality to take
//raw I/Q data from the radio, shift to baseband, decimate, demodulate,
//perform AGC, and send the audio to the sound card.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2013-07-28  Added single/double precision math macros
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
//==========================================================================================
#include "demodulator.h"

//---------------------------------------------------------------------------
// get_signal_levels
//
// Gets the approximate signal levels of the input samples
//
// Arguments:
//
//	numsamples		- Number of samples in the buffer
//	samples			- Pointer to the samples buffer
//	rms				- Receives the RMS calculation
//	noise			- Receives the Standard Deviation calculation

static void get_signal_levels(int numsamples, TYPECPX const* samples, TYPEREAL& rms, TYPEREAL& noise)
{
	numsamples = (numsamples + 63) / 64;

	rms = 0;
	noise = 0;

	if(numsamples == 0) return;

	// MEAN POWER
	//
	TYPEREAL mean = 0;
	for(int index = 0; index < numsamples; index++) {

		TYPECPX const& sample = samples[index];
		mean += sqrt((sample.re * sample.re) + (sample.im * sample.im));
	}
	mean /= numsamples;

	// RMS POWER
	//
	rms = sqrt(mean * mean);

	// STANDARD DEVIATION
	//
	for(int index = 0; index < numsamples; index++) {

		TYPECPX const& sample = samples[index];
		TYPEREAL power = sqrt((sample.re * sample.re) + (sample.im * sample.im)) - mean;
		noise += (power * power);
	}
	noise = sqrt(noise / numsamples);
}

//////////////////////////////////////////////////////////////////
//	Constructor/Destructor
//////////////////////////////////////////////////////////////////
CDemodulator::CDemodulator()
{
	m_DownConverterOutputRate = 48000.0;
	m_DemodOutputRate = 48000.0;
	m_pDemodInBuf = new TYPECPX[MAX_INBUFSIZE];
	m_pDemodTmpBuf = new TYPECPX[MAX_INBUFSIZE];
	m_InBufPos = 0;
	m_InBufLimit = 1000;
	m_DemodMode = -1;
	m_pFmDemod = NULL;
	m_pWFmDemod = NULL;
	m_USFm = true;
	SetDemodFreq(0.0);
}

CDemodulator::~CDemodulator()
{
	DeleteAllDemods();
	if(m_pDemodInBuf)
		delete m_pDemodInBuf;
	if(m_pDemodTmpBuf)
		delete m_pDemodTmpBuf;
}

//////////////////////////////////////////////////////////////////
//	Deletes all demod objects
//////////////////////////////////////////////////////////////////
void CDemodulator::DeleteAllDemods()
{
	if(m_pFmDemod)
		delete m_pFmDemod;
	if(m_pWFmDemod)
		delete m_pWFmDemod;
	m_pFmDemod = NULL;
	m_pWFmDemod = NULL;
}

//////////////////////////////////////////////////////////////////
//	Called to set/change the demodulator input sample rate
//////////////////////////////////////////////////////////////////
void CDemodulator::SetInputSampleRate(TYPEREAL InputRate)
{
	if(m_InputRate != InputRate)
	{
		m_InputRate = InputRate;
		//change any demod parameters that may occur with sample rate change
		switch(m_DemodMode)
		{
			case DEMOD_FM:
				m_DownConverterOutputRate = m_DownConvert.SetDataRate(m_InputRate, m_DesiredMaxOutputBandwidth);
				m_DemodOutputRate = m_DownConverterOutputRate;
				if(m_pFmDemod)
					m_pFmDemod->SetSampleRate(m_DownConverterOutputRate);
				break;
			case DEMOD_WFM:
				m_DownConverterOutputRate = m_DownConvert.SetWfmDataRate(m_InputRate, 100000);
				if(m_pWFmDemod)
					m_DemodOutputRate = m_pWFmDemod->SetSampleRate(m_DownConverterOutputRate, m_USFm);
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////
//	Called to set/change the active Demod object
//or if a demod parameter or filter parameter changes
//////////////////////////////////////////////////////////////////
void CDemodulator::SetDemod(int Mode, tDemodInfo CurrentDemodInfo)
{
#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_DownConvert.SetQuality(CurrentDemodInfo.WfmDownsampleQuality);

	m_DemodInfo = CurrentDemodInfo;
	if(m_DemodMode != Mode)	//do only if changes
	{
		DeleteAllDemods();		//remove current demod object
		m_DemodMode = Mode;
		m_DesiredMaxOutputBandwidth = m_DemodInfo.HiCutmax;
		
		//now create correct demodulator
		switch(m_DemodMode)
		{
			case DEMOD_FM:
				m_DownConverterOutputRate = m_DownConvert.SetDataRate(m_InputRate, m_DesiredMaxOutputBandwidth);
				m_pFmDemod = new CFmDemod(m_DownConverterOutputRate);
				m_DemodOutputRate = m_DownConverterOutputRate;
				break;
			case DEMOD_WFM:
				m_DownConverterOutputRate = m_DownConvert.SetWfmDataRate(m_InputRate, 100000);
				m_pWFmDemod = new CWFmDemod(m_DownConverterOutputRate);
				m_DemodOutputRate = m_pWFmDemod->GetDemodRate();
				break;
		}
	}

	if(m_DemodMode != DEMOD_WFM)
	{
		m_FastFIR.SetupParameters(m_DemodInfo.LowCut, m_DemodInfo.HiCut, 0, m_DownConverterOutputRate);
		m_Agc.SetParameters(m_DemodInfo.AgcOn, m_DemodInfo.AgcHangOn, m_DemodInfo.AgcThresh,
			m_DemodInfo.AgcManualGain, m_DemodInfo.AgcSlope, m_DemodInfo.AgcDecay, m_DownConverterOutputRate);
	}
	if(	m_pFmDemod != NULL)
		m_pFmDemod->SetSquelch(m_DemodInfo.SquelchValue);
	//set input buffer limit so that decimated output is abt 10mSec or more of data
	m_InBufLimit = static_cast<int>((m_DemodOutputRate/100.0) * m_InputRate/m_DemodOutputRate);	//process abt .01sec of output samples at a time
	m_InBufLimit &= 0xFFFFFF00;	//keep modulo 256 since decimation is only in power of 2
}

//////////////////////////////////////////////////////////////////
//	Called with complex data from radio and performs the demodulation
// with MONO audio output
//////////////////////////////////////////////////////////////////
int CDemodulator::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
int ret = 0;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(int i=0; i<InLength; i++)
	{	//place in demod buffer
		m_pDemodInBuf[m_InBufPos++] = pInData[i];
		if(m_InBufPos >= m_InBufLimit)
		{	//when have enough samples, call demod routine sequence

			//perform baseband tuning and decimation
			int n = m_DownConvert.ProcessData(m_InBufPos, m_pDemodInBuf, m_pDemodInBuf);

			if(m_DemodMode != DEMOD_WFM)
			{	//if not wideband FM mode do filtering and AGC
				//perform main bandpass filtering
				n = m_FastFIR.ProcessData(n, m_pDemodInBuf, m_pDemodTmpBuf);

				//perform AGC
				m_Agc.ProcessData(n, m_pDemodTmpBuf, m_pDemodTmpBuf );
			}

			// Get the signal levels
			TYPEREAL signal = 0, noise = 0;
			get_signal_levels(n, m_pDemodInBuf, signal, noise);

			// Smooth the signal level
			if(isnan(m_SignalLevel)) m_SignalLevel = signal;
			else m_SignalLevel = 0.95 * m_SignalLevel + 0.05 * signal;

			// Smooth the noise level
			if(isnan(m_NoiseLevel)) m_NoiseLevel = (noise);
			else m_NoiseLevel = 0.95 * m_NoiseLevel + 0.05 * (noise);

			//perform the desired demod action
			switch(m_DemodMode)
			{
				case DEMOD_FM:
					n = m_pFmDemod->ProcessData(n, m_DemodInfo.HiCut, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_WFM:
					n = m_pWFmDemod->ProcessData(n, m_pDemodInBuf, pOutData );
					break;
			}
			m_InBufPos = 0;
			ret += n;
		}
	}

	return ret;
}

//////////////////////////////////////////////////////////////////
//	Called with complex data from radio and performs the demodulation
// with STEREO audio output
//////////////////////////////////////////////////////////////////
int CDemodulator::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int ret = 0;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(int i=0; i<InLength; i++)
	{	//place in demod buffer
		m_pDemodInBuf[m_InBufPos++] = pInData[i];
		if(m_InBufPos >= m_InBufLimit)
		{	//when have enough samples, call demod routine sequence

			//perform baseband tuning and decimation
			int n = m_DownConvert.ProcessData(m_InBufPos, m_pDemodInBuf, m_pDemodInBuf);

			if(m_DemodMode != DEMOD_WFM)
			{	//if not wideband FM mode do filtering and AGC
				//perform main bandpass filtering
				n = m_FastFIR.ProcessData(n, m_pDemodInBuf, m_pDemodTmpBuf);

				//perform AGC
				m_Agc.ProcessData(n, m_pDemodTmpBuf, m_pDemodTmpBuf );
			}

			// Get the signal levels
			TYPEREAL signal = 0, noise = 0;
			get_signal_levels(n, m_pDemodInBuf, signal, noise);

			// Smooth the signal level
			if(isnan(m_SignalLevel)) m_SignalLevel = signal;
			else m_SignalLevel = 0.95 * m_SignalLevel + 0.05 * signal;

			// Smooth the noise level
			if(isnan(m_NoiseLevel)) m_NoiseLevel = (noise);
			else m_NoiseLevel = 0.95 * m_NoiseLevel + 0.05 * (noise);

			//perform the desired demod action
			switch(m_DemodMode)
			{
				case DEMOD_FM:
					n = m_pFmDemod->ProcessData(n, m_DemodInfo.HiCut, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_WFM:
					n = m_pWFmDemod->ProcessData(n, m_pDemodInBuf, pOutData );
					break;
			}

			m_InBufPos = 0;
			ret += n;
		}
	}

	return ret;
}
