// fft.cpp: implementation of the CFft class.
//  This is a somewhat modified version of Takuya OOURA's
//     original radix 4 FFT package.
//Copyright(C) 1996-1998 Takuya OOURA
//    (email: ooura@mmm.t.u-tokyo.ac.jp).
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#include <math.h>
#include "fft.h"

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////

#define K_AMPMAX 32767.0	//maximum sin wave Pk for 16 bit input data
#define K_MAXDB 0.0			//specifies total range of FFT
#define K_MINDB -220.0

#define OVER_LIMIT 32000.0	//limit for detecting over ranging inputs


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CFft::CFft()
{
	m_Overload = false;
	m_Invert = false;
	m_AveSize = 1;
	m_LastFFTSize = 0;
	m_AveCount = 0;
	m_TotalCount = 0;
	m_FFTSize = 1024;
	m_pWorkArea = NULL;
	m_pSinCosTbl = NULL;
	m_pWindowTbl = NULL;
	m_pFFTPwrAveBuf = NULL;
	m_pFFTAveBuf = NULL;
	m_pFFTInBuf = NULL;
	m_pFFTSumBuf = NULL;
	m_pTranslateTbl = NULL;
	m_dBCompensation = K_MAXDB;
	SetFFTParams( 2048, false ,0.0, 1000);
	SetFFTAve( 1);
}

CFft::~CFft()
{							// free all resources
	FreeMemory();
}

void CFft::FreeMemory()
{
	if(m_pWorkArea)
	{
		delete m_pWorkArea;
		m_pWorkArea = NULL;
	}
	if(m_pSinCosTbl)
	{
		delete m_pSinCosTbl;
		m_pSinCosTbl = NULL;
	}
	if(m_pWindowTbl)
	{
		delete m_pWindowTbl;
		m_pWindowTbl = NULL;
	}
	if(m_pFFTPwrAveBuf)
	{
		delete m_pFFTPwrAveBuf;
		m_pFFTPwrAveBuf = NULL;
	}
	if(m_pFFTAveBuf)
	{
		delete m_pFFTAveBuf;
		m_pFFTAveBuf = NULL;
	}
	if(m_pFFTSumBuf)
	{
		delete m_pFFTSumBuf;
		m_pFFTSumBuf = NULL;
	}
	if(m_pFFTInBuf)
	{
		delete m_pFFTInBuf;
		m_pFFTInBuf = NULL;
	}
	if(m_pTranslateTbl)
	{
		delete m_pTranslateTbl;
		m_pTranslateTbl = NULL;
	}
}

///////////////////////////////////////////////////////////////////
//FFT initialization and parameter setup function
///////////////////////////////////////////////////////////////////
void CFft::SetFFTAve( qint32 ave)
{
	if(m_AveSize != ave)
	{
		if(ave>0)
			m_AveSize = ave;
		else
			m_AveSize = 1;
	}
	ResetFFT();
}

