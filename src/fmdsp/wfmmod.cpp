// wfmdemod.cpp: implementation of the CWFmMod class.
//
//  This class Wideband stereo FM modulation
//
// History:
//	2011-08-18  Initial creation MSW
//	2011-08-18  Initial release
//	2013-07-28  Added single/double precision math macros
//////////////////////////////////////////////////////////////////////

//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2011 Moe Wheatley. All rights reserved.
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
#include "wfmmod.h"
#include "datatypes.h"
#include "rbdsconstants.h"

#define PILOT_FREQ 19000.0
#define DEVIATION_FREQ 75000.0

#define PILOT_LEVEL 0.1
#define AUDIO_LEVEL 0.9
#define RDS_LEVEL 0.0267

#define LEFT_FREQ 440.0
#define RIGHT_FREQ 666.0

#define STATE_TIME 2	//seconds

#define RDS_PICODE 0x75C8		//WMOE
#define RDS_GRPTYPE_0A (0<<12)	//Basic tuning info
#define RDS_PTYCODE (13<<5)		//Program type nostalgia


/////////////////////////////////////////////////////////////////////////////////
//	Construct WFM demod object
/////////////////////////////////////////////////////////////////////////////////
CWFmMod::CWFmMod()
{
	m_PilotAcc = 0.0;
	m_LeftAcc = 0.0;
	m_RightAcc = 0.0;
	m_ModAcc = 0.0;
	SetSampleRate(200000.0);
	StateTimer = 0;
	ModState = 0;
	TimerPeriod = 100000;
	m_SweepFreqNorm = 1.0;
	m_SweepFrequency = 0.0;
	m_SweepStopFrequency = 0.0;
	m_SweepRateInc = 0.0;
	m_LeftAmp = 1.0;
	m_RightAmp = 1.0;
}

/////////////////////////////////////////////////////////////////////////////////
//	Initialize variables with given SampleRate
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::SetSampleRate(TYPEREAL SampleRate)
{
	m_SampleRate = SampleRate;

	m_PilotInc = PILOT_FREQ*K_2PI/m_SampleRate;
	m_LeftInc = LEFT_FREQ*K_2PI/m_SampleRate;
	m_RightInc = RIGHT_FREQ*K_2PI/m_SampleRate;
	m_DeviationRate = DEVIATION_FREQ*K_2PI/m_SampleRate;

	TimerPeriod = static_cast<int>(STATE_TIME*SampleRate);
	StateTimer = TimerPeriod;
	ModState = 0;
	InitRDS();
}

/////////////////////////////////////////////////////////////////////////////////
//	Set Sweep generator settings
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::SetSweep(TYPEREAL SweepFreqNorm, TYPEREAL SweepFrequency, TYPEREAL SweepStopFrequency, TYPEREAL SweepRateInc)
{
	m_SweepFreqNorm = SweepFreqNorm;
	m_SweepFrequency = SweepFrequency;
	m_SweepStopFrequency = SweepStopFrequency;
	m_SweepRateInc = SweepRateInc;

}

