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

#ifndef __SCANNER_H_
#define __SCANNER_H_
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class scanner
//
// Implements the channel scanner

class scanner
{
public:

	// Destructor
	//
	~scanner();

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new scanner instance
	static std::unique_ptr<scanner> create(std::unique_ptr<rtldevice> device, bool isrdbs);

	// get_automatic_gain
	//
	// Gets the currently set automatic gain value
	bool get_automatic_gain(void) const;

	// get_manual_gain
	//
	// Gets the currently set manual gain value
	int get_manual_gain(void) const;

	// get_valid_manual_gains
	//
	// Gets the valid tuner manual gain values for the device
	void get_valid_manual_gains(std::vector<int>& dbs) const;

	// set_automatic_gain
	//
	// Sets the automatic gain flag
	void set_automatic_gain(bool autogain);

	// set_channel
	//
	// Sets the channel to be tuned
	void set_channel(uint32_t frequency);

	// set_manual_gain
	//
	// Sets the manual gain value
	void set_manual_gain(int manualgain);

	// stop
	//
	// Stops the scanner
	void stop(void);

private:

	scanner(scanner const&) = delete;
	scanner& operator=(scanner const&) = delete;

	// Instance Constructor
	//
	scanner(std::unique_ptr<rtldevice> device, bool isrbds);

	// DEFAULT_DEVICE_FREQUENCY
	//
	// Default device frequency
	static uint32_t const DEFAULT_DEVICE_FREQUENCY;

	// DEFAULT_DEVICE_SAMPLE_RATE
	//
	// Default device sample rate
	static uint32_t const DEFAULT_DEVICE_SAMPLE_RATE;

	//-----------------------------------------------------------------------
	// Private Member Functions

	// start
	//
	// Worker thread procedure used to scan the channel information
	void start(uint32_t frequency, uint32_t samplerate, bool isrdbs, scalar_condition<bool>& started);

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>	m_device;				// RTL-SDR device instance
	bool const					m_isrbds;				// RBDS vs. RDS flag
	bool						m_autogain = false;		// Automatic gain enabled/disabled
	int							m_manualgain = 0;		// Current manual gain value
	uint32_t					m_frequency = 0;		// Current frequency value

	// STREAM CONTROL
	//
	std::thread					m_worker;				// Data transfer thread
	scalar_condition<bool>		m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>			m_stopped{ false };		// Data transfer stopped flag
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SCANNER_H_
