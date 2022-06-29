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
#include "usbdevice.h"

#include <assert.h>
#include <cmath>

#include "libusb_exception.h"
#include "string_exception.h"

#ifdef __ANDROID__
#include <libusb.h>
#include "kodi/platform/android/System.h"
#endif

#pragma warning(push, 4)

// usbdevice::DEFAULT_DEVICE_INDEX (static)
//
// Default device index value
uint32_t const usbdevice::DEFAULT_DEVICE_INDEX = 0;

//---------------------------------------------------------------------------
// usbdevice Constructor (private)
//
// Arguments:
//
//	index		- Device index

usbdevice::usbdevice(uint32_t index)
{
	char		manufacturer[256] = { '\0' };		// Manufacturer string
	char		product[256] = { '\0' };			// Product string
	char		serialnumber[256] = { '\0' };		// Serial number string

#ifdef __ANDROID__
	kodi::platform::CInterfaceAndroidSystem system;
	libusb_set_option(nullptr, LIBUSB_OPTION_ANDROID_JNIENV, system.GetJNIEnv());
#endif
	
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

		// Turn off internal digital automatic gain control
		result = rtlsdr_set_agc_mode(m_device, 0);
		if (result < 0) throw string_exception(__func__, ": failed to set digital automatic gain control to off");
	}

	// Close the RTL-SDR device on any thrown exception
	catch(...) { rtlsdr_close(m_device); m_device = nullptr; throw; }
}

//---------------------------------------------------------------------------
// usbdevice Destructor

