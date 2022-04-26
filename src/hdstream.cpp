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
	m_device(std::move(device)), m_analogfallback(hdprops.analogfallback), m_muxname(""), 
	m_pcmgain(powf(10.0f, hdprops.outputgain / 10.0f))
{
	// Initialize the RTL-SDR device instance
	m_device->set_frequency_correction(tunerprops.freqcorrection + channelprops.freqcorrection);
	uint32_t samplerate = m_device->set_sample_rate(1488375);
	m_device->set_center_frequency(channelprops.frequency);

	// Adjust the device gain as specified by the channel properties
	m_device->set_automatic_gain_control(channelprops.autogain);
	if(channelprops.autogain == false) m_device->set_gain(channelprops.manualgain);

	// Initialize the HD Radio demodulator
	nrsc5_open_pipe(&m_nrsc5);
	nrsc5_set_mode(m_nrsc5, NRSC5_MODE_FM);
	nrsc5_set_callback(m_nrsc5, nrsc5_callback, this);

	// Initialize the wideband FM demodulator parameters
	tDemodInfo demodinfo = {};
	demodinfo.HiCutmax = 100000;
	demodinfo.HiCut = 100000;
	demodinfo.LowCut = -100000;
	demodinfo.SquelchValue = -160;
	demodinfo.WfmDownsampleQuality = DownsampleQuality::Medium;

	// Initialize the wideband FM demodulator
	m_fmdemod = std::unique_ptr<CDemodulator>(new CDemodulator());
	//m_fmdemod->SetUSFmVersion(false);	
	m_fmdemod->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
	m_fmdemod->SetDemod(DEMOD_WFM, demodinfo);
	m_fmdemod->SetDemodFreq(0);

	// Initialize the output resampler
	m_fmresampler = std::unique_ptr<CFractResampler>(new CFractResampler());
	m_fmresampler->Init(m_fmdemod->GetInputBufferLimit());

	// Create a worker thread on which to perform demodulation
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

	// Allocate and initialize the DEMUX_PACKET
	DEMUX_PACKET* packet = allocator(static_cast<int>(audiopacket->count * sizeof(int16_t)));
	if(packet != nullptr) {

		packet->iStreamId = STREAM_ID_AUDIO;
		packet->iSize = static_cast<int>(audiopacket->count * sizeof(int16_t));
		packet->duration = audiopacket->duration;
		packet->dts = packet->pts = m_dts;

		int16_t* data = reinterpret_cast<int16_t*>(&packet->pData[0]);
		for(size_t index = 0; index < audiopacket->count; index++) data[index] = static_cast<int16_t>(audiopacket->data[index] * m_pcmgain);

		m_dts += packet->duration;
	}

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
	audio.codec = "pcm_s16le";
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

	// Output gain is applied during demux for this stream type
	TYPEREAL const nogain = MPOW(10.0, (0.0f / 10.0));

	std::unique_lock<std::mutex> lock(m_queuelock);

	// NRSC5_EVENT_IQ
	//
	// If the HD Radio stream is not generating any audio packets, fall back on the
	// analog signal using the Wideband FM demodulator
	if((event->event == NRSC5_EVENT_IQ) && (m_analogfallback) && (!m_hdaudio)) {

		// The byte count must be exact for the analog demodulator to work right
		assert(event->iq.count == (m_fmdemod->GetInputBufferLimit() * 2));

		// The FM demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
		// (32767.0 / 127.5) = 256.9960784313725
		std::unique_ptr<TYPECPX[]> samples(new TYPECPX[event->iq.count / 2]);
		for(int index = 0; index < m_fmdemod->GetInputBufferLimit(); index++) {

			samples[index] = {

			#ifdef FMDSP_USE_DOUBLE_PRECISION
				(static_cast<TYPEREAL>(reinterpret_cast<uint8_t const*>(event->iq.data)[(index * 2)]) - 127.5) * 256.9960784313725,		// I
				(static_cast<TYPEREAL>(reinterpret_cast<uint8_t const*>(event->iq.data)[(index * 2) + 1]) - 127.5) * 256.9960784313725,	// Q
			#else
				(static_cast<TYPEREAL>(reinterpret_cast<uint8_t const*>(event->iq.data)[(index * 2)]) - 127.5f) * 256.9960784313725f,		// I
				(static_cast<TYPEREAL>(reinterpret_cast<uint8_t const*>(event->iq.data)[(index * 2) + 1]) - 127.5f) * 256.9960784313725f,	// Q
			#endif
			};
		}

		// Process the I/Q data, the original samples buffer can be reused/overwritten as it's processed
		int audiopackets = m_fmdemod->ProcessData(m_fmdemod->GetInputBufferLimit(), samples.get(), samples.get());

		// Remove any RDS data that was generated for now
		tRDS_GROUPS rdsgroup = {};
		while(m_fmdemod->GetNextRdsGroupData(&rdsgroup)) { /* DO NOTHING */ }

		// Resample the audio data into an int16_t buffer. Use a fixed output rate of 44.1KHz to match
		// NRSC5 and don't apply any output gain adjustments here, that will be done during demux
		std::unique_ptr<int16_t[]> audio(new int16_t[audiopackets * 2]);
		audiopackets = m_fmresampler->Resample(audiopackets, (m_fmdemod->GetOutputRate() / 44100),
			samples.get(), reinterpret_cast<TYPESTEREO16*>(audio.get()), nogain);

		// Generate and queue the demux_packet_t audio packet object
		std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
		packet->program = 0;
		packet->count = static_cast<size_t>(audiopackets) * 2;
		packet->duration = (audiopackets / static_cast<double>(44100)) * STREAM_TIME_BASE;
		packet->data = std::move(audio);
		m_queue.emplace(std::move(packet));
		queued = true;
	}

	// NRSC5_EVENT_AUDIO
	//
	else if(event->event == NRSC5_EVENT_AUDIO) {

		// When synchronization is first achieved, the samples will have already been
		// processed by the analog Wideband FM implementation above; we want to ignore
		// the first HD audio packet to produce the least audio disruption
		//
		// NOTE: This is not perfect as the number of audio samples generated from the 
		// same I/Q samples differ; shouldn't be too hard to figure out a better sync
		if(m_hdaudio) {

			// Filter out anything other than program zero for now
			if(event->audio.program == 0) {

				std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
				packet->program = event->audio.program;
				packet->count = event->audio.count;
				packet->duration = (event->audio.count / 2 / static_cast<double>(44100)) * STREAM_TIME_BASE;
				packet->data = std::make_unique<int16_t[]>(event->audio.count);
				memcpy(&packet->data[0], event->audio.data, event->audio.count * sizeof(int16_t));
				m_queue.emplace(std::move(packet));
				queued = true;
			}
		}

		else m_hdaudio = true;				// HD Radio is synced and producing audio
	}

	// NRSC5_EVENT_LOST_SYNC
	//
	// If synchronization has been lost, fall back to using the analog signal
	// until sync has been restored and a digital audio packet can be generated
	else if(event->event == NRSC5_EVENT_LOST_SYNC) m_hdaudio = false;

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

	// Continuously read data from the device until cancel_async() has been called. Use
	// the analog demodulator's packet size since it has to be precisely what's expected
	try { m_device->read_async(read_callback_func, m_fmdemod->GetInputBufferLimit() * 2); }
	catch(...) { m_worker_exception = std::current_exception(); }

	m_stopped.store(true);					// Thread is stopped
	m_cv.notify_all();						// Unblock any waiters
}

//---------------------------------------------------------------------------

#pragma warning(pop)
