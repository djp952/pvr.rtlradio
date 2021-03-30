//-----------------------------------------------------------------------------
// Copyright (c) 2020-2021 Michael G. Brehm
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

#ifndef __PROPS_H_
#define __PROPS_H_
#pragma once

#include <stdint.h>
#include <string>

#pragma warning(push, 4)

// channelprops
//
// Defines properties for a radio channel
struct channelprops {

	uint32_t		frequency;			// Station center frequency
	uint32_t		subchannel;			// Subchannel (HD Radio only)
	std::string		name;				// Station name / call sign
	std::string		logourl;			// Station logo URL
	bool			autogain;			// Flag indicating if automatic gain should be used
	int				manualgain;			// Manual gain value as 10*dB (i.e. 32.8dB = 328)
};

// channeltype
//
// Defines the type of a channel
enum channeltype {

	fmradio			= 0,				// Wideband FM radio
	hdradio			= 1,				// Hybrid Digital (HD) radio
	wxradio			= 2,				// VHF Weather radio
};

// fmprops
//
// Defines properties for the FM Radio digital signal processor
struct fmprops {

	bool			decoderds;			// Flag if RDS should be decoded or not
	bool			isrbds;				// Flag if region is RBDS (North America)
	int				downsamplequality;	// Downsample quality setting
	uint32_t		outputrate;			// Output sample rate in Hertz
	float			outputgain;			// Output gain in Decibels
};

// streamprops
//
// Defines stream-specific properties
struct streamprops {

	char const*		codec;				// Stream codec name
	int				pid;				// Stream PID
	int				channels;			// Stream number of channels
	int				samplerate;			// Stream sample rate
	int				bitspersample;		// Stream bits per sample
};

// tunerprops
//
// Defines tuner-specific properties
struct tunerprops {

	uint32_t		samplerate;			// Input sample rate in Hertz
	int				freqcorrection;		// Frequency correction (PPM)
};

// wxprops
//
// Defines properties for the Weather Radio digital signal processor
struct wxprops {

	uint32_t		outputrate;			// Output sample rate in Hertz
	float			outputgain;			// Output gain in Decibels
};

// get_channel_type (inline)
//
// Determines the channeltype value for a channelprops structure
inline enum channeltype get_channel_type(struct channelprops const& channelprops)
{
	// FM Radio / HD Radio
	if((channelprops.frequency >= 87500000) && (channelprops.frequency <= 108000000))
		return (channelprops.subchannel == 0) ? channeltype::fmradio : channeltype::hdradio;

	// Weather Radio
	else if((channelprops.frequency >= 162400000) && (channelprops.frequency <= 162550000) && (channelprops.subchannel == 0))
		return channeltype::wxradio;

	// Unknown; assume FM Radio
	return channeltype::fmradio;
}

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PROPS_H_
