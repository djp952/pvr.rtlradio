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

#ifndef __FMMETER_H_
#define __FMMETER_H_
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "fmdsp/datatypes.h"

#include "props.h"
#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class fmmeter
//
// Implements the FM signal meter

class fmmeter
{
public:

	// Destructor
	//
	~fmmeter();

	//-----------------------------------------------------------------------
	// Type Declarations

	// signal_status
	//
	// Structure used to report the current signal status
	struct signal_status {

		enum modulation		modulation;			// Channel modulation being analyzed
		float				power;				// Signal power level in dB
		float				noise;				// Signal noise level in dB
		float				snr;				// Signal-to-noise ratio in dB
		bool				overload;			// FFT input data is overloaded
		uint32_t			fftbandwidth;		// Overall bandwidth of the FFT
		int32_t				ffthighcut;			// High cut frequency from center
		int32_t				fftlowcut;			// Low cut frequency from center
		size_t				fftsize;			// Size of the FFT
		int	const*			fftdata;			// Pointer to the FFT data
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
	// Factory method, creates a new fmmeter instance
	static std::unique_ptr<fmmeter> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		uint32_t frequency, enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus,
		int onstatusrate);
	static std::unique_ptr<fmmeter> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		uint32_t frequency, enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus,
		int onstatusrate, exception_callback const& onexception);

	// get_automatic_gain
	//
	// Gets the currently set automatic gain value
	bool get_automatic_gain(void) const;

	// get_manual_gain
	//
	// Gets the currently set manual gain value
	int get_manual_gain(void) const;

	// get_modulation
	//
	// Gets the currently set modulation type
	enum modulation get_modulation(void) const;

	// get_valid_manual_gains
	//
	// Gets the valid tuner manual gain values for the device
	void get_valid_manual_gains(std::vector<int>& dbs) const;

	// set_automatic_gain
	//
	// Sets the automatic gain flag
	void set_automatic_gain(bool autogain);

	// set_manual_gain
	//
	// Sets the manual gain value
	void set_manual_gain(int manualgain);

	// set_modulation
	//
	// Sets the modulation type
	void set_modulation(enum modulation modulation);

	// start
	//
	// Starts the signal meter
	void start(TYPEREAL maxdb, TYPEREAL mindb, size_t height, size_t width);

	// stop
	//
	// Stops the signal meter
	void stop(void);

private:

	fmmeter(fmmeter const&) = delete;
	fmmeter& operator=(fmmeter const&) = delete;

	// Instance Constructor
	//
	fmmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, uint32_t frequency, 
		enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus,
		int onstatusrate, exception_callback const& onexception);

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>		m_device;				// RTL-SDR device instance
	struct tunerprops const			m_tunerprops;			// Tuner settings
	uint32_t const					m_frequency;			// Center frequency
	uint32_t const					m_fftwidth;				// FFT bandwidth

	bool							m_autogain = true;		// Automatic gain enabled/disabled
	int								m_manualgain = 0;		// Current manual gain value
	std::atomic<enum modulation>	m_modulation;			// Current modulation value
	std::thread						m_worker;				// Worker thread
	scalar_condition<bool>			m_stop{ false };		// Condition to stop worker
	std::atomic<bool>				m_stopped{ false };		// Worker stopped flag

	signal_status_callback const	m_onstatus;				// Status callback function
	int const						m_onstatusrate;			// Status callback rate (milliseconds)
	exception_callback const		m_onexception;			// Exception callback function
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __FMMETER_H_
