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

#include "fmdsp/demodulator.h"

#include "align.h"
#include "rdsdecoder.h"

#pragma warning(push, 4)

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
//	isrbds			- Flag indicating if RBDS should be used

scanner::scanner(std::unique_ptr<rtldevice> device, bool isrbds) : 
	m_device(std::move(device)), m_isrbds(isrbds)
{
	// Disable automatic gain control by default
	m_device->set_automatic_gain_control(false);

	// Set the manual gain to the lowest value by default
	std::vector<int> gains;
	m_device->get_valid_gains(gains);
	m_manualgain = (gains.size()) ? gains[0] : 0;
	m_device->set_gain(m_manualgain);
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
//	isrbds			- Flag indicating if RBDS should be used

std::unique_ptr<scanner> scanner::create(std::unique_ptr<rtldevice> device, bool isrbds)
{
	return std::unique_ptr<scanner>(new scanner(std::move(device), isrbds));
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
	assert(m_device);
	return m_device->get_valid_gains(dbs);
}

//---------------------------------------------------------------------------
// scanner::set_channel
//
// Sets the channel to be tuned
//
// Arguments:
//
//	frequency		- Frequency to set, specified in hertz

void scanner::set_channel(uint32_t frequency)
{
	assert(m_device);

	stop();						// Stop any scanning already in progress

	// Start a new scanning thread at the requested frequency
	scalar_condition<bool> started{ false };
	m_worker = std::thread(&scanner::start, this, frequency, DEFAULT_DEVICE_SAMPLE_RATE, m_isrbds, std::ref(started));
	started.wait_until_equals(true);

	m_frequency = frequency;
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
	m_autogain = autogain;
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
	assert(m_device);

	m_manualgain = m_device->set_gain(manualgain);
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
// scanner::start (private)
//
// Worker thread procedure used to scan the channel information
//
// Arguments:
//
//	frequency	- Frequency to be tuned, in Hertz
//	samplerate	- Sample rate of the RTL-SDR device
//	isrdbs		- Flag to expect RDBS or RDS data
//	started		- Condition variable to set when thread has started

void scanner::start(uint32_t frequency, uint32_t samplerate, bool isrbds, scalar_condition<bool>& started)
{
	std::unique_ptr<CDemodulator>	demodulator;			// FM demodulator instance
	rdsdecoder						rdsdecoder(isrbds);		// RDS decoder instance

	assert(m_device);

	// Set the sample rate and adjust the frequency to apply the DC offset
	uint32_t rate = m_device->set_sample_rate(samplerate);
	uint32_t hz = m_device->set_center_frequency(frequency + (rate / 4));

	// Initialize the demodulator parameters
	tDemodInfo demodinfo = {};

	// FIXED DEMODULATOR SETTINGS
	//
	demodinfo.txt.assign("WFM");
	demodinfo.HiCutmin = 100000;						// Not used by demodulator
	demodinfo.HiCutmax = 100000;
	demodinfo.LowCutmax = -100000;						// Not used by demodulator
	demodinfo.LowCutmin = -100000;
	demodinfo.Symetric = true;							// Not used by demodulator
	demodinfo.DefFreqClickResolution = 100000;			// Not used by demodulator
	demodinfo.FilterClickResolution = 10000;			// Not used by demodulator

	// VARIABLE DEMODULATOR SETTINGS
	//
	demodinfo.HiCut = 5000;								// Not used by demodulator
	demodinfo.LowCut = -5000;							// Not used by demodulator
	demodinfo.FreqClickResolution = 100000;				// Not used by demodulator
	demodinfo.Offset = 0;
	demodinfo.SquelchValue = -160;						// Not used by demodulator
	demodinfo.AgcSlope = 0;								// Not used by demodulator
	demodinfo.AgcThresh = -100;							// Not used by demodulator
	demodinfo.AgcManualGain = 30;						// Not used by demodulator
	demodinfo.AgcDecay = 200;							// Not used by demodulator
	demodinfo.AgcOn = false;							// Not used by demodulator
	demodinfo.AgcHangOn = false;						// Not used by demodulator

	// Initialize the wideband FM demodulator
	demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
	demodulator->SetUSFmVersion(isrbds);
	demodulator->SetInputSampleRate(static_cast<TYPEREAL>(rate));
	demodulator->SetDemod(DEMOD_WFM, demodinfo);
	demodulator->SetDemodFreq(static_cast<TYPEREAL>(hz - frequency));

	// The demodulator only works properly in this use pattern if it's fed the exact
	// number of input bytes it's looking for.  Excess bytes will be discarded, and
	// an insufficient number of bytes won't generate any output samples
	size_t const buffersize = demodulator->GetInputBufferLimit() * 2;
	std::unique_ptr<uint8_t[]> buffer(new uint8_t[buffersize]);

	// Create a heap array in which to hold the converted I/Q samples
	size_t const numsamples = (buffersize / 2);
	std::unique_ptr<TYPECPX[]> samples(new TYPECPX[numsamples]);

	m_device->begin_stream();		// Begin streaming from the device
	started = true;					// Signal that the thread has started

	// Loop until the worker thread has been signaled to stop
	while(m_stop.test(false) == true) {

		// Read the next chunk of raw data from the device, discard short reads
		size_t read = m_device->read(&buffer[0], buffersize);
		if(read < buffersize) continue;

		if(m_stop.test(true) == true) continue;			// Watch for stop

		for(size_t index = 0; index < numsamples; index += 2) {

			// The demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
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

		// Process the I/Q data, the original samples buffer can be reused/overwritten
		demodulator->ProcessData(static_cast<int>(numsamples), samples.get(), samples.get());

		// Process any RDS group data that was collected during demodulation
		tRDS_GROUPS rdsgroup = {};
		while(demodulator->GetNextRdsGroupData(&rdsgroup)) rdsdecoder.decode_rdsgroup(rdsgroup);

		if(m_stop.test(true) == true) continue;			// Watch for stop

		//
		// TODO: Set the updated state variables, when they exist
		//
	}

	m_stopped.store(true);					// Thread is stopped
}

//---------------------------------------------------------------------------

#pragma warning(pop)
