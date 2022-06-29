//////////////////////////////////////////////////////////////////////
// datatypes.h: Common data type declarations
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2013-07-28  Added single/double precision math macros
//////////////////////////////////////////////////////////////////////
#ifndef DATATYPES_H
#define DATATYPES_H

#include <math.h>
#include <stdint.h>

// uncomment to use double precision math
// #define FMDSP_USE_DOUBLE_PRECISION

// uncomment to enable thread safety mechanisms
// #define FMDSP_THREAD_SAFE

// Qt compatibility
//
typedef int8_t qint8;
typedef int16_t qint16;
typedef int32_t qint32;
typedef uint8_t quint8;
typedef uint16_t quint16;
typedef uint32_t quint32;

//define single or double precision reals and complex types
typedef float tSReal;
typedef double tDReal;

typedef struct _sCplx
{
	tSReal re;
	tSReal im;
}tSComplex;

typedef struct _dCplx
{
	tDReal re;
	tDReal im;
}tDComplex;

typedef struct _isCplx
{
	qint16 re;
	qint16 im;
}tStereo16;

typedef struct _iICplx
{
	qint32 re;
	qint32 im;
}tICpx32;


struct sSYSTEMTIME
{
	quint16 wYear;
	quint16 wMonth;
	quint16 wDayOfWeek;
	quint16 wDay;
	quint16 wHour;
	quint16 wMinute;
	quint16 wSecond;
	quint16 wMilliseconds;
};


typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
		unsigned char b2;
		unsigned char b3;
	}bytes;
	int all;
}tBtoL;

typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
	}bytes;
	signed short sall;
	unsigned short all;
}tBtoS;

#ifdef FMDSP_USE_DOUBLE_PRECISION
 #define TYPEREAL tDReal
 #define TYPECPX	tDComplex
#else
 #define TYPEREAL tSReal
 #define TYPECPX	tSComplex
#endif

#ifdef FMDSP_USE_DOUBLE_PRECISION
 #define MSIN(x) sin(x)
 #define MCOS(x) cos(x)
 #define MPOW(x,y) pow(x,y)
 #define MEXP(x) exp(x)
 #define MFABS(x) fabs(x)
 #define MLOG(x) log(x)
 #define MLOG10(x) log10(x)
 #define MSQRT(x) sqrt(x)
 #define MATAN(x) atan(x)
 #define MFMOD(x,y) fmod(x,y)
 #define MATAN2(x,y) atan2(x,y)
#else
 #define MSIN(x) sinf(x)
 #define MCOS(x) cosf(x)
 #define MPOW(x,y) powf(x,y)
 #define MEXP(x) expf(x)
 #define MFABS(x) fabsf(x)
 #define MLOG(x) logf(x)
 #define MLOG10(x) log10f(x)
 #define MSQRT(x) sqrtf(x)
 #define MATAN(x) atanf(x)
 #define MFMOD(x,y) fmodf(x,y)
 #define MATAN2(x,y) atan2f(x,y)
#endif

#define TYPESTEREO16 tStereo16
#define TYPEMONO16 qint16

//#define K_2PI (8.0*atan(1))	//maybe some compilers are't too smart to optimize out
#define K_2PI (2.0 * 3.14159265358979323846)
#define K_PI (3.14159265358979323846)
#define K_PI4 (K_PI/4.0)
#define K_PI2 (K_PI/2.0)
#define K_3PI4 (3.0*K_PI4)

#ifndef NULL
#define NULL 0
#endif


#endif // DATATYPES_H