/////////////////////////////////////////////////////////////////////////////////
//	Process WFM Mod STEREO version
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::GenerateData(int Length, TYPEREAL Amplitude, TYPECPX* pOutData)
{
TYPEREAL mod;
	CreateRdsSamples(Length, m_RdsOut);
	for(int i=0; i<Length; i++)
	{
#if 1
		m_LeftAcc += m_LeftInc;			//create left and right channel test tone increment
		m_RightAcc += m_RightInc;
		m_PilotAcc += m_PilotInc;		//create pilot freq increment
		mod = (m_LeftAmp*MSIN(m_LeftAcc) + m_RightAmp*MSIN(m_RightAcc))/2.0;	//mono component
		mod += ( (m_LeftAmp*MSIN(m_LeftAcc) - m_RightAmp*MSIN(m_RightAcc))/2.0) * MSIN(m_PilotAcc*2.0 );	//DSB left-right component
		mod *= AUDIO_LEVEL;
		mod += ( PILOT_LEVEL*MSIN(m_PilotAcc) );			//add pilot tone
		mod += ( RDS_LEVEL*m_RdsOut[i]*MSIN(m_PilotAcc*3.0) );			//add RDS data
#else	//sweep pilot tone for testing
		m_PilotAcc += ( m_SweepFrequency*m_SweepFreqNorm );
		m_SweepFrequency += m_SweepRateInc;	//inc sweep frequency
		if(m_SweepFrequency >= m_SweepStopFrequency)	//reached end of sweep?
			m_SweepRateInc = 0.0;						//stop sweep when end is reached
		mod = MSIN(m_PilotAcc);			//sweep pilot tone for testing
#endif
		m_ModAcc += (mod*m_DeviationRate);
		pOutData[i].re = Amplitude*MCOS(m_ModAcc);
		pOutData[i].im = Amplitude*MSIN(m_ModAcc);
	}
	while(m_PilotAcc>K_2PI)
		m_PilotAcc -= K_2PI;
	while(m_LeftAcc>K_2PI)
		m_LeftAcc -= K_2PI;
	while(m_RightAcc>K_2PI)
		m_RightAcc -= K_2PI;
	while(m_ModAcc>K_2PI)
		m_ModAcc -= K_2PI;
	while(m_ModAcc < -K_2PI)
		m_ModAcc += K_2PI;
#if 1	//state machine for stereo testing
	if(StateTimer++ > TimerPeriod/Length)
	{
		StateTimer = 0;
		switch(ModState)
		{
			case 0:
				m_LeftAmp = 1.0;
				m_RightAmp = 0.0;
				ModState = 1;
				break;
			case 1:
				m_LeftAmp = 0.0;
				m_RightAmp = 1.0;
				ModState = 2;
				break;
			case 2:
				m_LeftAmp = 1.0;
				m_RightAmp = 1.0;
				ModState = 3;
				break;
			case 3:
				m_LeftAmp = 0.0;
				m_RightAmp = 0.0;
				ModState = 0;
				break;
		}
	}
#endif
}
 
/////////////////////////////////////////////////////////////////////////////////
//	Initialize RDS generator variables
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::InitRDS()
{
	//create impulse response of bi-phase bits
	// This is basically the time domain shape of a single bi-phase bit
	// as defined for RDS and is close to a single cycle sine wave in shape
	m_RdsPulseLength = m_SampleRate / RDS_BITRATE;
	int Period = (int)(m_RdsPulseLength + .5);
	for(int i= 0; i<= Period; i++)
	{
		TYPEREAL t = (TYPEREAL)i/(m_SampleRate);
		TYPEREAL x = t*RDS_BITRATE;
		TYPEREAL x64 = 64.0*x;
		m_RdsPulseCoef[i+Period] = .75*MCOS(2.0*K_2PI*x)*( (1.0/(1.0/x-x64)) -
								(1.0/(9.0/x-x64)) );
		m_RdsPulseCoef[Period-i] = -.75*MCOS(2.0*K_2PI*x)*( (1.0/(1.0/x-x64)) -
								(1.0/(9.0/x-x64)) );
	}
	m_RdsPulseLength *= 2.0;
	m_RdsTime = 0.0;
	m_RdsSamplePeriod = 1.0/m_SampleRate;
	m_RdsD1 = 1;
	m_RdsD2 = 1;

#if 0		//debug hack to write m_RdsPulseCoef to a file for analysis
	QDir::setCurrent("d:/");
	QFile File;
	File.setFileName("rdscoef.txt");
	if(File.open(QIODevice::WriteOnly))
	{
		qDebug()<<"file Opened OK";
		char Buf[256];
		for(int n=0; n<Period*2; n++)
		{
			sprintf( Buf, "%g\r\n", m_RdsPulseCoef[n]);
			File.write(Buf);
		}
	}
	else
		qDebug()<<"file Failed to Open";
#endif

	//create some RDS messages for testing
	m_RdsBufLength = 0;
	m_RdsBitPtr = (1<<25);
	for(int i= 0; i<1; i++)
	{
		CreateRdsGroup(RDS_PICODE, RDS_GRPTYPE_0A|RDS_PTYCODE ,0x5A5A, 0xA5A5);
	}
	m_RdsBufPos = 0;
	m_RdsLastBit = 1;
}

/////////////////////////////////////////////////////////////////////////////////
//	Create a 4 block group from the parameters and create check words
//Group data is placed in m_RdsDataBuf[] for continuous transmitting
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::CreateRdsGroup(quint16 Blk1, quint16 Blk2, quint16 Blk3, quint16 Blk4)
{
	m_RdsDataBuf[m_RdsBufLength++] = CreateBlockWithCheckword(Blk1, OFFSET_WORD_BLOCK_A);
	m_RdsDataBuf[m_RdsBufLength++] = CreateBlockWithCheckword(Blk2, OFFSET_WORD_BLOCK_B);
	if(Blk2&GROUPB_BIT)
		m_RdsDataBuf[m_RdsBufLength++] = CreateBlockWithCheckword(Blk3, OFFSET_WORD_BLOCK_CP);
	else
		m_RdsDataBuf[m_RdsBufLength++] = CreateBlockWithCheckword(Blk3, OFFSET_WORD_BLOCK_C);
	m_RdsDataBuf[m_RdsBufLength++] = CreateBlockWithCheckword(Blk4, OFFSET_WORD_BLOCK_D);
}

