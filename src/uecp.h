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

#ifndef __UECP_H_
#define __UECP_H_
#pragma once

#include <stdint.h>
#include <vector>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// NOTE: The declarations in this header file were derived from the following
// source code references:
//
// rds-control (uecp.h)
// https://github.com/UoC-Radio/rds-control
// Copyright (C) 2013 Nick Kossifidis
// GPLv2
//
// xbmc (xbmc/cores/VideoPlayer/VideoPlayerRadioRDS.cpp)
// https://github.com/xbmc/xbmc/
// Copyright (C) 2005-2018 Team Kodi
// GPLv2
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
// UECP MESSAGE
//---------------------------------------------------------------------------

// Maximum UECP message size
//
#define UECP_MSG_LEN_MAX		255
#define UECP_MSG_MEL_LEN_MAX	UECP_MSG_LEN_MAX - 1

// uecp_message
//
// UECP message structure
#pragma pack(push, 1)
struct uecp_message {

	uint8_t		mec;							// Message element code
	uint8_t		dsn;							// Data set number
	uint8_t		psn;							// Program service number
	uint8_t		mel_len;						// Message element length
	uint8_t		mel_data[UECP_MSG_MEL_LEN_MAX];	// Message element data
};
#pragma pack(pop)

// Message Element Codes
//
#define UECP_MEC_PI					0x01	// Program Identification
#define UECP_MEC_PS					0x02	// Program Service name
#define UECP_MEC_PIN				0x06	// Program Item Number
#define UECP_MEC_DI_PTYI			0x04	// Decoder Information / Dynamic PTY Indicator
#define UECP_MEC_TA_TP				0x03	// Traffic Anouncement / Traffic Programme
#define UECP_MEC_MS					0x05	// Music / Speech switch
#define UECP_MEC_PTY				0x07	// Program Type
#define UECP_MEC_PTYN				0x3A	// Program Type Name
#define UECP_MEC_RT					0x0A	// RadioText
#define UECP_MEC_AF					0x13	// Alternative Frequencies List
#define UECP_MEC_EON_AF				0x14	// Enhanced Other Networks Information
#define UECP_MEC_SLOW_LABEL_CODES	0x1A	// Slow labeling codes
#define UECP_MEC_LINKAGE_INFO		0x2E	// Linkage information
#define UECP_EPP_TM_INFO			0x31	// EPP transmitter information
#define UECP_ODA_DATA				0x46	// Open Data Application (ODA) data

// Data Set Number
//
#define UECP_MSG_DSN_CURRENT_SET	0x00
#define UECP_MSG_DSN_MIN			1
#define UECP_MSG_DSN_MAX			0xFD
#define UECP_MSG_DSN_ALL_OTHER_SETS	0xFE
#define UECP_MSG_DSN_ALL_SETS		0xFF

// Program Service Number
//
#define UECP_MSG_PSN_MAIN			0x00
#define UECP_MSG_PSN_MIN			1
#define UECP_MSG_PSN_MAX			0xFF

// Message Element Length
//
#define UECP_MSG_MEL_NA				0xFF	// mel_len is not applicable

//-----------------------------------------------------------------------------
// UECP DATA FRAME
//-----------------------------------------------------------------------------

// Maximum UECP data frame size
//	
#define UECP_DF_MAX_LEN			(UECP_MSG_LEN_MAX + 6)		// <-- 261

// uecp_data_frame
//
// UECP data frame
#pragma pack(push, 1)
struct uecp_data_frame {

	uint16_t				addr; 		// Remote address
	uint8_t					seq;		// Sequence number
	uint8_t					msg_len;	// Message Length
	struct uecp_message		msg;		// Message (variable length)
	uint16_t				crc;		// CRC (CCITT)
};
#pragma pack(pop)

// Sequence flags
//
#define UECP_DF_SEQ_DISABLED	0

//-----------------------------------------------------------------------------
// UECP DATA PACKET
//-----------------------------------------------------------------------------

// uecp_data_packet
//
// Type definition for a UECP data packet
using uecp_data_packet = std::vector<uint8_t>;

// Maximum UECP data packet size
//
#define UECP_DP_MAX_LEN		(UECP_DF_MAX_LEN + 2)		// <-- 263

// Start/stop indicators
//
#define UECP_DP_START_BYTE		0xFE
#define UECP_DP_STOP_BYTE		0xFF

// uecp_create_data_packet
//
// Creates a UECP data packet from the provided data frame
uecp_data_packet uecp_create_data_packet(uecp_data_frame& frame);

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __UECP_H_
