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
#include "fmstream.h"

#pragma warning(push, 4)

// devicestream::DEFAULT_CHUNK_SIZE (static)
//
// Default stream chunk size
size_t const fmstream::DEFAULT_CHUNK_SIZE = (4 KiB);

// devicestream::DEFAULT_MEDIA_TYPE (static)
//
// Default media type to report for the stream
char const* fmstream::DEFAULT_MEDIA_TYPE = "audio/wav";		// <-- TODO

//---------------------------------------------------------------------------
// fmstream Constructor (private)
//
// Arguments:
//
//	chunksize	- Chunk size to use for the stream
//	frequency	- Frequency of the FM stream, specified in hertz

fmstream::fmstream(uint32_t frequency, size_t chunksize) : m_chunksize(chunksize)
{
	// Create the RTL-SDR device instance and tune to the specified frequency
	m_device = rtldevice::create(rtldevice::DEFAULT_DEVICE_INDEX);
	m_device->frequency(frequency);
}

//---------------------------------------------------------------------------
// fmstream Destructor

fmstream::~fmstream()
{
	close();
}

//---------------------------------------------------------------------------
// fmstream::canseek
//
// Gets a flag indicating if the stream allows seek operations
//
// Arguments:
//
//	NONE

bool fmstream::canseek(void) const
{
	return false;
}

//---------------------------------------------------------------------------
// fmstream::chunksize
//
// Gets the stream chunk size
//
// Arguments:
//
//	NONE

size_t fmstream::chunksize(void) const
{
	return m_chunksize;
}

//---------------------------------------------------------------------------
// fmstream::close
//
// Closes the stream
//
// Arguments:
//
//	NONE

void fmstream::close(void)
{
	m_device.reset();
}

//---------------------------------------------------------------------------
// fmstream::create (static)
//
// Factory method, creates a new fmstream instance
//
// Arguments:
//
//	frequency	- Frequency of the FM stream, specified in hertz

std::unique_ptr<fmstream> fmstream::create(uint32_t frequency)
{
	return create(frequency, DEFAULT_CHUNK_SIZE);
}

//---------------------------------------------------------------------------
// fmstream::create (static)
//
// Factory method, creates a new fmstream instance
//
// Arguments:
//
//	chunksize	- Chunk size to use for the stream
//	frequency	- Frequency of the FM stream, specified in hertz

std::unique_ptr<fmstream> fmstream::create(uint32_t frequency, size_t chunksize)
{
	return std::unique_ptr<fmstream>(new fmstream(frequency, chunksize));
}

//---------------------------------------------------------------------------
// fmstream::length
//
// Gets the length of the stream; or -1 if stream is real-time
//
// Arguments:
//
//	NONE

long long fmstream::length(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// fmstream::mediatype
//
// Gets the media type of the stream
//
// Arguments:
//
//	NONE

char const* fmstream::mediatype(void) const
{
	return DEFAULT_MEDIA_TYPE;
}

//---------------------------------------------------------------------------
// fmstream::position
//
// Gets the current position of the stream
//
// Arguments:
//
//	NONE

long long fmstream::position(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// fmstream::read
//
// Reads data from the live stream
//
// Arguments:
//
//	buffer		- Buffer to receive the live stream data
//	count		- Size of the destination buffer in bytes

size_t fmstream::read(uint8_t* buffer, size_t count)
{
	assert(m_device);

	return m_device->read(buffer, count);
}

//---------------------------------------------------------------------------
// fmstream::realtime
//
// Gets a flag indicating if the stream is real-time
//
// Arguments:
//
//	NONE

bool fmstream::realtime(void) const
{
	return true;
}

//---------------------------------------------------------------------------
// fmstream::seek
//
// Sets the stream pointer to a specific position
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

long long fmstream::seek(long long /*position*/, int /*whence*/)
{
	return -1;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
