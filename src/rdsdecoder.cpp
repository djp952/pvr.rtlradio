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
}

//---------------------------------------------------------------------------
// rdsdecoder Destructor

rdsdecoder::~rdsdecoder()
{
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
	unsigned int grouptype = (rdsgroup.BlockB >> 11) & 0x1F;
	//bool versioncode = ((rdsgroup.BlockB >> 11) & 0x01) == 0x01;

	//uint16_t pIdent = blockData[BLOCK__A] & 0xFFFF;                // "PI"
	//if(pIdent != m_ProgramIdentCode)
	//	Decode_PI(pIdent);

	//int pType = (blockData[BLOCK__B] >> 5) & 0x1F;            // "PTY"
	//if(pType != m_PTY)
	//	Decode_PTY(pType);

	switch(grouptype) {
		 
		case GRPTYPE_0A:
		case GRPTYPE_0B:
			//Decode_Type0___PS_DI_MS(blockData, VersionCode);
			break;
		case GRPTYPE_1A:
		case GRPTYPE_1B:
			//Decode_Type1___ProgItemNum_SlowLabelCodes(blockData, VersionCode);
			break;
		case GRPTYPE_2A:
		case GRPTYPE_2B:
			//Decode_Type2___Radiotext(blockData, VersionCode);
			break;
		case GRPTYPE_3A:
			//Decode_Type3A__AppIdentOpenData(blockData);
			break;
		case GRPTYPE_4A:
			//Decode_Type4A__Clock(blockData);
			break;
		case GRPTYPE_5A:
		case GRPTYPE_5B:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type5___TransparentDataChannels(blockData, VersionCode);
			break;
		case GRPTYPE_6A:
		case GRPTYPE_6B:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type6___InHouseApplications(blockData, VersionCode);
			break;
		case GRPTYPE_7A:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type7A__RadioPaging(blockData);
			break;
		case GRPTYPE_8A:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type8A__TrafficMessageChannel(blockData);
			break;
		case GRPTYPE_9A:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type9A__EmergencyWarningSystem(blockData);
			break;
		case GRPTYPE_10A:
			//Decode_Type10A_PTYN(blockData);
			break;
		case GRPTYPE_13A:
			//if(m_ODATypeMap[group_type] > 0)
			//	Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
			//else
			//	Decode_Type13A_EnhancedRadioPaging(blockData);
			break;
		case GRPTYPE_14A:
		case GRPTYPE_14B:
			//Decode_Type14__EnhancedOtherNetworksInfo(blockData, VersionCode);
			break;
		case GRPTYPE_15A:
			//Decode_Type15A_RBDS(blockData);
			break;
		case GRPTYPE_15B:
			//Decode_Type15B_FastSwitchingInfo(blockData);
			break;
		case GRPTYPE_3B:
		case GRPTYPE_4B:
		case GRPTYPE_7B:
		case GRPTYPE_8B:
		case GRPTYPE_9B:
		case GRPTYPE_10B:
		case GRPTYPE_11A:
		case GRPTYPE_11B:
		case GRPTYPE_12A:
		case GRPTYPE_12B:
		case GRPTYPE_13B:
		//	if(m_ODATypeMap[group_type] > 0)
		//	{
		//		Decode_Type____ODA(blockData, m_ODATypeMap[group_type]);
		//		break;
		//	}
			break;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::pop_uecp_packet
//
// Pops the topmost UECP packet out of the packet queue
//
// Arguments:
//
//	packet		- On success, contains the UECP packet data

bool rdsdecoder::pop_uecp_packet(uecp_packet& packet)
{
	if(m_uecppackets.empty()) return false;

	// Swap the front packet from the top of the queue and pop it off
	m_uecppackets.front().swap(packet);
	m_uecppackets.pop();

	return true;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
