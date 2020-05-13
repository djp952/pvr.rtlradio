//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __RTLDEVICE_H_
#define __RTLDEVICE_H_
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <rtl-sdr.h>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class rtldevice
//
// Interfaces with the RTL-SDR API to provide device management services

class rtldevice
{
public:

	// AUTOMATIC_BANDWIDTH
	//
	// Specifies that automatic bandwidth selection should be used
	static uint32_t const AUTOMATIC_BANDWIDTH;

	// DEFAULT_DEVICE_INDEX
	//
	// Default device index number
	static uint32_t const DEFAULT_DEVICE_INDEX;

	// Destructor
	//
	~rtldevice();

	//-----------------------------------------------------------------------
	// Member Functions

	// agcmode
	//
	// Enables/disables the automatic gain control of the device
	void agcmode(bool enable) const;

	// bandwidth
	//
	// Sets the bandwidth of the device
	void bandwidth(uint32_t hz) const;

	// create (static)
	//
	// Factory method, creates a new rtldevice instance
	static std::unique_ptr<rtldevice> create(void);
	static std::unique_ptr<rtldevice> create(uint32_t index);

	// frequency
	//
	// Gets/sets the center frequency of the device
	uint32_t frequency(void) const;
	void frequency(uint32_t hz) const;

	// frequencycorrection
	//
	// Gets/sets the frequency correction of the device
	uint32_t frequencycorrection(void) const;
	void frequencycorrection(int ppm) const;

	// gain
	//
	// Gets/sets the gain value of the device
	int gain(void) const;
	void gain(int db) const;

	// manufacturer
	//
	// Gets the manufacturer name of the device
	char const* manufacturer(void) const;

	// name
	//
	// Gets the name of the device
	char const* name(void) const;

	// product
	//
	// Gets the product name of the device
	char const* product(void) const;

	// read
	//
	// Reads data from the device
	size_t read(uint8_t* buffer, size_t count) const;

	// samplerate
	//
	// Gets/sets the sample rate of the device
	uint32_t samplerate(void) const;
	void samplerate(uint32_t hz) const;

	// serialnumber
	//
	// Gets the serial number of the device
	char const* serialnumber(void) const;

	// testmode
	//
	// Enables/disables the test mode of the device
	void testmode(bool enable) const;

	// validgains
	//
	// Gets the valid tuner gain values for the device
	void validgains(std::vector<int>& dbs) const;

private:

	rtldevice(rtldevice const&) = delete;
	rtldevice& operator=(rtldevice const&) = delete;

	// Instance Constructor
	//
	rtldevice(uint32_t index);

	//-----------------------------------------------------------------------
	// Private Member Functions

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

#endif	// __RTLDEVICE_H_
