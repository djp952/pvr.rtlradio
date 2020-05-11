//-----------------------------------------------------------------------------
// Copyright (c) 2016-2020 Michael G. Brehm
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

#ifndef __RTLDEVICE_H_
#define __RTLDEVICE_H_
#pragma once

#include <memory>
#include <string>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class rtldevice
//
// 

class rtldevice
{
public:

	// Destructor
	//
	~rtldevice();

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new rtldevice instance
	static std::unique_ptr<rtldevice> create(void);

	// name
	//
	// Gets the name of the device
	char const* name(void) const;

private:

	rtldevice(rtldevice const&) = delete;
	rtldevice& operator=(rtldevice const&) = delete;

	// Instance Constructor
	//
	rtldevice();

	//-----------------------------------------------------------------------
	// Private Member Functions

	//-----------------------------------------------------------------------
	// Member Variables

	std::string				m_name;				// Device name
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __RTLDEVICE_H_
