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

#ifndef __FMSTREAM_H_
#define __FMSTREAM_H_
#pragma once

#pragma warning(push, 4)

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "pvrstream.h"
#include "rtldevice.h"
#include "scalar_condition.h"

//---------------------------------------------------------------------------
// Class fmstream
//
// Implements a FM radio stream

class fmstream : public pvrstream
{
public:

	// Destructor
	//
	virtual ~fmstream();

	//-----------------------------------------------------------------------
	// Member Functions

	// canseek
	//
	// Flag indicating if the stream allows seek operations
	bool canseek(void) const;

	// chunksize
	//
	// Gets the stream chunk size
	size_t chunksize(void) const;

	// close
	//
	// Closes the stream
	void close(void);

	// create (static)
	//
	// Factory method, creates a new fmstream instance
	static std::unique_ptr<fmstream> create(struct pvrstream::allocator const& alloc, uint32_t frequency);
	static std::unique_ptr<fmstream> create(struct pvrstream::allocator const& alloc, uint32_t frequency, size_t chunksize);

	// demuxabort
	//
	// Aborts the demultiplexer
	void demuxabort(void);

	// demuxflush
	//
	// Flushes the demultiplexer
	void demuxflush(void);

	// demuxread
	//
	// Reads the next packet from the demultiplexer
	DemuxPacket* demuxread(void);

	// demuxreset
	//
	// Resets the demultiplexer
	void demuxreset(void);

	// length
	//
	// Gets the length of the stream
	long long length(void) const;

	// mediatype
	//
	// Gets the media type of the stream
	char const* mediatype(void) const;

	// position
	//
	// Gets the current position of the stream
	long long position(void) const;

	// read
	//
	// Reads available data from the stream
	size_t read(uint8_t* buffer, size_t count);

	// realtime
	//
	// Gets a flag indicating if the stream is real-time
	bool realtime(void) const;

	// seek
	//
	// Sets the stream pointer to a specific position
	long long seek(long long position, int whence);

private:

	fmstream(fmstream const&) = delete;
	fmstream& operator=(fmstream const&) = delete;

	// DEFAULT_DEVICE_BLOCK_SIZE
	//
	// Default device block size, in bytes
	static size_t const DEFAULT_DEVICE_BLOCK_SIZE;

	// DEFAULT_MEDIA_TYPE
	//
	// Default media type to report for the stream
	static char const* DEFAULT_MEDIA_TYPE;

	// DEFAULT_READ_MIN
	//
	// Default minimum amount of data to return from a read request
	static size_t const DEFAULT_READ_MINCOUNT;

	// DEFAULT_READ_TIMEOUT_MS
	//
	// Default amount of time for a read operation to succeed
	static unsigned int const DEFAULT_READ_TIMEOUT_MS;

	// DEFAULT_RINGBUFFER_SIZE
	//
	// Default ring buffer size, in bytes
	static size_t const DEFAULT_RINGBUFFER_SIZE;

	// DEFAULT_STREAM_CHUNK_SIZE
	//
	// Default stream chunk size
	static size_t const DEFAULT_STREAM_CHUNK_SIZE;

	// Instance Constructor
	//
	fmstream(struct pvrstream::allocator const& packetalloc, uint32_t frequency, size_t chunksize);

	//-----------------------------------------------------------------------
	// Private Member Functions

	// transfer
	//
	// Worker thread procedure used to transfer data into the ring buffer
	void transfer(scalar_condition<bool>& started);

	//-----------------------------------------------------------------------
	// Member Variables

	struct pvrstream::allocator const	m_alloc;				// Demux packet allocators
	std::unique_ptr<rtldevice>			m_device;				// RTL-SDR device instance
	std::unique_ptr<FmDecoder>			m_decoder;				// SoftFM decoder instance

	// STREAM PROPERTIES
	//
	size_t const						m_chunksize;			// Stream chunk size
	size_t const						m_readmincount;			// Minimum read byte count
	unsigned int const					m_readtimeout;			// Read timeout in milliseconds

	// STREAM CONTROL
	//
	mutable std::mutex					m_lock;					// Synchronization object
	std::condition_variable				m_cv;					// Transfer event condvar
	std::thread							m_worker;				// Data transfer thread
	scalar_condition<bool>				m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>					m_stopped{ false };		// Data transfer stopped flag

	// RING BUFFER
	//
	size_t const						m_buffersize;			// Size of the ring buffer
	std::unique_ptr<uint8_t[]>			m_buffer;				// Ring buffer stroage
	std::atomic<size_t>					m_bufferhead{ 0 };		// Head (write) buffer position
	std::atomic<size_t>					m_buffertail{ 0 };		// Tail (read) buffer position
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __FMSTREAM_H_
