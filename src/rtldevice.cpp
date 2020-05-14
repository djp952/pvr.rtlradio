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
#include "rtldevice.h"

#include <assert.h>
#include <cmath>

#include "libusb_exception.h"
#include "string_exception.h"

#pragma warning(push, 4)

// rtldevice::AUTOMATIC_BANDWIDTH (static)
//
// Specifies that automatic bandwidth selection should be used
uint32_t const rtldevice::AUTOMATIC_BANDWIDTH = 0;

// rtldevice::DEFAULT_DEVICE_INDEX (static)
//
// Default device index value
uint32_t const rtldevice::DEFAULT_DEVICE_INDEX = 0;

//---------------------------------------------------------------------------
// rtldevice Constructor (private)
//
// Arguments:
//
//	index		- Device index

rtldevice::rtldevice(uint32_t index)
{
	char		manufacturer[256] = { '\0' };		// Manufacturer string
	char		product[256] = { '\0' };			// Product string
	char		serialnumber[256] = { '\0' };		// Serial number string

	// Make sure that the specified index is going to correspond with an actual device
	uint32_t devicecount = rtlsdr_get_device_count();
	if(index >= devicecount) throw string_exception(__func__, ": invalid RTL-SDR device index");

	// Attempt to open the RTL-SDR device with the specified index
	int result = rtlsdr_open(&m_device, index);
	if(result < 0) throw string_exception(__func__, ": unable to open RTL-SDR device with index ", index);

	// Retrieve the name of the device with the specified index
	char const* name = rtlsdr_get_device_name(index);
	if(name != nullptr) m_name.assign(name);

	try {

		// The device type must not be UNKNOWN otherwise it won't work properly
		enum rtlsdr_tuner type = rtlsdr_get_tuner_type(m_device);
		if(type == rtlsdr_tuner::RTLSDR_TUNER_UNKNOWN) throw string_exception(__func__, ": RTL-SDR device tuner type is unknown");

		// Attempt to retrieve the USB strings for the connected device
		result = rtlsdr_get_usb_strings(m_device, manufacturer, product, serialnumber);
		if(result == 0) {

			m_manufacturer.assign(manufacturer);
			m_product.assign(product);
			m_serialnumber.assign(serialnumber);
		}
	}

	// Close the RTL-SDR device on any thrown exception
	catch(...) { rtlsdr_close(m_device); m_device = nullptr; throw; }
}
	
//---------------------------------------------------------------------------
// rtldevice Destructor

rtldevice::~rtldevice()
{
	if(m_device != nullptr) rtlsdr_close(m_device);
}

//---------------------------------------------------------------------------
// rtldevice::agcmode
//
// Enables/disables the automatic gain control mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void rtldevice::agcmode(bool enable) const
{
	assert(m_device != nullptr);

	rtlsdr_set_agc_mode(m_device, (enable) ? 1 : 0);
}

//---------------------------------------------------------------------------
// rtldevice::bandwidth
//
// Sets the bandwidth of the device
//
// Arguments:
//
//	hz		- Bandwidth to set, specified in hertz

void rtldevice::bandwidth(uint32_t hz) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_tuner_bandwidth(m_device, hz);
	if(result < 0) throw string_exception(__func__, ": failed to set device bandwidth to ", hz, "Hz");
}

//---------------------------------------------------------------------------
// rtldevice::create (static)
//
// Factory method, creates a new rtldevice instance
//
// Arguments:
//
//	NONE

std::unique_ptr<rtldevice> rtldevice::create(void)
{
	return create(DEFAULT_DEVICE_INDEX);
}

//---------------------------------------------------------------------------
// rtldevice::create (static)
//
// Factory method, creates a new rtldevice instance
//
// Arguments:
//
//	index		- Device index

std::unique_ptr<rtldevice> rtldevice::create(uint32_t index)
{
	return std::unique_ptr<rtldevice>(new rtldevice(index));
}

//---------------------------------------------------------------------------
// rtldevice::frequency
//
// Gets the center frequency of the device
//
// Arguments:
//
//	NONE

uint32_t rtldevice::frequency(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_center_freq(m_device);
}

//---------------------------------------------------------------------------
// rtldevice::frequency
//
// Sets the center frequency of the device
//
// Arguments:
//
//	hz		- Frequency to set, specified in hertz

void rtldevice::frequency(uint32_t hz) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_center_freq(m_device, hz);
	if(result < 0) throw string_exception(__func__, ": failed to set device frequency to ", hz, "Hz");
}

//---------------------------------------------------------------------------
// rtldevice::frequencycorrection
//
// Gets the frequency correction of the device
//
// Arguments:
//
//	NONE

uint32_t rtldevice::frequencycorrection(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_freq_correction(m_device);
}

