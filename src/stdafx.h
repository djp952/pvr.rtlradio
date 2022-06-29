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

#ifndef __STDAFX_H_
#define __STDAFX_H_
#pragma once

//---------------------------------------------------------------------------
// Windows
//---------------------------------------------------------------------------

#if defined(_WINDOWS)

#define WINVER			_WIN32_WINNT_WIN8
#define	_WIN32_WINNT	_WIN32_WINNT_WIN8
#define	_WIN32_IE		_WIN32_IE_IE80
#define NOMINMAX

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <wchar.h>					// Prevents redefinition of WCHAR_MIN by libusb

#define TARGET_WINDOWS
#define HAS_ANGLE
#define HAS_GLES 3					// ANGLE supports GLESv3

#define LIBANGLE_IMPLEMENTATION
#define ANGLE_EXPORT
#define ANGLE_UTIL_EXPORT
#define EGLAPI
#define GL_APICALL
#define GL_API

#define GL_GLES_PROTOTYPES 1
#define EGL_EGL_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1
#define EGL_EGLEXT_PROTOTYPES 1

// Android
#elif defined(__ANDROID__)

#define TARGET_POSIX
#define TARGET_LINUX
#define TARGET_ANDROID
#define HAS_GLES 3					// Android supports GLESv3

// MacOS
#elif defined(__APPLE__)

#define TARGET_POSIX
#define TARGET_DARWIN
#define TARGET_DARWIN_OSX
#define HAS_GL 1

// Linux
#else

#define TARGET_POSIX
#define TARGET_LINUX
#define HAS_GLES 2					// Assume GLESv2 support on Linux

#endif

#include <assert.h>
#include <stdint.h>

// KiB / MiB / GiB
//
#define KiB		*(1 << 10)
#define MiB		*(1 << 20)
#define GiB		*(1 << 30)

// KHz / MHz
//
#define KHz		*(1000)
#define MHz		*(1000000)

// MS / US
//
#define MS		*(1000)
#define US		*(1000000)

//---------------------------------------------------------------------------

#endif	// __STDAFX_H_
