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
#include "string_exception.h"

#pragma warning(push, 4)

// fmstream::MAX_SAMPLE_QUEUE
//
// Maximum number of queued sample sets from the device
size_t const fmstream::MAX_SAMPLE_QUEUE = 100;

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
	m_device(std::move(device)), m_decoderds(fmprops.decoderds), m_rdsdecoder(fmprops.isrbds), 
	m_muxfrequency(channelprops.frequency / 1000000.0f), m_pcmsamplerate(fmprops.outputrate), 
	m_pcmgain(MPOW(10.0, (fmprops.outputgain / 10.0)))
{
	// The sample rate must be within 900001Hz - 3200000Hz
	if((tunerprops.samplerate < 900001) || (tunerprops.samplerate > 3200000))
		throw string_exception(__func__, ": Tuner device sample rate must be in the range of 900001Hz to 3200000Hz");

	// Initialize the RTL-SDR device instance
	m_device->set_frequency_correction(tunerprops.freqcorrection);
	uint32_t samplerate = m_device->set_sample_rate(tunerprops.samplerate);
	uint32_t frequency = m_device->set_center_frequency(channelprops.frequency + (samplerate / 4));		// DC offset

	// Initialize the demodulator parameters
	tDemodInfo demodinfo = {};

	// FIXED DEMODULATOR SETTINGS
	//
	demodinfo.txt.assign("WFM");
	demodinfo.HiCutmin = 100000;						// Not used by demodulator
	demodinfo.HiCutmax = 100000;
	demodinfo.LowCutmax = -100000;						// Not used by demodulator
	demodinfo.LowCutmin = -100000;
	demodinfo.Symetric = true;							// Not used by demodulator
	demodinfo.DefFreqClickResolution = 100000;			// Not used by demodulator
	demodinfo.FilterClickResolution = 10000;			// Not used by demodulator

	// VARIABLE DEMODULATOR SETTINGS
	//
	demodinfo.HiCut = 5000;								// Not used by demodulator
	demodinfo.LowCut = -5000;							// Not used by demodulator
	demodinfo.FreqClickResolution = 100000;				// Not used by demodulator
	demodinfo.Offset = 0;
	demodinfo.SquelchValue = -160;						// Not used by demodulator
	demodinfo.AgcSlope = 0;								// Not used by demodulator
	demodinfo.AgcThresh = -100;							// Not used by demodulator
	demodinfo.AgcManualGain = 30;						// Not used by demodulator
	demodinfo.AgcDecay = 200;							// Not used by demodulator
	demodinfo.AgcOn = false;							// Not used by demodulator
	demodinfo.AgcHangOn = false;						// Not used by demodulator

	// Initialize the wideband FM demodulator
	m_demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
	m_demodulator->SetUSFmVersion(fmprops.isrbds);
	m_demodulator->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
	m_demodulator->SetDemod(DEMOD_WFM, demodinfo);
	m_demodulator->SetDemodFreq(static_cast<TYPEREAL>(frequency - channelprops.frequency));

	// If the output sample rate is zero, use the demodulator output rate
	if(m_pcmsamplerate == 0) m_pcmsamplerate = static_cast<uint32_t>(m_demodulator->GetOutputRate());

	// Otherwise create an initialize the output resampler instance
	else {

		m_resampler = std::unique_ptr<CFractResampler>(new CFractResampler());
		m_resampler->Init(m_demodulator->GetInputBufferLimit());
	}

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

DemuxPacket* fmstream::demuxread(std::function<DemuxPacket*(int)> const& allocator)
{
	bool				stopped = false;		// Flag if data transfer has stopped

	// If there is an RDS UECP packet available, handle it before demodulating more audio
	uecp_data_packet uecp_packet;
	if(m_rdsdecoder.pop_uecp_data_packet(uecp_packet) && (!uecp_packet.empty())) {

		// The user may have opted to disable RDS.  The packet from the decoder still
		// needs to be popped from the queue, but don't do anything with it ...
		if(m_decoderds) {

			// Allocate and initialize the UECP demultiplexer packet
			int packetsize = static_cast<int>(uecp_packet.size());
			DemuxPacket* packet = allocator(packetsize);
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

	// If the worker thread was stopped, return an empty demultiplexer packet
	if(stopped) return allocator(0);

	// Pop off the topmost packet of samples from the queue<> and release the lock
	std::unique_ptr<TYPECPX[]> samples(std::move(m_queue.front()));
	m_queue.pop();
	lock.unlock();

	// If the packet of samples is null, the writer has indicated there was a problem
	if(!samples) {

		m_dts = DVD_TIME_BASE;				// Reset the current decode time stamp

		// Create a STREAMCHANGE packet that has no data
		DemuxPacket* packet = allocator(0);
		if(packet) packet->iStreamId = DMX_SPECIALID_STREAMCHANGE;

		return packet;				// Return the generated packet
	}

	// Process the I/Q data, the original samples buffer can be reused/overwritten as it's processed
	int audiopackets = m_demodulator->ProcessData(m_demodulator->GetInputBufferLimit(), samples.get(), samples.get());

	// Process any RDS group data that was collected during demodulation
	tRDS_GROUPS rdsgroup = {};
	while(m_demodulator->GetNextRdsGroupData(&rdsgroup)) m_rdsdecoder.decode_rdsgroup(rdsgroup);

	// Determine the size of the demultiplexer packet data and allocate it
	int packetsize = audiopackets * sizeof(TYPESTEREO16);
	DemuxPacket* packet = allocator(packetsize);
	if(packet == nullptr) return nullptr;

	// Resample the complex audio data directly into the packet data buffer.  If a fractional resampler was
	// initialized, use that to downsample the data otherwise use the fast 1:1 'native' resampler
	if(m_resampler) audiopackets = m_resampler->Resample(audiopackets, (m_demodulator->GetOutputRate() / m_pcmsamplerate),
		samples.get(), reinterpret_cast<TYPESTEREO16*>(packet->pData), m_pcmgain);
	else audiopackets = native_resample(audiopackets, samples.get(), reinterpret_cast<TYPESTEREO16*>(packet->pData), m_pcmgain);

	// Set up the demultiplexer packet with the proper size, duration and dts
	packet->iStreamId = STREAM_ID_AUDIO;
	packet->iSize = audiopackets * sizeof(TYPESTEREO16);
	packet->duration = 10000.0;
	packet->dts = packet->pts = m_dts;

	// Increment the decode time stamp value based on the calculated duration
	m_dts += 10000.0;

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
	// If the callsign for the station is known, use that with an -FM suffix, otherwise use the frequency
	if(m_rdsdecoder.has_rbds_callsign()) return std::string(m_rdsdecoder.get_rbds_callsign()) + "-FM";

	// Otherwise, use the channel frequency
	char buf[64] = {0};
	snprintf(buf, std::extent<decltype(buf)>::value, "%.1f FM", m_muxfrequency);
	return std::string(buf);
}

//---------------------------------------------------------------------------
// fmstream::native_resample
//
// Resamples the input data into stereo PCM samples at the native rate
//
// Arguments:
//
//	numsamples		- Number of input samples to process
//	insamples		- Buffer containing the input samples
//	outsamples		- Buffer to hold the output samples
//	gain			- Gain value to apply to the samples

int fmstream::native_resample(int numsamples, TYPECPX const* insamples, TYPESTEREO16* outsamples, TYPEREAL gain) const
{
	// Simple operation; just multiply the value by the gain and avoid over/underflow during conversion ...
	for(int index = 0; index < numsamples; index++) {

	#ifdef FMDSP_USE_DOUBLE_PRECISION
		outsamples[index].re = static_cast<qint16>(std::max(-32767.0, std::min(32767.0, insamples[index].re * gain)));
		outsamples[index].im = static_cast<qint16>(std::max(-32767.0, std::min(32767.0, insamples[index].im * gain)));
	#else
		outsamples[index].re = static_cast<qint16>(std::max(-32767.0f, std::min(32767.0f, insamples[index].re * gain)));
		outsamples[index].im = static_cast<qint16>(std::max(-32767.0f, std::min(32767.0f, insamples[index].im * gain)));
	#endif
	}

	return numsamples;
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
// fmstream::signalstrength
//
// Gets the signal strength as a percentage
//
// Arguments:
//
//	NONE

int fmstream::signalstrength(void) const
{
	return 0;
}

//---------------------------------------------------------------------------
// fmstream::signaltonoise
//
// Gets the signal to noise ratio as a percentage
//
// Arguments:
//
//	NONE

int fmstream::signaltonoise(void) const
{
	return 0;
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

			samples = std::unique_ptr<TYPECPX[]>(new TYPECPX[readsize]);
			for(int index = 0; index < m_demodulator->GetInputBufferLimit(); index++) {

				// The demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
				// 256.996 = (32767.0 / 127.5) = 256.9960784313725
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
		if(m_queue.size() < MAX_SAMPLE_QUEUE) m_queue.push(std::move(samples));
		else {

			m_queue = sample_queue_t();							// Replace the queue<>
			m_queue.push(nullptr);								// Push a resync packet (null)
			if(samples) m_queue.push(std::move(samples));		// Push samples
		}

		// Notify any threads waiting on the lock that the queue<> has been updated
		m_cv.notify_all();
	};

	// Begin streaming from the device and inform the caller that the thread is running
	m_device->begin_stream();
	started = true;

	// Continuously read data from the device until cancel_async() has been called
	m_device->read_async(read_callback_func, static_cast<uint32_t>(readsize));

	// Thread is stopped once read_async() returns
	m_stopped.store(true);
}

//---------------------------------------------------------------------------

#pragma warning(pop)
