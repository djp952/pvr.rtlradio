//---------------------------------------------------------------------------
// Copyright (c) 2020-2021 Michael G. Brehm
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
#include "fmmeter.h"

#include <algorithm>
#include <cmath>
#include <string.h>

#include "align.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// fmmeter Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//  frequency		- Center frequency to be tuned for the channel
//  bandwidth		- Bandwidth of the channel to be analyzed
//	onstatus		- Signal status callback function
//	statusrate		- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

fmmeter::fmmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, uint32_t frequency,
	uint32_t bandwidth, signal_status_callback const& onstatus, int statusrate, exception_callback const& onexception) : 
	m_device(std::move(device)), m_tunerprops(tunerprops), m_frequency(frequency), m_bandwidth(bandwidth), 
	m_onstatus(onstatus), m_onstatusrate(std::min(statusrate / 10, 10)), m_onexception(onexception)
{
	// Set the default frequency, sample rate, and frequency correction offset
	m_device->set_center_frequency(m_frequency);
	m_device->set_sample_rate(m_tunerprops.samplerate);
	m_device->set_frequency_correction(m_tunerprops.freqcorrection);

	// Enable automatic gain control on the device by default
	m_device->set_automatic_gain_control(true);

	// Set the manual gain to the lowest value by default
	std::vector<int> gains;
	m_device->get_valid_gains(gains);
	m_manualgain = (gains.size()) ? gains[0] : 0;
}

//---------------------------------------------------------------------------
// fmmeter destructor

fmmeter::~fmmeter()
{
	stop();						// Stop the fmmeter
	m_device.reset();			// Release the RTL-SDR device
}

//---------------------------------------------------------------------------
// fmmeter::create (static)
//
// Factory method, creates a new fmmeter instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//  frequency		- Center frequency to be tuned for the channel
//  bandwidth		- Bandwidth of the channel to be analyzed
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)

std::unique_ptr<fmmeter> fmmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	uint32_t frequency, uint32_t bandwidth, signal_status_callback const& onstatus, int onstatusrate)
{
	auto onexception = [](std::exception const&) -> void { /* DO NOTHING */ };
	return std::unique_ptr<fmmeter>(new fmmeter(std::move(device), tunerprops, frequency, bandwidth, onstatus, onstatusrate, onexception));
}

//---------------------------------------------------------------------------
// fmmeter::create (static)
//
// Factory method, creates a new fmmeter instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//  frequency		- Center frequency to be tuned for the channel
//  bandwidth		- Bandwidth of the channel to be analyzed
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

std::unique_ptr<fmmeter> fmmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	uint32_t frequency, uint32_t bandwidth, signal_status_callback const& onstatus, int onstatusrate, exception_callback const& onexception)
{
	return std::unique_ptr<fmmeter>(new fmmeter(std::move(device), tunerprops, frequency, bandwidth, onstatus, onstatusrate, onexception));
}

//---------------------------------------------------------------------------
// fmmeter::get_automatic_gain
//
// Gets the automatic gain flag
//
// Arguments:
//
//	NONE

bool fmmeter::get_automatic_gain(void) const
{
	return m_autogain;
}

//---------------------------------------------------------------------------
// fmmeter::get_manual_gain
//
// Gets the manual gain value, specified in tenths of a decibel
//
// Arguments:
//
//	NONE

int fmmeter::get_manual_gain(void) const
{
	return m_manualgain;
}

//---------------------------------------------------------------------------
// fmmeter::get_valid_manual_gains
//
// Gets the valid tuner manual gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void fmmeter::get_valid_manual_gains(std::vector<int>& dbs) const
{
	return m_device->get_valid_gains(dbs);
}

//---------------------------------------------------------------------------
// fmmeter::set_automatic_gain
//
// Sets the automatic gain mode of the device
//
// Arguments:
//
//	autogain	- Flag to enable/disable automatic gain mode

void fmmeter::set_automatic_gain(bool autogain)
{
	m_device->set_automatic_gain_control(autogain);
	if(!autogain) m_device->set_gain(m_manualgain);

	m_autogain = autogain;
}

//---------------------------------------------------------------------------
// fmmeter::set_manual_gain
//
// Sets the manual gain value of the device
//
// Arguments:
//
//	manualgain	- Gain to set, specified in tenths of a decibel

