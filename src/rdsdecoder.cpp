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
	m_ps_data.fill(0x00);				// Initialize PS buffer
	m_rt_data.fill(0x00);				// Initialize RT buffer
}

//---------------------------------------------------------------------------
// rdsdecoder Destructor

rdsdecoder::~rdsdecoder()
{
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_basictuning
//
// Decodes Group Type 0A and 0B - Basic Tuning and swithing information
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_basictuning(tRDS_GROUPS const& rdsgroup)
{
	uint8_t codebits = rdsgroup.BlockB & 0x03;
	m_ps_data[codebits * 2] = (rdsgroup.BlockD >> 8) & 0xFF;
	m_ps_data[codebits * 2 + 1]= rdsgroup.BlockD & 0xFF;

	// Accumulate segments until all 4 (0xF) have been received
	m_ps_ready |= (0x01 << codebits);
	if(m_ps_ready == 0x0F) {

		// UECP_MEC_PS
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PS;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects the 8 characters to start at the address of mel_len
		// when processing UECP_MEC_PS
		uint8_t* mel_data = &message->mel_len;
		memcpy(mel_data, &m_ps_data[0], m_ps_data.size());

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 8;				// mec, dsn, psn + mel_data[8]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Reset the segment accumulator back to zero
		m_ps_ready = 0x00;
	}
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
	bool hascr = false;				// Flag if carriage return detected

	// Get the text segment address and A/B indicators from the data
	uint8_t textsegmentaddress = (rdsgroup.BlockB & 0x000F);
	uint8_t const ab = ((rdsgroup.BlockB & 0x0010) >> 4);

	// Set the initial A/B flag the first time it's been seen
	if(!m_rt_init) { m_rt_ab = ab; m_rt_init = true; }

	// Clear any existing radio text when the A/B flag changes
	if(ab != m_rt_ab) {

		m_rt_ab = ab;				// Toggle A/B
		m_rt_data.fill(0x00);		// Clear existing RT data
		m_rt_ready = 0x0000;		// Clear existing segment flags
	}

	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);

	// Group A contains two RadioText segments in block C and D
	if(groupa) {

		size_t offset = (textsegmentaddress << 2);
		m_rt_data[offset + 0] = (rdsgroup.BlockC >> 8) & 0xFF;
		m_rt_data[offset + 1] = rdsgroup.BlockC & 0xFF;
		m_rt_data[offset + 2] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[offset + 3] = rdsgroup.BlockD & 0xFF;

		// Check if a carriage return has been sent in this text segment
		hascr = ((m_rt_data[offset + 0] == 0x0D) || (m_rt_data[offset + 1] == 0x0D) ||
			(m_rt_data[offset + 2] == 0x0D) || (m_rt_data[offset + 3] == 0x0D));
	}

	// Group B contains one RadioText segment in block D
	else {

		size_t offset = (textsegmentaddress << 1);
		m_rt_data[offset + 0] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[offset + 1] = rdsgroup.BlockD & 0xFF;

		// Check if a carriage return has been sent in this text segment
		hascr = ((m_rt_data[offset + 0] == 0x0D) || (m_rt_data[offset + 1] == 0x0D));
	}

	// Indicate that this segment has been received, and if a CR was detected flag
	// all remaining text segments as received (they're not going to come anyway)
	m_rt_ready |= (0x01 << textsegmentaddress);
	while((hascr) && (++textsegmentaddress < 16)) m_rt_ready |= (0x01 << textsegmentaddress);

	// The RT information is ready to be sent if all 16 segments have been retrieved
	// for a Group A signal, or the first 8 segments of a Group B signal
	if((groupa) ? m_rt_ready == 0xFFFF : ((m_rt_ready & 0x00FF) == 0x00FF)) {

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

		// Reset the segment accumulator back to zero
		m_rt_ready = 0x0000;
	}
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

		// Group Type 0: Basic Tuning and switching information
		//
		case 0:
			decode_basictuning(rdsgroup);
			break;

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
