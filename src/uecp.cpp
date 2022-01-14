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
#include "uecp.h"

#include <algorithm>
#include <assert.h>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// uecp_crc16_ccit
//
// Calculates the CRC of a UECP data frame
//
// Arguments:
//
//	data		- The buffer containing the data frame
//	len			- Length of the input buffer, in bytes
//
// rds-control (uecp.c)
// https://github.com/UoC-Radio/rds-control
// Copyright (C) 2013 Nick Kossifidis
// GPLv2

static uint16_t uecp_crc16_ccitt(uint8_t const* data, size_t len)
{
	uint16_t crc = 0xFFFF;

	for(size_t i = 0; i < len; i++)
	{
		crc = (uint8_t)(crc >> 8) | (crc << 8);
		crc ^= data[i];
		crc ^= (uint8_t)(crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}

	return ((crc ^= 0xFFFF) & 0xFFFF);
}

//---------------------------------------------------------------------------
// uecp_create_data_packet
//
// Creates a uecp_data_packet from a constructed uecp_data_frame
//
// Arguments:
//
//	frame		- uecp_data_frame to convert into a data packet

uecp_data_packet uecp_create_data_packet(uecp_data_frame& frame)
{
	uecp_data_packet		packet;				// UECP data packet

	// Calculate the length of the data frame
	size_t framelength = 4 + frame.msg_len;

	// Get a pointer to the raw frame data 
	uint8_t* framedata = reinterpret_cast<uint8_t*>(&frame);

	// Generate the CRC16 value for the frame and append it to the end
	uint16_t framecrc = uecp_crc16_ccitt(reinterpret_cast<uint8_t const*>(&frame), framelength);
	framedata[framelength++] = (framecrc >> 8) & 0xFF;
	framedata[framelength++] = (framecrc & 0xFF);

	packet.reserve(UECP_DP_MAX_LEN);		// Reserve typical maximum
	packet.push_back(UECP_DP_START_BYTE);	// Start the packet

	// All of the data within the packet must be "byte stuffed", which indicates that
	// any value greater than 0xFC requires a 2-byte escape sequence
	for(size_t index = 0; index < framelength; index++) {

		switch(framedata[index]) {

			// 0xFD -> 0xFD+0x00 | 0xFE -> 0xFD+0x01 | 0xFF -> 0xFD+0x02
			case 0xFD: packet.push_back(0xFD); packet.push_back(0x00); break;
			case 0xFE: packet.push_back(0xFD); packet.push_back(0x01); break;
			case 0xFF: packet.push_back(0xFD); packet.push_back(0x02); break;
			default: packet.push_back(framedata[index]);
		}
	}

	packet.push_back(UECP_DP_STOP_BYTE);	// Finish the packet

	return packet;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