///////////////////////////////////////////////////////////////////
//FFT initialization and parameter setup function
///////////////////////////////////////////////////////////////////
void CFft::SetFFTParams( qint32 size,
						 bool invert,
						 TYPEREAL dBCompensation,
						 TYPEREAL SampleFreq)
{
qint32 i;
	if(size==0)
		return;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	m_BinMin = 0;		//force recalculation of plot variables
	m_BinMax = 0;
	m_StartFreq = 0;
	m_StopFreq = 0;
	m_PlotWidth = 0;
	m_Invert = invert;
	m_SampleFreq = SampleFreq;
	if( m_dBCompensation != dBCompensation )
	{							//reset of FFT params
		m_LastFFTSize = 0;
		m_dBCompensation = dBCompensation;
	}

	if( size<MIN_FFT_SIZE )
		m_FFTSize = MIN_FFT_SIZE;
	else if( size>MAX_FFT_SIZE )
		m_FFTSize = MAX_FFT_SIZE;
	else
		m_FFTSize = size;

	if(m_LastFFTSize != m_FFTSize)
	{
		m_LastFFTSize = m_FFTSize;
		FreeMemory();
		m_pWindowTbl = new TYPEREAL[m_FFTSize];
		m_pSinCosTbl = new TYPEREAL[m_FFTSize/2];
		m_pWorkArea = new qint32[ (qint32)MSQRT((TYPEREAL)m_FFTSize)+2];
		m_pFFTPwrAveBuf = new TYPEREAL[m_FFTSize];
		m_pFFTAveBuf = new TYPEREAL[m_FFTSize];
		m_pFFTSumBuf = new TYPEREAL[m_FFTSize];
		for(i=0; i<m_FFTSize; i++)
		{
			m_pFFTPwrAveBuf[i] = 0.0;
			m_pFFTAveBuf[i] = 0.0;
			m_pFFTSumBuf[i] = 0.0;
		}
		m_pWorkArea[0] = 0;
		m_pFFTInBuf = new TYPEREAL[m_FFTSize*2];
		m_pTranslateTbl = new qint32[m_FFTSize];
		for(i=0; i<m_FFTSize*2; i++)
			m_pFFTInBuf[i] = 0.0;
		makewt(m_FFTSize/2, m_pWorkArea, m_pSinCosTbl);

//////////////////////////////////////////////////////////////////////
// A pure input sin wave ... Asin(wt)... will produce an fft output 
//   peak of (N*A/1)^2  where N is FFT_SIZE.
//		Kx = 2 for complex, 4 for real FFT
// To convert to a Power dB range:
//   PdBmax = 10*log10( (N*A/Kx)^2 + K_C ) + K_B
//   PdBmin = 10*log10( 0 + K_C ) + K_B
//  if (N*A/Kx)^2 >> K_C 
//  Then K_B = PdBmax - 20*log10( N*A/Kx )
//       K_C = 10 ^ ( (PdBmin-K_B)/10 )
//  for power range of 0 to 100 dB with input(A) of 32767 and N=262144
//			K_B = -86.63833  and K_C = 4.6114145e8
// To eliminate the multiply by 10, divide by 10 so for an output
//		range of 0 to -120dB the stored value is 0.0 to -12.0
//   so final constant K_B = -8.663833		
///////////////////////////////////////////////////////////////////////
		m_K_B = m_dBCompensation - 20*MLOG10( (TYPEREAL)m_FFTSize*K_AMPMAX/2.0 );
		m_K_C = MPOW( 10.0, (K_MINDB-m_K_B)/10.0 );
		m_K_B = m_K_B/10.0;
TYPEREAL WindowGain;
#if 0
		WindowGain = 1.0;
		for(i=0; i<m_FFTSize; i++)	//Rectangle(no window)
			m_pWindowTbl[i] = 1.0*WindowGain;
#endif
#if 0
		WindowGain = 2.0;
		for(i=0; i<m_FFTSize; i++)	//Hann
			m_pWindowTbl[i] = WindowGain*(.5  - .5 *MCOS( (K_2PI*i)/(m_FFTSize-1) ));
#endif
#if 0
		WindowGain = 1.852;
		for(i=0; i<m_FFTSize; i++)	//Hamming
			m_pWindowTbl[i] = WindowGain*(.54  - .46 *MCOS( (K_2PI*i)/(m_FFTSize-1) ));
#endif
#if 0
		WindowGain = 2.8;
		for(i=0; i<m_FFTSize; i++)	//Blackman-Nuttall
			m_pWindowTbl[i] = WindowGain*(0.3635819
				- 0.4891775*MCOS( (K_2PI*i)/(m_FFTSize-1) )
				+ 0.1365995*MCOS( (2.0*K_2PI*i)/(m_FFTSize-1) )
				- 0.0106411*MCOS( (3.0*K_2PI*i)/(m_FFTSize-1) ) );
#endif
#if 0
		WindowGain = 2.82;
		for(i=0; i<m_FFTSize; i++)	//Blackman-Harris
			m_pWindowTbl[i] = WindowGain*(0.35875
				- 0.48829*MCOS( (K_2PI*i)/(m_FFTSize-1) )
				+ 0.14128*MCOS( (2.0*K_2PI*i)/(m_FFTSize-1) )
				- 0.01168*MCOS( (3.0*K_2PI*i)/(m_FFTSize-1) ) );
#endif
#if 1
		WindowGain = 2.8;
		for(i=0; i<m_FFTSize; i++)	//Nuttall
			m_pWindowTbl[i] = WindowGain*(0.355768
				- 0.487396*MCOS( (K_2PI*i)/(m_FFTSize-1) )
				+ 0.144232*MCOS( (2.0*K_2PI*i)/(m_FFTSize-1) )
				- 0.012604*MCOS( (3.0*K_2PI*i)/(m_FFTSize-1) ) );
#endif
#if 0
		WindowGain = 1.0;
		for(i=0; i<m_FFTSize; i++)	//Flat Top 4 term
			m_pWindowTbl[i] = WindowGain*(1.0
					- 1.942604  * MCOS( (K_2PI*i)/(m_FFTSize-1) )
					+ 1.340318 * MCOS( (2.0*K_2PI*i)/(m_FFTSize-1) )
					- 0.440811 * MCOS( (3.0*K_2PI*i)/(m_FFTSize-1) )
					+ 0.043097  * MCOS( (4.0*K_2PI*i)/(m_FFTSize-1) )
				);

#endif
	}

#ifdef FMDSP_THREAD_SAFE
	lock.unlock();
#endif

	ResetFFT();
}

