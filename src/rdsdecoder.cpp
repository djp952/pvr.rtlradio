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
#include "rdsdecoder.h"

#include "fmdsp/rbdsconstants.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// rdsdecoder Constructor
//
// Arguments:
//
//	NONE

rdsdecoder::rdsdecoder()
{
	m_rt_data.fill(0x00);				// Initialize RT buffer
}

//---------------------------------------------------------------------------
// rdsdecoder Destructor

rdsdecoder::~rdsdecoder()
{
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_radiotext
//
// Decodes Group Type 2A and 2B - RadioText
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_radiotext(tRDS_GROUPS const& rdsgroup)
{
	// Get the text segment address and A/B indicators from the data
	uint8_t const textsegmentaddress = ((rdsgroup.BlockB & 0x000F) << 2);
	uint8_t const ab = ((rdsgroup.BlockB & 0x0010) >> 4);

	// Don't start collecting text until the start of a new message
	if(textsegmentaddress == 0x00) { m_rt_ab = ab; m_rt_init = true; }

	// Don't do anything if the start of a message hasn't been seen yet
	if(!m_rt_init) return;

	// Clear any existing radio text when the A/B flag changes
	if(ab != m_rt_ab) { m_rt_ab = ab; m_rt_data.fill(0x00); }

	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);

	// Group A contains two RadioText segments in block C and D
	if(groupa) {

		m_rt_data[textsegmentaddress + 0] = (rdsgroup.BlockC >> 8) & 0xFF;
		m_rt_data[textsegmentaddress + 1] = rdsgroup.BlockC & 0xFF;
		m_rt_data[textsegmentaddress + 2] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[textsegmentaddress + 3] = rdsgroup.BlockD & 0xFF;
	}

	// Group B contains one RadioText segment in block D
	else {

		m_rt_data[textsegmentaddress + 0] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[textsegmentaddress + 1] = rdsgroup.BlockD & 0xFF;
	}

	// UECP_MEC_RT
	//
	struct uecp_data_frame frame = {};
	struct uecp_message* message = &frame.msg;

	message->mec = UECP_MEC_RT;
	message->dsn = UECP_MSG_DSN_CURRENT_SET;
	message->psn = UECP_MSG_PSN_MAIN;

	message->mel_len = 0x01;
	message->mel_data[0] = m_rt_ab;

	for(size_t index = 0; index < m_rt_data.size(); index++) {

		if(m_rt_data[index] == 0x00) break;
		message->mel_data[message->mel_len++] = m_rt_data[index];
	}

	frame.seq = UECP_DF_SEQ_DISABLED;
	frame.msg_len = 4 + message->mel_len;

	// Convert the UECP data frame into a packet and queue it up
	m_uecp_packets.emplace(uecp_create_data_packet(frame));
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_rdsgroup
//
// Decodes the next RDS group
//
// Arguments:
//
//	rdsgroup		- Next RDS group to be processed

void rdsdecoder::decode_rdsgroup(tRDS_GROUPS const& rdsgroup)
{
	// Determine the group type code
	uint8_t grouptypecode = (rdsgroup.BlockB & 0xF000) >> 12;

	// Invoke the proper handler for the specified group type code
	switch(grouptypecode) {

		// Group Type 2: RadioText
		//
		case 2:
			decode_radiotext(rdsgroup);
			break;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::pop_uecp_data_packet
//
// Pops the topmost UECP data packet out of the packet queue
//
// Arguments:
//
//	packet		- On success, contains the UECP packet data

bool rdsdecoder::pop_uecp_data_packet(uecp_data_packet& packet)
{
	if(m_uecp_packets.empty()) return false;

	// Swap the front packet from the top of the queue and pop it off
	m_uecp_packets.front().swap(packet);
	m_uecp_packets.pop();

	return true;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
