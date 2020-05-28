// fskdemod.cpp: implementation of the CFskDemod class.
//
//  This class takes I/Q baseband data and performs
// FSK demodulation and basic DSC decode into raw bytes
// History:
//	2012-10-19  Initial creation MSW
//	2017-09-17  Modified MSW
//////////////////////////////////////////////////////////////////////
#include "fskdemod.h"
#include "datatypes.h"
#include "fircoef.h"

#define SHIFT_FREQ 170.0

//ATC time constants in seconds
#define ATTACK_TIMECONST 0.025
#define DECAY_TIMECONST	0.3

////////////////////////////////////////////////////////////////////////////////
//index to table is 10 bit DSC symbol to read.
// value in table is 255 if not a valid character
//else returns 7 bit character value for 10 bit symbol.
// lsb is first data bit in transmission order
const quint8 CHARTBL[0x400] = {
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0000 to 000F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0010 to 001F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0020 to 002F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0030 to 003F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0040 to 004F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0050 to 005F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0060 to 006F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,127,  //0070 to 007F
255,255,255,255,255,255,255,  7,255,255,255, 11,255, 13, 14,255,  //0080 to 008F
255,255,255, 19,255, 21, 22,255,255, 25, 26,255, 28,255,255,255,  //0090 to 009F
255,255,255, 35,255, 37, 38,255,255, 41, 42,255, 44,255,255,255,  //00A0 to 00AF
255, 49, 50,255, 52,255,255,255, 56,255,255,255,255,255,255,255,  //00B0 to 00BF
255,255,255, 67,255, 69, 70,255,255, 73, 74,255, 76,255,255,255,  //00C0 to 00CF
255, 81, 82,255, 84,255,255,255, 88,255,255,255,255,255,255,255,  //00D0 to 00DF
255, 97, 98,255,100,255,255,255,104,255,255,255,255,255,255,255,  //00E0 to 00EF
112,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //00F0 to 00FF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0100 to 010F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 31,  //0110 to 011F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 47,  //0120 to 012F
255,255,255,255,255,255,255, 55,255,255,255, 59,255, 61, 62,255,  //0130 to 013F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 79,  //0140 to 014F
255,255,255,255,255,255,255, 87,255,255,255, 91,255, 93, 94,255,  //0150 to 015F
255,255,255,255,255,255,255,103,255,255,255,107,255,109,110,255,  //0160 to 016F
255,255,255,115,255,117,118,255,255,121,122,255,124,255,255,255,  //0170 to 017F
255,  1,  2,255,  4,255,255,255,  8,255,255,255,255,255,255,255,  //0180 to 018F
 16,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0190 to 019F
 32,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01A0 to 01AF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01B0 to 01BF
 64,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01C0 to 01CF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01D0 to 01DF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01E0 to 01EF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //01F0 to 01FF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0200 to 020F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0210 to 021F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0220 to 022F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 63,  //0230 to 023F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0240 to 024F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 95,  //0250 to 025F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,111,  //0260 to 026F
255,255,255,255,255,255,255,119,255,255,255,123,255,125,126,255,  //0270 to 027F
255,255,255,  3,255,  5,  6,255,255,  9, 10,255, 12,255,255,255,  //0280 to 028F
255, 17, 18,255, 20,255,255,255, 24,255,255,255,255,255,255,255,  //0290 to 029F
255, 33, 34,255, 36,255,255,255, 40,255,255,255,255,255,255,255,  //02A0 to 02AF
 48,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //02B0 to 02BF
255, 65, 66,255, 68,255,255,255, 72,255,255,255,255,255,255,255,  //02C0 to 02CF
 80,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //02D0 to 02DF
 96,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //02E0 to 02EF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //02F0 to 02FF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 15,  //0300 to 030F
255,255,255,255,255,255,255, 23,255,255,255, 27,255, 29, 30,255,  //0310 to 031F
255,255,255,255,255,255,255, 39,255,255,255, 43,255, 45, 46,255,  //0320 to 032F
255,255,255, 51,255, 53, 54,255,255, 57, 58,255, 60,255,255,255,  //0330 to 033F
255,255,255,255,255,255,255, 71,255,255,255, 75,255, 77, 78,255,  //0340 to 034F
255,255,255, 83,255, 85, 86,255,255, 89, 90,255, 92,255,255,255,  //0350 to 035F
255,255,255, 99,255,101,102,255,255,105,106,255,108,255,255,255,  //0360 to 036F
255,113,114,255,116,255,255,255,120,255,255,255,255,255,255,255,  //0370 to 037F
  0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0380 to 038F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //0390 to 039F
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03A0 to 03AF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03B0 to 03BF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03C0 to 03CF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03D0 to 03DF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03E0 to 03EF
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //03F0 to 03FF
};


