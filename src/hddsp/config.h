//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __NRSC5_CONFIG_H_
#define __NRSC5_CONFIG_H_
#pragma once

#define LIBRARY_DEBUG_LEVEL 5

// Windows
#if defined(_WINDOWS)

#define HAVE_COMPLEX_I
#define HAVE_FAAD2
#define strdup _strdup

// Android
#elif defined(__ANDROID__)

#define HAVE_STRNDUP
#define HAVE_CMPLXF
#define HAVE_COMPLEX_I

#if(__ANDROID_API__ < 23)
// src/compat/bionic/complex.cpp
float cabsf(float _Complex __z);
float cargf(float _Complex __z);
float _Complex cexpf(float _Complex __z);
float cimagf(float _Complex __z);
float _Complex conjf(float _Complex __z);
float crealf(float _Complex __z);
#endif

// MacOS
#elif defined(__APPLE__)

#define HAVE_PTHREAD_SETNAME_NP
#define HAVE_STRNDUP
#define HAVE_CMPLXF
#define HAVE_COMPLEX_I
#define HAVE_FAAD2

// Linux
#else

#define HAVE_PTHREAD_SETNAME_NP
#define HAVE_STRNDUP
#define HAVE_CMPLXF
#define HAVE_COMPLEX_I
#define HAVE_FAAD2

#endif

#ifdef HAVE_FAAD2
#define USE_FAAD2
#endif

#ifndef HAVE_STRNDUP
#include <stdlib.h>
char* strndup(char const* s, size_t n);
#endif

#endif	// __NRSC5_CONFIG_H_
