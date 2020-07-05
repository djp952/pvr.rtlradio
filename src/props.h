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

#pragma warning(push, 4)

// channelprops
//
// Defines properties for a channel detected via a scan
struct channelprops {

	uint32_t		frequency;		// Station center frequency
	char const*		callsign;		// Station call sign
	int				gain;			// Default gain value as 10*dB (i.e. 32.8dB = 328)
};

// deviceprops
//
// Defines RTL-SDR device specific properties
struct deviceprops {

	uint32_t		samplerate;		// Device sample rate in hertz
	bool			agc;			// Enable/disable device automatic gain control
	int				manualgain;		// Device manual gain value as 10*dB (i.e. 32.8dB = 328)
};

// fmprops
//
// Defines FM Radio DSP specific properties
struct fmprops {

	uint32_t		frequency;		// Station center frequency
	uint32_t		samplerate;		// Output sample rate, in hertz

	int				hicut;			// 5000
	int				lowcut;			// -5000
	//int			offset;			// 0
	int				agcslope;		// 0
	int				agcthresh;		// -100
	int				agcmanualgain;	// 30
	int				agcdecay;		// 200
	bool			agcon;			// true
	bool			agchangon;		// false
};

// hdradioprops
//
// Defines HD Radio DSP specific properties
//struct hdradioprops {
//};

// streamprops
//
// Defines stream-specific properties
struct streamprops {

	char const*		codec;			// Stream codec name
	int				pid;			// Stream PID
	int				channels;		// Stream number of channels
	int				samplerate;		// Stream sample rate
	int				bitspersample;	// Stream bits per sample
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __PROPS_H_