///////////////////////////////////////////////////////////////////
//  Resets the FFT buffers and averaging variables.
///////////////////////////////////////////////////////////////////
void CFft::ResetFFT()
{
#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	for(qint32 i=0; i<m_FFTSize;i++)
	{
		m_pFFTAveBuf[i] = 0.0;
		m_pFFTSumBuf[i] = 0.0;
	}
	m_AveCount = 0;
	m_TotalCount = 0;
}

//////////////////////////////////////////////////////////////////////
// "InBuf[]" is first multiplied by a window function, checked for overflow
//	and then placed in the FFT input buffers and the FFT performed.
//	For real data there should be  m_FFTSize/2 InBuf data points
//	For complex data there should be  m_FFTSize InBuf data points
//////////////////////////////////////////////////////////////////////
qint32 CFft::PutInDisplayFFT(qint32 n, TYPECPX* InBuf)
{
qint32 i;
	m_Overload = false;

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	TYPEREAL dtmp1;
	for(i=0; i<n; i++)
	{
		// Determine overload
		m_Overload = ((InBuf[i].im > OVER_LIMIT) || (InBuf[i].re > OVER_LIMIT));

		dtmp1 = m_pWindowTbl[i];
		//NOTE: For some reason I and Q are swapped(demod I/Q does not apear to be swapped)
		//possibly an issue with the FFT ?
		((TYPECPX*)m_pFFTInBuf)[i].im = dtmp1 * (InBuf[i].re);//window the I data
		((TYPECPX*)m_pFFTInBuf)[i].re = dtmp1 * (InBuf[i].im);	//window the Q data
	}
	//Calculate the complex FFT
	bitrv2(m_FFTSize*2, m_pWorkArea + 2, m_pFFTInBuf);
	CpxFFT(m_FFTSize*2, m_pFFTInBuf, m_pSinCosTbl);

	return m_TotalCount;
}

