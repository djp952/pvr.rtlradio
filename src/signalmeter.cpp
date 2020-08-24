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
#include "signalmeter.h"

#include <algorithm>

#include "align.h"

#pragma warning(push, 4)

// signalmeter::DEFAULT_DEVICE_BLOCK_SIZE
//
// Default device block size
size_t const signalmeter::DEFAULT_DEVICE_BLOCK_SIZE = (16 KiB);

// signalmeter::DEFAULT_DEVICE_FREQUENCY
//
// Default device frequency
uint32_t const signalmeter::DEFAULT_DEVICE_FREQUENCY = (87900 KHz);		// 87.9 MHz

// signalmeter::DEFAULT_DEVICE_SAMPLE_RATE
//
// Default device sample rate
uint32_t const signalmeter::DEFAULT_DEVICE_SAMPLE_RATE = (1200 KHz);

//---------------------------------------------------------------------------
// signalmeter Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties

signalmeter::signalmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops) : m_device(std::move(device))
{
	// Disable automatic gain control on the device by default
	m_device->set_automatic_gain_control(false);

	// Set the default frequency, sample rate, and frequency correction offset
	m_frequency = m_device->set_center_frequency(DEFAULT_DEVICE_FREQUENCY);
	m_device->set_sample_rate(DEFAULT_DEVICE_SAMPLE_RATE);
	m_device->set_frequency_correction(tunerprops.freqcorrection);

	// Set the manual gain to the lowest value by default
	std::vector<int> gains;
	m_device->get_valid_gains(gains);
	m_manualgain = (gains.size()) ? gains[0] : 0;
	m_manualgain = m_device->set_gain(m_manualgain);
}

//---------------------------------------------------------------------------
// signalmeter destructor

signalmeter::~signalmeter()
{
	stop();						// Stop the signalmeter
	m_device.reset();			// Release the RTL-SDR device
}

//---------------------------------------------------------------------------
// signalmeter::create (static)
//
// Factory method, creates a new signalmeter instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties

std::unique_ptr<signalmeter> signalmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops)
{
	return std::unique_ptr<signalmeter>(new signalmeter(std::move(device), tunerprops));
}

//---------------------------------------------------------------------------
// signalmeter::get_automatic_gain
//
// Gets the automatic gain flag
//
// Arguments:
//
//	NONE

bool signalmeter::get_automatic_gain(void) const
{
	return m_autogain;
}

//---------------------------------------------------------------------------
// signalmeter::get_frequency
//
// Gets the current tuned frequency
//
// Arguments:
//
//	NONE

uint32_t signalmeter::get_frequency(void) const
{
	return m_frequency;
}

//---------------------------------------------------------------------------
// signalmeter::get_manual_gain
//
// Gets the manual gain value, specified in tenths of a decibel
//
// Arguments:
//
//	NONE

int signalmeter::get_manual_gain(void) const
{
	return m_manualgain;
}

//---------------------------------------------------------------------------
// signalmeter::get_valid_manual_gains
//
// Gets the valid tuner manual gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void signalmeter::get_valid_manual_gains(std::vector<int>& dbs) const
{
	return m_device->get_valid_gains(dbs);
}

//---------------------------------------------------------------------------
// signalmeter::set_automatic_gain
//
// Sets the automatic gain mode of the device
//
// Arguments:
//
//	autogain	- Flag to enable/disable automatic gain mode

void signalmeter::set_automatic_gain(bool autogain)
{
	assert(m_device);

	m_device->set_automatic_gain_control(autogain);
	if(!autogain) m_device->set_gain(m_manualgain);

	m_autogain = autogain;
}

//---------------------------------------------------------------------------
// signalmeter::set_frequency
//
// Sets the frequency to be tuned
//
// Arguments:
//
//	frequency		- Frequency to set, specified in hertz

void signalmeter::set_frequency(uint32_t frequency)
{
	m_frequency = m_device->set_center_frequency(frequency);
}

//---------------------------------------------------------------------------
// signalmeter::set_manual_gain
//
// Sets the manual gain value of the device
//
// Arguments:
//
//	manualgain	- Gain to set, specified in tenths of a decibel

void signalmeter::set_manual_gain(int manualgain)
{
	m_manualgain = (m_autogain) ? manualgain : m_device->set_gain(manualgain);
}

//---------------------------------------------------------------------------
// signalmeter::start
//
// Starts the signal meter
//
// Arguments:
//
//	NONE

void signalmeter::start(void)
{
	scalar_condition<bool>		started{ false };		// Thread start condition variable

	stop();						// Stop the signal meter if it's already running

	// Define and launch the I/Q signal meter thread
	m_worker = std::thread([&]() -> void {

		m_device->begin_stream();					// Begin streaming from the device
		started = true;								// Signal that the thread has started

		// Loop until the worker thread has been signaled to stop
		while(m_stop.test(false) == true) {

			// TODO: STUFF FROM RTL_POWER HERE

			if(m_stop.test(true) == true) continue;			// Watch for stop
		}

		m_stopped.store(true);					// Thread is stopped
	});

	// Wait until the worker thread indicates that it has started
	started.wait_until_equals(true);
}

//---------------------------------------------------------------------------
// signalmeter::stop
//
// Stops the signal meter
//
// Arguments:
//
//	NONE

void signalmeter::stop(void)
{
	m_stop = true;								// Signal worker thread to stop
	if(m_device) m_device->cancel_async();		// Cancel any async read operations
	if(m_worker.joinable()) m_worker.join();	// Wait for thread
	m_stop = false;								// Reset stop signal
}

//---------------------------------------------------------------------------

#pragma warning(pop)
