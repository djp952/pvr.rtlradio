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

#ifndef __MUXSCANNER_H_
#define __MUXSCANNER_H_
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "props.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class muxscanner
//
// Defines the interface required for providing muxscanner/ensemble information

class muxscanner
{
public:

	// Constructor / Destructor
	//
	muxscanner() {}
	virtual ~muxscanner() {}

	//-----------------------------------------------------------------------
	// Type Declarations

	// subchannel
	//
	// Structure used to report subchannel properties
	struct subchannel {

		uint32_t		number;				// Subchannel number
		std::string		name;				// Subchannel name
	};

	// multiplex
	//
	// Structure used to report the multiplex properties
	struct multiplex {

		bool							sync;				// Sync (lock) flag
		std::string						name;				// Multiplex name
		std::vector<struct subchannel>	subchannels;		// Multiplex subchannels
	};

	// callback
	//
	// Callback function invoked when the multiplex properties have changed
	using callback = std::function<void(struct multiplex const& status)>;

	//-----------------------------------------------------------------------
	// Member Functions

	// inputsamples
	//
	// Pipes input samples into the muliplex scanner
	virtual void inputsamples(uint8_t const* samples, size_t length) = 0;

private:

	muxscanner(muxscanner const&) = delete;
	muxscanner& operator=(muxscanner const&) = delete;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __MUXSCANNER_H_
