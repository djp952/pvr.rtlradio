//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "filedevice.h"

#include <chrono>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include "string_exception.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// filedevice Constructor (private)
//
// Arguments:
//
//	filename	- Target file name
//	samplerate	- Target file sample rate

filedevice::filedevice(char const* filename, uint32_t samplerate) : m_samplerate(samplerate)
{
	if(filename == nullptr) throw std::invalid_argument("filename");

	// Canonicalize the path to avoid path traversal vulnerability
#ifdef _WINDOWS
	char buf[_MAX_PATH] = {};
	if(_fullpath(buf, filename, _MAX_PATH) != nullptr) m_filename.assign(buf);
	else throw std::invalid_argument("filename");
#else
	char buf[PATH_MAX] = {};
	if(realpath(filename, buf) != nullptr) m_filename.assign(buf);
	else throw std::invalid_argument("filename");
#endif

	// Attempt to open the target file in read-only binary mode
	m_file = fopen(m_filename.c_str(), "rb");
	if(m_file == nullptr) throw string_exception(__func__, ": fopen() failed");
}

//---------------------------------------------------------------------------
// filedevice Destructor

filedevice::~filedevice()
{
	if(m_file) fclose(m_file);
	m_file = nullptr;
}

//---------------------------------------------------------------------------
// filedevice::begin_stream
//
// Starts streaming data from the device
//
// Arguments:
//
//	NONE

void filedevice::begin_stream(void) const
{
}

//---------------------------------------------------------------------------
// filedevice::cancel_async
//
// Cancels any pending asynchronous read operations from the device
//
// Arguments:
//
//	NONE

void filedevice::cancel_async(void) const
{
	// If the asynchronous operation is stopped, do nothing
	if(m_stopped.test(true) == true) return;

	m_stop = true;							// Flag a stop condition
	m_stopped.wait_until_equals(true);		// Wait for async to stop
}

//---------------------------------------------------------------------------
// filedevice::create (static)
//
// Factory method, creates a new filedevice instance
//
// Arguments:
//
//	filename	- Target file name
//	samplerate	- Target file sample rate

std::unique_ptr<filedevice> filedevice::create(char const* filename, uint32_t samplerate)
{
	return std::unique_ptr<filedevice>(new filedevice(filename, samplerate));
}

//---------------------------------------------------------------------------
// filedevice::get_device_name
//
// Gets the name of the device
//
// Arguments:
//
//	NONE

char const* filedevice::get_device_name(void) const
{
	return m_filename.c_str();
}

//---------------------------------------------------------------------------
// filedevice::get_valid_gains
//
// Gets the valid tuner gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void filedevice::get_valid_gains(std::vector<int>& /*dbs*/) const
{
}

//---------------------------------------------------------------------------
// filedevice::read
//
// Reads data from the device
//
// Arguments:
//
//	buffer		- Buffer to receive the data
//	count		- Size of the destination buffer, specified in bytes

size_t filedevice::read(uint8_t* buffer, size_t count) const
{
	assert(m_file != nullptr);
	assert(m_samplerate != 0);

	auto start = std::chrono::steady_clock::now();

	// Synchronously read the requested amount of data from the input file
	size_t read = fread(buffer, sizeof(uint8_t), count, m_file);

	if(read > 0) {

		// Determine how long this operation should take to execute to maintain sample rate
		int duration = static_cast<int>(static_cast<double>(read) / ((m_samplerate * 2) / 1000000.0));
		auto end = start + std::chrono::microseconds(duration);

		// Yield until the calculated duration has expired
		while(std::chrono::steady_clock::now() < end) { std::this_thread::yield(); }
	}

	return read;
}

//---------------------------------------------------------------------------
// filedevice::read_async
//
// Asynchronously reads data from the device
//
// Arguments:
//
//	callback		- Asynchronous read callback function
//	bufferlength	- Output buffer length in bytes

void filedevice::read_async(rtldevice::asynccallback const& callback, uint32_t bufferlength) const
{
	std::unique_ptr<uint8_t[]>	buffer(new uint8_t[bufferlength]);		// Input data buffer

	m_stop = false;
	m_stopped = false;

	try {

		// Continuously read data from the device until the stop condition is set
		while(m_stop.test(true) == false) {

			// Try to read enough data to fill the input buffer
			size_t cb = read(&buffer[0], bufferlength);
			callback(&buffer[0], cb);
		}

		m_stopped = true;							// Operation has been stopped
	}

	// Ensure that the stopped condition is set on an exception
	catch(...) { m_stopped = true; throw; }
}

//---------------------------------------------------------------------------
// filedevice::set_automatic_gain_control
//
// Enables/disables the automatic gain control mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void filedevice::set_automatic_gain_control(bool /*enable*/) const
{
}

//---------------------------------------------------------------------------
// filedevice::set_center_frequency
//
// Sets the center frequency of the device
//
// Arguments:
//
//	hz		- Frequency to set, specified in hertz

uint32_t filedevice::set_center_frequency(uint32_t hz) const
{
	return hz;
}

//---------------------------------------------------------------------------
// filedevice::set_frequency_correction
//
// Sets the frequency correction of the device
//
// Arguments:
//
//	ppm		- Frequency correction to set, specified in parts per million

int filedevice::set_frequency_correction(int ppm) const
{
	return ppm;
}

//---------------------------------------------------------------------------
// filedevice::set_gain
//
// Sets the gain of the device
//
// Arguments:
//
//	db			- Gain to set, specified in tenths of a decibel

int filedevice::set_gain(int db) const
{
	return db;
}

//---------------------------------------------------------------------------
// filedevice::set_sample_rate
//
// Sets the sample rate of the device
//
// Arguments:
//
//	hz		- Sample rate to set, specified in hertz

uint32_t filedevice::set_sample_rate(uint32_t hz) const
{
	return hz;
}

//---------------------------------------------------------------------------
// filedevice::set_test_mode
//
// Enables/disables the test mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void filedevice::set_test_mode(bool /*enable*/) const
{
}

//---------------------------------------------------------------------------

#pragma warning(pop)
