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
#include <chrono>
#include <memory.h>

#include "align.h"

#pragma warning(push, 4)

// fmstream::DEFAULT_DEVICE_BLOCK_SIZE
//
// Default device block size
size_t const fmstream::DEFAULT_DEVICE_BLOCK_SIZE = (16 KiB);

// fmstream::DEFAULT_DEVICE_SAMPLE_RATE
//
// Default device sample rate
uint32_t const fmstream::DEFAULT_DEVICE_SAMPLE_RATE = (1 MHz);

// fmstream::DEFAULT_OUTPUT_CHANNELS
//
// Default number of PCM output channels
int const fmstream::DEFAULT_OUTPUT_CHANNELS = 1;

// fmstream::DEFAULT_OUTPUT_SAMPLE_RATE
//
// Default PCM output sample rate
double const fmstream::DEFAULT_OUTPUT_SAMPLE_RATE = (48.0 KHz);

// fmstream::DEFAULT_RINGBUFFER_SIZE
//
// Default ring buffer size
size_t const fmstream::DEFAULT_RINGBUFFER_SIZE = (8 MiB);

//---------------------------------------------------------------------------
// fmstream Constructor (private)
//
// Arguments:
//
//	params		- Stream creation parameters

fmstream::fmstream(struct streamparams const& params) : m_params(params), 
	m_blocksize(align::up(DEFAULT_DEVICE_BLOCK_SIZE, 16 KiB)),
	m_samplerate(DEFAULT_DEVICE_SAMPLE_RATE), m_pcmsamplerate(DEFAULT_OUTPUT_SAMPLE_RATE),
	m_buffersize(align::up(DEFAULT_RINGBUFFER_SIZE, 16 KiB))
{
	assert(params.demuxalloc && params.demuxfree);

	// Allocate the ring buffer
	m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[m_buffersize]);
	if(!m_buffer) throw std::bad_alloc();

	// TODO: "DC offset" tuning?

	// Create and initialize the RTL-SDR device instance
	m_device = rtldevice::create(rtldevice::DEFAULT_DEVICE_INDEX);
	uint32_t samplerate = m_device->set_sample_rate(m_samplerate);
	/*uint32_t frequency =*/ m_device->set_center_frequency(params.frequency);
	m_device->set_bandwidth(200 KHz);

	// Adjust the device gain as specified by the stream parameters
	// TODO: do I want to use these or not?  Probably manual gain at least?
	//m_device->set_automatic_gain_control(params.agc);
	//if(params.agc == false) m_device->set_gain(params.gain * 10);

	// Create and initialize the CDemodulator instance
	tDemodInfo t{};

	// FIXED SETTINGS
	//
	t.txt.assign("WFM");
	t.HiCutmin = 100000;
	t.HiCutmax = 100000;
	t.LowCutmax = -100000;
	t.LowCutmin = -100000;
	t.Symetric = true;
	t.DefFreqClickResolution = 100000;
	t.FilterClickResolution = 10000;

	// VARIABLE SETTINGS
	//
	// TODO: These should come in via configuration parameters
	t.HiCut = 5000;
	t.LowCut = -5000;
	t.FreqClickResolution = t.DefFreqClickResolution;
	t.Offset = 0;
	t.SquelchValue = -160;
	t.AgcSlope = 0;
	t.AgcThresh = -100;
	t.AgcManualGain = 30;
	t.AgcDecay = 200;
	t.AgcOn = true;
	t.AgcHangOn = false;

	m_demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
	m_demodulator->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
	m_demodulator->SetDemod(DEMOD_WFM, t);
	//m_demodulator->SetDemodFreq(frequency);	// <-- TODO: What does this do?
	m_demodulator->SetUSFmVersion(true);

	// TODO: How big does this really need to be?
	m_resampler = std::unique_ptr<CFractResampler>(new CFractResampler());
	m_resampler->Init(512000);

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
// fmstream::close
//
// Closes the stream
//
// Arguments:
//
//	NONE