//---------------------------------------------------------------------------
// rtldevice::frequencycorrection
//
// Sets the frequency correction of the device
//
// Arguments:
//
//	ppm		- Frequency correction to set, specified in parts per million

void rtldevice::frequencycorrection(int ppm) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_freq_correction(m_device, ppm);
	if(result < 0) throw string_exception(__func__, ": failed to set device frequency correction to ", ppm, "ppm");
}

//---------------------------------------------------------------------------
// rtldevice::gain
//
// Gets the gain of the device
//
// Arguments:
//
//	NONE

int rtldevice::gain(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_tuner_gain(m_device);
}

//---------------------------------------------------------------------------
// rtldevice::gain
//
// Sets the gain of the device
//
// Arguments:
//
//	db			- Gain to set, specified in tenths of a decibel

void rtldevice::gain(int db) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_tuner_gain(m_device, db);
	if(result < 0) throw string_exception(__func__, ": failed to set device gain to ", db, "dB/10");
}

//---------------------------------------------------------------------------
// rtldevice::manufacturer
//
// Gets the manufacturer name of the device
//
// Arguments:
//
//	NONE

char const* rtldevice::manufacturer(void) const
{
	return m_manufacturer.c_str();
}

//---------------------------------------------------------------------------
// rtldevice::name
//
// Gets the name of the device
//
// Arguments:
//
//	NONE

char const* rtldevice::name(void) const
{
	return m_name.c_str();
}

//---------------------------------------------------------------------------
// rtldevice::product
//
// Gets the product name of the device
//
// Arguments:
//
//	NONE

char const* rtldevice::product(void) const
{
	return m_product.c_str();
}

//---------------------------------------------------------------------------
// rtldevice::read
//
// Reads data from the device
//
// Arguments:
//
//	buffer		- Buffer to receive the data
//	count		- Size of the destination buffer, specified in bytes

size_t rtldevice::read(uint8_t* buffer, size_t count) const
{
	int			bytesread = 0;			// Bytes read from the device

	assert(m_device != nullptr);

	// rtlsdr_read_sync returns the underlying libusb error code when it fails
	int result = rtlsdr_read_sync(m_device, buffer, static_cast<int>(count), &bytesread);
	if(result < 0) throw string_exception(__func__, ": ", libusb_exception(result).what());

	return static_cast<size_t>(bytesread);
}

//---------------------------------------------------------------------------
// rtldevice::samplerate
//
// Gets the sample rate of the device
//
// Arguments:
//
//	NONE

uint32_t rtldevice::samplerate(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_sample_rate(m_device);
}

//---------------------------------------------------------------------------
// rtldevice::samplerate
//
// Sets the sample rate of the device
//
// Arguments:
//
//	hz		- Sample rate to set, specified in hertz

void rtldevice::samplerate(uint32_t hz) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_sample_rate(m_device, hz);
	if(result < 0) throw string_exception(__func__, ": failed to set device sample rate to ", hz, "Hz");
}

//---------------------------------------------------------------------------
// rtldevice::serialnumber
//
// Gets the serial number of the device
//
// Arguments:
//
//	NONE

char const* rtldevice::serialnumber(void) const
{
	return m_serialnumber.c_str();
}

//---------------------------------------------------------------------------
// rtldevice::stream
//
// Starts streaming data from the device
//
// Arguments:
//
//	NONE

void rtldevice::stream(void) const
{
	assert(m_device != nullptr);

	// Reset the device buffer to start the streaming interface
	int result = rtlsdr_reset_buffer(m_device);
	if(result < 0) throw string_exception(__func__, ": unable to reset RTL-SDR device buffer");
}

//---------------------------------------------------------------------------
// rtldevice::testmode
//
// Enables/disables the test mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void rtldevice::testmode(bool enable) const
{
	assert(m_device != nullptr);

	rtlsdr_set_testmode(m_device, (enable) ? 1 : 0);
}

//---------------------------------------------------------------------------
// rtldevice::validgains
//
// Gets the valid tuner gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void rtldevice::validgains(std::vector<int>& dbs) const
{
	assert(m_device != nullptr);

	dbs.clear();

	// Determine the number of valid gain values for the tuner device
	int numgains = rtlsdr_get_tuner_gains(m_device, nullptr);
	if(numgains < 0) throw string_exception(__func__, ": unable to determine valid tuner gain values");
	else if(numgains == 0) return;

	// Reallocate the vector<> to hold all of the values and execute the operation again
	dbs.resize(numgains);
	if(rtlsdr_get_tuner_gains(m_device, dbs.data()) != numgains)
		throw string_exception(__func__, ": size mismatch reading valid tuner gain values");
}

//---------------------------------------------------------------------------

#pragma warning(pop)
