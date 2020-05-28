//////////////////////////////////////////////////////////////////////
// wfmmod.h: interface for the CFskMod class.
//
// History:
//	2012-10-23  Initial creation MSW
//	2012-10-23
/////////////////////////////////////////////////////////////////////
#ifndef FSKMOD_H
#define FSKMOD_H
#include "datatypes.h"



class CFskMod
{
public:
	CFskMod();
	void GenerateData(int InLength,TYPEREAL Amplitude, TYPECPX* pOutData);
	void SetSampleRate(TYPEREAL SampleRate, TYPEREAL FreqOffset);

private:
	void Init();
	int CreateDSCData();

	int m_FskState;
	int m_MsgDelay;
	int m_MsgTimer;
	int m_BitPos;
	int m_MsgPos;
	quint16 m_TxShiftReg;
	TYPEREAL m_SampleRate;
	TYPEREAL m_MarkInc;
	TYPEREAL m_SpaceInc;
	TYPEREAL m_FskAcc;
	TYPEREAL m_SymbTime;
	TYPEREAL m_SymbInc;
	TYPEREAL m_SymbPeriod;

};

#endif // FSKMOD_H
