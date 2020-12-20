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

#ifndef __SIGNALMETER_H_
#define __SIGNALMETER_H_
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "fmdsp/demodulator.h"

#include "props.h"
#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class signalmeter
//
// Implements the signal meter

class signalmeter
{
public:

	// Destructor
	//
	~signalmeter();

	//-----------------------------------------------------------------------
	// Type Declarations

	// signal_status
	//
	// Structure used to report the current signal status
	struct signal_status {

		TYPEREAL			power;				// Signal power level in dB
		TYPEREAL			noise;				// Signal noise level in dB
		TYPEREAL			snr;				// Signal-to-noise ratio in dB
		bool				stereo;				// Flag if Stereo signal is present
		bool				rds;				// Flag if RDS signal is present
	};

	// exception_callback
	//
	// Callback function invoked when an exception has occurred on the worker thread
	using exception_callback = std::function<void(std::exception const& ex)>;

	// signal_status_callback
	//
	// Callback function invoked when the signal status has changed
	using signal_status_callback = std::function<void(struct signal_status const& status)>;

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new signalmeter instance
	static std::unique_ptr<signalmeter> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
		struct fmprops const& fmprops, signal_status_callback const& onstatus, int onstatusrate);
	static std::unique_ptr<signalmeter> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct fmprops const& fmprops, signal_status_callback const& onstatus, int onstatusrate, exception_callback const& onexception);

	// get_automatic_gain
	//
	// Gets the currently set automatic gain value
	bool get_automatic_gain(void) const;

	// get_frequency
	//
	// Gets the currently set frequency
	uint32_t get_frequency(void) const;

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

	// set_frequency
	//
	// Sets the frequency to be tuned
	void set_frequency(uint32_t frequency);

	// set_manual_gain
	//
	// Sets the manual gain value
	void set_manual_gain(int manualgain);

	// start
	//
	// Starts the signal meter
	void start(void);

	// stop
	//
	// Stops the signal meter
	void stop(void);

private:

	signalmeter(signalmeter const&) = delete;
	signalmeter& operator=(signalmeter const&) = delete;

	// Instance Constructor
	//
	signalmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, struct fmprops const& fmprops, 
		signal_status_callback const& onstatus, int onstatusrate, exception_callback const& onexception);

	// DEFAULT_DEVICE_FREQUENCY
	//
	// Default device frequency
	static uint32_t const DEFAULT_DEVICE_FREQUENCY;

	// DEFAULT_DEVICE_SAMPLE_RATE
	//
	// Default device sample rate
	static uint32_t const DEFAULT_DEVICE_SAMPLE_RATE;

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>		m_device;				// RTL-SDR device instance
	struct tunerprops const			m_tunerprops;			// Tuner settings
	struct fmprops const			m_fmprops;				// FM Radio settings

	bool							m_autogain = false;		// Automatic gain enabled/disabled
	int								m_manualgain = 0;		// Current manual gain value
	uint32_t						m_frequency = 0;		// Current frequency value
	std::atomic<bool>				m_freqchange{ false };	// Frequency changed flag
	std::thread						m_worker;				// Worker thread
	scalar_condition<bool>			m_stop{ false };		// Condition to stop worker
	std::atomic<bool>				m_stopped{ false };		// Worker stopped flag

	signal_status_callback const	m_onstatus;				// Status callback function
	int const						m_onstatusrate;			// Status callback rate (milliseconds)
	exception_callback const		m_onexception;			// Exception callback function
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SIGNALMETER_H_