//////////////////////////////////////////////////////////////////////
// The bin range is "start" to "stop" Hz.
// The range of start to stop frequencies are mapped to the users
// plot screen size so the users buffer will be filled with an array
// of integers whos value is the pixel height and the index of the
//  array is the x pixel coordinate.
// The function returns true if the input is overloaded
//   This routine converts the data to 32 bit integers and is useful
//   when displaying fft data on the screen. 
//		MaxHeight = Plot height in pixels(zero is top and increases down)
//		MaxWidth =  Plot width in pixels
//		StartFreq = freq in Hz
//		StopFreq = freq in Hz
//		MaxdB = FFT dB level corresponding to output value == 0
//			must be <= to K_MAXDB
//		MindB = FFT dB level  corresponding to output value == MaxHeight
//			must be >= to K_MINDB
//////////////////////////////////////////////////////////////////////
bool CFft::GetScreenIntegerFFTData(qint32 MaxHeight,
								qint32 MaxWidth,
								TYPEREAL MaxdB,
								TYPEREAL MindB,
								qint32 StartFreq,
								qint32 StopFreq,
								qint32* OutBuf )
{
qint32 i;
qint32 y;
qint32 x;
qint32 m;
qint32 ymax = -10000;
qint32 xprev = -1;
qint32 maxbin;
TYPEREAL dBmaxOffset = MaxdB/10.0;
TYPEREAL dBGainFactor = -10.0/(MaxdB-MindB);

#ifdef FMDSP_THREAD_SAFE
	std::unique_lock<std::mutex> lock(m_Mutex);
#endif

	if( (m_StartFreq != StartFreq) ||
		(m_StopFreq != StopFreq) ||
		(m_PlotWidth != MaxWidth) )
	{	//if something has changed need to redo translate table
		m_StartFreq = StartFreq;
		m_StopFreq = StopFreq;
		m_PlotWidth = MaxWidth;
		maxbin = m_FFTSize - 1;
		m_BinMin = (qint32)((TYPEREAL)StartFreq*(TYPEREAL)m_FFTSize/m_SampleFreq);
		m_BinMin += (m_FFTSize/2);
		m_BinMax = (qint32)((TYPEREAL)StopFreq*(TYPEREAL)m_FFTSize/m_SampleFreq);
		m_BinMax += (m_FFTSize/2);
		if(m_BinMin < 0)	//don't allow these go outside the translate table
			m_BinMin = 0;
		if(m_BinMin >= maxbin)
			m_BinMin = maxbin;
		if(m_BinMax < 0)
			m_BinMax = 0;
		if(m_BinMax >= maxbin)
			m_BinMax = maxbin;
		if( (m_BinMax-m_BinMin) > m_PlotWidth )
		{
			//if more FFT points than plot points
			for( i=m_BinMin; i<=m_BinMax; i++)
				m_pTranslateTbl[i] = ( (i-m_BinMin)*m_PlotWidth )/(m_BinMax - m_BinMin);
		}
		else
		{
			//if more plot points than FFT points
			for( i=0; i<m_PlotWidth; i++)
				m_pTranslateTbl[i] = m_BinMin + ( i*(m_BinMax - m_BinMin) )/m_PlotWidth;
		}
	}

	m = (m_FFTSize);
	if( (m_BinMax-m_BinMin) > m_PlotWidth )
	{
		//if more FFT points than plot points
		for( i=m_BinMin; i<=m_BinMax; i++ )
		{
			if(m_Invert)
				y = (qint32)((TYPEREAL)MaxHeight*dBGainFactor*(m_pFFTAveBuf[(m-i)] - dBmaxOffset));
			else
				y = (qint32)((TYPEREAL)MaxHeight*dBGainFactor*(m_pFFTAveBuf[i] - dBmaxOffset));
			if(y<0)
				y = 0;
			if(y > MaxHeight)
				y = MaxHeight;
			x = m_pTranslateTbl[i];	//get fft bin to plot x coordinate transform
			if( x==xprev )	// still mappped to same fft bin coordinate
			{
				if(y < ymax)		//store only the max value
				{
					OutBuf[x] = y;
					ymax = y;
				}
			}
			else
			{
				OutBuf[x] = y;
				xprev = x;
				ymax = y;
			}
		}
	}
	else
	{
		//if more plot points than FFT points
		for( x=0; x<m_PlotWidth; x++ )
		{
			i = m_pTranslateTbl[x];	//get plot to fft bin coordinate transform
			if(m_Invert)
				y = (qint32)((TYPEREAL)MaxHeight*dBGainFactor*(m_pFFTAveBuf[(m-i)] - dBmaxOffset));
			else
				y = (qint32)((TYPEREAL)MaxHeight*dBGainFactor*(m_pFFTAveBuf[i] - dBmaxOffset));
			if(y<0)
				y = 0;
			if(y > MaxHeight)
				y = MaxHeight;
			OutBuf[x] = y;
		}
	}

	return m_Overload;
}