//DSC decoder states
#define STATE_PHASEDET 0
#define STATE_DECODEDX 1
#define STATE_DECODERX 2
#define STATE_ECCDX 3
#define STATE_ECCRX 4

#define MSG_TIMEOUT 70		//number of RX/DX characters to read before giving up


/////////////////////////////////////////////////////////////////////////////////
//	Construct FSK demod object
/////////////////////////////////////////////////////////////////////////////////
CFskDemod::CFskDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
TYPEREAL PhzInc = K_2PI*SHIFT_FREQ/(2.0*m_SampleRate);	// FreqShift/2
	m_MarkOsc1.re = 1.0;	//initialize oscillator unit vectors that will get rotated
	m_MarkOsc1.im = 0.0;
	m_SpaceOsc1.re = 1.0;
	m_SpaceOsc1.im = 0.0;

	m_MarkOscCos = cos(-PhzInc);
	m_MarkOscSin = sin(-PhzInc);
	m_SpaceOscCos = cos(PhzInc);
	m_SpaceOscSin = sin(PhzInc);

	//init integrators
	m_MarkIntegratorIndex = 0;
	m_SpaceIntegratorIndex = 0;
	for(int i=0; i<INTEGRATOR_BUF_LEN; i++)
	{
		m_MarkIntegrationBuf[i].re = 0.0;
		m_MarkIntegrationBuf[i].im = 0.0;
		m_SpaceIntegrationBuf[i].re = 0.0;
		m_SpaceIntegrationBuf[i].im = 0.0;
	}

	//calculate ATC dual time constant filter values.
	m_AttackAlpha = (1.0-exp(-1.0/(m_SampleRate*ATTACK_TIMECONST)) );
	m_DecayAlpha = (1.0-exp(-1.0/(m_SampleRate*DECAY_TIMECONST)) );
	m_MarkAve = 0.0;
	m_SpaceAve = 0.0;

	//Init LP filter for complex input. abt +-250Hz
	m_Fir.InitConstFir(FSK1_LENGTH, FSK1_COEF, FSK1_COEF, m_SampleRate);

	//Init Post data IIR LP filter
	m_OutIir.InitLP(TYPEREAL(150.0), TYPEREAL(0.707), m_SampleRate);

	//create Hi-Q resonator at the bit rate to recover bit sync position Q==200
	m_BitSyncFilter.InitBP(100.0, 200.0, m_SampleRate);

	//init some decoder variables
	m_RxShiftReg = 0;
	m_RxBitPos = 0;
	m_RxDecodeState = STATE_PHASEDET;
	m_RxDecodeTimer = 0;
	for(int i=0; i<16; i++)
		m_ShiftReg[i] = 0;

	//just for testing, create 1700Hz osc for shifting baseband IQ into audio range for soundcard
	m_AudioShiftOsc1.re = 1.0;	//initialize interocitor unit vectors that will get rotated
	m_AudioShiftOsc1.im = 0.0;
	m_AudioShiftOscCos = cos(K_2PI*1700.0/m_SampleRate);
	m_AudioShiftOscSin = sin(K_2PI*1700.0/m_SampleRate);
}


/////////////////////////////////////////////////////////////////////////////////
//	FSK demod Process 'InLength' samples of 'pInData' into 'pOutData'
/////////////////////////////////////////////////////////////////////////////////
int CFskDemod::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPECPX cxIn;
TYPECPX cxtmp;
TYPEREAL Mpwr;
TYPEREAL Spwr;
TYPEREAL OscGn;
TYPECPX Osc;
	//lowpass Filter +/-250Hz
	m_Fir.ProcessFilter(InLength, pInData, pInData);