void fmmeter::set_manual_gain(int manualgain)
{
	m_manualgain = (m_autogain) ? manualgain : m_device->set_gain(manualgain);
}

//---------------------------------------------------------------------------
// fmmeter::start
//
// Starts the signal meter
//
// Arguments:
//
//	NONE

void fmmeter::start(void)
{
	scalar_condition<bool>		started{ false };		// Thread start condition variable

	stop();						// Stop the signal meter if it's already running

	// Define and launch the I/Q signal meter thread
	m_worker = std::thread([&]() -> void {

		started = true;					// Signal that the thread has started

		try {

			// Create and initialize the FM demodulator instance
			std::unique_ptr<CDemodulator> demodulator(new CDemodulator()); 
			
			tDemodInfo demodinfo = {};
			demodinfo.HiCutmax = 100000;
			demodinfo.HiCut = (m_bandwidth / 2);
			demodinfo.LowCut = -demodinfo.HiCut;
			demodinfo.SquelchValue = -160;

			demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
			demodulator->SetInputSampleRate(static_cast<TYPEREAL>(m_tunerprops.samplerate));
			demodulator->SetDemod(DEMOD_FM, demodinfo);
			
			// Create the input buffers to hold the raw and converted I/Q samples read from the device
			size_t const numsamples = demodulator->GetInputBufferLimit();
			size_t const numbytes = numsamples * 2;

			std::unique_ptr<uint8_t[]> buffer(new uint8_t[numbytes]);
			std::unique_ptr<TYPECPX[]> samples(new TYPECPX[numsamples]);

			int iterations = 0;					// Counter for callback loop iterations

			m_device->begin_stream();			// Begin streaming from the device

			// Loop until the worker thread has been signaled to stop
			while(m_stop.test(false) == true) {

				// Read the next block of raw 8-bit I/Q samples from the input device
				size_t read = 0;
				while(read < numbytes) read += m_device->read(&buffer[read], numbytes - read);

				// Convert the raw 8-bit I/Q samples into scaled complex I/Q samples
				for(size_t index = 0; index < numsamples; index++) {

					// The demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
					// (32767.0 / 127.5) = 256.9960784313725
					samples[index] = {

					#ifdef FMDSP_USE_DOUBLE_PRECISION
						(static_cast<TYPEREAL>(buffer[(index * 2)]) - 127.5) * 256.9960784313725,		// I
						(static_cast<TYPEREAL>(buffer[(index * 2) + 1]) - 127.5) * 256.9960784313725,	// Q
					#else
						(static_cast<TYPEREAL>(buffer[(index * 2)]) - 127.5f) * 256.9960784313725f,		// I
						(static_cast<TYPEREAL>(buffer[(index * 2) + 1]) - 127.5f) * 256.9960784313725f,	// Q
					#endif
					};
				}

				// Run the I/Q samples through the demodulator to calculate the signal levels
				demodulator->ProcessData(static_cast<int>(numsamples), samples.get(), samples.get());

				// Only invoke the callback at roughly the requested rate
				if((++iterations % m_onstatusrate) == 0) {

					struct signal_status status = {};
					status.power = demodulator->GetSignalLevel();;
					status.noise = demodulator->GetNoiseLevel();
					status.snr = demodulator->GetSignalToNoiseLevel();;

					if(m_onstatus) m_onstatus(status);
					iterations = 0;
				}
			}
		}

		// Pass any thrown exception into the callback, if one has been specified
		catch(std::exception const& ex) { if(m_onexception) m_onexception(ex); }
		
		m_stopped.store(true);			// Thread is stopped
	});

	// Wait until the worker thread indicates that it has started
	started.wait_until_equals(true);
}

//---------------------------------------------------------------------------
// fmmeter::stop
//
// Stops the signal meter
//
// Arguments:
//
//	NONE

void fmmeter::stop(void)
{
	m_stop = true;								// Signal worker thread to stop
	if(m_device) m_device->cancel_async();		// Cancel any async read operations
	if(m_worker.joinable()) m_worker.join();	// Wait for thread
	m_stop = false;								// Reset stop signal
}

//---------------------------------------------------------------------------

#pragma warning(pop)
