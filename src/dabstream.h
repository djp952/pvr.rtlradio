//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __DABSTREAM_H_
#define __DABSTREAM_H_
#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "dabdsp/radio-receiver.h"
#include "dabdsp/ringbuffer.h"

#include "props.h"
#include "pvrstream.h"
#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class dabstream
//
// Implements a DAB stream

class dabstream : public pvrstream, private InputInterface, private ProgrammeHandlerInterface, 
	private RadioControllerInterface
{
public:

	// Destructor
	//
	virtual ~dabstream();

	//-----------------------------------------------------------------------
	// Member Functions

	// canseek
	//
	// Flag indicating if the stream allows seek operations
	bool canseek(void) const override;

	// close
	//
	// Closes the stream
	void close(void) override;

	// create (static)
	//
	// Factory method, creates a new dab instance
	static std::unique_ptr<dabstream> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops, struct dabprops const& dabprops, uint32_t subchannel);

	// demuxabort
	//
	// Aborts the demultiplexer
	void demuxabort(void) override;

	// demuxflush
	//
	// Flushes the demultiplexer
	void demuxflush(void) override;

	// demuxread
	//
	// Reads the next packet from the demultiplexer
	DEMUX_PACKET* demuxread(std::function<DEMUX_PACKET* (int)> const& allocator) override;

	// demuxreset
	//
	// Resets the demultiplexer
	void demuxreset(void) override;

	// devicename
	//
	// Gets the device name associated with the stream
	std::string devicename(void) const override;

	// enumproperties
	//
	// Enumerates the stream properties
	void enumproperties(std::function<void(struct streamprops const& props)> const& callback) override;

	// length
	//
	// Gets the length of the stream
	long long length(void) const override;

	// muxname
	//
	// Gets the mux name associated with the stream
	std::string muxname(void) const override;

	// position
	//
	// Gets the current position of the stream
	long long position(void) const override;

	// read
	//
	// Reads available data from the stream
	size_t read(uint8_t* buffer, size_t count) override;

	// realtime
	//
	// Gets a flag indicating if the stream is real-time
	bool realtime(void) const override;

	// seek
	//
	// Sets the stream pointer to a specific position
	long long seek(long long position, int whence) override;

	// servicename
	//
	// Gets the service name associated with the stream
	std::string servicename(void) const override;

	// signalquality
	//
	// Gets the signal quality as percentages
	void signalquality(int& quality, int& snr) const override;

