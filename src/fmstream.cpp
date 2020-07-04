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

// fmstream::DEFAULT_DEVICE_BLOCK_SIZE
//
// Default device block size
size_t const fmstream::DEFAULT_DEVICE_BLOCK_SIZE = (16 KiB);

// fmstream::DEFAULT_DEVICE_SAMPLE_RATE
//
// Default device sample rate
uint32_t const fmstream::DEFAULT_DEVICE_SAMPLE_RATE = (1 MHz);

// fmstream::DEFAULT_RINGBUFFER_SIZE
//
// Default ring buffer size
size_t const fmstream::DEFAULT_RINGBUFFER_SIZE = (8 MiB);

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
//	deviceprops		- RTL-SDR device properties
//	fmprops			- FM signal processor properties

fmstream::fmstream(struct deviceprops const& deviceprops, struct fmprops const& fmprops) : 
	m_blocksize(align::up(DEFAULT_DEVICE_BLOCK_SIZE, 16 KiB)),
	m_samplerate(DEFAULT_DEVICE_SAMPLE_RATE), m_pcmsamplerate(fmprops.samplerate),
	m_buffersize(align::up(DEFAULT_RINGBUFFER_SIZE, 16 KiB))
{
	// The only allowable output sample rates for this stream are 44100Hz and 48000Hz
	if((m_pcmsamplerate != 44100) && (m_pcmsamplerate != 48000))
		throw string_exception(__func__, ": FM DSP output sample rate must be set to either 44.1KHz or 48.0KHz");

	// Allocate the ring buffer
	m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[m_buffersize]);
	if(!m_buffer) throw std::bad_alloc();

	// TODO: "DC offset" tuning? Can't get that work properly at all yet
	// TODO: Should the sample rate be matched with the decoder? (1000000 vs 998400, for example)

	// Create and initialize the RTL-SDR device instance
	m_device = rtldevice::create(rtldevice::DEFAULT_DEVICE_INDEX);
	uint32_t samplerate = m_device->set_sample_rate(m_samplerate);
	/*uint32_t frequency =*/ m_device->set_center_frequency(fmprops.centerfrequency);
	m_device->set_bandwidth(200 KHz);		// <-- TODO: 250KHz?

	// Adjust the device gain as specified by the parameters
	m_device->set_automatic_gain_control(deviceprops.agc);
	if(deviceprops.agc == false) m_device->set_gain(deviceprops.manualgain);

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
	demodinfo.HiCut = fmprops.hicut;
	demodinfo.LowCut = fmprops.lowcut;
	demodinfo.FreqClickResolution = 100000;				// Not used by demodulator
	demodinfo.Offset = 0;								// <--- TODO: what does this do?
	demodinfo.SquelchValue = -160;						// Not used by demodulator
	demodinfo.AgcSlope = fmprops.agcslope;
	demodinfo.AgcThresh = fmprops.agcthresh;
	demodinfo.AgcManualGain = fmprops.agcmanualgain;
	demodinfo.AgcDecay = fmprops.agcdecay;
	demodinfo.AgcOn = fmprops.agcon;
	demodinfo.AgcHangOn = fmprops.agchangon;

	// Initialize the wideband FM demodulator
	// TODO: there is quite a bit of "chicken and the egg" stuff with this class, put a new
	// constructor on it that accepts all the proper parameters at once or at least fix the
	// weird order of operations here ...
	m_demodulator = std::unique_ptr<CDemodulator>(new CDemodulator());
	m_demodulator->SetUSFmVersion(true);
	m_demodulator->SetInputSampleRate(static_cast<TYPEREAL>(samplerate));
	m_demodulator->SetDemod(DEMOD_WFM, demodinfo);
	//m_demodulator->SetDemodFreq(frequency);	// <-- TODO: What does this do?

	// Initialize the output resampler
	m_resampler = std::unique_ptr<CFractResampler>(new CFractResampler());
	m_resampler->Init(m_demodulator->GetInputBufferLimit());

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
//	deviceprops		- RTL-SDR device properties
//	fmprops			- FM signal processor properties

