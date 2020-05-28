// fskmod.cpp: implementation of the CFskMod class.
//
//  This class creates a DSC FSK signal
//
// History:
//	2012-10-23  Initial creation MSW
//	2012-10-23
//////////////////////////////////////////////////////////////////////

#include "fskmod.h"
#include "datatypes.h"


#define SHIFT_FREQ 170.0
#define SYMB_FREQ 100.0

#define MSG_SPACING 5.333		//seconds between msgs


const quint16 SYMBTBL[129] = {
	 0x0380, 0x0181, 0x0182, 0x0283, 0x0184, 0x0285, 0x0286, 0x0087, 0x0188, 0x0289,  //  0 to   9
	 0x028A, 0x008B, 0x028C, 0x008D, 0x008E, 0x030F, 0x0190, 0x0291, 0x0292, 0x0093,  // 10 to  19
	 0x0294, 0x0095, 0x0096, 0x0317, 0x0298, 0x0099, 0x009A, 0x031B, 0x009C, 0x031D,  // 20 to  29
	 0x031E, 0x011F, 0x01A0, 0x02A1, 0x02A2, 0x00A3, 0x02A4, 0x00A5, 0x00A6, 0x0327,  // 30 to  39
	 0x02A8, 0x00A9, 0x00AA, 0x032B, 0x00AC, 0x032D, 0x032E, 0x012F, 0x02B0, 0x00B1,  // 40 to  49
	 0x00B2, 0x0333, 0x00B4, 0x0335, 0x0336, 0x0137, 0x00B8, 0x0339, 0x033A, 0x013B,  // 50 to  59
	 0x033C, 0x013D, 0x013E, 0x023F, 0x01C0, 0x02C1, 0x02C2, 0x00C3, 0x02C4, 0x00C5,  // 60 to  69
	 0x00C6, 0x0347, 0x02C8, 0x00C9, 0x00CA, 0x034B, 0x00CC, 0x034D, 0x034E, 0x014F,  // 70 to  79
	 0x02D0, 0x00D1, 0x00D2, 0x0353, 0x00D4, 0x0355, 0x0356, 0x0157, 0x00D8, 0x0359,  // 80 to  89
	 0x035A, 0x015B, 0x035C, 0x015D, 0x015E, 0x025F, 0x02E0, 0x00E1, 0x00E2, 0x0363,  // 90 to  99
	 0x00E4, 0x0365, 0x0366, 0x0167, 0x00E8, 0x0369, 0x036A, 0x016B, 0x036C, 0x016D,  //100 to 109
	 0x016E, 0x026F, 0x00F0, 0x0371, 0x0372, 0x0173, 0x0374, 0x0175, 0x0176, 0x0277,  //110 to 119
	 0x0378, 0x0179, 0x017A, 0x027B, 0x017C, 0x027D, 0x027E, 0x007F, 0x0155
};

const quint8 DSC_MSG[] =
{
	128,128,	//sync dots
	125,111,125,110,125,109,125,108,125,107,125,106,
	120,105,120,104,
	0,120,36,120,60,0,0,36,30,60,108,0,30,30,96,108,
	82,30,0,96,0,82,118,0,126,0,126,118,
	126,126,126,126,126,126,126,126,126,126,
	117,126,67,126,117,117,117,67,
	255
};

/////////////////////////////////////////////////////////////////////////////////
//	Construct FSK mod object
/////////////////////////////////////////////////////////////////////////////////
CFskMod::CFskMod()
{
	SetSampleRate(12500.0, 0.0);
}

/////////////////////////////////////////////////////////////////////////////////
//	Initialize variables with given SampleRate
/////////////////////////////////////////////////////////////////////////////////
void CFskMod::SetSampleRate(TYPEREAL SampleRate, TYPEREAL FreqOffset)
{
	m_SampleRate = SampleRate;
	m_FskAcc = 0.0;
	m_SymbTime = 0.0;
	m_SymbPeriod = 1.0/SYMB_FREQ;
	m_SymbInc = 1.0/SampleRate;
	m_MarkInc = -K_2PI*(SHIFT_FREQ/2.0 - FreqOffset)/SampleRate;
	m_SpaceInc = K_2PI*(SHIFT_FREQ/2.0 + FreqOffset)/SampleRate;
	m_FskState = false;
	m_MsgDelay = (int)(SYMB_FREQ*MSG_SPACING);
	m_MsgTimer = m_MsgDelay;
	m_BitPos = 0;
	m_TxShiftReg = 0;
	m_MsgPos = 0;
}


/////////////////////////////////////////////////////////////////////////////////
//	Create FSK Modulation Signal
/////////////////////////////////////////////////////////////////////////////////
void CFskMod::GenerateData(int Length, TYPEREAL SigAmplitude, TYPECPX* pOutData)
{
int i;
	for( i=0; i<Length; i++)
	{
		if(m_FskState != 0)
		{
			pOutData[i].re = SigAmplitude*cos(m_FskAcc);
			pOutData[i].im = SigAmplitude*sin(m_FskAcc);
			if(1 == m_FskState)
				m_FskAcc += m_MarkInc;
			else
				m_FskAcc += m_SpaceInc;
		}
		else
		{
			pOutData[i].re  = 0.0;
			pOutData[i].im  = 0.0;
		}
		m_SymbTime += m_SymbInc;
		if( m_SymbTime >= m_SymbPeriod)
		{
			m_SymbTime -= m_SymbPeriod;
			m_FskState = CreateDSCData();
		}
	}
	m_FskAcc = (double)fmod((double)m_FskAcc, K_2PI);	//keep radian counter bounded
}

/////////////////////////////////////////////////////////////////////////////////
//	Create DSC Data Signal
// returns -1 for space, +1 for Mark, and 0 for no signal
/////////////////////////////////////////////////////////////////////////////////
int CFskMod::CreateDSCData()
{
	if(m_MsgTimer)
	{	//here if during intermsg time
		m_MsgTimer--;
		m_BitPos = 0;
		m_MsgPos = 0;
		return 0;
	}
	//here to generate DSC bit stream
	if(0==m_BitPos)
	{
		quint8 ch = DSC_MSG[m_MsgPos++];
		if(255 == ch )
			m_MsgTimer = m_MsgDelay;
		else
			m_TxShiftReg = SYMBTBL[ch];
	}
	else
		m_TxShiftReg >>= 1;
	//inc current Bit position modulo 10
	if(	++m_BitPos >= 10)
		m_BitPos = 0;
	if(m_TxShiftReg&1)
		return 1;
	else
		return -1;

}
 