void fmstream::close(void)
{
	m_stop = true;								// Signal worker thread to stop
	if(m_device) m_device->cancel_async();		// Cancel any async read operations
	if(m_worker.joinable()) m_worker.join();	// Wait for thread
	m_device.reset();							// Release RTL-SDR device
}

//---------------------------------------------------------------------------
// fmstream::create (static)
//
// Factory method, creates a new fmstream instance
//
// Arguments:
//
//	params		- Stream creation parameters

std::unique_ptr<fmstream> fmstream::create(struct streamparams const& params)
{
	return std::unique_ptr<fmstream>(new fmstream(params));
}

//---------------------------------------------------------------------------
// fmstream::demuxabort
//
// Aborts the demultiplexer
//
// Arguments:
//
//	NONE

void fmstream::demuxabort(void)
{
}

//---------------------------------------------------------------------------
// fmstream::demuxflush
//
// Flushes the demultiplexer
//
// Arguments:
//
//	NONE

void fmstream::demuxflush(void)
{
}

//---------------------------------------------------------------------------
// fmstream::demuxread
//
// Reads the next packet from the demultiplexer
//
// Arguments:
//
//	NONE

DemuxPacket* fmstream::demuxread(void)
{
	size_t				head = 0;				// Current head position
	size_t				tail = 0;				// Current tail position
	size_t				available = 0;			// Available bytes to read
	bool				stopped = false;		// Flag if data transfer has stopped

	// Use a constant read size of 20K (10Khz worth of I/Q samples), the demodulator
	// is designed to work with about 1/10th of a second of data, and this typically
	// provides exactly 480 audio samples which is a good match for 48000Hz PCM output
	// TODO: Does 9984 or 10000 work better here?
	size_t const minreadsize = 9984 * 2;

	// Wait up to 10ms for there to be available data in the ring buffer
	std::unique_lock<std::mutex> lock(m_lock);
	if(m_cv.wait_until(lock, std::chrono::system_clock::now() + std::chrono::milliseconds(10), [&]() -> bool {

		tail = m_buffertail.load();				// Copy the atomic<> tail position
		head = m_bufferhead.load();				// Copy the atomic<> head position
		stopped = m_stopped.load();				// Copy the atomic<> stopped flag

		// Calculate the amount of space available in the buffer (2 byte alignment)
		available = align::down((tail > head) ? (m_buffersize - tail) + head : head - tail, 2);

		// The result from the predicate is true if data available or stopped
		return ((available >= minreadsize) || (stopped));

	}) == false) return 0;

	// If the wait loop was broken by the worker thread stopping, make one more pass
	// to ensure that no additional data was first written by the thread
	if((available < minreadsize) && (stopped)) {

		tail = m_buffertail.load();				// Copy the atomic<> tail position
		head = m_bufferhead.load();				// Copy the atomic<> head position

		// Calculate the amount of space available in the buffer (2 byte alignment)
		available = align::down((tail > head) ? (m_buffersize - tail) + head : head - tail, 2);
	}

	// If there is still not enough data to be processed, return an empty demuxer packet
	if(available < minreadsize) return m_params.demuxalloc(0);

	// Convert the raw data into a vector<> of I/Q samples for the demodulator
	assert(available % 2 == 0);
	available = std::min(available, minreadsize);

	std::vector<TYPECPX> samples(available / 2);
	for(size_t index = 0; index < samples.size(); index++) {

		// TODO: Performance here.  Demodulator seems to work just as well without converting the
		// -1/+1 value to -32767/+32767 and the double cast may be too much for slow systems
		samples[index].re = ((static_cast<TYPEREAL>(m_buffer[tail]) - TYPEREAL(127.5)) / TYPEREAL(127.5)) * TYPEREAL(32767.0);
		samples[index].im = ((static_cast<TYPEREAL>(m_buffer[tail + 1]) - TYPEREAL(127.5)) / TYPEREAL(127.5)) * TYPEREAL(32767.0);

		tail += 2;								// Increment new tail position
		if(tail >= m_buffersize) tail = 0;		// Handle buffer rollover
	}

	// Modify the atomic<> tail position to mark the ring buffer space as free
	m_buffertail.store(tail);

	// Process the raw I/Q data, the original sample vector<> can be reused/overwritten as it's processed
	int audiopackets = m_demodulator->ProcessData(static_cast<int>(samples.size()), samples.data(), samples.data());

	// Determine the size of the demuxer packet data and allocate it
	int packetsize = audiopackets * sizeof(TYPESTEREO16);
	DemuxPacket* packet = m_params.demuxalloc(packetsize);
	if(packet == nullptr) return nullptr;

	// Resample the audio data directly into the packet buffer
	// TODO: Gain argument - should always be 1.0?
	// TODO: Use constant for 48000.0
	int stereopackets = m_resampler->Resample(audiopackets, m_demodulator->GetOutputRate() / TYPEREAL(48000.0), samples.data(),
		reinterpret_cast<TYPESTEREO16*>(packet->pData), 1.0);

	// Calcuate the duration of the demultiplexer packet, in microseconds
	double duration = ((stereopackets / m_pcmsamplerate) US);

	// Set up the demultiplexer packet with the proper packet size and duration
	// TODO: Use constant for stream ID
	packet->iStreamId = 1;
	packet->iSize = stereopackets * sizeof(TYPESTEREO16);
	packet->duration = duration;
	packet->pts = m_pts;

	// Increment the program time stamp value based on the calculated duration
	m_pts += duration;

	return packet;
}

