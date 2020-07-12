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
#include "fmscanner.h"

#include <string>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// fmscanner::scan (static)
//
// Scans for FM radio channels
//
// Arguments:
//
//	deviceprops		- RTL-SDR device properties
//	callback		- Callback function to invoke when a channel is found

void fmscanner::scan(struct deviceprops const& /*deviceprops*/, scan_callback const& callback)
{
	struct channelprops props = {};

	// DUMMY CHANNEL 1 - 95.1 WRBS-FM
	std::string channel_1_name("WRBS-FM");
	props.frequency = 95100000;
	props.subchannel = 0;
	props.callsign = channel_1_name.c_str();
	props.autogain = false;
	props.manualgain = 328;
	callback(props);

	// DUMMY CHANNEL 2 - 99.1 WDCH-FM
	std::string channel_2_name("WDCH-FM");
	props.frequency = 99100000;
	props.subchannel = 0;
	props.callsign = channel_2_name.c_str();
	props.autogain = false;
	props.manualgain = 328;
	callback(props);
}

//---------------------------------------------------------------------------

#pragma warning(pop)
