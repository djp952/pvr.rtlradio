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
#include "fmmeter.h"

#include <algorithm>
#include <cmath>
#include <string.h>

#include "align.h"
#include "fmdsp/fft.h"

#pragma warning(push, 4)

// fmmeter::MAX_SAMPLE_QUEUE
//
// Fixed input sample rate for the meter
uint32_t const fmmeter::INPUT_SAMPLE_RATE = 1600 KHz;

//---------------------------------------------------------------------------
// fmmeter Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//  frequency		- Center frequency to be tuned for the channel
//  modulation		- Modulation of the channel to be analyzed
//	fftwidth		- Bandwidth of the FFT data to be generated
//	onstatus		- Signal status callback function
//	statusrate		- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

fmmeter::fmmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, uint32_t frequency,
	enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus, int statusrate, 
	exception_callback const& onexception) : m_device(std::move(device)), m_tunerprops(tunerprops), 
	m_frequency(frequency), m_modulation(modulation), m_fftwidth(fftwidth), m_onstatus(onstatus), 
	m_onstatusrate(std::min(statusrate / 10, 10)), m_onexception(onexception)
{
	// Set the default frequency, sample rate, and frequency correction offset
	m_device->set_center_frequency(m_frequency);
	m_device->set_sample_rate(INPUT_SAMPLE_RATE);
	m_correction = m_device->set_frequency_correction(tunerprops.freqcorrection);

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
//  modulation		- Modulation of the channel to be anayzed
//	fftwidth		- Bandwidth of the FFT data to be generated
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)

