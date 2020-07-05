//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __FMSCANNER_H_
#define __FMSCANNER_H_
#pragma once

#include <functional>
#include <memory>

#include "props.h"

#pragma warning(push, 4)

// fmscanner_scan_callback
//
// Callback function passed to fmscanner::scan
using scan_callback = std::function<void(struct channelprops const& channel)>;

//---------------------------------------------------------------------------
// Class fmscanner
//
// Implements the FM radio channel scanner

class fmscanner
{
public:

	// scan_callback
	//
	// Callback function passed to scan()
	using scan_callback = std::function<void(struct channelprops const& channel)>;

	//-----------------------------------------------------------------------
	// Member Functions

	// scan (static)
	//
	// Scans for FM radio channels
	static void scan(struct deviceprops const& deviceprops, scan_callback const& callback);

private:

	fmscanner() = delete;
	fmscanner(fmscanner const&) = delete;
	fmscanner& operator=(fmscanner const&) = delete;
	~fmscanner() = delete;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __FMSCANNER_H_
