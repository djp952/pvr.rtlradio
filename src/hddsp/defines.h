#pragma once

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// Sample rate before decimation
#define SAMPLE_RATE 1488375
// FFT length in samples
#define FFT_FM 2048
#define FFT_AM 256
// cyclic preflex length in samples
#define CP_FM 112
#define CP_AM 14
#define FFTCP_FM (FFT_FM + CP_FM)
#define FFTCP_AM (FFT_AM + CP_AM)
// OFDM symbols per L1 block
#define BLKSZ 32
// symbols processed by each invocation of acquire_process
#define ACQUIRE_SYMBOLS (BLKSZ * 2)
// index of first lower sideband subcarrier
#define LB_START ((FFT_FM / 2) - 546)
// index of last upper sideband subcarrier
#define UB_END ((FFT_FM / 2) + 546)
// index of AM carrier
#define CENTER_AM (FFT_AM / 2)
// indexes of AM subcarriers
#define REF_INDEX_AM 1
#define PIDS_1_INDEX_AM 27
#define PIDS_2_INDEX_AM 53
#define TERTIARY_INDEX_AM 2
#define SECONDARY_INDEX_AM 28
#define PRIMARY_INDEX_AM 57
#define MAX_INDEX_AM 81
// bits per P1 frame
#define P1_FRAME_LEN_FM 146176
#define P1_FRAME_LEN_AM 3750
// bits per encoded P1 frame
#define P1_FRAME_LEN_ENCODED_FM (P1_FRAME_LEN_FM * 5 / 2)
#define P1_FRAME_LEN_ENCODED_AM (P1_FRAME_LEN_AM * 12 / 5)
// bits per PIDS frame
#define PIDS_FRAME_LEN 80
// bits per encoded PIDS frame
#define PIDS_FRAME_LEN_ENCODED_FM (PIDS_FRAME_LEN * 5 / 2)
#define PIDS_FRAME_LEN_ENCODED_AM (PIDS_FRAME_LEN * 3)
// bits per P3 frame
#define P3_FRAME_LEN_FM 4608
#define P3_FRAME_LEN_AM 24000
// bits per encoded P3 frame
#define P3_FRAME_LEN_ENCODED_FM (P3_FRAME_LEN_FM * 2)
#define P3_FRAME_LEN_ENCODED_AM (P3_FRAME_LEN_AM * 3 / 2)
// bits per L2 PCI
#define PCI_LEN 24
// bytes per L2 PDU (max)
#define MAX_PDU_LEN ((P1_FRAME_LEN_FM - PCI_LEN) / 8)
// bytes per L2 PDU in P1 frame (AM)
#define P1_PDU_LEN_AM 466
// number of programs (max)
#define MAX_PROGRAMS 8
// number of streams per program (max)
#define MAX_STREAMS 4
// number of subcarriers per AM partition
#define PARTITION_WIDTH_AM 25

#define log_debug(...) \
            do { if (LIBRARY_DEBUG_LEVEL <= 1) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)
#define log_info(...) \
            do { if (LIBRARY_DEBUG_LEVEL <= 2) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)
#define log_warn(...) \
            do { if (LIBRARY_DEBUG_LEVEL <= 3) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)
