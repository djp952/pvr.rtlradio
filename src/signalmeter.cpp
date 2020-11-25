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
#include <cmath>
#include <string.h>

#include "align.h"

#pragma warning(push, 4)

// signalmeter::DEFAULT_DEVICE_FREQUENCY
//
// Default device frequency
uint32_t const signalmeter::DEFAULT_DEVICE_FREQUENCY = (87900 KHz);		// 87.9 MHz

// signalmeter::DEFAULT_DEVICE_SAMPLE_RATE
//
// Default device sample rate
uint32_t const signalmeter::DEFAULT_DEVICE_SAMPLE_RATE = (1 MHz);

// signalmeter::FFT_BIN_SIZE
//
// Fast fourier transform size (must be a power of two)
int const signalmeter::FFT_BIN_SIZE = 4096;

// FFT_OUTPUT_HEIGHT
//
// Fast fourier transform output height (y-axis)
int const signalmeter::FFT_OUTPUT_HEIGHT = 512;

// FFT_OUTPUT_WIDTH
//
// Fast fourier transform output width (x-axis)
int const signalmeter::FFT_OUTPUT_WIDTH = 512;

//---------------------------------------------------------------------------
// signalmeter Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//	onstatus		- Signal status callback function
//	statusrate		- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

signalmeter::signalmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, signal_status_callback const& onstatus, 
	int statusrate, exception_callback const& onexception) : m_device(std::move(device)), m_onstatus(onstatus), 
	m_onstatusrate(std::min(statusrate / 10, 10)), m_onexception(onexception)
{
	// Disable automatic gain control on the device by default
	m_device->set_automatic_gain_control(false);

	// Set the default frequency, sample rate, and frequency correction offset
	m_device->set_center_frequency(DEFAULT_DEVICE_FREQUENCY);
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
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)

