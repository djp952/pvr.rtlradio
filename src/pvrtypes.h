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

// downsample_quality
//
// Defines the FM DSP downsample quality factor
enum downsample_quality {

	fast = 0,				// Optimize for speed
	standard = 1,			// Standard quality
	maximum = 2,			// Optimize for quality
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

	// device_frequency_correction
	//
	// Frequency correction calibration value for the device
	int device_frequency_correction;

	// device_connection_tcp_port
	//
	// The port number of the rtl_tcp host to connect to
	int device_connection_tcp_port;

	// region_regioncode
	//
	// The region in which the RTL-SDR device is operating
	enum regioncode region_regioncode;

	// fmradio_enable_rds
	//
	// Enables passing decoded RDS information to Kodi
	bool fmradio_enable_rds;

	// fmradio_prepend_channel_numbers
	//
	// Flag to include the channel number in the channel name
	bool fmradio_prepend_channel_numbers;

	// fmradio_sample_rate
	//
	// Sample rate value for the FM DSP
	int fmradio_sample_rate;

	// fmradio_downsample_quality
	//
	// Specifies the FM DSP downsample quality factor
	enum downsample_quality fmradio_downsample_quality;

	// fmradio_output_samplerate
	//
	// Specifies the output sample rate for the FM DSP
	int fmradio_output_samplerate;

	// fmradio_output_gain
	//
	// Specifies the output gain for the FM DSP
	float fmradio_output_gain;

	// hdradio_enable
	//
	// Enables the HD Radio DSP
	bool hdradio_enable;

	// hdradio_prepend_channel_numbers
	//
	// Flag to include the channel number in the channel name
	bool hdradio_prepend_channel_numbers;

	// hdradio_output_gain
	//
	// Specifies the output gain for the HD DSP
	float hdradio_output_gain;

	// dabradio_enable
	//
	// Enables/disables the DAB DSP
	bool dabradio_enable;

	// dabradio_output_gain
	//
	// Specifies the output gain for the DAB DSP
	float dabradio_output_gain;

	// wxradio_enable
	//
	// Enables the WX DSP
	bool wxradio_enable;

	// wxradio_sample_rate
	//
	// Sample rate value for the WX DSP
	int wxradio_sample_rate;

	// wxradio_output_samplerate
	//
	// Specifies the output sample rate for the WX DSP
	int wxradio_output_samplerate;

	// wxradio_output_gain
	//
	// Specified the output gain for the WX DSP
	float wxradio_output_gain;
};

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PVRTYPES_H_