std::unique_ptr<fmmeter> fmmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	uint32_t frequency, enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus, int onstatusrate)
{
	auto onexception = [](std::exception const&) -> void { /* DO NOTHING */ };
	return std::unique_ptr<fmmeter>(new fmmeter(std::move(device), tunerprops, frequency, modulation, fftwidth, onstatus, onstatusrate, onexception));
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
//  modulation		- Modulation of the channel to be anayzed
//	fftwidth		- Bandwidth of the FFT data to be generated
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

std::unique_ptr<fmmeter> fmmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	uint32_t frequency, enum modulation modulation, uint32_t fftwidth, signal_status_callback const& onstatus, int onstatusrate, exception_callback const& onexception)
{
	return std::unique_ptr<fmmeter>(new fmmeter(std::move(device), tunerprops, frequency, modulation, fftwidth, onstatus, onstatusrate, onexception));
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
// fmmeter::get_frequency_correction
//
// Gets the frequency correction value
//
// Arguments:
//
//	NONE

int fmmeter::get_frequency_correction(void) const
{
	return m_correction;
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
// fmmeter::get_modulation
//
// Gets the currently set modulation type
//
// Arguments:
//
//	NONE

enum modulation fmmeter::get_modulation(void) const
{
	return m_modulation.load();
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
// fmmeter::set_frequency_correction
//
// Sets the manual gain value of the device
//
// Arguments:
//
//	ppm		- New frequency correction, in parts per million

void fmmeter::set_frequency_correction(int ppm)
{
	m_correction = m_device->set_frequency_correction(ppm);
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
// fmmeter::set_modulation
//
// Sets the modulation type of the channel being analyzed
//
// Arguments:
//
//	modulation		- Modulation type of the channel

void fmmeter::set_modulation(enum modulation modulation)
{
	m_modulation.store(modulation);
}

//---------------------------------------------------------------------------
// fmmeter::start
//
// Starts the signal meter
//
// Arguments:
//
//	NONE

void fmmeter::start(TYPEREAL maxdb, TYPEREAL mindb, size_t height, size_t width)
{
	scalar_condition<bool>		started{ false };		// Thread start condition variable

	stop();						// Stop the signal meter if it's already running

	// Define and launch the I/Q signal meter thread
	m_worker = std::thread([&](TYPEREAL maxdb, TYPEREAL mindb, size_t height, size_t width) -> void {

		CFft			fft;				// Fast fourier transform instance

		// The FFT size (number of bins) needs to be a power of two equal to or larger
		// than the output width; start at 512 and increase until we find that
		size_t fftsize = 512;
		while(width > fftsize) fftsize <<= 1;

		// Initialize the fast fourier transform instance
		fft.SetFFTParams(static_cast<int>(fftsize), false, 0.0, INPUT_SAMPLE_RATE);
		fft.SetFFTAve(10);

		started = true;					// Signal that the thread has started

		try {

			TYPEREAL avgpower = NAN;			// Average power level
			TYPEREAL avgnoise = NAN;			// Average noise level

			// Grab around 16K samples from the stream at a time, align to fftsize
			size_t numsamples = align::up(16 KiB, static_cast<unsigned int>(fftsize));
			size_t const numbytes = numsamples * 2;

			// Create all of the required data buffers
			std::unique_ptr<uint8_t[]>	buffer(new uint8_t[numbytes]);
			std::unique_ptr<TYPECPX[]>	samples(new TYPECPX[numsamples]);
			std::unique_ptr<int[]>		fftbuffer(new int[width + 1]);

			// TODO: see below, this is a lame way to do this
			int iterations = 0;					// Counter for callback loop iterations

			m_device->begin_stream();			// Begin streaming from the device

			// Loop until the worker thread has been signaled to stop
			while(m_stop.test(false) == true) {

				// The currently set modulation affects how the signal is analyzed
				enum modulation modulation = m_modulation.load();
				uint32_t bandwidth = (modulation == modulation::wx) ? 10 KHz : 200 KHz;

				// Read the next block of raw 8-bit I/Q samples from the input device
				size_t read = 0;
				while(read < numbytes) read += m_device->read(&buffer[read], numbytes - read);

				// Only invoke the callback at roughly the requested rate
				// TODO: This can be made to be a set amount of time, this way is cheesy
				if((++iterations % m_onstatusrate) == 0) {
					
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

					// Put the I/Q samples into the fast fourier transform instance (numsamples is aligned to fftsize)
					size_t index = 0;
					while(index < numsamples) {

						fft.PutInDisplayFFT(static_cast<qint32>(fftsize), &samples[index]);
						index += fftsize;
					}

					// Convert the captured FFT data into bitmapped points that can be measured/displayed
					bool overload = fft.GetScreenIntegerFFTData(static_cast<qint32>(height), static_cast<qint32>(width), maxdb, mindb,
						-(static_cast<int32_t>(m_fftwidth) / 2), (static_cast<int32_t>(m_fftwidth) / 2), fftbuffer.get());

					// Use the FFT data to calculate the power and noise levels
					TYPEREAL const hz = static_cast<TYPEREAL>(width) / static_cast<TYPEREAL>(m_fftwidth);
					TYPEREAL const center = (static_cast<TYPEREAL>(width) / 2.0);
					
					// Power = peak amplitude within 1KHz of the center frequency
					size_t power = height;
					size_t const powerlowcut = static_cast<size_t>(center - (hz * 500));	// -500 Hz
					size_t const powerhighcut = static_cast<size_t>(center + (hz * 500));	// +500 Hz
					
					for(index = powerlowcut; index < powerhighcut; index++) {

						size_t val = fftbuffer[index];
						if(val < power) power = val;			// FFT: 0 = MAX, height = MIN
					}

					// Noise = average amplitude of 5Khz below the low cut and 5KHz above the high cut
					TYPEREAL noise = 0.0;
					size_t const lowcut = static_cast<size_t>(center - (static_cast<TYPEREAL>(bandwidth / 2) * hz));
					size_t const highcut = static_cast<size_t>(center + (static_cast<TYPEREAL>(bandwidth / 2) * hz));
					size_t const fivekhz = static_cast<size_t>(hz * 5.0 KHz);

					for(index = (lowcut - fivekhz); index < lowcut; index++) noise += static_cast<TYPEREAL>(fftbuffer[index]);
					for(index = highcut; index < (highcut + fivekhz); index++) noise += static_cast<TYPEREAL>(fftbuffer[index]);
					noise /= static_cast<TYPEREAL>(fivekhz * 2);

					// Convert the power and noise into decibels based on the FFT range
					TYPEREAL const powerdb = ((mindb - maxdb) * (static_cast<TYPEREAL>(power) / static_cast<TYPEREAL>(height))) + maxdb;
					TYPEREAL const noisedb = ((mindb - maxdb) * (noise / static_cast<TYPEREAL>(height))) + maxdb;

					// Smooth the power and noise levels out a bit
					avgpower = (std::isnan(avgpower)) ? powerdb : 0.85 * avgpower + 0.15 * powerdb;
					avgnoise = (std::isnan(avgnoise)) ? noisedb : 0.85 * avgnoise + 0.15 * noisedb;

					// Send all of the calculated information into the status callback
					struct signal_status status = {};

					status.modulation = modulation;								// Channel modulation
					status.power = static_cast<float>(avgpower);				// Power in dB
					status.noise = static_cast<float>(avgnoise);				// Noise in dB
					status.snr = static_cast<float>(avgpower + -avgnoise);		// SNR in dB
					status.overload = overload;									// FFT is clipped
					status.fftbandwidth = m_fftwidth;							// Overall FFT bandwidth
					status.ffthighcut = (bandwidth / 2);						// High cut bandwidth
					status.fftlowcut = -status.ffthighcut;						// Low cut bandwidth
					status.fftsize = width;										// Length of the FFT graph data
					status.fftdata = fftbuffer.get();							// FFT graph data

					if(m_onstatus) m_onstatus(status);
					iterations = 0;
				}
			}
		}

		// Pass any thrown exception into the callback, if one has been specified
		catch(std::exception const& ex) { if(m_onexception) m_onexception(ex); }
		
		m_stopped.store(true);			// Thread is stopped
	
	}, maxdb, mindb, height, width);

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
