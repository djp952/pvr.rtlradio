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

#ifndef __USBDEVICE_H_
#define __USBDEVICE_H_
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <rtl-sdr.h>

#include "rtldevice.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class usbdevice
//
// Implements device management for a local USB connected RTL-SDR

class usbdevice : public rtldevice
{
public:

	// DEFAULT_DEVICE_INDEX
	//
	// Default device index number
	static uint32_t const DEFAULT_DEVICE_INDEX;

	// Destructor
	//
	virtual ~usbdevice();

	//-----------------------------------------------------------------------
	// Member Functions

	// begin_stream
	//
	// Starts streaming data from the device
	void begin_stream(void) const override;

	// cancel_async
	//
	// Cancels any pending asynchronous read operations from the device
	void cancel_async(void) const override;

	// create (static)
	//
	// Factory method, creates a new usbdevice instance
	static std::unique_ptr<usbdevice> create(void);
	static std::unique_ptr<usbdevice> create(uint32_t index);

	// get_center_frequency
	//
	// Gets the center frequency of the device
	uint32_t get_center_frequency(void) const;

	// get_device_name
	//
	// Gets the name of the device
	char const* get_device_name(void) const override;

	// get_frequency_correction
	//
	// Gets the frequency correction of the device
	int get_frequency_correction(void) const;

	// get_gain
	//
	// Gets the gain value of the device
	int get_gain(void) const;

	// get_manufacturer_name
	//
	// Gets the manufacturer name of the device
	char const* get_manufacturer_name(void) const;

	// get_product_name
	//
	// Gets the product name of the device
	char const* get_product_name(void) const;

	// get_sample_rate
	//
	// Gets the sample rate of the device
	uint32_t get_sample_rate(void) const;

	// get_serial_number
	//
	// Gets the serial number of the device
	char const* get_serial_number(void) const;

	// get_valid_gains
	//
	// Gets the valid tuner gain values for the device
	void get_valid_gains(std::vector<int>& dbs) const override;

	// read
	//
	// Reads data from the device
	size_t read(uint8_t* buffer, size_t count) const override;

	// read_async
	//
	// Asynchronously reads data from the device
	void read_async(rtldevice::asynccallback const& callback, uint32_t bufferlength) const override;

	// set_automatic_gain_control
	//
	// Enables/disables the automatic gain control of the device
	void set_automatic_gain_control(bool enable) const override;

	// set_center_frequency
	//
	// Sets the center frequency of the device
	uint32_t set_center_frequency(uint32_t hz) const override;

	// set_frequency_correction
	//
	// Sets the frequency correction of the device
	int set_frequency_correction(int ppm) const override;

	// set_gain
	//
	// Sets the gain value of the device
	int set_gain(int db) const override;

	// set_sample_rate
	//
	// Sets the sample rate of the device
	uint32_t set_sample_rate(uint32_t hz) const override;

	// set_test_mode
	//
	// Enables/disables the test mode of the device
	void set_test_mode(bool enable) const override;

private:

	usbdevice(usbdevice const&) = delete;
	usbdevice& operator=(usbdevice const&) = delete;

	// Instance Constructor
	//
	usbdevice(uint32_t index);

	//-----------------------------------------------------------------------
	// Member Variables

	rtlsdr_dev_t*			m_device = nullptr;		// Device instance

	std::string				m_name;					// Device name
	std::string				m_manufacturer;			// Device manufacturer
	std::string				m_product;				// Device product name
	std::string				m_serialnumber;			// Device serial number
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __USBDEVICE_H_