/////////////////////////////////////////////////////////////////////////////////
//	Create data blk with chkword from 16 bit 'Data' with 'BlockOffset'.
// Returns 26 bit block with check word.
/////////////////////////////////////////////////////////////////////////////////
quint32 CWFmMod::CreateBlockWithCheckword(quint16 Data, quint32 BlockOffset)
{
quint32 block = (quint32)Data<<10;	//put 16 msg data bits into block
	for(int i=0; i<NUMBITS_MSG; i++)
	{	//do matrix operation on data bits 15 to 0
		//Since generator matrix is in a systematic form
		//(first 16 columns are a diagonal identity matrix),
		//just XOR from table where message bit is a one.
		if(Data & 0x8000)	//if msg bit 15 is 1, XOR with generator matrix value
			block = block ^ CHKWORDGEN[i];
		Data <<= 1;		//go to next bit position
	}
	block = block ^ BlockOffset;	//add in block offset word
	return block;
}


/////////////////////////////////////////////////////////////////////////////////
//	Creates 'InLength' real samples of RDS modulation data at m_SampleRate.
//	Calls 'CreateNextRdsBit()' every bit time to fetch the next source data bit.
// 'm_RdsTime' is a floating point time variable that is used to create the integer
// table index's since table length is not exact multiple of data period.
/////////////////////////////////////////////////////////////////////////////////
void CWFmMod::CreateRdsSamples(int InLength , TYPEREAL* pBuf)
{
int n1;
int n2;
TYPEREAL rdsperiod = 1.0/RDS_BITRATE;
TYPEREAL rds2period = 2.0/RDS_BITRATE;
	for(int i= 0; i<InLength; i++)
	{
		n1 = (int)( m_RdsTime * m_SampleRate);	//create integer index
		//calculate index positions of both pointers
		if(m_RdsTime > rdsperiod)
			n2 = (int)( (m_RdsTime-rdsperiod) * m_SampleRate);
		else
			n2 = (int)( (m_RdsTime+rdsperiod) * m_SampleRate);
		//if a pointer wraps to zero, get next new data bit value
		if(0==n1)
			m_RdsD1 = static_cast<int>(CreateNextRdsBit());
		if(0==n2)
			m_RdsD2 = static_cast<int>(CreateNextRdsBit());
		//get both table values and add togehter for output sample value
		pBuf[i] = m_RdsD1*m_RdsPulseCoef[n1] + m_RdsD2*m_RdsPulseCoef[n2];
		//manage running floating point time position
		m_RdsTime += m_RdsSamplePeriod;
		if(m_RdsTime >= rds2period)
			m_RdsTime -= rds2period;
	}

}


/////////////////////////////////////////////////////////////////////////////////
//	Gets next data bit from m_RdsDataBuf[] and converts to a + or - 1.0 value
// used by CreateRdsSamples() to produce the modulation waveform
/////////////////////////////////////////////////////////////////////////////////
TYPEREAL CWFmMod::CreateNextRdsBit()
{
int bit;
	//get next bit from 26 bit wide buffer msbit first
	if( m_RdsBitPtr & m_RdsDataBuf[m_RdsBufPos] )
		bit = 1;
	else
		bit = 0;
	m_RdsBitPtr >>= 1;	//shift bit pointer  to next bit
	if(0 == m_RdsBitPtr)
	{	//reached end of 26bit word so go to next word in m_RdsDataBuf[]
		m_RdsBitPtr = (1<<25);
		m_RdsBufPos++;
		if(m_RdsBufPos >= m_RdsBufLength)
			m_RdsBufPos = 0;	//reached end of m_RdsDataBuf[] so start over
	}
	//differential encode output bit by XOR with previous output bit
	//return +1.0 for a '1' and -1.0 for a '0'
	if( m_RdsLastBit ^ bit )
	{
		m_RdsLastBit = 1;
		return 1.0;
	}
	else
	{
		m_RdsLastBit = 0;
		return -1.0;
	}
}