//g_pTestBench->DisplayData(InLength, pInData, m_SampleRate, PROFILE_2);

	for(int i=0; i<InLength; i++)
	{
		cxIn = pInData[i];	//make copy of complex I/Q input sample

		//First Process MARK('1") signal path
		//create -85Hz Mark correlator Osc samples using quadrature oscillator
		Osc.re = m_MarkOsc1.re * m_MarkOscCos - m_MarkOsc1.im * m_MarkOscSin;
		Osc.im = m_MarkOsc1.im * m_MarkOscCos + m_MarkOsc1.re * m_MarkOscSin;
		OscGn = 1.95 - (m_MarkOsc1.re*m_MarkOsc1.re + m_MarkOsc1.im*m_MarkOsc1.im);
		m_MarkOsc1.re = OscGn * Osc.re;		//keeps amplitude bounded
		m_MarkOsc1.im = OscGn * Osc.im;
		//
		//Cpx multiply Input sample by Mark shift frequency(bring -85Hz to 0Hz)
		cxtmp.re = ((cxIn.re * Osc.re) + (cxIn.im * Osc.im));
		cxtmp.im = ((cxIn.im * Osc.re) - (cxIn.re * Osc.im));

		//place shifted Mark I/Q sample in Circular Integrator Buffer
		//that is one symbol time deep
		m_MarkIntegrationBuf[m_MarkIntegratorIndex++] = cxtmp;
		if(m_MarkIntegratorIndex >= INTEGRATOR_BUF_LEN)
			m_MarkIntegratorIndex = 0;

		//integrate over the last 'INTEGRATOR_BUF_LEN' samples
		cxtmp.re = 0.0; cxtmp.im = 0.0;
		for(int j=0; j<INTEGRATOR_BUF_LEN; j++)
		{
			cxtmp.re += m_MarkIntegrationBuf[j].re;
			cxtmp.im += m_MarkIntegrationBuf[j].im;
		}
		//calc correlation energy in Mark signal
		Mpwr = sqrt(cxtmp.re*cxtmp.re + cxtmp.im*cxtmp.im);

		//create average Mark energy with dual time constant agc for ATC
		if(Mpwr>m_MarkAve)	//if magnitude is rising (use m_AttackAlpha time constant)
			m_MarkAve = (1.0-m_AttackAlpha)*m_MarkAve + m_AttackAlpha*Mpwr;
		else				//else magnitude is falling (use m_DecayAlpha time constant)
			m_MarkAve = (1.0-m_DecayAlpha)*m_MarkAve + m_DecayAlpha*Mpwr;

		//Now Process SPACE('0') signal path
		//create +85Hz Space correlator Osc samples using quadrature oscillator
		Osc.re = m_SpaceOsc1.re * m_SpaceOscCos - m_SpaceOsc1.im * m_SpaceOscSin;
		Osc.im = m_SpaceOsc1.im * m_SpaceOscCos + m_SpaceOsc1.re * m_SpaceOscSin;
		OscGn = 1.95 - (m_SpaceOsc1.re*m_SpaceOsc1.re + m_SpaceOsc1.im*m_SpaceOsc1.im);
		m_SpaceOsc1.re = OscGn * Osc.re;		//keeps amplitude bounded
		m_SpaceOsc1.im = OscGn * Osc.im;

		//Cpx multiply Input sample by Space shift frequency(bring +85Hz to 0Hz)
		cxtmp.re = ((cxIn.re * Osc.re) + (cxIn.im * Osc.im));
		cxtmp.im = ((cxIn.im * Osc.re) - (cxIn.re * Osc.im));

		//place shifted Mark I/Q sample in Circular Integrator Buffer
		//that is one symbol time deep
		m_SpaceIntegrationBuf[m_SpaceIntegratorIndex++] = cxtmp;
		if(m_SpaceIntegratorIndex>=INTEGRATOR_BUF_LEN)
			m_SpaceIntegratorIndex = 0;

		//integrate over the last 'INTEGRATOR_BUF_LEN' samples
		cxtmp.re = 0.0; cxtmp.im = 0.0;
		for(int j=0; j<INTEGRATOR_BUF_LEN; j++)
		{
			cxtmp.re += m_SpaceIntegrationBuf[j].re;
			cxtmp.im += m_SpaceIntegrationBuf[j].im;
		}
		//calc correlation energy in Space signal
		Spwr = sqrt(cxtmp.re*cxtmp.re + cxtmp.im*cxtmp.im);

		//create average Space energy with dual time constant agc for ATC
		if(Spwr>m_SpaceAve)	//if magnitude is rising (use m_AttackAlpha time constant)
			m_SpaceAve = (1.0-m_AttackAlpha)*m_SpaceAve + m_AttackAlpha*Spwr;
		else				//else magnitude is falling (use m_DecayAlpha time constant)
			m_SpaceAve = (1.0-m_DecayAlpha)*m_SpaceAve + m_DecayAlpha*Spwr;

		//calculate mark-space difference and shift by ATC threshold value
		//
		m_Outbuf[i] = (Mpwr - Spwr + m_SpaceAve/2.0 - m_MarkAve/2.0);

		//perform post filter 1 sample at a time in this loop(should be done before ATC?)
		m_OutIir.ProcessFilter(1, &m_Outbuf[i], &m_Outbuf[i]);

		//create absolute value version of output to use in bit sync algorithm
		m_SyncSignal[i] = fabs(m_Outbuf[i]);
	}

	//now Create Bit sync signal with High Q Resonator IIR BP Filter
	//using squared magnitude of data.  Positive peak of this signal
	//correspondes to the maximum bit energy position so good place to sample
	m_BitSyncFilter.ProcessFilter(InLength, m_SyncSignal, m_SyncSignal);
	for(int i=0; i<InLength; i++)
	{
		//the best bit sync position is at the positive peak of the m_SyncSignal waveform
		TYPEREAL CurrentSlope = m_SyncSignal[i] - m_LastSync;	//current slope
		//see if at the top peak of the sync waveform(slope changes from pos to neg)
		if( (CurrentSlope < 0.0) && (m_LastSyncSlope >= 0.0) )
		{	//are at sample time so use previous bit time value since we are one sample behind in sync position
			ProcessNewDataBit( (m_LastData>0) );	//go process new DSC bit
		}

		m_LastSync = m_SyncSignal[i];		//save previous states
		m_LastSyncSlope = CurrentSlope;
		m_LastData = m_Outbuf[i];

		pOutData[i].im = m_Outbuf[i]/10.0;	//use pInData as debug output buf for display
		pOutData[i].re = m_SyncSignal[i]/3.0;	//use pInData as debug output buf for display
	}