std::unique_ptr<fmstream> fmstream::create(struct deviceprops const& deviceprops, struct fmprops const& fmprops)
{
	return std::unique_ptr<fmstream>(new fmstream(deviceprops, fmprops));
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
	size_t				head = 0;				// Current head position
	size_t				tail = 0;				// Current tail position
	size_t				available = 0;			// Available bytes to read
	bool				stopped = false;		// Flag if data transfer has stopped

	// If there is an RDS UECP packet available, handle it before demodulating more audio
	uecp_data_packet uecp_packet;
	if(m_rdsdecoder.pop_uecp_data_packet(uecp_packet) && (!uecp_packet.empty())) {

		// Allocate and initialize the UECP demultiplexer packet
		int packetsize = static_cast<int>(uecp_packet.size());
		DemuxPacket* packet = allocator(packetsize);
		if(packet == nullptr) return nullptr;

		packet->iStreamId = STREAM_ID_UECP;
		packet->iSize = packetsize;
		packet->pts = m_pts;

		// Copy the UECP data into the demultiplexer packet and return it
		memcpy(packet->pData, uecp_packet.data(), uecp_packet.size());
		return packet;
	}

	// The demodulator only works properly in this use pattern if it's fed the exact
	// number of input bytes it's looking for.  Excess bytes will be discarded, and
	// an insufficient number of bytes won't generate any output samples
	size_t const minreadsize = m_demodulator->GetInputBufferLimit() * 2;

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
	if(available < minreadsize) return allocator(0);

	// Adjust the input size to match the optimal setting from the decoder
	available = std::min(available, minreadsize);

	std::vector<TYPECPX> samples(available / 2);
	for(size_t index = 0; index < samples.size(); index++) {

		// The demodulator expects the I/Q samples in the range of -32767.0 through +32767.0
		samples[index].re = ((static_cast<TYPEREAL>(m_buffer[tail]) - 127.5) / 127.5) * 32767.0;		// I
		samples[index].im = ((static_cast<TYPEREAL>(m_buffer[tail + 1]) - 127.5) / 127.5) * 32767.0;	// Q

		tail += 2;								// Increment new tail position
		if(tail >= m_buffersize) tail = 0;		// Handle buffer rollover
	}

	// Modify the atomic<> tail position to mark the ring buffer space as free
	m_buffertail.store(tail);

	// Process the raw I/Q data, the original sample vector<> can be reused/overwritten as it's processed
	int audiopackets = m_demodulator->ProcessData(static_cast<int>(samples.size()), samples.data(), samples.data());

	// Process any RDS group data that was collected during demodulation
	tRDS_GROUPS rdsgroup = {};
	while(m_demodulator->GetNextRdsGroupData(&rdsgroup)) m_rdsdecoder.decode_rdsgroup(rdsgroup);

	// Determine the size of the demultiplexer packet data and allocate it
	int packetsize = audiopackets * sizeof(TYPESTEREO16);
	DemuxPacket* packet = allocator(packetsize);
	if(packet == nullptr) return nullptr;

	// Resample the audio data directly into the allocated packet buffer
	int stereopackets = m_resampler->Resample(audiopackets, m_demodulator->GetOutputRate() / m_pcmsamplerate, 
		samples.data(), reinterpret_cast<TYPESTEREO16*>(packet->pData), 1.0);

	// Calcuate the duration of the demultiplexer packet, in microseconds
	double duration = ((stereopackets / static_cast<double>(m_pcmsamplerate)) US);

	// Set up the demultiplexer packet with the proper size, duration and pts
	packet->iStreamId = STREAM_ID_AUDIO;
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
	streamprops uecp = {};
	uecp.codec = "rds";
	uecp.pid = STREAM_ID_UECP;
	callback(uecp);
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
// fmstream::signalstrength
//
// Gets the signal strength as a percentage
//
// Arguments:
//
//	NONE

int fmstream::signalstrength(void) const
{
	static const double LN25 = MLOG(25);		// natural log of 25

	TYPEREAL db = static_cast<int>(m_demodulator->GetSMeterAve());

	// The dynamic range of an 8-bit ADC is 48dB, use anything -48dB or more as 0%,
	// and anything at or over 0dB as 100% signal strength
	if(db <= -48.0) return 0;
	else if(db >= 0.0) return 100;

	// Convert db into a percentage based on a linear scale from -48dB to 0dB
	db = (1.0 - (db / -48.0)) * 100;

	// While this is certainly not technically accurate, scale the percentage
	// using a natural logarithm to bump up the value into something realistic
	return static_cast<int>(MLOG(db / 4) * (100 / LN25));
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
	TYPEREAL db = static_cast<int>(m_demodulator->GetSMeterAve());

	// The actual SNR is difficult to calculate and track (I tried), so use the
	// delta between the signal and -44dB (-48dB [ADC] + -4dB [device]) as the SNR

	if(db <= -44.0) return 0;				// No signal
	else if(db >= 0.0) return 100;			// Maximum signal

	return static_cast<int>((abs(-44.0 - db) * 100) / 44.0);
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
