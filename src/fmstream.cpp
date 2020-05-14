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

#include <algorithm>

#pragma warning(disable:4267)
#include <FmDecode.h>
#pragma warning(default:4267)

#pragma warning(push, 4)

// devicestream::DEFAULT_DEVICE_BLOCK_SIZE (static)
//
// Default device block size
size_t const fmstream::DEFAULT_DEVICE_BLOCK_SIZE = (64 KiB);

// devicestream::DEFAULT_MEDIA_TYPE (static)
//
// Default media type to report for the stream
char const* fmstream::DEFAULT_MEDIA_TYPE = "audio/wav";		// <-- TODO

// dvrstream::DEFAULT_READ_MIN (static)
//
// Default minimum amount of data to return from a read request
size_t const fmstream::DEFAULT_READ_MINCOUNT = (1 KiB);

// dvrstream::DEFAULT_READ_TIMEOUT_MS (static)
//
// Default amount of time for a read operation to succeed
unsigned int const fmstream::DEFAULT_READ_TIMEOUT_MS = 2500;

// httpstream::DEFAULT_RINGBUFFER_SIZE (static)
//
// Default ring buffer size, in bytes
size_t const fmstream::DEFAULT_RINGBUFFER_SIZE = (2 MiB);

// devicestream::DEFAULT_STREAM_CHUNK_SIZE (static)
//
// Default stream chunk size
size_t const fmstream::DEFAULT_STREAM_CHUNK_SIZE = (4 KiB);

//---------------------------------------------------------------------------
// fmstream Constructor (private)
//
// Arguments:
//
//	chunksize	- Chunk size to use for the stream
//	frequency	- Frequency of the FM stream, specified in hertz

fmstream::fmstream(uint32_t frequency, size_t chunksize) : m_chunksize(chunksize), m_readmincount(DEFAULT_READ_MINCOUNT), 
	m_readtimeout(DEFAULT_READ_TIMEOUT_MS), m_buffersize(DEFAULT_RINGBUFFER_SIZE)
{
	// Allocate the ring buffer
	m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[m_buffersize]);
	if(!m_buffer) throw std::bad_alloc();

	// Create the RTL-SDR device instance and tune to the specified frequency
	m_device = rtldevice::create(rtldevice::DEFAULT_DEVICE_INDEX);
	m_device->frequency(frequency);

	// Create a worker thread on which to perform the transfer operations
	scalar_condition<bool> started{ false };
	m_worker = std::thread(&fmstream::transfer, this, std::ref(started));
	started.wait_until_equals(true);
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
	// Signal the worker thread to stop and wait for it to do so
	m_stop = true;
	if(m_worker.joinable()) m_worker.join();

	// Release the RTL-SDR device instance
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
	return create(frequency, DEFAULT_STREAM_CHUNK_SIZE);
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
	size_t				bytesread = 0;			// Total bytes actually read
	size_t				head = 0;				// Current head position
	size_t				tail = 0;				// Current tail position
	size_t				available = 0;			// Available bytes to read
	bool				stopped = false;		// Flag if data transfer has stopped

	// Verify that the read timeout has been set to at least one millisecond
	assert(m_readtimeout >= 1U);

	if(buffer == nullptr) throw std::invalid_argument("buffer");
	if(count > m_buffersize) throw std::invalid_argument("count");
	if(count == 0) return 0;

	std::unique_lock<std::mutex> lock(m_lock);

	// Wait up to the timeout for there to be the minimum amount of available data in the
	// buffer, if there is not the condvar will be triggered on the next write or a thread stop.
	if(m_cv.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(m_readtimeout), [&]() -> bool {

		tail = m_buffertail.load();				// Copy the atomic<> tail position
		head = m_bufferhead.load();				// Copy the atomic<> head position
		stopped = m_stopped.load();				// Copy the atomic<> stopped flag

		// Calculate the amount of space available in the buffer
		available = (tail > head) ? (m_buffersize - tail) + head : head - tail;

		// The result from the predicate is true if enough data or stopped
		return ((available >= m_readmincount) || (stopped));

	}) == false) return 0;

	// If the wait loop was broken by the worker thread stopping, make one more pass
	// to ensure that no additional data was first written by the thread
	if((available < m_readmincount) && (stopped)) {

		tail = m_buffertail.load();				// Copy the atomic<> tail position
		head = m_bufferhead.load();				// Copy the atomic<> head position

		// Calculate the amount of space available in the buffer
		available = (tail > head) ? (m_buffersize - tail) + head : head - tail;
	}

	// Copy the available data into the destination buffer
	count = std::min(available, count);
	while(count) {

		// If the tail is behind the head linearly, take the data between them otherwise
		// take the data between the end of the buffer and the tail
		size_t chunk = (tail < head) ? std::min(count, head - tail) : std::min(count, m_buffersize - tail);
		memcpy(&buffer[bytesread], &m_buffer[tail], chunk);

		tail += chunk;						// Increment the tail position
		bytesread += chunk;					// Increment number of bytes read
		count -= chunk;						// Decrement remaining bytes

		// If the tail has reached the end of the buffer, reset it back to zero
		if(tail >= m_buffersize) tail = 0;
	}

	m_buffertail.store(tail);				// Modify atomic<> tail position

	return bytesread;
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
// fmstream::transfer (private)
//
// Worker thread procedure used to transfer data into the ring buffer
//
// Arguments:
//
//	started		- Condition variable to set when thread has started

void fmstream::transfer(scalar_condition<bool>& started)
{
	std::vector<uint8_t> block(DEFAULT_DEVICE_BLOCK_SIZE);		// Device data block

	assert(m_device);

	m_device->stream();			// Begin streaming from the device
	started = true;				// Signal that the thread has started

	// Continue to write data from the device into the ring buffer until told to stop
	while(m_stop.test(false) == true) {

		size_t		head = 0;					// Current ring buffer head position
		size_t		tail = 0;					// Current ring buffer tail position
		size_t		available = 0;				// Available ring buffer space
		size_t		byteswritten = 0;			// Bytes written into the ring buffer

		// Read the next block of data from the device
		size_t cb = m_device->read(block.data(), block.size());

		do {

			// Copy the current head and tail positions, this works without a lock by operating
			// on the state of the buffer at the time of the request
			head = m_bufferhead.load();
			tail = m_buffertail.load();

			// Check to ensure that there is enough space in the ring buffer to hold the data
			available = (head < tail) ? tail - head : (m_buffersize - head) + tail;

		} while((available < cb) && (m_stop.wait_until_equals(true, 10) == false));

		// Stop the thread if that was the reason the loop terminated
		if(available < cb) break;

		// Copy the device data into the ring buffer
		while(cb) {

			// If the head is behind the tail linearly, take the data between them otherwise
			// take the data between the end of the buffer and the head
			size_t chunk = (head < tail) ? std::min(cb, tail - head) : std::min(cb, m_buffersize - head);
			memcpy(&m_buffer[head], &reinterpret_cast<uint8_t const*>(block.data())[byteswritten], chunk);

			head += chunk;					// Increment the head position
			byteswritten += chunk;			// Increment number of bytes written
			cb -= chunk;					// Decrement remaining bytes

			// If the head has reached the end of the buffer, reset it back to zero
			if(head >= m_buffersize) head = 0;
		}

		// Adjust the atomic<> head position and notify that data has been written
		m_bufferhead.store(head);
		m_cv.notify_all();
	}

	m_stopped.store(true);					// Thread is stopped
	m_cv.notify_all();						// Notify any waiters
}

//---------------------------------------------------------------------------

#pragma warning(pop)
