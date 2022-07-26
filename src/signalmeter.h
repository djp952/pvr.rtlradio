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

#ifndef __SIGNALMETER_H_
#define __SIGNALMETER_H_
#pragma once

#include <functional>
#include <memory>

#include "fmdsp/fastfir.h"
#include "fmdsp/fft.h"

#include "props.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class signalmeter
//
// Implements the FM signal meter

class signalmeter
{
public:

	// Destructor
	//
	~signalmeter() = default;

	//-----------------------------------------------------------------------
	// Type Declarations

	// signal_status
	//
	// Structure used to report the current signal status
	struct signal_status {

		float			power;				// Signal power level in dB
		float			noise;				// Signal noise level in dB
		float			snr;				// Signal-to-noise ratio in dB
		bool			overload;			// FFT input data is overloaded
		int32_t			lowcut;				// Low cut plot index
		int32_t			highcut;			// High cut plot index
		size_t			plotsize;			// Size of the signal plot data array
		int	const*		plotdata;			// Pointer to the signal plot data
	};

	// exception_callback
	//
	// Callback function invoked when an exception has occurred on the worker thread
	using exception_callback = std::function<void(std::exception const& ex)>;

	// status_callback
	//
	// Callback function invoked when the signal status has changed
	using status_callback = std::function<void(struct signal_status const& status)>;

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new signalmeter instance
	static std::unique_ptr<signalmeter> create(struct signalprops const& signalprops, struct signalplotprops const& plotprops,
		uint32_t rate, status_callback const& onstatus);

	// inputsamples
	//
	// Pipes input samples into the signal meter instance
	void inputsamples(uint8_t const* samples, size_t length);

private:

	signalmeter(signalmeter const&) = delete;
	signalmeter& operator=(signalmeter const&) = delete;

	// DEFAULT_FFT_SIZE
	//
	// Default FFT size (bins)
	static size_t const DEFAULT_FFT_SIZE;

	// RING_BUFFER_SIZE
	//
	// Input ring buffer size
	static size_t const RING_BUFFER_SIZE;

	// Instance Constructor
	//
	signalmeter(struct signalprops const& signalprops, struct signalplotprops const& plotprops, uint32_t rate, 
		status_callback const& onstatus);

	//-----------------------------------------------------------------------
	// Private Member Functions

	// processsamples
	//
	// Processes the available input samples
	void processsamples(void);

	//-----------------------------------------------------------------------
	// Member Variables

	struct signalprops const		m_signalprops;			// Signal properties
	struct signalplotprops const	m_plotprops;			// Signal plot properties
	status_callback const			m_onstatus;				// Status callback function

	// FFT
	//
	CFastFIR				m_fir;							// Finite impulse response filter
	size_t					m_fftsize{ DEFAULT_FFT_SIZE };	// FFT size (bins)
	CFft					m_fft;							// FFT instance
	size_t					m_fftminbytes{ 0 };				// Number of bytes required to process
	float					m_avgpower{ NAN };				// Average power level
	float					m_avgnoise{ NAN };				// Average noise level

	// RING BUFFER
	//
	std::unique_ptr<uint8_t[]>		m_buffer;				// Input ring buffer
	size_t							m_head{ 0 };			// Ring buffer head position
	size_t							m_tail{ 0 };			// Ring buffer tail position
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SIGNALMETER_H_