private:

	dabstream(dabstream const&) = delete;
	dabstream& operator=(dabstream const&) = delete;

	// DEFAULT_AUDIO_RATE
	//
	// The default audio output sample rate
	static int const DEFAULT_AUDIO_RATE;

	// MAX_PACKET_QUEUE
	//
	// Maximum number of queued demux packets
	static size_t const MAX_PACKET_QUEUE;

	// RING_BUFFER_SIZE
	//
	// Input ring buffer size
	static size_t const RING_BUFFER_SIZE;

	// SAMPLE_RATE
	//
	// Fixed device sample rate required for DAB
	static uint32_t const SAMPLE_RATE;

	// STREAM_ID_AUDIOBASE
	//
	// Base stream identifier for the audio output stream
	static int const STREAM_ID_AUDIOBASE;

	// STREAM_ID_ID3TAG
	//
	// Stream identifier for the ID3v2 tag output stream
	static int const STREAM_ID_ID3TAG;

	// Instance Constructor
	//
	dabstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops, struct dabprops const& dabprops, uint32_t subchannel);

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// demux_packet_t
	//
	// Defines the conents of a queued demux packet
	struct demux_packet_t {

		int							streamid = 0;
		int							size = 0;
		double						duration = 0;
		double						dts = 0;
		double						pts = 0;
		std::unique_ptr<uint8_t[]>	data;
	};

	// demux_queue_t
	//
	// Defines the type of the demux queue
	using demux_queue_t = std::queue<std::unique_ptr<demux_packet_t>>;

	// eventid_t
	//
	// Defines a worker thread event identifier
	enum class eventid_t {

		InputFailure,					// An input failure has occurred
		ServiceDetected,				// A new service has been detected
	};

	// event_queue_t
	//
	// Defines the type of the worker thread event queue
	using event_queue_t = std::queue<eventid_t>;

	//-----------------------------------------------------------------------
	// Private Member Functions

	// worker
	//
	// Worker thread procedure used to transfer and process data
	void worker(scalar_condition<bool>& started);

	//-----------------------------------------------------------------------
	// InputInterface Implementation

	// getSamples
	//
	// Reads the specified number of samples from the input device
	int32_t getSamples(DSPCOMPLEX* buffer, int32_t size) override;

	// getSamplesToRead
	//
	// Gets the number of input samples that are available to read from input
	int32_t getSamplesToRead(void) override;

	// is_ok
	//
	// Determines if the input is still "OK"
	bool is_ok(void) override;

	// restart
	//
	// Restarts the input
	bool restart(void) override;

	//-----------------------------------------------------------------------
	// ProgrammeHandlerInterface

	// onNewAudio
	//
	// Invoked when a new packet of audio data has been decoded
	void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode) override;

	// onNewDynamicLabel
	//
	// Invoked when a new dynamic label has been decoded
	void onNewDynamicLabel(const std::string& label) override;

	// onMOT
	//
	// Invoked when a new slide has been decoded
	void onMOT(const mot_file_t& mot_file) override;

	//-----------------------------------------------------------------------
	// RadioControllerInterface

	// onFrequencyCorrectorChange
	//
	// Invoked when the frequency correction has been changed
	void onFrequencyCorrectorChange(int fine, int coarse) override;

	// onInputFailure
	//
	// Invoked when the receiver has shut down to an input failure
	void onInputFailure(void) override;

	// onServiceDetected
	//
	// Invoked when a new service was detected
	void onServiceDetected(uint32_t sId) override;

	// onSetEnsembleLabel
	//
	// Invoked when the ensemble label has changed
	void onSetEnsembleLabel(DabLabel& label) override;

	// onSNR
	//
	// Invoked when the Signal-to-Nosie Radio has been calculated
	void onSNR(float snr) override;

	// onSyncChange
	//
	// Invoked when signal synchronization was acquired or lost
	void onSyncChange(bool isSync) override;
		
	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>		m_device;				// RTL-SDR device instance
	aligned_ptr<RadioReceiver>		m_receiver;				// RadioReceiver instance
	RingBuffer<uint8_t>				m_ringbuffer;			// I/Q sample ring buffer

	// STREAM CONTROL
	//
	uint32_t const		m_subchannel;						// Ensemble subchannel number
	float const			m_pcmgain;							// Output gain
	std::atomic<bool>	m_streamok{ true };					// "OK" flag for the stream
	double				m_dts{ STREAM_TIME_BASE };			// Current decode time stamp
	std::atomic<int>	m_audioid{ STREAM_ID_AUDIOBASE };	// Current audio stream id
	std::atomic<int>	m_audiorate{ DEFAULT_AUDIO_RATE };	// Current audio output rate

	// DEMUX QUEUE
	//
	demux_queue_t					m_queue;				// queue<> of demux objects
	mutable std::mutex				m_queuelock;			// Synchronization object
	std::condition_variable			m_queuecv;				// Event condition variable

	// WORKER THREAD
	//
	std::thread						m_worker;				// Data transfer thread
	std::exception_ptr				m_worker_exception;		// Exception on worker thread
	scalar_condition<bool>			m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>				m_stopped{ false };		// Data transfer stopped flag
	event_queue_t					m_events;				// queue<> of worker events
	mutable std::mutex				m_eventslock;			// Synchronization object
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __HDSTREAM_H_
