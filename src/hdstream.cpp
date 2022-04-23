//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "hdstream.h"

#include <algorithm>
#include <chrono>
#include <memory.h>

#include "align.h"
#include "endianness.h"
#include "string_exception.h"

#pragma warning(push, 4)

// hdstream::STREAM_ID_AUDIO
//
// Stream identifier for the audio output stream
int const hdstream::STREAM_ID_AUDIO = 1;

//---------------------------------------------------------------------------
// hdstream Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//	channelprops	- Channel properties
//	hdprops			- HD Radio digital signal processor properties

hdstream::hdstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	struct channelprops const& channelprops, struct hdprops const& hdprops) :
	m_device(std::move(device)), m_muxname(""), m_pcmgain(powf(10.0f, hdprops.outputgain / 10.0f))
{
	// Initialize the RTL-SDR device instance
	m_device->set_frequency_correction(tunerprops.freqcorrection + channelprops.freqcorrection);
	m_device->set_sample_rate(1488375);
	m_device->set_center_frequency(channelprops.frequency);

	// Adjust the device gain as specified by the channel properties
	m_device->set_automatic_gain_control(channelprops.autogain);
	if(channelprops.autogain == false) m_device->set_gain(channelprops.manualgain);

	nrsc5_open_pipe(&m_nrsc5);							// Open NRSC5 in PIPE mode
	nrsc5_set_mode(m_nrsc5, NRSC5_MODE_FM);				// Set for FM mode
	nrsc5_set_callback(m_nrsc5, nrsc5_callback, this);	// Register the callback method

	// Create a worker thread on which to perform the data transfer operations
	scalar_condition<bool> started{ false };
	m_worker = std::thread(&hdstream::transfer, this, std::ref(started));
	started.wait_until_equals(true);
}

//---------------------------------------------------------------------------
// hdstream Destructor

hdstream::~hdstream()
{
	close();
}

//---------------------------------------------------------------------------
// hdstream::canseek
//
// Gets a flag indicating if the stream allows seek operations
//
// Arguments:
//
//	NONE

bool hdstream::canseek(void) const
{
	return false;
}

//---------------------------------------------------------------------------
// hdstream::close
//
// Closes the stream
//
// Arguments:
//
//	NONE

void hdstream::close(void)
{
	m_stop = true;								// Signal worker thread to stop
	if(m_device) m_device->cancel_async();		// Cancel any async read operations
	if(m_worker.joinable()) m_worker.join();	// Wait for thread

	nrsc5_close(m_nrsc5);						// Close NRSC5
	m_nrsc5 = nullptr;							// Reset NRSC5 API handle

	m_device.reset();							// Release RTL-SDR device
}

//---------------------------------------------------------------------------
// hdstream::create (static)
//
// Factory method, creates a new hdstream instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tunder device properties
//	channelprops	- Channel properties
//	hdprops			- HD Radio digital signal processor properties

std::unique_ptr<hdstream> hdstream::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	struct channelprops const& channelprops, struct hdprops const& hdprops)
{
	return std::unique_ptr<hdstream>(new hdstream(std::move(device), tunerprops, channelprops, hdprops));
}

//---------------------------------------------------------------------------
// hdstream::demuxabort
//
// Aborts the demultiplexer
//
// Arguments:
//
//	NONE

void hdstream::demuxabort(void)
{
}

//---------------------------------------------------------------------------
// hdstream::demuxflush
//
// Flushes the demultiplexer
//
// Arguments:
//
//	NONE

void hdstream::demuxflush(void)
{
}

//---------------------------------------------------------------------------
// hdstream::demuxread
//
// Reads the next packet from the demultiplexer
//
// Arguments:
//
//	allocator		- DemuxPacket allocation function

DEMUX_PACKET* hdstream::demuxread(std::function<DEMUX_PACKET*(int)> const& allocator)
{
	// Wait up to 100ms for there to be a packet available for processing, don't use
	// an unconditional wait here; unlike analog radio there won't always be data
	std::unique_lock<std::mutex> lock(m_queuelock);
	if(!m_cv.wait_for(lock, std::chrono::milliseconds(100), [&]() -> bool { return ((m_queue.size() > 0) || m_stopped.load() == true); }))
		return allocator(0);

	// If the worker thread was stopped, check for and re-throw any exception that occurred,
	// otherwise assume it was stopped normally and return an empty demultiplexer packet
	if(m_stopped.load() == true) {

		if(m_worker_exception) std::rethrow_exception(m_worker_exception);
		else return allocator(0);
	}

	// Pop off the topmost object from the queue<> and release the lock
	std::unique_ptr<demux_packet_t> audiopacket(std::move(m_queue.front()));
	m_queue.pop();
	lock.unlock();

	// Calcuate the duration of the audio data; the sample rate is always 44.1KHz
	double duration = (audiopacket->count / 2 / static_cast<double>(44100)) * STREAM_TIME_BASE;

	// Allocate and initialize the DEMUX_PACKET
	DEMUX_PACKET* packet = allocator(static_cast<int>(audiopacket->count * sizeof(int16_t)));
	if(packet != nullptr) {

		packet->iStreamId = STREAM_ID_AUDIO;
		packet->iSize = static_cast<int>(audiopacket->count * sizeof(int16_t));
		packet->duration = duration;
		packet->dts = packet->pts = m_dts;

		int16_t* data = reinterpret_cast<int16_t*>(&packet->pData[0]);
		for(size_t index = 0; index < audiopacket->count; index++) data[index] = static_cast<int16_t>(audiopacket->data[index] * m_pcmgain);
	}

	m_dts += duration;

	return packet;
}