//g_pTestBench->DisplayData(InLength, pOutData, m_SampleRate, PROFILE_3);

	//for testing, create 1700Hz shifted copy of IQ data to get sent to soundcard
	for(int i=0; i<InLength; i++)
	{
		cxIn = pInData[i];	//make copy of complex I/Q input sample
		//create 1700Hz Osc samples using quadrature oscillator
		Osc.re = m_AudioShiftOsc1.re * m_AudioShiftOscCos - m_AudioShiftOsc1.im * m_AudioShiftOscSin;
		Osc.im = m_AudioShiftOsc1.im * m_AudioShiftOscCos + m_AudioShiftOsc1.re * m_AudioShiftOscSin;
		OscGn = 1.95 - (m_AudioShiftOsc1.re*m_AudioShiftOsc1.re + m_AudioShiftOsc1.im*m_AudioShiftOsc1.im);
		m_AudioShiftOsc1.re = OscGn * Osc.re;		//keeps amplitude bounded
		m_AudioShiftOsc1.im = OscGn * Osc.im;
		//
		//Cpx multiply Input sample by audio frequency(bring baseband up to 1700Hz)
		pOutData[i].re = ( (cxIn.re * Osc.re) + (cxIn.im * Osc.im) );
		pOutData[i].im = ( (cxIn.im * Osc.re) - (cxIn.re * Osc.im) );
	}

	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	FSK demod Process 'InLength' samples of 'pInData' into 'pOutData'
/////////////////////////////////////////////////////////////////////////////////
int CFskDemod::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
TYPECPX cxIn;
TYPECPX cxtmp;
TYPEREAL Mpwr;
TYPEREAL Spwr;
TYPEREAL OscGn;
TYPECPX Osc;
	//lowpass Filter +/-250Hz
	m_Fir.ProcessFilter(InLength, pInData, pInData);
