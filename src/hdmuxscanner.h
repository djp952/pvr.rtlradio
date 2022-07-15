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

#ifndef __HDMUXSCANNER_H_
#define __HDMUXSCANNER_H_
#pragma once

#include <memory>
#include <vector>

#include "hddsp/nrsc5.h"

#include "muxscanner.h"
#include "props.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class hdmuxscanner
//
// Implements the multiplex scanner for HD Radio

class hdmuxscanner : public muxscanner
{
public:

	// Destructor
	//
	virtual ~hdmuxscanner();

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new hdmuxscanner instance
	static std::unique_ptr<hdmuxscanner> create(uint32_t samplerate, uint32_t frequency, callback const& callback);

	// inputsamples
	//
	// Pipes input samples into the muliplex scanner
	void inputsamples(uint8_t const* samples, size_t length) override;

private:

	hdmuxscanner(hdmuxscanner const&) = delete;
	hdmuxscanner& operator=(hdmuxscanner const&) = delete;

	// Instance Constructor
	//
	hdmuxscanner(uint32_t samplerate, uint32_t frequency, callback const& callback);

	// SAMPLE_RATE
	//
	// Fixed device sample rate required for HD Radio
	static uint32_t const SAMPLE_RATE;

	//-----------------------------------------------------------------------
	// Private Member Functions

	// nrsc5_callback (static)
	//
	// NRSC5 library event callback function
	static void nrsc5_callback(nrsc5_event_t const* event, void* arg);

	// nrsc5_callback
	//
	// NRSC5 library event callback function
	void nrsc5_callback(nrsc5_event_t const* event);

	//-----------------------------------------------------------------------
	// Member Variables

	nrsc5_t*				m_nrsc5;			// NRSC5 demodulator handle
	callback const			m_callback;			// Callback function
	struct multiplex		m_muxdata = {};		// Multiplex data
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __HDMUXSCANNER_H_