std::unique_ptr<signalmeter> signalmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	signal_status_callback const& onstatus, int onstatusrate)
{
	auto onexception = [](std::exception const&) -> void { /* DO NOTHING */ };
	return std::unique_ptr<signalmeter>(new signalmeter(std::move(device), tunerprops, onstatus, onstatusrate, onexception));
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
//	onstatus		- Signal status callback function
//	onstatusrate	- Rate at which the status callback will be invoked (milliseconds)
//	onexception		- Exception callback function

std::unique_ptr<signalmeter> signalmeter::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	signal_status_callback const& onstatus, int onstatusrate, exception_callback const& onexception)
{
	return std::unique_ptr<signalmeter>(new signalmeter(std::move(device), tunerprops, onstatus, onstatusrate, onexception));
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
// signalmeter::rollingaverage (private, static)
//
// Computes a rolling average of a value over a number of iterations
//
// Arguments:
//
//	iterations		- Number of iterations for the average
//	average			- The current averaged value
//	input			- The new input to apply to the average

TYPEREAL signalmeter::rollingaverage(int iterations, TYPEREAL average, TYPEREAL input)
{
	average -= average / iterations;
	average += input / iterations;

	return average;
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

		started = true;					// Signal that the thread has started

		try {

			// The sampling rate for the signal meter isn't adjustable
			uint32_t const samplerate = DEFAULT_DEVICE_SAMPLE_RATE;

			// Create and initialize the wideband FM demodulator instance
			std::unique_ptr<CDemodulator> demodulator(new CDemodulator()); 
			tDemodInfo demodinfo = {};
			demodinfo.txt.assign("WFM");
			demodinfo.DownsampleQuality = DownsampleQuality::Low;
			demodulator->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
			demodulator->SetDemod(DEMOD_WFM, demodinfo);

			size_t const numsamples = demodulator->GetInputBufferLimit();
			size_t const numbytes = numsamples * 2;

			// The signal levels are averaged based on the callback rate
			TYPEREAL avgpower = std::numeric_limits<TYPEREAL>::quiet_NaN();
			TYPEREAL avgnoise = std::numeric_limits<TYPEREAL>::quiet_NaN();
			TYPEREAL avgsnr = std::numeric_limits<TYPEREAL>::quiet_NaN();

			bool stereo = false;			// Flag if stereo has been detected
			bool rds = false;				// Flag if RDS has been detected

			// Create and initialize the fast fourtier transform instance
			size_t const fftsize = FFT_BIN_SIZE;
			std::unique_ptr<CFft> fft(new CFft());
			fft->SetFFTParams(fftsize, false, 0, static_cast<TYPEREAL>(samplerate));

			// Create the input buffers to hold the raw and converted I/Q samples read from the device
			std::unique_ptr<uint8_t[]> buffer(new uint8_t[numbytes]);
			std::unique_ptr<TYPECPX[]> samples(new TYPECPX[numsamples]);

			// Create and initialize the output buffer for the fast fourier transform
			std::unique_ptr<qint32[]> fftdata(new qint32[FFT_OUTPUT_WIDTH + 1]);
			memset(&fftdata[0], 0, sizeof(qint32) * (FFT_OUTPUT_WIDTH + 1));
			
			size_t fftoffset = 0;				// Offset into the FFT output buffer
			int iterations = 0;					// Counter for callback loop iterations

			m_device->begin_stream();			// Begin streaming from the device

			// Loop until the worker thread has been signaled to stop
			while(m_stop.test(false) == true) {

				// If the frequency changed everything needs to be reset
				bool expected = true;
				if(m_freqchange.compare_exchange_strong(expected, false)) {

					avgpower = std::numeric_limits<TYPEREAL>::quiet_NaN();
					avgnoise = std::numeric_limits<TYPEREAL>::quiet_NaN();
					avgsnr = std::numeric_limits<TYPEREAL>::quiet_NaN();

					stereo = false;
					rds = false;

					fft->ResetFFT();
					memset(&fftdata[0], 0, sizeof(qint32) * (FFT_OUTPUT_WIDTH + 1));
				}

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

				// *** TODO: Clean this mess up or make a helper class
				// Perform the fast fourier transform on the raw I/Q samples
				size_t samplesoffset = 0;

				// If there was leftover data from the last iteration, load the remaining data into the FFT
				if(fftoffset > 0) {

					size_t count = fftsize - fftoffset;
					fft->PutInDisplayFFT(static_cast<qint32>(count), &samples[0]);
					samplesoffset += count;
					fftoffset = 0;
				}

				size_t fftchunks = align::down(numsamples - samplesoffset, fftsize) / fftsize;
				while(fftchunks--) {

					fft->PutInDisplayFFT(fftsize, &samples[samplesoffset]);
					samplesoffset += fftsize;
				}

				fft->GetScreenIntegerFFTData(FFT_OUTPUT_HEIGHT, FFT_OUTPUT_WIDTH, 0.0, -64.0, -100 KHz, 100 KHz, &fftdata[0]);

				if(samplesoffset < numsamples) {

					size_t count = numsamples - samplesoffset;
					assert(count < fftsize);

					fft->PutInDisplayFFT(static_cast<qint32>(count), &samples[samplesoffset]);
					fftoffset = count;
				}
				// ***

				// Now run the I/Q samples through the demodulator
				demodulator->ProcessData(static_cast<int>(numsamples), samples.get(), samples.get());

				// Get the signal levels from the demodulator
				TYPEREAL power = demodulator->GetBasebandLevel();
				TYPEREAL noise = demodulator->GetNoiseLevel();
				TYPEREAL snr = demodulator->GetSignalToNoiseLevel();

				// Determine if there is a stereo lock
				if(demodulator->GetStereoLock(nullptr)) stereo = true;

				// Pull out any RDS group data that was collected during demodulation
				tRDS_GROUPS rdsgroup = {};
				while(demodulator->GetNextRdsGroupData(&rdsgroup)) { rds = true; }

				// Apply a rolling average on the values based on the callback rate
				avgpower = (std::isnan(avgpower)) ? power : rollingaverage(m_onstatusrate, avgpower, power);
				avgnoise = (std::isnan(avgnoise)) ? noise : rollingaverage(m_onstatusrate, avgnoise, noise);
				avgsnr = (std::isnan(avgsnr)) ? snr : rollingaverage(m_onstatusrate, avgsnr, snr);

				// Only invoke the callback at roughly the requested rate
				if((++iterations % m_onstatusrate) == 0) {

					struct signal_status status = {};
					status.power = avgpower;
					status.noise = avgnoise;
					status.snr = avgsnr;
					status.stereo = stereo;
					status.rds = rds;

					status.fftheight = FFT_OUTPUT_HEIGHT;
					status.fftwidth = FFT_OUTPUT_WIDTH;
					status.fftdata = fftdata.get();

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
