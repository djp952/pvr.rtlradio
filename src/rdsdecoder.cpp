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
#include "rdsdecoder.h"

#include <cstring>

#include "fmdsp/rbdsconstants.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// rdsdecoder Constructor
//
// Arguments:
//
//	isrbds		- Flag if input will be RBDS (North America) or RDS

rdsdecoder::rdsdecoder(bool isrbds) : m_isrbds(isrbds)
{
	m_ps_data.fill(0x00);				// Initialize PS buffer
	m_rt_data.fill(0x00);				// Initialize RT buffer
	m_rbds_callsign.fill(0x00);			// Initialize callsign buffer
}

//---------------------------------------------------------------------------
// rdsdecoder Destructor

rdsdecoder::~rdsdecoder()
{
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_applicationidentification
//
// Decodes Group Type 3A - Application Identification
//
// Arguments:
//
//	rdsgroiup	- RDS group to be processed

void rdsdecoder::decode_applicationidentification(tRDS_GROUPS const& rdsgroup)
{
	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);
	
	if(groupa) {

		// Determine what Open Data Application(s) are present by checking
		// the Application ID (AID) against known values
		//
		// (https://www.nrscstandards.org/committees/dsm/archive/rds-oda-aids.pdf)
		switch(rdsgroup.BlockD) {

			// 0x4BD7: RadioText+ (RT+)
			case 0x4BD7:
				m_oda_rtplus = true;
				m_rtplus_group = (rdsgroup.BlockB >> 1) & 0x0F;
				m_rtplus_group_ab = rdsgroup.BlockB & 0x01;
				break;

			// 0xCD46: Traffic Message Channel (RDS-TMC)
			case 0xCD46:
				m_oda_rdstmc = true;
				break;
		}
	}
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
	uint8_t const ps_codebits = rdsgroup.BlockB & 0x03;

	m_ps_data[ps_codebits * 2] = (rdsgroup.BlockD >> 8) & 0xFF;
	m_ps_data[ps_codebits * 2 + 1]= rdsgroup.BlockD & 0xFF;

	// Accumulate segments until all 4 (0xF) have been received
	m_ps_ready |= (0x01 << ps_codebits);
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
// rdsdecoder::decode_programidentification
//
// Decodes Program Identification (PI)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_programidentification(tRDS_GROUPS const& rdsgroup)
{
	uint16_t const pi = rdsgroup.BlockA;

	// Indicate a change to the Program Identification flags
	if(pi != m_pi) {

		// UECP_MEC_PI
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PI;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single word for PI at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = pi & 0xFF;
		*reinterpret_cast<uint8_t*>(&message->mel_data[0]) = (pi >> 8) & 0xFF;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 2;				// mec, dsn, psn + mel_data[2]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current PI flags
		m_pi = pi;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_programtype
//
// Decodes Program Type (PTY)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_programtype(tRDS_GROUPS const& rdsgroup)
{
	uint8_t const pty = (rdsgroup.BlockB >> 5) & 0x1F;

	// Indicate a change to the Program Type flags
	if(pty != m_pty) {

		// UECP_MEC_PTY
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PTY;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single byte for PTY at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = pty;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 1;				// mec, dsn, psn + mel_data[1]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current PTY flags
		m_pty = pty;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_radiotextplus
//
// Decodes the RadioText+ (RT+) ODA
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_radiotextplus(tRDS_GROUPS const& rdsgroup)
{
	// Determine if the group A/B flag matches that set for the RT+ application
	if(m_rtplus_group_ab == ((rdsgroup.BlockB >> 11) & 0x01)) {

		// UECP_ODA_DATA
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		// Kodi treats UECP_ODA_DATA as a custom data packet	
		message->mec = UECP_ODA_DATA;
		message->dsn = 8;						// ODA data length
		message->psn = 0x4B;					// High byte of ODA AID (0x4BD7)
		message->mel_len = 0xD7;				// Low byte of ODA AID (0x4BD7)

		// Pack the BlockB, BlockC, and BlockD data from the RDS group into the packet
		message->mel_data[0] = (rdsgroup.BlockB >> 8) & 0xFF;
		message->mel_data[1] = rdsgroup.BlockB & 0xFF;
		message->mel_data[2] = (rdsgroup.BlockC >> 8) & 0xFF;
		message->mel_data[3] = rdsgroup.BlockC & 0xFF;
		message->mel_data[4] = (rdsgroup.BlockD >> 8) & 0xFF;
		message->mel_data[5] = rdsgroup.BlockD & 0xFF;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 4 + 6;					// mec, dsn, psn, mel_data + 6 bytes

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));
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
	uint8_t textsegmentaddress = rdsgroup.BlockB & 0x000F;
	uint8_t const ab = (rdsgroup.BlockB >> 4) & 0x01;

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

		size_t offset = static_cast<size_t>(textsegmentaddress << 2);
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

		size_t offset = static_cast<size_t>(textsegmentaddress << 1);
		m_rt_data[offset + 0] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[offset + 1] = rdsgroup.BlockD & 0xFF;

		// Check if a carriage return has been sent in this text segment
		hascr = ((m_rt_data[offset + 0] == 0x0D) || (m_rt_data[offset + 1] == 0x0D));
	}

	// Indicate that this segment has been received, and if a CR was detected flag
	// all remaining text segments as received (they're not going to come anyway)
	m_rt_ready |= (0x01 << textsegmentaddress);
	while((hascr) && (++textsegmentaddress < 16)) {

		// Clear any RT information that may have been previously set
		size_t offset = static_cast<size_t>(textsegmentaddress << 2);
		m_rt_data[offset + 0] = 0x00;
		m_rt_data[offset + 1] = 0x00;
		if(groupa) m_rt_data[offset + 2] = 0x00;
		if(groupa) m_rt_data[offset + 3] = 0x00;

		// Flag this segment as ready in the bitmask
		m_rt_ready |= (0x01 << textsegmentaddress);
	}

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
// rdsdecoder::decode_rbds_programidentification
//
// Decodes RBDS Program Identification (PI)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_rbds_programidentification(tRDS_GROUPS const& rdsgroup)
{
	uint16_t pi = rdsgroup.BlockA;

	// Indicate a change to the Program Identification flags
	if(pi != m_rbds_pi) {

		uint8_t countrycode = 0xA0;			// US

		m_rbds_pi = pi;
		m_rbds_callsign.fill('\0');
		m_rbds_nationalcode.clear();

		// SPECIAL CASE: AFxx -> xx00
		//
		if((pi & 0xAF00) == 0xAF00) pi <<= 8;

		// SPECIAL CASE: Axxx -> x0xx
		//
		if((pi & 0xA000) == 0xA000) pi = (((pi & 0xF00) << 4) | (pi & 0xFF));

		// NATIONALLY/REGIONALLY-LINKED RADIO STATION CODES
		//
		if((pi & 0xB000) == 0xB000) {

			// The low byte of these PI codes defines what nationally/regionally linked
			// station code should be returned instead of a station call sign
			switch(pi & 0xFF) {

				case 0x0001: m_rbds_nationalcode = "NPR-1"; break;
				case 0x0002: m_rbds_nationalcode = "CBC Radio One"; break;
				case 0x0003: m_rbds_nationalcode = "CBC Radio Two"; break;
				case 0x0004: m_rbds_nationalcode = "CBC Premiere Chaine"; break;
				case 0x0005: m_rbds_nationalcode = "CBC Espace Musique"; break;
				case 0x0006: m_rbds_nationalcode = "CBC"; break;
				case 0x0007: m_rbds_nationalcode = "CBC"; break;
				case 0x0008: m_rbds_nationalcode = "CBC"; break;
				case 0x0009: m_rbds_nationalcode = "CBC"; break;
				case 0x000A: m_rbds_nationalcode = "NPR-2"; break;
				case 0x000B: m_rbds_nationalcode = "NPR-3"; break;
				case 0x000C: m_rbds_nationalcode = "NPR-4"; break;
				case 0x000D: m_rbds_nationalcode = "NPR-5"; break;
				case 0x000E: m_rbds_nationalcode = "NPR-6"; break;
			}

			// If a Canadian Nationally/Regionally linked code was set switch to Canada
			if(!m_rbds_nationalcode.empty() && (m_rbds_nationalcode[0] == 'C')) countrycode = 0xA1;
		}

		// USA 3-LETTER-ONLY (ref: NRSC-4-B 04.2011 Table D.7)
		//
		else if((pi >= 0x9950) && (pi <= 0x9EFF)) {

			// The 3-letter only callsigns are static and represented in a lookup table
			for(auto const iterator : CALL3TABLE) {

				if(iterator.pi == pi) {

					m_rbds_callsign[0] = iterator.csign[0];
					m_rbds_callsign[1] = iterator.csign[1];
					m_rbds_callsign[2] = iterator.csign[2];
					break;
				}
			}
		}

		// USA EAST (Wxxx)
		//
		else if((pi >= 21672) && (pi <= 39247)) {

			uint16_t char1 = (pi - 21672) / 676;
			uint16_t char2 = ((pi - 21672) - (char1 * 676)) / 26;
			uint16_t char3 = ((pi - 21672) - (char1 * 676) - (char2 * 26));

			m_rbds_callsign[0] = 'W';
			m_rbds_callsign[1] = static_cast<char>(static_cast<uint16_t>('A') + char1);
			m_rbds_callsign[2] = static_cast<char>(static_cast<uint16_t>('A') + char2);
			m_rbds_callsign[3] = static_cast<char>(static_cast<uint16_t>('A') + char3);
		}

		// USA WEST (Kxxx)
		//
		else if((pi >= 4096) && (pi <= 21671)) {

			uint16_t char1 = (pi - 4096) / 676;
			uint16_t char2 = ((pi - 4096) - (char1 * 676)) / 26;
			uint16_t char3 = ((pi - 4096) - (char1 * 676) - (char2 * 26));

			m_rbds_callsign[0] = 'K';
			m_rbds_callsign[1] = static_cast<char>(static_cast<uint16_t>('A') + char1);
			m_rbds_callsign[2] = static_cast<char>(static_cast<uint16_t>('A') + char2);
			m_rbds_callsign[3] = static_cast<char>(static_cast<uint16_t>('A') + char3);
		}

		// CANADA
		//
		// Reverse engineered from "Program information codes for radio broadcasting stations"
		// (https://www.ic.gc.ca/eic/site/smt-gst.nsf/eng/h_sf08741.html)
		//
		else if((pi & 0xC000) == 0xC000) {

			countrycode = 0xA1;					// CA

			// Determine the offset and increment values from the PI code
			uint16_t offset = ((pi - 0xC000) - 257) / 255;
			uint16_t increment = (pi - 0xC000) - offset;

			// Calculate the individual character code values (interpretation differs for each)
			uint16_t char1 = (increment - 257) / (26 * 27);
			uint16_t char2 = ((increment - 257) - (char1 * (26 * 27))) / 27;
			uint16_t char3 = ((increment - 257) - (char1 * (26 * 27))) - (char2 * 27);

			// Convert the second character of the call sign first as there is a small range of
			// documented valid characters available; anything out of range should be ignored
			if(char1 == 0) m_rbds_callsign[1] = 'F';
			else if(char1 == 1) m_rbds_callsign[1] = 'H';
			else if(char1 == 2) m_rbds_callsign[1] = 'I';
			else if(char1 == 3) m_rbds_callsign[1] = 'J';
			else if(char1 == 4) m_rbds_callsign[1] = 'K';

			// Only fill in the rest of the callsign if char1 was in the valid range
			if(m_rbds_callsign[1] != '\0') {

				// The first character is always 'C'
				m_rbds_callsign[0] = 'C';

				// The third character is always present and is zero-based from 'A'
				m_rbds_callsign[2] = static_cast<char>(static_cast<uint16_t>('A') + char2);

				// The fourth character is optional and one-based from 'A'
				if(char3 != 0) m_rbds_callsign[3] = static_cast<char>(static_cast<uint16_t>('A') + (char3 - 1));
			}
		}

		// MEXICO
		//
		else if((pi & 0xF000) == 0xF000) {

			countrycode = 0xA5;					// MX
			
			// TODO - I need some manner of reference material here
		}

		// Generate a couple fake UECP packets anytime the PI changes to allow Kodi
		// to get the internals right for North American broadcasts with RBDS
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		// UECP_MEC_PI
		//
		message->mec = UECP_MEC_PI;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single word for PI at the address of mel_len; for RBDS
		// hard-code it to 0xB000 which points to this row in the lookup data which
		// has all three required country codes "US", "CA" and "MX" and can be properly 
		// set via the UECP_EPP_TM_INFO packet by specifying a PSN of 0xA0 for "US",
		// 0xA1 for "CA", and 0xA5 for "MX":
		// 
		//   {"US","CA","BR","DO","LC","MX","__"}, // B
		//
		*reinterpret_cast<uint8_t*>(&message->mel_len) = 0xB0;
		*reinterpret_cast<uint8_t*>(&message->mel_data[0]) = 0x00;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 2;				// mec, dsn, psn + mel_data[2]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// UECP_EPP_TM_INFO
		//
		message->mec = UECP_EPP_TM_INFO;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = countrycode;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3;					// mec, dsn, psn

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));
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
	// Ignore spurious RDS packets that contain no data
	// todo: figure out how this happens; could be a bug in the FM DSP
	if((rdsgroup.BlockA == 0x0000) && (rdsgroup.BlockB == 0x0000) && (rdsgroup.BlockC == 0x0000) && (rdsgroup.BlockD == 0x0000)) return;

	// Determine the group type code
	uint8_t grouptypecode = (rdsgroup.BlockB >> 12) & 0x0F;

	// Program Identification
	//
	if(m_isrbds) decode_rbds_programidentification(rdsgroup);
	else decode_programidentification(rdsgroup);

	// Program Type
	//
	decode_programtype(rdsgroup);

	// Group Type 0: Basic Tuning and switching information
	if(grouptypecode == 0) decode_basictuning(rdsgroup);

	// Group Type 1: Slow Labelling Codes
	else if(grouptypecode == 1) decode_slowlabellingcodes(rdsgroup);

	// Group Type 2: RadioText
	else if(grouptypecode == 2) decode_radiotext(rdsgroup);

	// Group Type 3: Application Identification
	else if(grouptypecode == 3) decode_applicationidentification(rdsgroup);

	// RadioText+ (RT+) Open Data Application
	if(m_oda_rtplus && grouptypecode == m_rtplus_group) decode_radiotextplus(rdsgroup);
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_slowlabellingcodes
//
// Decodes Group 1A - Slow Labelling Codes
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_slowlabellingcodes(tRDS_GROUPS const& rdsgroup)
{
	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);

	if(groupa) {

		// UECP_MEC_SLOW_LABEL_CODES
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_SLOW_LABEL_CODES;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		//message->psn = UECP_MSG_PSN_MAIN;

		// For whatever reason, Kodi expects the high byte of the data to 
		// be in the message PSN field -- could be a bug in Kodi, but whatever
		message->psn = ((rdsgroup.BlockC >> 8) & 0xFF);
		*reinterpret_cast<uint8_t*>(&message->mel_len) = (rdsgroup.BlockC & 0xFF);

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 1;				// mec, dsn, psn + mel_data[1]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::get_rdbs_callsign
//
// Retrieves the RBDS call sign (if present)
//
// Arguments:
//
//	NONE

std::string rdsdecoder::get_rbds_callsign(void) const
{
	// If this is a nationally/regionally linked station return that string
	if(!m_rbds_nationalcode.empty()) return m_rbds_nationalcode;

	// The callsign may not have every letter assigned, trim any NULLs at the end
	std::string callsign = std::string(m_rbds_callsign.begin(), m_rbds_callsign.end());
	auto trimpos = callsign.find('\0');
	if(trimpos != std::string::npos) callsign.erase(trimpos);

	// If this wasn't a nationally/regionally-linked station, append the "-FM" suffix
	return callsign + "-FM";
}

//---------------------------------------------------------------------------
// rdsdecoder::has_radiotextplus
//
// Flag indicating that the RadioText+ (RT+) ODA is present
//
// Arguments:
//
//	NONE

bool rdsdecoder::has_radiotextplus(void) const
{
	return m_oda_rtplus;
}

//---------------------------------------------------------------------------
// rdsdecoder::has_rbds_callsign
//
// Flag indicating that the RDBS call sign has been decoded
//
// Arguments:
//
//	NONE

bool rdsdecoder::has_rbds_callsign(void) const
{
	// SPECIAL CASE: If the first nibble of the RBDS PI code is 1 the callsign data
	// cannot be decoded if the RDS-TMC ODA is also present (NRSC-4-B, section D.4.7)
	if(((m_rbds_pi & 0x1000) == 0x1000) && (m_oda_rdstmc)) return false;

	return (!m_rbds_nationalcode.empty()) || (m_rbds_callsign[0] != '\0');
}

//---------------------------------------------------------------------------
// rdsdecoder::has_rdstmc
//
// Flag indicating that the Traffic Message Channel (RDS-TMC) ODA is present
//
// Arguments:
//
//	NONE

bool rdsdecoder::has_rdstmc(void) const
{
	return m_oda_rdstmc;
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
