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

#ifndef __RTLDEVICE_H_
#define __RTLDEVICE_H_
#pragma once

#include <functional>
#include <vector>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class rtldevice
//
// Defines the interface required for managing an RTL-SDR device

class rtldevice
{
public:

	// asynccallback
	//
	// Callback function passed to readasync
	using asynccallback = std::function<void(uint8_t const*, size_t)>;

	// Constructor / Destructor
	//
	rtldevice() {}
	virtual ~rtldevice() {}

	//-----------------------------------------------------------------------
	// Member Functions

	// begin_stream
	//
	// Starts streaming data from the device
	virtual void begin_stream(void) const = 0;

	// cancel_async
	//
	// Cancels any pending asynchronous read operations from the device
	virtual void cancel_async(void) const = 0;

	// get_device_name
	//
	// Gets the name of the device
	virtual char const* get_device_name(void) const = 0;

	// get_valid_gains
	//
	// Gets the valid tuner gain values for the device
	virtual void get_valid_gains(std::vector<int>& dbs) const = 0;

	// read
	//
	// Reads data from the device
	virtual size_t read(uint8_t* buffer, size_t count) const = 0;

	// read_async
	//
	// Asynchronously reads data from the device
	virtual void read_async(asynccallback const& callback, uint32_t bufferlength) const = 0;

	// set_automatic_gain_control
	//
	// Enables/disables the automatic gain control of the device
	virtual void set_automatic_gain_control(bool enable) const = 0;

	// set_center_frequency
	//
	// Sets the center frequency of the device
	virtual uint32_t set_center_frequency(uint32_t hz) const = 0;

	// set_frequency_correction
	//
	// Sets the frequency correction of the device
	virtual int set_frequency_correction(int ppm) const = 0;

	// set_gain
	//
	// Sets the gain value of the device
	virtual int set_gain(int db) const = 0;

	// set_sample_rate
	//
	// Sets the sample rate of the device
	virtual uint32_t set_sample_rate(uint32_t hz) const = 0;

	// set_test_mode
	//
	// Enables/disables the test mode of the device
	virtual void set_test_mode(bool enable) const = 0;

private:

	rtldevice(rtldevice const&) = delete;
	rtldevice& operator=(rtldevice const&) = delete;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __RTLDEVICE_H_