//---------------------------------------------------------------------------
// fmstream::demuxreset
//
// Resets the demultiplexer
//
// Arguments:
//
//	NONE

void fmstream::demuxreset(void)
{
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

size_t fmstream::read(uint8_t* /*buffer*/, size_t /*count*/)
{
	return 0;
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
// fmstream::samplerate
//
// Gets the sample rate of the stream
//
// Arguments:
//
//	NONE

int fmstream::samplerate(void) const
{
	return static_cast<int>(m_pcmsamplerate);
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
	assert(m_device);

	m_device->begin_stream();		// Begin streaming from the device
	started = true;					// Signal that the thread has started

	// Loop until the worker thread has been signaled to stop
	while(m_stop.test(false) == true) {

		size_t		head = 0;				// Ring buffer head (write) position
		size_t		tail = 0;				// Ring buffer tail (read) position
		size_t		available = 0;			// Amount of available ring buffer space

		// Wait up to 10ms for the ring buffer to have enough space to write the next block
		do {

			head = m_bufferhead.load();			// Acquire the current buffer head position
			tail = m_buffertail.load();			// Acquire the current buffer tail position

			// Calculate the total amount of available free space in the ring buffer
			available = (head < tail) ? tail - head : (m_buffersize - head) + tail;

		} while((available < DEFAULT_DEVICE_BLOCK_SIZE) && (m_stop.wait_until_equals(true, 10) == false));

		// If the wait loop was broken due to a stop signal, terminate the thread
		if(available < DEFAULT_DEVICE_BLOCK_SIZE) break;

		// Transfer the available data from the device into the ring buffer
		size_t count = DEFAULT_DEVICE_BLOCK_SIZE;
		while(count) {

			// If the head is behind the tail linearly, take the data between them otherwise 
			// take the data between the end of the buffer and the head
			size_t chunk = (head < tail) ? std::min(count, tail - head) : std::min(count, m_buffersize - head);

			// Read the chunk directly into the ring buffer
			chunk = m_device->read(&m_buffer[head], chunk);

			head += chunk;				// Increment the head position
			count -= chunk;				// Decrement remaining bytes to be transferred

			// If the head has reached the end of the buffer, reset it back to zero
			if(head >= m_buffersize) head = 0;
		}

		// Adjust the atomic<> head position and notify that new data is available
		m_bufferhead.store(head);
		m_cv.notify_all();
	}

	m_stopped.store(true);					// Thread is stopped
	m_cv.notify_all();						// Notify any waiters
}

//---------------------------------------------------------------------------

#pragma warning(pop)
