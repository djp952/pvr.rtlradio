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

#ifndef __PVRTYPES_H_
#define __PVRTYPES_H_
#pragma once

#include <string>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------------------

// MENUHOOK_XXXXXX
//
// Menu hook identifiers
static int const MENUHOOK_SETTING_IMPORTCHANNELS = 10;
static int const MENUHOOK_SETTING_EXPORTCHANNELS = 11;
static int const MENUHOOK_SETTING_CLEARCHANNELS = 12;

//---------------------------------------------------------------------------
// DATA TYPES
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TYPE DECLARATIONS
//---------------------------------------------------------------------------

// device_connection
//
// Defines the RTL-SDR device connection type
enum device_connection {

	usb = 0,				// Locally connected USB device
	rtltcp = 1,				// Device connected via rtl_tcp
};

// rds_standard
//
// Defines the Radio Data System (RDS) standard
enum rds_standard {

	automatic = 0,				// Automatically detect RDS standard
	rds = 1,				// Global RDS standard
	rbds = 2,				// North American RBDS standard
};

// settings
//
// Defines all of the configurable addon settings
struct settings {

	// device_connection
	//
	// The type of device (USB vs network, for example)
	enum device_connection device_connection;

	// device_connection_usb_index
	//
	// The index of a USB connected device
	int device_connection_usb_index;

	// device_connection_tcp_host
	//
	// The IP address of the rtl_tcp host to connect to
	std::string device_connection_tcp_host;

	// device_connection_tcp_port
	//
	// The port number of the rtl_tcp host to connect to
	int device_connection_tcp_port;

	// fmradio_rds_standard
	//
	// Specifies the Radio Data System (RDS) standard
	enum rds_standard fmradio_rds_standard;

	// fmradio_output_samplerate
	//
	// Specifies the output sample rate for the FM DSP
	int fmradio_output_samplerate;
};

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PVRTYPES_H_
