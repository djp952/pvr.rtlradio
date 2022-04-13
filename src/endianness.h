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

#ifndef __ENDIANNESS_H_
#define __ENDIANNESS_H_
#pragma once

//---------------------------------------------------------------------------
// get_system_endianness
//
// Helper routine to determine if the system is little or big-endian
//
// https://stackoverflow.com/questions/1001307/detecting-endianness-programmatically-in-a-c-program

enum class endianness
{
	little = 0,
	big = 1,
};

inline endianness get_system_endianness()
{
	const int value{ 0x01 };
	const void* address = static_cast<const void*>(&value);
	const unsigned char* least_significant_address = static_cast<const unsigned char*>(address);
	return (*least_significant_address == 0x01) ? endianness::little : endianness::big;
}

//---------------------------------------------------------------------------

#endif	// __ENDIANNESS_H_