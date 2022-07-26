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
#include "fmstream.h"

#include <algorithm>
#include <chrono>
#include <memory.h>

#include "align.h"
#include "string_exception.h"

#pragma warning(push, 4)

// fmstream::MAX_SAMPLE_QUEUE
//
// Maximum number of queued sample sets from the device
size_t const fmstream::MAX_SAMPLE_QUEUE = 200;		// ~2sec

// fmstream::STREAM_ID_AUDIO
//
// Stream identifier for the audio output stream
int const fmstream::STREAM_ID_AUDIO = 1;

// fmstream::STREAM_ID_UECP
//
// Stream identifier for the UECP output stream
int const fmstream::STREAM_ID_UECP = 2;

//---------------------------------------------------------------------------
// fmstream Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//	channelprops	- Channel properties
//	fmprops			- FM digital signal processor properties

fmstream::fmstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
	struct channelprops const& channelprops, struct fmprops const& fmprops) :
	m_device(std::move(device)), m_decoderds(fmprops.decoderds), m_rdsdecoder(fmprops.isnorthamerica),
	m_muxname(generate_mux_name(channelprops)), m_pcmsamplerate(fmprops.outputrate), 
	m_pcmgain(MPOW(10.0, (fmprops.outputgain / 10.0)))
{
	// The sample rate must be within 900001Hz - 3200000Hz
	if((fmprops.samplerate < 900001) || (fmprops.samplerate > 3200000))
		throw string_exception(__func__, ": Tuner device sample rate must be in the range of 900001Hz to 3200000Hz");

	// The only allowable output sample rates for this stream are 44100Hz and 48000Hz
	if((m_pcmsamplerate != 44100) && (m_pcmsamplerate != 48000))
		throw string_exception(__func__, ": DSP output sample rate must be set to either 44.1KHz or 48.0KHz");

	// Initialize the RTL-SDR device instance
	m_device->set_frequency_correction(tunerprops.freqcorrection + channelprops.freqcorrection);
	uint32_t samplerate = m_device->set_sample_rate(fmprops.samplerate);
	uint32_t frequency = m_device->set_center_frequency(channelprops.frequency + (samplerate / 4));		// DC offset

	// Initialize the demodulator parameters
	//
	tDemodInfo demodinfo = {};
	demodinfo.HiCutmax = 100000;
	demodinfo.HiCut = 100000;
	demodinfo.LowCut = -100000;
	demodinfo.SquelchValue = -160;
	demodinfo.WfmDownsampleQuality = static_cast<enum DownsampleQuality>(fmprops.downsamplequality);

	// Initialize the wideband FM demodulator
	m_demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
	m_demodulator->SetUSFmVersion(fmprops.isnorthamerica);
	m_demodulator->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
	m_demodulator->SetDemod(DEMOD_WFM, demodinfo);
	m_demodulator->SetDemodFreq(static_cast<TYPEREAL>(frequency - channelprops.frequency));

	// Initialize the output resampler
	m_resampler = std::unique_ptr<CFractResampler>(new CFractResampler());
	m_resampler->Init(m_demodulator->GetInputBufferLimit());

	// Adjust the device gain as specified by the channel properties
	m_device->set_automatic_gain_control(channelprops.autogain);
	if(channelprops.autogain == false) m_device->set_gain(channelprops.manualgain);

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
//	device			- RTL-SDR device instance
//	tunerprops		- Tunder device properties
//	channelprops	- Channel properties
//	fmprops			- FM digital signal processor properties

std::unique_ptr<fmstream> fmstream::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	struct channelprops const& channelprops, struct fmprops const& fmprops)
{
	return std::unique_ptr<fmstream>(new fmstream(std::move(device), tunerprops, channelprops, fmprops));
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
//	allocator		- DemuxPacket allocation function

DEMUX_PACKET* fmstream::demuxread(std::function<DEMUX_PACKET*(int)> const& allocator)
{
	// If there is an RDS UECP packet available, handle it before demodulating more audio
	uecp_data_packet uecp_packet;
	if(m_rdsdecoder.pop_uecp_data_packet(uecp_packet) && (!uecp_packet.empty())) {

		// The user may have opted to disable RDS.  The packet from the decoder still
		// needs to be popped from the queue, but don't do anything with it ...
		if(m_decoderds) {

			// Allocate and initialize the UECP demultiplexer packet
			int packetsize = static_cast<int>(uecp_packet.size());
			DEMUX_PACKET* packet = allocator(packetsize);
			if(packet == nullptr) return nullptr;

			packet->iStreamId = STREAM_ID_UECP;
			packet->iSize = packetsize;

			// Copy the UECP data into the demultiplexer packet and return it
			memcpy(packet->pData, uecp_packet.data(), uecp_packet.size());
			return packet;
		}
	}

	// Wait for there to be a packet of samples available for processing
	std::unique_lock<std::mutex> lock(m_queuelock);
	m_cv.wait(lock, [&]() -> bool { return ((m_queue.size() > 0) || m_stopped.load() == true); });

	// If the worker thread was stopped, check for and re-throw any exception that occurred,
	// otherwise assume it was stopped normally and return an empty demultiplexer packet
	if(m_stopped.load() == true) {

		if(m_worker_exception) std::rethrow_exception(m_worker_exception);
		else return allocator(0);
	}

	// Pop off the topmost packet of samples from the queue<> and release the lock
	std::unique_ptr<TYPECPX[]> samples(std::move(m_queue.front()));
	m_queue.pop();
	lock.unlock();

	// If the packet of samples is null, the writer has indicated there was a problem
	if(!samples) {

		m_dts = STREAM_TIME_BASE;			// Reset the current decode time stamp

		// Create a STREAMCHANGE packet that has no data
		DEMUX_PACKET* packet = allocator(0);
		if(packet) packet->iStreamId = DEMUX_SPECIALID_STREAMCHANGE;

		return packet;				// Return the generated packet
	}

	// Process the I/Q data, the original samples buffer can be reused/overwritten as it's processed
	int audiopackets = m_demodulator->ProcessData(m_demodulator->GetInputBufferLimit(), samples.get(), samples.get());

	// Process any RDS group data that was collected during demodulation
	tRDS_GROUPS rdsgroup = {};
	while(m_demodulator->GetNextRdsGroupData(&rdsgroup)) m_rdsdecoder.decode_rdsgroup(rdsgroup);

	// Determine the size of the demultiplexer packet data and allocate it
	int packetsize = audiopackets * sizeof(TYPESTEREO16);
	DEMUX_PACKET* packet = allocator(packetsize);
	if(packet == nullptr) return nullptr;

	// Resample the audio data directly into the allocated packet buffer
	audiopackets = m_resampler->Resample(audiopackets, (m_demodulator->GetOutputRate() / m_pcmsamplerate),
		samples.get(), reinterpret_cast<TYPESTEREO16*>(packet->pData), m_pcmgain);

	// Calculate the proper duration for the packet
	double duration = (audiopackets / static_cast<double>(m_pcmsamplerate)) * STREAM_TIME_BASE;

	// Set up the demultiplexer packet with the proper size, duration and dts
	packet->iStreamId = STREAM_ID_AUDIO;
	packet->iSize = audiopackets * sizeof(TYPESTEREO16);
	packet->duration = duration;
	packet->dts = packet->pts = m_dts;

	// Increment the decode time stamp value based on the calculated duration
	m_dts += duration;

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
// fmstream::devicename
//
// Gets the device name associated with the stream
//
// Arguments:
//
//	NONE

std::string fmstream::devicename(void) const
{
	return std::string(m_device->get_device_name());
}

//---------------------------------------------------------------------------
// fmstream::enumproperties
//
// Enumerates the stream properties
//
// Arguments:
//
//	callback		- Callback to invoke for each stream

void fmstream::enumproperties(std::function<void(struct streamprops const& props)> const& callback)
{
	// AUDIO STREAM
	//
	streamprops audio = {};
	audio.codec = "pcm_s16le";
	audio.pid = STREAM_ID_AUDIO;
	audio.channels = 2;
	audio.samplerate = static_cast<int>(m_pcmsamplerate);
	audio.bitspersample = 16;
	callback(audio);

	// UECP STREAM
	//
	if(m_decoderds) {

		streamprops uecp = {};
		uecp.codec = "rds";
		uecp.pid = STREAM_ID_UECP;
		callback(uecp);
	}
}

//---------------------------------------------------------------------------
// fmstream::generate_mux_name (private)
//
// Generates the mux name to associate with the stream
//
// Arguments:
//
//	channelprops		- Channel properties structure

std::string fmstream::generate_mux_name(struct channelprops const& channelprops) const
{
	// Set the default mux name to the frequency in Megahertz
	char buf[64] = { 0 };
	snprintf(buf, std::extent<decltype(buf)>::value, "%.1f FM", 
		(static_cast<float>(channelprops.frequency) / 1000000.0f));
	return std::string(buf);
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
// fmstream::muxname
//
// Gets the mux name associated with the stream
//
// Arguments:
//
//	NONE

std::string fmstream::muxname(void) const
{
	// If the callsign for the station is known, use that with an -FM suffix, otherwise use the default
	return (m_rdsdecoder.has_rbds_callsign()) ? m_rdsdecoder.get_rbds_callsign() : m_muxname;
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
// fmstream::servicename
//
// Gets the service name associated with the stream
//
// Arguments:
//
//	NONE

std::string fmstream::servicename(void) const
{
	return std::string("Wideband FM radio");
}

//---------------------------------------------------------------------------
// fmstream::signalquality
//
// Gets the signal quality as a percentage
//
// Arguments:
//
//	NONE

void fmstream::signalquality(int& quality, int& snr) const
{
	TYPEREAL demodquality = 0;
	TYPEREAL demodsnr = 0;

	m_demodulator->GetSignalLevels(demodquality, demodsnr);

	// For wideband FM, adjust the range such that 80% is nominal for
	// signal quality and 60% is nominal for signal-to-noise; this 
	// adjustment is based on observation and (perceived) output quality
	quality = std::max(0, std::min(100, static_cast<int>(100.0 * (demodquality / 0.80))));
	snr = std::max(0, std::min(100, static_cast<int>(100.0 * (demodsnr / 0.60))));
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
	assert(m_demodulator);
	assert(m_device);

	// The I/Q samples from the device come in as a pair of 8 bit unsigned integers
	size_t const readsize = m_demodulator->GetInputBufferLimit() * 2;

	// read_callback_func (local)
	//
	// Asynchronous read callback function for the RTL-SDR device
	auto read_callback_func = [&](uint8_t const* buffer, size_t count) -> void {

		std::unique_ptr<TYPECPX[]> samples;			// Array of I/Q samples to return

		// If the proper amount of data was returned by the callback, convert it into
		// the floating-point I/Q sample data for the demodulator to process
		if(count == readsize) {

			samples = std::unique_ptr<TYPECPX[]>(new TYPECPX[readsize / 2]);
			for(int index = 0; index < m_demodulator->GetInputBufferLimit(); index++) {

				// The demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
				// (32767.0 / 127.5) = 256.9960784313725
				samples[index] = {

				#ifdef FMDSP_USE_DOUBLE_PRECISION
					(static_cast<TYPEREAL>(buffer[(index * 2)]) - 127.5) * 256.9960784313725,		// I
					(static_cast<TYPEREAL>(buffer[(index * 2) + 1]) - 127.5) * 256.9960784313725,	// Q
				#else
					(static_cast<TYPEREAL>(buffer[(index * 2)]) - 127.5f) * 256.9960784313725f,		// I
					(static_cast<TYPEREAL>(buffer[(index * 2) + 1]) - 127.5f) * 256.9960784313725f,	// Q
				#endif
				};
			}
		}

		// Push the converted samples into the queue<> for processing.  If there is insufficient space
		// left in the queue<>, the samples aren't being processed quickly enough to keep up with the rate
		std::unique_lock<std::mutex> lock(m_queuelock);
		if(m_queue.size() < MAX_SAMPLE_QUEUE) m_queue.emplace(std::move(samples));
		else {

			m_queue = sample_queue_t();							// Replace the queue<>
			m_queue.push(nullptr);								// Push a resync packet (null)
			if(samples) m_queue.emplace(std::move(samples));	// Push samples
		}

		// Notify any threads waiting on the lock that the queue<> has been updated
		m_cv.notify_all();
	};

	// Begin streaming from the device and inform the caller that the thread is running
	m_device->begin_stream();
	started = true;

	// Continuously read data from the device until cancel_async() has been called
	try { m_device->read_async(read_callback_func, static_cast<uint32_t>(readsize)); }
	catch(...) { m_worker_exception = std::current_exception(); }

	m_stopped.store(true);					// Thread is stopped
	m_cv.notify_all();						// Unblock any waiters
}

//---------------------------------------------------------------------------

#pragma warning(pop)
