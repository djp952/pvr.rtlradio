//-----------------------------------------------------------------------------
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

// fmprops
//
// Defines properties for the FM digital signal processor
struct fmprops {

	bool			decoderds;			// Flag if RDS should be decoded or not
	bool			isrbds;				// Flag if region is RBDS (North America)
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

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PROPS_H_
