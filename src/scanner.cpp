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
#include "scanner.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// scanner Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance

scanner::scanner(std::unique_ptr<rtldevice> device) : m_device(std::move(device))
{
}

//---------------------------------------------------------------------------
// scanner destructor

scanner::~scanner()
{
}

//---------------------------------------------------------------------------
// scanner::create (static)
//
// Factory method, creates a new scanner instance
//
// Arguments:
//
//	device			- RTL-SDR device instance

std::unique_ptr<scanner> scanner::create(std::unique_ptr<rtldevice> device)
{
	return std::unique_ptr<scanner>(new scanner(std::move(device)));
}

//---------------------------------------------------------------------------

#pragma warning(pop)
