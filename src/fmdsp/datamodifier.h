#ifndef DATAMODIFIER_H
#define DATAMODIFIER_H

#include "datatypes.h"

class CDataModifier
{
public:
	CDataModifier();
	void Init(TYPEREAL SampleRate);
	void ProcessBlock(TYPECPX* pBuf, int NumSamples);

	void SetSweepStart(int start);
	int GetSweepStart(void){return static_cast<int>(m_SweepStartFrequency);}
	void SetSweepStop(int stop);
	int GetSweepStop(void){return static_cast<int>(m_SweepStopFrequency);}
	void SetSweepRate( TYPEREAL rate);
	int GetSweepRate(void){return static_cast<int>(m_SweepRate);}
	void SetSignalPower( TYPEREAL dB);
	TYPEREAL GetSignalPower(void){return m_SignalPower;}
	void SetNoisePower( TYPEREAL dB);
	TYPEREAL GetNoisePower(void){return m_NoisePower;}

private:
	TYPEREAL m_SweepFrequency;
	TYPEREAL m_SweepFreqNorm;
	TYPEREAL m_SweepAcc;
	TYPEREAL m_SweepRateInc;
	TYPEREAL m_SampleRate;
	TYPEREAL m_SweepStartFrequency;
	TYPEREAL m_SweepStopFrequency;
	TYPEREAL m_SweepRate;
	TYPEREAL m_SignalAmplitude;
	TYPEREAL m_NoiseAmplitude;
	TYPEREAL m_SignalPower;
	TYPEREAL m_NoisePower;

	bool m_SweepDirUp;
};

#endif // DATAMODIFIER_H
