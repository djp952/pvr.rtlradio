//////////////////////////////////////////////////////////////////////
// fskdemod.h: interface for the CFskDemod class.
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef FSKDEMOD_H
#define FSKDEMOD_H
#include "datatypes.h"
#include "fir.h"
#include "iir.h"

#include <string>

#define INTEGRATOR_BUF_LEN 125

class CFskDemod
{
public:
	CFskDemod(TYPEREAL samplerate);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
private:
	void ProcessNewDataBit(bool Bit);

	TYPEREAL m_SampleRate;
	TYPECPX m_MarkOsc1;
	TYPEREAL m_MarkOscCos;
	TYPEREAL m_MarkOscSin;
	TYPECPX m_SpaceOsc1;
	TYPEREAL m_SpaceOscCos;
	TYPEREAL m_SpaceOscSin;

	qint32 m_MarkIntegratorIndex;
	qint32 m_SpaceIntegratorIndex;
	TYPECPX m_MarkIntegrationBuf[INTEGRATOR_BUF_LEN];
	TYPECPX m_SpaceIntegrationBuf[INTEGRATOR_BUF_LEN];

	CFir m_Fir;
	CIir m_OutIir;
	CIir m_BitSyncFilter;

	TYPEREAL m_Outbuf[2048];
	TYPEREAL m_SyncSignal[2048];
	TYPEREAL m_LastSync;
	TYPEREAL m_LastSyncSlope;
	TYPEREAL m_LastData;

	TYPEREAL m_MarkAve;
	TYPEREAL m_SpaceAve;
	TYPEREAL m_AttackAlpha;
	TYPEREAL m_DecayAlpha;

	quint16 m_RxShiftReg;
	qint32 m_RxBitPos;
	quint32 m_RxDecodeState;
	qint32 m_RxDecodeTimer;

	quint8 m_Ecc;
	quint16 m_ShiftReg[16];		//used for 16 10 bit shift registers
	quint8 m_DxBuf[3];
	quint8 m_RxBuf[256];
	int m_DxBufIndex;
	int m_RxBufIndex;

	TYPECPX m_AudioShiftOsc1;
	TYPEREAL m_AudioShiftOscCos;
	TYPEREAL m_AudioShiftOscSin;


};

#endif // FSKDEMOD_H
