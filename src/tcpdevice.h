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

#ifndef __TCPDEVICE_H_
#define __TCPDEVICE_H_
#pragma once

#include <memory>
#include <rtl-sdr.h>
#include <string>
#include <vector>

#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class tcpdevice
//
// Implements device management for an RTL-SDR connected over TCP.
//
// Portions based on:
//
// gr-osmosdr (rtl_tcp_source_c.cc)
// https://github.com/osmocom/gr-osmosdr
// Copyright (C) 2012 Dimitri Stolnikov
// GPLv3

class tcpdevice : public rtldevice
{
public:

	// Destructor
	//
	virtual ~tcpdevice();

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
	// Factory method, creates a new tcpdevice instance
	static std::unique_ptr<tcpdevice> create(char const* host, uint16_t port);

	// get_device_name
	//
	// Gets the name of the device
	char const* get_device_name(void) const override;

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

	tcpdevice(tcpdevice const&) = delete;
	tcpdevice& operator=(tcpdevice const&) = delete;

	// Instance Constructor
	//
	tcpdevice(char const* host, uint16_t port);

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// struct command
	//
	// Structure used to send a command to the connected device
#ifdef _WINDOWS
	#pragma pack(push, 1)
	struct device_command {

		uint8_t		cmd;
		uint32_t	param;
	};
	#pragma pack(pop)
#else
	struct device_command {

		uint8_t		cmd;
		uint32_t 	param;
	} __attribute__((packed));
#endif

	// struct device_info
	//
	// Retrieves information about the connected device (from rtl_tcp.c)

	struct device_info {

		char		magic[4];
		uint32_t	tuner_type;
		uint32_t	tuner_gain_count;
	};
	
	// device_info structure size must be 12 bytes in length (rtl_tcp.c)
	static_assert(sizeof(device_info) == 12, "device_info structure size must be 12 bytes in length");

	//-----------------------------------------------------------------------
	// Private Member Functions

	// close_socket (static)
	//
	// Closes an open socket, implementation specific
	static void close_socket(int socket);

	//-----------------------------------------------------------------------
	// Member Variables

	int					m_socket	= -1;						// TCP/IP socket
	rtlsdr_tuner		m_tunertype = RTLSDR_TUNER_UNKNOWN;		// Tuner type
	std::string			m_name;									// Device name

	// ASYNCHRONOUS SUPPORT
	//
	mutable scalar_condition<bool>	m_stop{ false };		// Flag to stop async
	mutable scalar_condition<bool>	m_stopped{ true };		// Async stopped condition

	static std::vector<int> const	s_gaintable_e4k;		// RTLSDR_TUNER_E4000
	static std::vector<int> const	s_gaintable_fc0012;		// RTLSDR_TUNER_FC0012
	static std::vector<int> const	s_gaintable_fc0013;		// RTLSDR_TUNER_FC0013
	static std::vector<int> const	s_gaintable_fc2580;		// RTLSDR_TUNER_FC2580
	static std::vector<int> const	s_gaintable_r82xx;		// RTLSDR_TUNER_R820T/R828D
	static std::vector<int> const	s_gaintable_unknown;	// RTLSDR_TUNER_UNKNOWN
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __TCPDEVICE_H_
