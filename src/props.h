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

#ifndef __PROPS_H_
#define __PROPS_H_
#pragma once

#include <stdint.h>
#include <string>

#pragma warning(push, 4)

enum class modulation;

// channelprops
//
// Defines properties for a radio channel
struct channelprops {

	uint32_t			frequency;			// Center frequency
	enum modulation		modulation;			// Modulation
	std::string			name;				// Channel name
	std::string			logourl;			// Channel logo URL
	bool				autogain;			// Flag indicating if automatic gain should be used
	int					manualgain;			// Manual gain value as 10*dB (i.e. 32.8dB = 328)
	int					freqcorrection;		// Frequency correction for this channel
};

// dabprops
//
// Defines properties for the DAB digital signal processor
struct dabprops {

	float			outputgain;			// Output gain in Decibels
};

// fmprops
//
// Defines properties for the FM Radio digital signal processor
struct fmprops {

	bool			decoderds;			// Flag if RDS should be decoded or not
	bool			isnorthamerica;		// Flag if region is North America
	uint32_t		samplerate;			// Input sample rate in Hertz
	int				downsamplequality;	// Downsample quality setting
	uint32_t		outputrate;			// Output sample rate in Hertz
	float			outputgain;			// Output gain in Decibels
};

// hdprops
//
// Defines properties for the HD Radio digital signal processor
struct hdprops {

	float			outputgain;			// Output gain in Decibels
};

// modulation
//
// Defines the modulation of a channel
enum class modulation {

	fm		= 0,			// Wideband FM radio
	hd		= 1,			// Hybrid Digital radio
	dab		= 2,			// Digital Audio Broadcast radio
	wx		= 3,			// VHF Weather radio
};

// regioncode
//
// Defines the possible region codes
enum class regioncode {

	notset			= 0,	// Unknown
	world			= 1,	// FM
	northamerica	= 2,	// FM/HD/WX
	europe			= 3,	// FM/DAB
};

// signalplotprops
//
// Defines signal meter plot properties
struct signalplotprops {

	size_t			height;				// Plot height
	size_t			width;				// Plot width
	float			mindb;				// Plot minimum dB value
	float			maxdb;				// Plot maximum dB value
};

// signalprops
//
// Defines signal-specific properties
struct signalprops {

	uint32_t		samplerate;			// Signal sampling rate
	uint32_t		bandwidth;			// Signal bandwidth
	int32_t			lowcut;				// low cut from center
	int32_t			highcut;			// high cut from center
	uint32_t		offset;				// Signal frequency offset
	bool			filter;				// Flag to filter the signal
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

// subchannelprops
//
// Defines properties for a radio subchannel
struct subchannelprops {

	uint32_t			number;			// Subchannel number
	std::string			name;			// Subchannel name
	std::string			logourl;		// Subchannel logo URL
};

// tunerprops
//
// Defines tuner-specific properties
struct tunerprops {

	int				freqcorrection;		// Frequency correction (PPM)
};

// wxprops
//
// Defines properties for the Weather Radio digital signal processor
struct wxprops {

	uint32_t		samplerate;			// Input sample rate in Hertz
	uint32_t		outputrate;			// Output sample rate in Hertz
	float			outputgain;			// Output gain in Decibels
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PROPS_H_