//g_pTestBench->DisplayData(InLength, pInData, m_SampleRate, PROFILE_2);

	for(int i=0; i<InLength; i++)
	{
		cxIn = pInData[i];	//make copy of complex I/Q input sample

		//First Process MARK('1") signal path
		//create -85Hz Mark correlator Osc samples using quadrature oscillator
		Osc.re = m_MarkOsc1.re * m_MarkOscCos - m_MarkOsc1.im * m_MarkOscSin;
		Osc.im = m_MarkOsc1.im * m_MarkOscCos + m_MarkOsc1.re * m_MarkOscSin;
		OscGn = 1.95 - (m_MarkOsc1.re*m_MarkOsc1.re + m_MarkOsc1.im*m_MarkOsc1.im);
		m_MarkOsc1.re = OscGn * Osc.re;		//keeps amplitude bounded
		m_MarkOsc1.im = OscGn * Osc.im;
		//
		//Cpx multiply Input sample by Mark shift frequency(bring -85Hz to 0Hz)
		cxtmp.re = ((cxIn.re * Osc.re) + (cxIn.im * Osc.im));
		cxtmp.im = ((cxIn.im * Osc.re) - (cxIn.re * Osc.im));

		//place shifted Mark I/Q sample in Circular Integrator Buffer
		//that is one symbol time deep
		m_MarkIntegrationBuf[m_MarkIntegratorIndex++] = cxtmp;
		if(m_MarkIntegratorIndex >= INTEGRATOR_BUF_LEN)
			m_MarkIntegratorIndex = 0;

		//integrate over the last 'INTEGRATOR_BUF_LEN' samples
		cxtmp.re = 0.0; cxtmp.im = 0.0;
		for(int j=0; j<INTEGRATOR_BUF_LEN; j++)
		{
			cxtmp.re += m_MarkIntegrationBuf[j].re;
			cxtmp.im += m_MarkIntegrationBuf[j].im;
		}
		//calc correlation energy in Mark signal
		Mpwr = sqrt(cxtmp.re*cxtmp.re + cxtmp.im*cxtmp.im);

		//create average Mark energy with dual time constant agc for ATC
		if(Mpwr>m_MarkAve)	//if magnitude is rising (use m_AttackAlpha time constant)
			m_MarkAve = (1.0-m_AttackAlpha)*m_MarkAve + m_AttackAlpha*Mpwr;
		else				//else magnitude is falling (use m_DecayAlpha time constant)
			m_MarkAve = (1.0-m_DecayAlpha)*m_MarkAve + m_DecayAlpha*Mpwr;

		//Now Process SPACE('0') signal path
		//create +85Hz Space correlator Osc samples using quadrature oscillator
		Osc.re = m_SpaceOsc1.re * m_SpaceOscCos - m_SpaceOsc1.im * m_SpaceOscSin;
		Osc.im = m_SpaceOsc1.im * m_SpaceOscCos + m_SpaceOsc1.re * m_SpaceOscSin;
		OscGn = 1.95 - (m_SpaceOsc1.re*m_SpaceOsc1.re + m_SpaceOsc1.im*m_SpaceOsc1.im);
		m_SpaceOsc1.re = OscGn * Osc.re;		//keeps amplitude bounded
		m_SpaceOsc1.im = OscGn * Osc.im;

		//Cpx multiply Input sample by Space shift frequency(bring +85Hz to 0Hz)
		cxtmp.re = ((cxIn.re * Osc.re) + (cxIn.im * Osc.im));
		cxtmp.im = ((cxIn.im * Osc.re) - (cxIn.re * Osc.im));

		//place shifted Mark I/Q sample in Circular Integrator Buffer
		//that is one symbol time deep
		m_SpaceIntegrationBuf[m_SpaceIntegratorIndex++] = cxtmp;
		if(m_SpaceIntegratorIndex>=INTEGRATOR_BUF_LEN)
			m_SpaceIntegratorIndex = 0;

		//integrate over the last 'INTEGRATOR_BUF_LEN' samples
		cxtmp.re = 0.0; cxtmp.im = 0.0;
		for(int j=0; j<INTEGRATOR_BUF_LEN; j++)
		{
			cxtmp.re += m_SpaceIntegrationBuf[j].re;
			cxtmp.im += m_SpaceIntegrationBuf[j].im;
		}
		//calc correlation energy in Space signal
		Spwr = sqrt(cxtmp.re*cxtmp.re + cxtmp.im*cxtmp.im);

		//create average Space energy with dual time constant agc for ATC
		if(Spwr>m_SpaceAve)	//if magnitude is rising (use m_AttackAlpha time constant)
			m_SpaceAve = (1.0-m_AttackAlpha)*m_SpaceAve + m_AttackAlpha*Spwr;
		else				//else magnitude is falling (use m_DecayAlpha time constant)
			m_SpaceAve = (1.0-m_DecayAlpha)*m_SpaceAve + m_DecayAlpha*Spwr;

		//calculate mark-space difference and shift by ATC threshold value
		//
		m_Outbuf[i] = (Mpwr - Spwr + m_SpaceAve/2.0 - m_MarkAve/2.0);

		//perform post filter 1 sample at a time in this loop(should be done before ATC?)
		m_OutIir.ProcessFilter(1, &m_Outbuf[i], &m_Outbuf[i]);

		//create absolute value version of output to use in bit sync algorithm
		m_SyncSignal[i] = fabs(m_Outbuf[i]);
	}

	//now Create Bit sync signal with High Q Resonator IIR BP Filter
	//using squared magnitude of data.  Positive peak of this signal
	//correspondes to the maximum bit energy position so good place to sample
	m_BitSyncFilter.ProcessFilter(InLength, m_SyncSignal, m_SyncSignal);
	for(int i=0; i<InLength; i++)
	{
		//the best bit sync position is at the positive peak of the m_SyncSignal waveform
		TYPEREAL CurrentSlope = m_SyncSignal[i] - m_LastSync;	//current slope
		//see if at the top peak of the sync waveform(slope changes from pos to neg)
		if( (CurrentSlope < 0.0) && (m_LastSyncSlope >= 0.0) )
		{	//are at sample time so use previous bit time value since we are one sample behind in sync position
			ProcessNewDataBit( (m_LastData>0) );	//go process new DSC bit
		}

		m_LastSync = m_SyncSignal[i];		//save previous states
		m_LastSyncSlope = CurrentSlope;
		m_LastData = m_Outbuf[i];

	}