usbdevice::~usbdevice()
{
	if(m_device != nullptr) rtlsdr_close(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::begin_stream
//
// Starts streaming data from the device
//
// Arguments:
//
//	NONE

void usbdevice::begin_stream(void) const
{
	assert(m_device != nullptr);

	// Reset the device buffer to start the streaming interface
	int result = rtlsdr_reset_buffer(m_device);
	if(result < 0) throw string_exception(__func__, ": unable to reset RTL-SDR device buffer");
}

//---------------------------------------------------------------------------
// usbdevice::cancel_async
//
// Cancels any pending asynchronous read operations from the device
//
// Arguments:
//
//	NONE

void usbdevice::cancel_async(void) const
{
	assert(m_device != nullptr);

	rtlsdr_cancel_async(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::create (static)
//
// Factory method, creates a new usbdevice instance
//
// Arguments:
//
//	NONE

std::unique_ptr<usbdevice> usbdevice::create(void)
{
	return create(DEFAULT_DEVICE_INDEX);
}

//---------------------------------------------------------------------------
// usbdevice::create (static)
//
// Factory method, creates a new usbdevice instance
//
// Arguments:
//
//	index		- Device index

std::unique_ptr<usbdevice> usbdevice::create(uint32_t index)
{
	return std::unique_ptr<usbdevice>(new usbdevice(index));
}

//---------------------------------------------------------------------------
// usbdevice::get_center_frequency
//
// Gets the center frequency of the device
//
// Arguments:
//
//	NONE

uint32_t usbdevice::get_center_frequency(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_center_freq(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::get_device_name
//
// Gets the name of the device
//
// Arguments:
//
//	NONE

char const* usbdevice::get_device_name(void) const
{
	return m_name.c_str();
}

//---------------------------------------------------------------------------
// usbdevice::get_frequency_correction
//
// Gets the frequency correction of the device
//
// Arguments:
//
//	NONE

int usbdevice::get_frequency_correction(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_freq_correction(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::get_gain
//
// Gets the gain of the device
//
// Arguments:
//
//	NONE

int usbdevice::get_gain(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_tuner_gain(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::get_manufacturer_name
//
// Gets the manufacturer name of the device
//
// Arguments:
//
//	NONE

char const* usbdevice::get_manufacturer_name(void) const
{
	return m_manufacturer.c_str();
}

//---------------------------------------------------------------------------
// usbdevice::get_product_name
//
// Gets the product name of the device
//
// Arguments:
//
//	NONE

char const* usbdevice::get_product_name(void) const
{
	return m_product.c_str();
}

//---------------------------------------------------------------------------
// usbdevice::get_sample_rate
//
// Gets the sample rate of the device
//
// Arguments:
//
//	NONE

uint32_t usbdevice::get_sample_rate(void) const
{
	assert(m_device != nullptr);

	return rtlsdr_get_sample_rate(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::get_serial_number
//
// Gets the serial number of the device
//
// Arguments:
//
//	NONE

char const* usbdevice::get_serial_number(void) const
{
	return m_serialnumber.c_str();
}

//---------------------------------------------------------------------------
// usbdevice::get_valid_gains
//
// Gets the valid tuner gain values for the device
//
// Arguments:
//
//	dbs			- vector<> to retrieve the valid gain values

void usbdevice::get_valid_gains(std::vector<int>& dbs) const
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
// usbdevice::read
//
// Reads data from the device
//
// Arguments:
//
//	buffer		- Buffer to receive the data
//	count		- Size of the destination buffer, specified in bytes

size_t usbdevice::read(uint8_t* buffer, size_t count) const
{
	int			bytesread = 0;			// Bytes read from the device

	assert(m_device != nullptr);

	// rtlsdr_read_sync returns the underlying libusb error code when it fails
	int result = rtlsdr_read_sync(m_device, buffer, static_cast<int>(count), &bytesread);
	if(result < 0) throw string_exception(__func__, ": ", libusb_exception(result).what());

	return static_cast<size_t>(bytesread);
}

//---------------------------------------------------------------------------
// usbdevice::read_async
//
// Asynchronously reads data from the device
//
// Arguments:
//
//	callback		- Asynchronous read callback function
//	bufferlength	- Output buffer length in bytes

void usbdevice::read_async(rtldevice::asynccallback const& callback, uint32_t bufferlength) const
{
	assert(m_device != nullptr);

	// rtlsdr_read_async_cb_t callback conversion function
	auto callreadfunc = [](unsigned char* buf, uint32_t len, void* ctx) -> void {

		asynccallback const* func = reinterpret_cast<asynccallback const*>(ctx);
		if(func) (*func)(reinterpret_cast<uint8_t const*>(buf), static_cast<size_t>(len));
	};

	// Get the address of the callback std::function<> to pass as a context pointer
	void const* pcallback = std::addressof(callback);

	// rtlsdr_read_async returns the underlying libusb error code when it fails
	int result = rtlsdr_read_async(m_device, callreadfunc, const_cast<void*>(pcallback), 0, bufferlength);
	if(result < 0) throw string_exception(__func__, ": ", libusb_exception(result).what());
}

//---------------------------------------------------------------------------
// usbdevice::set_automatic_gain_control
//
// Enables/disables the automatic gain control mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void usbdevice::set_automatic_gain_control(bool enable) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_tuner_gain_mode(m_device, (enable) ? 0 : 1);
	if(result < 0) throw string_exception(__func__, ": failed to set tuner automatic gain control to ", (enable) ? "on" : "off");
}

//---------------------------------------------------------------------------
// usbdevice::set_center_frequency
//
// Sets the center frequency of the device
//
// Arguments:
//
//	hz		- Frequency to set, specified in hertz

uint32_t usbdevice::set_center_frequency(uint32_t hz) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_center_freq(m_device, hz);
	if(result < 0) throw string_exception(__func__, ": failed to set device frequency to ", hz, "Hz");

	return rtlsdr_get_center_freq(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::set_frequency_correction
//
// Sets the frequency correction of the device
//
// Arguments:
//
//	ppm		- Frequency correction to set, specified in parts per million

int usbdevice::set_frequency_correction(int ppm) const
{
	assert(m_device != nullptr);

	// NOTE: rtlsdr_set_freq_correction will return -2 if the requested value matches what has
	// already been applied to the device; this is not an error condition
	int result = rtlsdr_set_freq_correction(m_device, ppm);
	if((result < 0) && (result != -2)) throw string_exception(__func__, ": failed to set device frequency correction to ", ppm, "ppm");

	return rtlsdr_get_freq_correction(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::set_gain
//
// Sets the gain of the device
//
// Arguments:
//
//	db			- Gain to set, specified in tenths of a decibel

int usbdevice::set_gain(int db) const
{
	std::vector<int>	validgains;			// Gains allowed by the device

	assert(m_device != nullptr);

	// Get the list of valid gain values for the device
	get_valid_gains(validgains);
	if(validgains.size() == 0) throw string_exception(__func__, ": failed to retrieve valid device gain values");

	// Select the gain value that's closest to what has been requested
	int nearest = validgains[0];
	for(size_t index = 0; index < validgains.size(); index++) {

		if(std::abs(db - validgains[index]) < std::abs(db - nearest)) nearest = validgains[index];
	}

	// Attempt to set the gain to the detected nearest gain value
	int result = rtlsdr_set_tuner_gain(m_device, nearest);
	if(result < 0) throw string_exception(__func__, ": failed to set device gain to ", db, "dB/10");

	// Return the gain value that was actually used
	return nearest;
}

//---------------------------------------------------------------------------
// usbdevice::set_sample_rate
//
// Sets the sample rate of the device
//
// Arguments:
//
//	hz		- Sample rate to set, specified in hertz

uint32_t usbdevice::set_sample_rate(uint32_t hz) const
{
	assert(m_device != nullptr);

	int result = rtlsdr_set_sample_rate(m_device, hz);
	if(result < 0) throw string_exception(__func__, ": failed to set device sample rate to ", hz, "Hz");

	return rtlsdr_get_sample_rate(m_device);
}

//---------------------------------------------------------------------------
// usbdevice::set_test_mode
//
// Enables/disables the test mode of the device
//
// Arguments:
//
//	enable		- Flag to enable/disable test mode

void usbdevice::set_test_mode(bool enable) const
{
	assert(m_device != nullptr);

	rtlsdr_set_testmode(m_device, (enable) ? 1 : 0);
}

//---------------------------------------------------------------------------

#pragma warning(pop)
