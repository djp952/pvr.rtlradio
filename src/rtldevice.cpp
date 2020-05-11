//---------------------------------------------------------------------------
// Copyright (c) 2020 Michael G. Brehm
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

#include "stdafx.h"
#include "rtldevice.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// rtldevice Constructor (private)
//
// Arguments:
//
//	NONE

rtldevice::rtldevice()
{
}
	
//---------------------------------------------------------------------------
// rtldevice Destructor

rtldevice::~rtldevice()
{
}

//---------------------------------------------------------------------------
// rtldevice::create (static)
//
// Factory method, creates a new rtldevice instance
//
// Arguments:
//
//	NONE

std::unique_ptr<rtldevice> rtldevice::create(void)
{
	return std::unique_ptr<rtldevice>(new rtldevice());
}

//---------------------------------------------------------------------------
// rtldevice::name
//
// Gets the name of the device
//
// Arguments:
//
//	NONE

char const* rtldevice::name(void) const
{
	return m_name.c_str();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