///////////////////////////////////////////////////////////////////
//Interface for doing fast convolution filters.  Takes complex data
// in pInOutBuf and does fwd or rev FFT and places back in same buffer.
///////////////////////////////////////////////////////////////////
void CFft::FwdFFT( TYPECPX* pInOutBuf)
{
	bitrv2(m_FFTSize*2, m_pWorkArea + 2, (TYPEREAL*)pInOutBuf);
	CpxFFT(m_FFTSize*2, (TYPEREAL*)pInOutBuf, m_pSinCosTbl);
}

void CFft::RevFFT( TYPECPX* pInOutBuf)
{
	bitrv2conj(m_FFTSize*2, m_pWorkArea + 2, (TYPEREAL*)pInOutBuf);
	cftbsub(m_FFTSize*2, (TYPEREAL*)pInOutBuf, m_pSinCosTbl);
}


///////////////////////////////////////////////////////////////////
// Nitty gritty fft routines by Takuya OOURA(Updated to his new version 4-18-02)
// Routine calculates real FFT
///////////////////////////////////////////////////////////////////
void CFft::rftfsub(qint32 n, TYPEREAL *a, qint32 nc, TYPEREAL *c)
{
qint32 j, k, kk, ks, m;
TYPEREAL wkr, wki, xr, xi, yr, yi;

	m_TotalCount++;
 	if(m_AveCount < m_AveSize)
		m_AveCount++;
    m = n >> 1;
    ks = 2 * nc/m;
    kk = 0;
	for (j = 2; j < m; j += 2 ) 
	{
		k = n - j;
		kk += ks;
		wkr = 0.5 - c[nc - kk];
		wki = c[kk];
		xr = a[j] - a[k];
		xi = a[j + 1] + a[k + 1];
		yr = wkr * xr - wki * xi;
		yi = wkr * xi + wki * xr;
		a[j] -= yr;
		xi = a[j]*a[j];
		a[j+1] -= yi;
		xi += ( a[j+1]*a[j+1]);
		a[k] += yr;
		xr = a[k]*a[k];
		a[k+1] -= yi;
		xr += (a[k+1]*a[k+1]);

		//xr is real power  xi is imag power terms
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
		{
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + xi;
			m_pFFTSumBuf[k] = m_pFFTSumBuf[k] + xr;
		}
		else
		{
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + xi;
			m_pFFTSumBuf[k] = m_pFFTSumBuf[k] - m_pFFTPwrAveBuf[k] + xr;
		}
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(TYPEREAL)m_AveCount;
		m_pFFTPwrAveBuf[k] = m_pFFTSumBuf[k]/(TYPEREAL)m_AveCount;

		m_pFFTAveBuf[j] = MLOG10(m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
		m_pFFTAveBuf[k] = MLOG10(m_pFFTPwrAveBuf[k] + m_K_C) + m_K_B;

	}

	a[0] *= a[0];						//calc DC term
	xr = a[m]*a[m]+a[m+1]*a[m+1];		//calculate N/4(middle) term

	//xr is real power  a[0] is imag power terms
	//perform moving average on power up to m_AveSize then do exponential averaging after that
	if(m_TotalCount <= m_AveSize)
	{
		m_pFFTSumBuf[0] = m_pFFTSumBuf[0] + a[0];
		m_pFFTSumBuf[n/2] = m_pFFTSumBuf[n/2] + xr;
	}
	else
	{
		m_pFFTSumBuf[0] = m_pFFTSumBuf[0] - m_pFFTPwrAveBuf[0] + a[0];
		m_pFFTSumBuf[n/2] = m_pFFTSumBuf[n/2] - m_pFFTPwrAveBuf[n/2] + xr;
	}
	m_pFFTPwrAveBuf[0] = m_pFFTSumBuf[0]/(TYPEREAL)m_AveCount;
	m_pFFTPwrAveBuf[n/2] = m_pFFTSumBuf[n/2]/(TYPEREAL)m_AveCount;

	m_pFFTAveBuf[0] = MLOG10(m_pFFTPwrAveBuf[0] + m_K_C) + m_K_B;
	m_pFFTAveBuf[n/2] = MLOG10(m_pFFTPwrAveBuf[n/2] + m_K_C) + m_K_B;

}


///////////////////////////////////////////////////////////////////
// Routine calculates complex FFT
///////////////////////////////////////////////////////////////////
void CFft::CpxFFT(qint32 n, TYPEREAL *a, TYPEREAL *w)
{
qint32 j, j1, j2, j3, l;
TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
	m_TotalCount++;
 	if(m_AveCount < m_AveSize)
		m_AveCount++;
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
	//n = 2*FFTSIZE 
	n = n>>1;
	//now n = FFTSIZE

	// FFT output index 0 to N/2-1
	// is frequency output 0 to +Fs/2 Hz  ( 0 Hz DC term ) 
	for( l=0,j=n/2; j<n; l+=2,j++)
	{
		x0r = (a[l]*a[l]) + (a[l+1]*a[l+1]);
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + x0r;
		else
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + x0r;
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(TYPEREAL)m_AveCount;
		m_pFFTAveBuf[j] = MLOG10( m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
	}
	// FFT output index N/2 to N-1  (times 2 since complex samples)
	// is frequency output -Fs/2 to 0  
	for( l=n,j=0; j<n/2; l+=2,j++)
	{
		x0r = (a[l]*a[l]) + (a[l+1]*a[l+1]);
		//perform moving average on power up to m_AveSize then do exponential averaging after that
		if(m_TotalCount <= m_AveSize)
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] + x0r;
		else
			m_pFFTSumBuf[j] = m_pFFTSumBuf[j] - m_pFFTPwrAveBuf[j] + x0r;
		m_pFFTPwrAveBuf[j] = m_pFFTSumBuf[j]/(TYPEREAL)m_AveCount;
		m_pFFTAveBuf[j] = MLOG10( m_pFFTPwrAveBuf[j] + m_K_C) + m_K_B;
	}

}

///////////////////////////////////////////////////////////////////
/* -------- initializing routines -------- */
///////////////////////////////////////////////////////////////////
void CFft::makewt(qint32 nw, qint32 *ip, TYPEREAL *w)
{
qint32 j, nwh;
TYPEREAL delta, x, y;
    
    ip[0] = nw;
    ip[1] = 1;
    if (nw > 2) {
        nwh = nw >> 1;
		delta = MATAN(1.0) / nwh;
        w[0] = 1;
        w[1] = 0;
		w[nwh] = MCOS(delta * nwh);
        w[nwh + 1] = w[nwh];
        if (nwh > 2) {
            for (j = 2; j < nwh; j += 2) {
				x = MCOS(delta * j);
				y = MSIN(delta * j);
                w[j] = x;
                w[j + 1] = y;
                w[nw - j] = y;
                w[nw - j + 1] = x;
            }
            bitrv2(nw, ip + 2, w);
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::makect(qint32 nc, qint32 *ip, TYPEREAL *c)
{
qint32 j, nch;
TYPEREAL delta;
    
	ip[1] = nc;
    if (nc > 1) {
        nch = nc >> 1;
		delta = MATAN(1.0) / nch;
		c[0] = MCOS(delta * nch);
        c[nch] = 0.5 * c[0];
        for (j = 1; j < nch; j++) {
			c[j] = 0.5 * MCOS(delta * j);
			c[nc - j] = 0.5 * MSIN(delta * j);
        }
    }
}

///////////////////////////////////////////////////////////////////
/* -------- child routines -------- */
///////////////////////////////////////////////////////////////////
void CFft::bitrv2(qint32 n, qint32 *ip, TYPEREAL *a)
{
qint32 j, j1, k, k1, l, m, m2;
TYPEREAL xr, xi, yr, yi;
    
    ip[0] = 0;
    l = n;
    m = 1;
    while ((m << 3) < l) {
        l >>= 1;
        for (j = 0; j < m; j++) {
            ip[m + j] = ip[j] + l;
        }
        m <<= 1;
    }
    m2 = 2 * m;
    if ((m << 3) == l) {
        for (k = 0; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 -= m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            j1 = 2 * k + m2 + ip[k];
            k1 = j1 + m2;
            xr = a[j1];
            xi = a[j1 + 1];
            yr = a[k1];
            yi = a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
        }
    } else {
        for (k = 1; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cftfsub(qint32 n, TYPEREAL *a, TYPEREAL *w)
{
qint32 j, j1, j2, j3, l;
TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cft1st(qint32 n, TYPEREAL *a, TYPEREAL *w)
{
qint32 j, k1, k2;
TYPEREAL wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    x0r = a[0] + a[2];
    x0i = a[1] + a[3];
    x1r = a[0] - a[2];
    x1i = a[1] - a[3];
    x2r = a[4] + a[6];
    x2i = a[5] + a[7];
    x3r = a[4] - a[6];
    x3i = a[5] - a[7];
    a[0] = x0r + x2r;
    a[1] = x0i + x2i;
    a[4] = x0r - x2r;
    a[5] = x0i - x2i;
    a[2] = x1r - x3i;
    a[3] = x1i + x3r;
    a[6] = x1r + x3i;
    a[7] = x1i - x3r;
    wk1r = w[2];
    x0r = a[8] + a[10];
    x0i = a[9] + a[11];
    x1r = a[8] - a[10];
    x1i = a[9] - a[11];
    x2r = a[12] + a[14];
    x2i = a[13] + a[15];
    x3r = a[12] - a[14];
    x3i = a[13] - a[15];
    a[8] = x0r + x2r;
    a[9] = x0i + x2i;
    a[12] = x2i - x0i;
    a[13] = x0r - x2r;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[10] = wk1r * (x0r - x0i);
    a[11] = wk1r * (x0r + x0i);
    x0r = x3i + x1r;
    x0i = x3r - x1i;
    a[14] = wk1r * (x0i - x0r);
    a[15] = wk1r * (x0i + x0r);
    k1 = 0;
    for (j = 16; j < n; j += 16) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        x0r = a[j] + a[j + 2];
        x0i = a[j + 1] + a[j + 3];
        x1r = a[j] - a[j + 2];
        x1i = a[j + 1] - a[j + 3];
        x2r = a[j + 4] + a[j + 6];
        x2i = a[j + 5] + a[j + 7];
        x3r = a[j + 4] - a[j + 6];
        x3i = a[j + 5] - a[j + 7];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 4] = wk2r * x0r - wk2i * x0i;
        a[j + 5] = wk2r * x0i + wk2i * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 2] = wk1r * x0r - wk1i * x0i;
        a[j + 3] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 6] = wk3r * x0r - wk3i * x0i;
        a[j + 7] = wk3r * x0i + wk3i * x0r;
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        x0r = a[j + 8] + a[j + 10];
        x0i = a[j + 9] + a[j + 11];
        x1r = a[j + 8] - a[j + 10];
        x1i = a[j + 9] - a[j + 11];
        x2r = a[j + 12] + a[j + 14];
        x2i = a[j + 13] + a[j + 15];
        x3r = a[j + 12] - a[j + 14];
        x3i = a[j + 13] - a[j + 15];
        a[j + 8] = x0r + x2r;
        a[j + 9] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 12] = -wk2i * x0r - wk2r * x0i;
        a[j + 13] = -wk2i * x0i + wk2r * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 10] = wk1r * x0r - wk1i * x0i;
        a[j + 11] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 14] = wk3r * x0r - wk3i * x0i;
        a[j + 15] = wk3r * x0i + wk3i * x0r;
    }
}

///////////////////////////////////////////////////////////////////
void CFft::cftmdl(qint32 n, qint32 l, TYPEREAL *a, TYPEREAL *w)
{
qint32 j, j1, j2, j3, k, k1, k2, m, m2;
TYPEREAL wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    m = l << 2;
    for (j = 0; j < l; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x0r - x2r;
        a[j2 + 1] = x0i - x2i;
        a[j1] = x1r - x3i;
        a[j1 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
    }
    wk1r = w[2];
    for (j = m; j < l + m; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x2i - x0i;
        a[j2 + 1] = x0r - x2r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j1] = wk1r * (x0r - x0i);
        a[j1 + 1] = wk1r * (x0r + x0i);
        x0r = x3i + x1r;
        x0i = x3r - x1i;
        a[j3] = wk1r * (x0i - x0r);
        a[j3 + 1] = wk1r * (x0i + x0r);
    }
    k1 = 0;
    m2 = 2 * m;
    for (k = m2; k < n; k += m2) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        for (j = k; j < l + k; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = wk2r * x0r - wk2i * x0i;
            a[j2 + 1] = wk2r * x0i + wk2i * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        for (j = k + m; j < l + (k + m); j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = -wk2i * x0r - wk2r * x0i;
            a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
    }
}

void CFft::bitrv2conj(int n, int *ip, TYPEREAL *a)
{
	int j, j1, k, k1, l, m, m2;
	TYPEREAL xr, xi, yr, yi;

	ip[0] = 0;
	l = n;
	m = 1;
	while ((m << 3) < l) {
		l >>= 1;
		for (j = 0; j < m; j++) {
			ip[m + j] = ip[j] + l;
		}
		m <<= 1;
	}
	m2 = 2 * m;
	if ((m << 3) == l) {
		for (k = 0; k < m; k++) {
			for (j = 0; j < k; j++) {
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += 2 * m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 -= m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += 2 * m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
			}
			k1 = 2 * k + ip[k];
			a[k1 + 1] = -a[k1 + 1];
			j1 = k1 + m2;
			k1 = j1 + m2;
			xr = a[j1];
			xi = -a[j1 + 1];
			yr = a[k1];
			yi = -a[k1 + 1];
			a[j1] = yr;
			a[j1 + 1] = yi;
			a[k1] = xr;
			a[k1 + 1] = xi;
			k1 += m2;
			a[k1 + 1] = -a[k1 + 1];
		}
	} else {
		a[1] = -a[1];
		a[m2 + 1] = -a[m2 + 1];
		for (k = 1; k < m; k++) {
			for (j = 0; j < k; j++) {
				j1 = 2 * j + ip[k];
				k1 = 2 * k + ip[j];
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
				j1 += m2;
				k1 += m2;
				xr = a[j1];
				xi = -a[j1 + 1];
				yr = a[k1];
				yi = -a[k1 + 1];
				a[j1] = yr;
				a[j1 + 1] = yi;
				a[k1] = xr;
				a[k1 + 1] = xi;
			}
			k1 = 2 * k + ip[k];
			a[k1 + 1] = -a[k1 + 1];
			a[k1 + m2 + 1] = -a[k1 + m2 + 1];
		}
	}
}

void CFft::cftbsub(int n, TYPEREAL *a, TYPEREAL *w)
{
	int j, j1, j2, j3, l;
	TYPEREAL x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

	l = 2;
	if (n > 8) {
		cft1st(n, a, w);
		l = 8;
		while ((l << 2) < n) {
			cftmdl(n, l, a, w);
			l <<= 2;
		}
	}
	if ((l << 2) == n) {
		for (j = 0; j < l; j += 2) {
			j1 = j + l;
			j2 = j1 + l;
			j3 = j2 + l;
			x0r = a[j] + a[j1];
			x0i = -a[j + 1] - a[j1 + 1];
			x1r = a[j] - a[j1];
			x1i = -a[j + 1] + a[j1 + 1];
			x2r = a[j2] + a[j3];
			x2i = a[j2 + 1] + a[j3 + 1];
			x3r = a[j2] - a[j3];
			x3i = a[j2 + 1] - a[j3 + 1];
			a[j] = x0r + x2r;
			a[j + 1] = x0i - x2i;
			a[j2] = x0r - x2r;
			a[j2 + 1] = x0i + x2i;
			a[j1] = x1r - x3i;
			a[j1 + 1] = x1i - x3r;
			a[j3] = x1r + x3i;
			a[j3 + 1] = x1i + x3r;
		}
	} else {
		for (j = 0; j < l; j += 2) {
			j1 = j + l;
			x0r = a[j] - a[j1];
			x0i = -a[j + 1] + a[j1 + 1];
			a[j] += a[j1];
			a[j + 1] = -a[j + 1] - a[j1 + 1];
			a[j1] = x0r;
			a[j1 + 1] = x0i;
		}
	}
}