#define log_error(...) \
            do { if (LIBRARY_DEBUG_LEVEL <= 4) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define U8_F(x) ( (((float)(x)) - 127) / 128 )
#define U8_Q15(x) ( ((int16_t)(x) - 127) * 64 )

#ifdef _MSC_VER
typedef _Fcomplex fcomplex_t;
#else
typedef float complex fcomplex_t;
#endif

#ifndef HAVE_CMPLXF
#ifdef _MSC_VER
#define CMPLXF(x,y) _FCbuild(x,y)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#define CMPLXF(x,y) __builtin_complex((float)(x), (float)(y))
#elif defined(HAVE_IMAGINARY_I)
#define CMPLXF(x,y) ((fcomplex_t)((float)(x) + _Imaginary_I * (float)(y)))
#elif defined(HAVE_COMPLEX_I)
#define CMPLXF(x,y) ((fcomplex_t)((float)(x) + _Complex_I * (float)(y)))
#endif
#endif

#ifdef _MSC_VER
#define CMPLXFSET(x) _FCbuild(x,0)
#else
#define CMPLXFSET(x) x
#endif

#ifdef _MSC_VER
#define CMPLXFADD(x,y) _FCbuild(x._Val[0] + y._Val[0], x._Val[1] + y._Val[1])
#else
#define CMPLXFADD(x,y) x + y
#endif

#ifdef _MSC_VER
#define CMPLXFSUB(x,y) _FCbuild(x._Val[0] - y._Val[0], x._Val[1] - y._Val[1])
#else
#define CMPLXFSUB(x,y) x - y
#endif

#ifdef _MSC_VER
#define CMPLXFNEG(x) _FCbuild(-x._Val[0], -x._Val[1])
#else
#define CMPLXFNEG(x) -x
#endif

#ifdef _MSC_VER
#define CMPLXFMUL(x,y) _FCmulcc(x,y)
#else
#define CMPLXFMUL(x,y) x * y
#endif

#ifdef _MSC_VER
#define CMPLXFMULF(x,y) _FCmulcr(x,y)
#else
#define CMPLXFMULF(x,y) x * y
#endif

#ifdef _MSC_VER
static _Fcomplex _FCdivcc(_Fcomplex lhs, _Fcomplex rhs)
{
	float re = lhs._Val[0];
	float im = lhs._Val[1];
	const float rhs_re = rhs._Val[0];
	const float rhs_im = rhs._Val[1];

	//
	// Logic taken from C++ standard library std::complex<> division operation
	// TODO: clean up
	//

	if(isnan(rhs_re) || isnan(rhs_im)) { // set NaN result
		re = NAN;
		im = re;
	}
	else if((rhs_im < 0 ? -rhs_im : +rhs_im)
		< (rhs_re < 0 ? -rhs_re : +rhs_re)) { // |_Right.imag()| < |_Right.real()|
		float wr = rhs_im / rhs_re;
		float wd = rhs_re + wr * rhs_im;

		if(isnan(wd) || wd == 0) { // set NaN result
			re = NAN;
			im = re;
		}
		else { // compute representable result
			float tmp = (re + im * wr) / wd;
			im = (im - re * wr) / wd;
			re = tmp;
		}
	}
	else if(rhs_im == 0) { // set NaN result
		re = NAN;
		im = re;
	}
	else { // 0 < |_Right.real()| <= |_Right.imag()|
		float wr = rhs_re / rhs_im;
		float wd = rhs_im + wr * rhs_re;

		if(isnan(wd) || wd == 0) { // set NaN result
			re = NAN;
			im = re;
		}
		else { // compute representable result
			float tmp = (re * wr + im) / wd;
			im = (im * wr - re) / wd;
			re = tmp;
		}
	}

	return _FCbuild(re, im);
}
#define CMPLXFDIV(x,y) _FCdivcc(x, y)
#else
#define CMPLXFDIV(x,y) x / y
#endif

#ifdef _MSC_VER
static inline _Fcomplex _FCdivcr(_Fcomplex lhs, float rhs)
{
	return (isnan(rhs))? _FCbuild(NAN, NAN) : _FCbuild(lhs._Val[0] / rhs, lhs._Val[1] / rhs);
}
#define CMPLXFDIVF(x,y) _FCdivcr(x, y)
#else
#define CMPLXFDIVF(x,y) x / y
#endif

#ifdef _MSC_VER
#define PARITY(x) ((__popcnt16(x) & 0x1) == 0x1)
#else
#define PARITY(x) __builtin_parity(x)
#endif

typedef struct {
    int16_t r, i;
} cint16_t;

static inline cint16_t cf_to_cq15(fcomplex_t x)
{
    cint16_t cq15;
    cq15.r = crealf(x) * 32767.0f;
    cq15.i = cimagf(x) * 32767.0f;
    return cq15;
}

static inline fcomplex_t cq15_to_cf(cint16_t cq15)
{
    return CMPLXF((float)cq15.r / 32767.0f, (float)cq15.i / 32767.0f);
}

static inline fcomplex_t cq15_to_cf_conj(cint16_t cq15)
{
    return CMPLXF((float)cq15.r / 32767.0f, (float)cq15.i / -32767.0f);
}

#ifndef _MSC_VER
static inline float normf(fcomplex_t v)
{
    float realf = crealf(v);
    float imagf = cimagf(v);
    return realf * realf + imagf * imagf;
}
#endif

static inline void fftshift(fcomplex_t *x, unsigned int size)
{
    int i, h = size / 2;
    for (i = 0; i < h; i += 4)
    {
        fcomplex_t t1 = x[i], t2 = x[i+1], t3 = x[i+2], t4 = x[i+3];
        x[i] = x[i + h];
        x[i+1] = x[i+1 + h];
        x[i+2] = x[i+2 + h];
        x[i+3] = x[i+3 + h];
        x[i + h] = t1;
        x[i+1 + h] = t2;
        x[i+2 + h] = t3;
        x[i+3 + h] = t4;
    }
}