//---------------------------------------------------------------------------
// hdstream::demuxreset
//
// Resets the demultiplexer
//
// Arguments:
//
//	NONE

void hdstream::demuxreset(void)
{
}

//---------------------------------------------------------------------------
// hdstream::devicename
//
// Gets the device name associated with the stream
//
// Arguments:
//
//	NONE

std::string hdstream::devicename(void) const
{
	return std::string(m_device->get_device_name());
}

//---------------------------------------------------------------------------
// hdstream::enumproperties
//
// Enumerates the stream properties
//
// Arguments:
//
//	callback		- Callback to invoke for each stream

void hdstream::enumproperties(std::function<void(struct streamprops const& props)> const& callback)
{
	// AUDIO STREAM
	//
	streamprops audio = {};
	audio.codec = (get_system_endianness() == endianness::little) ? "pcm_s16le" : "pcm_s16be";
	audio.pid = STREAM_ID_AUDIO;
	audio.channels = 2;
	audio.samplerate = 44100;
	audio.bitspersample = 16;
	callback(audio);
}

//---------------------------------------------------------------------------
// hdstream::length
//
// Gets the length of the stream; or -1 if stream is real-time
//
// Arguments:
//
//	NONE

long long hdstream::length(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// hdstream::muxname
//
// Gets the mux name associated with the stream
//
// Arguments:
//
//	NONE

std::string hdstream::muxname(void) const
{
	return m_muxname;
}

//---------------------------------------------------------------------------
// hdstream::nrsc5_callback (private, static)
//
// NRSC5 library event callback function
//
// Arguments:
//
//	event	- NRSC5 event being raised
//	arg		- Implementation-specific context pointer

void hdstream::nrsc5_callback(nrsc5_event_t const* event, void* arg)
{
	assert(arg != nullptr);
	reinterpret_cast<hdstream*>(arg)->nrsc5_callback(event);
}

//---------------------------------------------------------------------------
// hdstream::nrsc5_callback (private)
//
// NRSC5 library event callback function
//
// Arguments:
//
//	event	- NRSC5 event being raised

void hdstream::nrsc5_callback(nrsc5_event_t const* event)
{
	bool	queued = false;		// Flag if an item was queued

	std::unique_lock<std::mutex> lock(m_queuelock);

	// NRSC5_EVENT_AUDIO
	//
	if(event->event == NRSC5_EVENT_AUDIO) {

		// Filter out anything other than program zero for now
		if(event->audio.program == 0) {

			std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
			packet->program = event->audio.program;
			packet->count = event->audio.count;
			packet->data = std::make_unique<int16_t[]>(event->audio.count);
			memcpy(&packet->data[0], event->audio.data, event->audio.count * sizeof(int16_t));
			m_queue.emplace(std::move(packet));
			queued = true;
		}
	}

	if(queued) m_cv.notify_all();			// Notify queue was updated
}

//---------------------------------------------------------------------------
// hdstream::position
//
// Gets the current position of the stream
//
// Arguments:
//
//	NONE

long long hdstream::position(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// hdstream::read
//
// Reads data from the live stream
//
// Arguments:
//
//	buffer		- Buffer to receive the live stream data
//	count		- Size of the destination buffer in bytes

size_t hdstream::read(uint8_t* /*buffer*/, size_t /*count*/)
{
	return 0;
}

//---------------------------------------------------------------------------
// hdstream::realtime
//
// Gets a flag indicating if the stream is real-time
//
// Arguments:
//
//	NONE

bool hdstream::realtime(void) const
{
	return true;
}

//---------------------------------------------------------------------------
// hdstream::seek
//
// Sets the stream pointer to a specific position
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

long long hdstream::seek(long long /*position*/, int /*whence*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// hdstream::servicename
//
// Gets the service name associated with the stream
//
// Arguments:
//
//	NONE

std::string hdstream::servicename(void) const
{
	return std::string("TODO");
}

//---------------------------------------------------------------------------
// hdstream::signalquality
//
// Gets the signal quality as percentages
//
// Arguments:
//
//	NONE

void hdstream::signalquality(int& quality, int& snr) const
{
	quality = snr = 0;
}

//---------------------------------------------------------------------------
// hdstream::transfer (private)
//
// Worker thread procedure used to transfer data into the ring buffer
//
// Arguments:
//
//	started		- Condition variable to set when thread has started

void hdstream::transfer(scalar_condition<bool>& started)
{
	assert(m_device);
	assert(m_nrsc5);

	// read_callback_func (local)
	//
	// Asynchronous read callback function for the RTL-SDR device
	auto read_callback_func = [&](uint8_t const* buffer, size_t count) -> void {

		// Pipe the samples into NRSC5, it will invoke the necessary callback(s)
		nrsc5_pipe_samples_cu8(m_nrsc5, const_cast<uint8_t*>(buffer), static_cast<unsigned int>(count));
	};

	// Begin streaming from the device and inform the caller that the thread is running
	m_device->begin_stream();
	started = true;

	// Continuously read data from the device until cancel_async() has been called
	try { m_device->read_async(read_callback_func, 32768); }
	catch(...) { m_worker_exception = std::current_exception(); }

	m_stopped.store(true);					// Thread is stopped
	m_cv.notify_all();						// Unblock any waiters
}

//---------------------------------------------------------------------------

#pragma warning(pop)
