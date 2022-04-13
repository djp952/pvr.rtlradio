//---------------------------------------------------------------------------
// Copyright (c) 2020-2022 Michael G. Brehm
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------

#if defined(__ANDROID__) && (__ANDROID_API__ < 23)

#include <complex.h>
#include <complex>

//---------------------------------------------------------------------------
// ANDROID 21 C99 COMPLEX NUMBER HELPERS
//
// Android API 21 (5.0) does not fully support C99 complex number support and
// a number of helper routines are required for the NRSC5 HD Radio demodulator
// to build on that platform/version without modifications

#ifdef __cplusplus
extern "C" {
#endif

	float cabsf(float _Complex __z)
	{
		float* c = reinterpret_cast<float*>(&__z);
		std::complex<float> cpp(c[0], c[1]);
		return std::abs(cpp);
	}

	float cargf(float _Complex __z)
	{
		float* c = reinterpret_cast<float*>(&__z);
		std::complex<float> cpp(c[0], c[1]);
		return std::arg(cpp);
	}

	float _Complex cexpf(float _Complex __z)
	{
		float* c = reinterpret_cast<float*>(&__z);
		std::complex<float> cpp(c[0], c[1]);
		std::complex<float> result(std::exp(cpp));
		return { result.real(), result.imag() };
	}	
	
	float cimagf(float _Complex __z)
	{
		return reinterpret_cast<float*>(&__z)[1];
	}

	float _Complex conjf(float _Complex __z)
	{
		float* c = reinterpret_cast<float*>(&__z);
		std::complex<float> cpp(c[0], c[1]);
		std::complex<float> result(std::conj(cpp));
		return { result.real(), result.imag() };
	}

	float crealf(float _Complex __z)
	{
		return reinterpret_cast<float*>(&__z)[0];
	}

#ifdef __cplusplus
}
#endif

#endif	// defined(__ANDROID__) && (__ANDROID_API__ < 23)

//---------------------------------------------------------------------------