//g_pTestBench->DisplayData(InLength, pOutData, m_SampleRate, PROFILE_3);


	for(int i=0; i<InLength; i++)
	{
		pOutData[i] = pInData[i].re;
	}

	return InLength;
}


/////////////////////////////////////////////////////////////////////////////////
//	DSC state decoder to find bit phase position and get raw characters.
/////////////////////////////////////////////////////////////////////////////////
void CFskDemod::ProcessNewDataBit(bool Bit)
{
quint8 ch;
int i;
int dxphzcnt = 0;
int rxphzcnt = 0;
	//shift in new data bit into 160 bit shift register consisting of 16 ten bit shift registers
	for(i=0; i<16; i++)
	{
		m_ShiftReg[i] >>= 1;
		if(i<15)
		{
			if(m_ShiftReg[i+1] & 1)	//shift in bit from upper register
				m_ShiftReg[i] |= 0x0200;
		}
		else
		{
			if(Bit)
				m_ShiftReg[15] |= 0x0200;
		}
	}
	// go through all Rx Phase positions and count matches to Rx phase characters
	//this will detect a phase sequence even if already decoding a msg.
	if(111 == CHARTBL[ m_ShiftReg[1] ]) rxphzcnt++;
	if(110 == CHARTBL[ m_ShiftReg[3] ]) rxphzcnt++;
	if(109 == CHARTBL[ m_ShiftReg[5] ]) rxphzcnt++;
	if(108 == CHARTBL[ m_ShiftReg[7] ]) rxphzcnt++;
	if(107 == CHARTBL[ m_ShiftReg[9] ]) rxphzcnt++;
	if(106 == CHARTBL[ m_ShiftReg[11] ]) rxphzcnt++;
	if(105 == CHARTBL[ m_ShiftReg[13] ]) rxphzcnt++;
	if(104 == CHARTBL[ m_ShiftReg[15] ]) rxphzcnt++;

	if(125 == CHARTBL[ m_ShiftReg[0] ]) dxphzcnt++;
	if(125 == CHARTBL[ m_ShiftReg[2] ]) dxphzcnt++;
	if(125 == CHARTBL[ m_ShiftReg[4] ]) dxphzcnt++;
	if(125 == CHARTBL[ m_ShiftReg[6] ]) dxphzcnt++;
	if(125 == CHARTBL[ m_ShiftReg[8] ]) dxphzcnt++;
	if(125 == CHARTBL[ m_ShiftReg[10] ]) dxphzcnt++;

	if( (rxphzcnt>=3) || ((rxphzcnt==2)&&(dxphzcnt>=1)) || ((rxphzcnt==1)&&(dxphzcnt>=2)))
	{
//qDebug()<<"Got Phase Sequence pattern";
		m_DxBufIndex = 0;
		m_RxBufIndex = 0;
		m_DxBuf[m_DxBufIndex++] = CHARTBL[ m_ShiftReg[12] ];	//read in first A Dx field
		m_DxBuf[m_DxBufIndex++] = CHARTBL[ m_ShiftReg[14] ];	//read in second A Dx field
		m_RxDecodeState = STATE_DECODEDX;
		m_RxBitPos = -10;	//set sync bit position so shifts in next full symbol before executing decode state machine
		m_Ecc = 0;
		m_RxDecodeTimer = MSG_TIMEOUT;
	}
	if(0==m_RxBitPos)
	{
		if(m_RxDecodeTimer > 0)		//dec msg timeout
		{
			m_RxDecodeTimer--;
		}
		else
		{
			m_RxDecodeState = STATE_PHASEDET;
		}
		switch(	m_RxDecodeState)
		{
			case STATE_PHASEDET:	//looking for initial phazing position so do nothing
				break;
			case STATE_DECODEDX:	//here to store next Dx character into
				ch = CHARTBL[ m_ShiftReg[15] ];
				m_DxBuf[m_DxBufIndex++] = ch;
				if(m_DxBufIndex>2)	//only need to save last 3 Dx characters
					m_DxBufIndex = 0;
				m_RxDecodeState = STATE_DECODERX;
				break;
			case STATE_DECODERX:
				ch = CHARTBL[ m_ShiftReg[15] ];
				if(ch!=255)	//if Rx char is ok use it
					m_RxBuf[m_RxBufIndex] = ch;
				else	//else use Dx char
					m_RxBuf[m_RxBufIndex] = m_DxBuf[m_DxBufIndex];

				if(1==m_RxBufIndex)	//if second 'A' position is error then use first char 'A'
				{
					if(255 == m_RxBuf[1] )
						m_RxBuf[1] = m_RxBuf[0];
					m_Ecc = m_RxBuf[1];		//init ecc value
				}
				else
					m_Ecc ^= m_RxBuf[m_RxBufIndex];
				if( (117==m_RxBuf[m_RxBufIndex]) || (122==m_RxBuf[m_RxBufIndex]) || (127==m_RxBuf[m_RxBufIndex]) )
				{
					m_RxDecodeState = STATE_ECCDX;
				}
				else
				{
					m_RxDecodeState = STATE_DECODEDX;
				}
				m_RxBufIndex++;
				break;
			case STATE_ECCDX:	//here to store next Dx character
				ch = CHARTBL[ m_ShiftReg[15] ];
				m_DxBuf[m_DxBufIndex++] = ch;
				if(m_DxBufIndex>2)	//only need to save last 3 Dx characters
					m_DxBufIndex = 0;
				m_RxDecodeState = STATE_ECCRX;
				break;
			case STATE_ECCRX:
				ch = CHARTBL[ m_ShiftReg[15] ];
				if(ch!=255)	//if ECC char is ok use it
					m_RxBuf[m_RxBufIndex] = ch;
				else	//else use Dx ECC
					m_RxBuf[m_RxBufIndex] = m_DxBuf[m_DxBufIndex];

				m_RxDecodeState = STATE_PHASEDET;
				//create/Reset Hi-Q resonator at the bit rate to recover bit sync position Q==200
				m_BitSyncFilter.InitBP(100.0, 200.0, m_SampleRate);
				break;
		}
	}

	//inc current Bit position modulo 10
	if(	++m_RxBitPos >= 10)
		m_RxBitPos = 0;

}
