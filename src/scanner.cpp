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
#include "scanner.h"

#include <algorithm>

#include "fmdsp/fft.h"

#include "align.h"

#pragma warning(push, 4)

// scanner::DEFAULT_DEVICE_BLOCK_SIZE
//
// Default device block size
size_t const scanner::DEFAULT_DEVICE_BLOCK_SIZE = (16 KiB);

// scanner::DEFAULT_DEVICE_FREQUENCY
//
// Default device frequency
uint32_t const scanner::DEFAULT_DEVICE_FREQUENCY = (87900 KHz);		// 87.9 MHz

// scanner::DEFAULT_DEVICE_SAMPLE_RATE
//
// Default device sample rate
uint32_t const scanner::DEFAULT_DEVICE_SAMPLE_RATE = (1024 KHz);

//---------------------------------------------------------------------------
// scanner Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties

scanner::scanner(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops) : m_device(std::move(device))
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
// scanner destructor

scanner::~scanner()
{
	stop();						// Stop the scanner
	m_device.reset();			// Release the RTL-SDR device
}

//---------------------------------------------------------------------------
// scanner::create (static)
//
// Factory method, creates a new scanner instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties

std::unique_ptr<scanner> scanner::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops)
{
	return std::unique_ptr<scanner>(new scanner(std::move(device), tunerprops));
}

//---------------------------------------------------------------------------
// scanner::get_automatic_gain
//
// Gets the automatic gain flag
//
// Arguments:
//
//	NONE

bool scanner::get_automatic_gain(void) const
{
	return m_autogain;
}

//---------------------------------------------------------------------------
// scanner::get_frequency
//
// Gets the current tuned frequency
//
// Arguments:
//
//	NONE

uint32_t scanner::get_frequency(void) const
{
	return m_frequency;
}

//---------------------------------------------------------------------------
// scanner::get_manual_gain
//
// Gets the manual gain value, specified in tenths of a decibel
//
// Arguments:
//
//	NONE

int scanner::get_manual_gain(void) const
{
	return m_manualgain;
}

//---------------------------------------------------------------------------
// scanner::get_valid_manual_gains
//
// Gets the valid tuner manual gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void scanner::get_valid_manual_gains(std::vector<int>& dbs) const
{
	return m_device->get_valid_gains(dbs);
}

//---------------------------------------------------------------------------
// scanner::set_automatic_gain
//
// Sets the automatic gain mode of the device
//
// Arguments:
//
//	autogain	- Flag to enable/disable automatic gain mode

void scanner::set_automatic_gain(bool autogain)
{
	assert(m_device);
	
	m_device->set_automatic_gain_control(autogain);
	if(!autogain) m_device->set_gain(m_manualgain);
	
	m_autogain = autogain;
}

//---------------------------------------------------------------------------
// scanner::set_frequency
//
// Sets the frequency to be tuned
//
// Arguments:
//
//	frequency		- Frequency to set, specified in hertz

void scanner::set_frequency(uint32_t frequency)
{
	m_frequency = m_device->set_center_frequency(frequency);
}

//---------------------------------------------------------------------------
// scanner::set_manual_gain
//
// Sets the manual gain value of the device
//
// Arguments:
//
//	manualgain	- Gain to set, specified in tenths of a decibel

void scanner::set_manual_gain(int manualgain)
{
	m_manualgain = (m_autogain) ? manualgain : m_device->set_gain(manualgain);
}

//---------------------------------------------------------------------------
// scanner::start
//
// Starts the scanner
//
// Arguments:
//
//	NONE

void scanner::start(void)
{
	scalar_condition<bool>		started{ false };		// Thread start condition variable

	stop();						// Implicitly stop the scanner if it's running already

	// Define and launch the I/Q sample scanner thread
	m_worker = std::thread([&]() -> void {

		CFft					fft;				// Fast fourier transform
		static size_t const		fftsize = 2 KiB;	// FFT size

		// Initialize the fast fourier transform instance
		fft.SetFFTParams(static_cast<int>(fftsize), false, 0.0, DEFAULT_DEVICE_SAMPLE_RATE);
		fft.SetFFTAve(1);

		// Create a heap array to hold the raw sample data from the device
		size_t const buffersize = DEFAULT_DEVICE_BLOCK_SIZE;
		std::unique_ptr<uint8_t[]> buffer(new uint8_t[buffersize]);

		// Create a heap array in which to hold the converted I/Q samples
		size_t const numsamples = (buffersize / 2);
		std::unique_ptr<TYPECPX[]> samples(new TYPECPX[numsamples]);

		m_device->begin_stream();					// Begin streaming from the device
		started = true;								// Signal that the thread has started

		// Loop until the worker thread has been signaled to stop
		while(m_stop.test(false) == true) {

			// Read the next chunk of raw data from the device, discard short reads
			size_t read = m_device->read(&buffer[0], buffersize);
			if(read < buffersize) { assert(false);  continue; }

			if(m_stop.test(true) == true) continue;			// Watch for stop

			for(size_t index = 0; index < numsamples; index += 2) {

				// The FFT expects the I/Q samples in the range of -32767.0 through +32767.0
				// 256.996 = (32767.0 / 127.5) = 256.9960784313725
				samples[index] = {

				#ifdef FMDSP_USE_DOUBLE_PRECISION
					(static_cast<TYPEREAL>(buffer[index]) - 127.5) * 256.996,			// I
					(static_cast<TYPEREAL>(buffer[index + 1]) - 127.5) * 256.996,		// Q
				#else
					(static_cast<TYPEREAL>(buffer[index]) - 127.5f) * 256.996f,			// I
					(static_cast<TYPEREAL>(buffer[index + 1]) - 127.5f) * 256.996f,		// Q
				#endif
				};
			}

			if(m_stop.test(true) == true) continue;			// Watch for stop

			// Put the I/Q samples into the fast fourier transform instance
			fft.PutInDisplayFFT(static_cast<qint32>(std::min(fftsize, numsamples)), samples.get());

			// ////
			// TODO: FIGURE OUT HOW TO GET THE DATA FROM THE FFT SUCCESSFULLY
			// ////
		}

		m_stopped.store(true);					// Thread is stopped
	});
		
	started.wait_until_equals(true);
}

//---------------------------------------------------------------------------
// scanner::stop
//
// Stops the scanner
//
// Arguments:
//
//	NONE

void scanner::stop(void)
{
	m_stop = true;								// Signal worker thread to stop
	if(m_device) m_device->cancel_async();		// Cancel any async read operations
	if(m_worker.joinable()) m_worker.join();	// Wait for thread
	m_stop = false;								// Reset stop signal
}

//---------------------------------------------------------------------------

#pragma warning(pop)
