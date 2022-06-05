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

#ifndef __FILEDEVICE_H_
#define __FILEDEVICE_H_
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class filedevice
//
// Implements a dummy device that reads the I/Q samples from a file.  This is
// only intended for debugging purposes as there is no control over the 
// parameters like frequency, sample rate, etc; those will have been set at
// the time when the file was captured

class filedevice : public rtldevice
{
public:

	// Destructor
	//
	virtual ~filedevice();

	//-----------------------------------------------------------------------
	// Member Functions

	// begin_stream
	//
	// Starts streaming data from the device
	void begin_stream(void) const override;

	// cancel_async
	//
	// Cancels any pending asynchronous read operations from the device
	void cancel_async(void) const override;

	// create (static)
	//
	// Factory method, creates a new filedevice instance
	static std::unique_ptr<filedevice> create(char const* filename, uint32_t samplerate);

	// get_device_name
	//
	// Gets the name of the device
	char const* get_device_name(void) const override;

	// get_valid_gains
	//
	// Gets the valid tuner gain values for the device
	void get_valid_gains(std::vector<int>& dbs) const override;

	// read
	//
	// Reads data from the device
	size_t read(uint8_t* buffer, size_t count) const override;

	// read_async
	//
	// Asynchronously reads data from the device
	void read_async(rtldevice::asynccallback const& callback, uint32_t bufferlength) const override;

	// set_automatic_gain_control
	//
	// Enables/disables the automatic gain control of the device
	void set_automatic_gain_control(bool enable) const override;

	// set_center_frequency
	//
	// Sets the center frequency of the device
	uint32_t set_center_frequency(uint32_t hz) const override;

	// set_frequency_correction
	//
	// Sets the frequency correction of the device
	int set_frequency_correction(int ppm) const override;

	// set_gain
	//
	// Sets the gain value of the device
	int set_gain(int db) const override;

	// set_sample_rate
	//
	// Sets the sample rate of the device
	uint32_t set_sample_rate(uint32_t hz) const override;

	// set_test_mode
	//
	// Enables/disables the test mode of the device
	void set_test_mode(bool enable) const override;

private:

	filedevice(filedevice const&) = delete;
	filedevice& operator=(filedevice const&) = delete;

	// Instance Constructor
	//
	filedevice(char const* filename, uint32_t samplerate);

	//-----------------------------------------------------------------------
	// Member Variables

	std::string				m_filename;						// File name
	uint32_t const			m_samplerate;					// Sample rate
	FILE*					m_file = nullptr;				// File handle

	// ASYNCHRONOUS SUPPORT
	//
	mutable scalar_condition<bool>	m_stop{ false };		// Flag to stop async
	mutable scalar_condition<bool>	m_stopped{ true };		// Async stopped condition
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __FILEDEVICE_H_
