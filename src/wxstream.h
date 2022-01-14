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

#ifndef __WXSTREAM_H_
#define __WXSTREAM_H_
#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "fmdsp/demodulator.h"
#include "fmdsp/fractresampler.h"

#include "props.h"
#include "pvrstream.h"
#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class wxstream
//
// Implements a Weather Radio stream

class wxstream : public pvrstream
{
public:

	// Destructor
	//
	virtual ~wxstream();

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
	// Factory method, creates a new wxstream instance
	static std::unique_ptr<wxstream> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops, struct wxprops const& wxprops);

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
	DEMUX_PACKET* demuxread(std::function<DEMUX_PACKET*(int)> const& allocator) override;

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

	wxstream(wxstream const&) = delete;
	wxstream& operator=(wxstream const&) = delete;

	// MAX_SAMPLE_QUEUE
	//
	// Maximum number of queued sample sets from device
	static size_t const MAX_SAMPLE_QUEUE;

	// STREAM_ID_AUDIO
	//
	// Stream identifier for the audio output stream
	static int const STREAM_ID_AUDIO;

	// Instance Constructor
	//
	wxstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops, struct wxprops const& wxprops);

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// sample_queue_item_t
	//
	// Defines the type of a single sample_queue_t entry
	using sample_queue_item_t = std::unique_ptr<TYPECPX[]>;

	// sample_queue_t
	//
	// Defines the type of the input sample queue
	using sample_queue_t = std::queue<sample_queue_item_t>;

	//-----------------------------------------------------------------------
	// Private Member Functions

	// generate_mux_name
	//
	// Generates the mux name to associate with the stream
	std::string generate_mux_name(struct channelprops const& channelprops) const;
	
	// transfer
	//
	// Worker thread procedure used to transfer data into the ring buffer
	void transfer(scalar_condition<bool>& started);

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>			m_device;					// RTL-SDR device instance
	std::unique_ptr<CDemodulator>		m_demodulator;				// CuteSDR demodulator instance
	std::unique_ptr<CFractResampler>	m_resampler;				// CuteSDR resampler instance

	std::string	const					m_muxname;					// Generated mux name
	uint32_t const						m_pcmsamplerate;			// Output sample rate
	TYPEREAL const						m_pcmgain;					// Output gain
	double								m_dts{ STREAM_TIME_BASE };	// Current decode time stamp

	// STREAM CONTROL
	//
	sample_queue_t						m_queue;					// queue<> of prepared samples
	mutable std::mutex					m_queuelock;				// Synchronization object
	std::condition_variable				m_cv;						// Transfer event condvar
	std::thread							m_worker;					// Data transfer thread
	std::exception_ptr					m_worker_exception;			// Exception on worker thread
	scalar_condition<bool>				m_stop{ false };			// Condition to stop data transfer
	std::atomic<bool>					m_stopped{ false };			// Data transfer stopped flag
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __WXSTREAM_H_
