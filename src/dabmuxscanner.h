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

#ifndef __DABMUXSCANNER_H_
#define __DABMUXSCANNER_H_
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "dabdsp/radio-receiver.h"
#include "dabdsp/ringbuffer.h"

#include "muxscanner.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class dabmuxscanner
//
// Implements the multiplex scanner for DAB

class dabmuxscanner : public muxscanner, private InputInterface, private ProgrammeHandlerInterface,
	private RadioControllerInterface
{
public:

	// Destructor
	//
	virtual ~dabmuxscanner();

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new dabmuxscanner instance
	static std::unique_ptr<dabmuxscanner> create(uint32_t samplerate, callback const& callback);

	// inputsamples
	//
	// Pipes input samples into the muliplex scanner
	void inputsamples(uint8_t const* samples, size_t length) override;

private:

	dabmuxscanner(dabmuxscanner const&) = delete;
	dabmuxscanner& operator=(dabmuxscanner const&) = delete;

	// RING_BUFFER_SIZE
	//
	// Input ring buffer size
	static size_t const RING_BUFFER_SIZE;

	// SAMPLE_RATE
	//
	// Fixed device sample rate required for DAB
	static uint32_t const SAMPLE_RATE;

	// Instance Constructor
	//
	dabmuxscanner(uint32_t samplerate, callback const& callback);

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// eventid_t
	//
	// Defines a worker thread event identifier
	enum class eventid_t {

		LostSync,					// Synchronization has been lost
		ServiceDetected,			// A new service has been detected
		SetEnsembleLabel,			// The ensemble label has been detected
		SetServiceLabel,			// A service label has been detected
		Sync,						// Synchronization has been achieved
	};

	// event_t
	//
	// Defines a worker thread event
	struct event_t {

		eventid_t		eventid;
		uint32_t		serviceid;
	};

	// event_queue_t
	//
	// Defines the worker thread event queue
	using event_queue_t = std::queue<event_t>;

	// timepoint_t
	//
	// Defines a point in time based on steady_clock
	using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;

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
	// RadioControllerInterface

	// onServiceDetected
	//
	// Invoked when a new service was detected
	void onServiceDetected(uint32_t sId) override;

	// onSetEnsembleLabel
	//
	// Invoked when the ensemble label has changed
	void onSetEnsembleLabel(DabLabel& label) override;

	// onSetServiceLabel
	//
	// Invoked when a service label has changed
	void onSetServiceLabel(uint32_t sId, DabLabel& label) override;

	// onSyncChange
	//
	// Invoked when signal synchronization was acquired or lost
	void onSyncChange(bool isSync) override;

	//-----------------------------------------------------------------------
	// Member Variables

	callback const					m_callback;			// Callback function 
	struct multiplex				m_muxdata = {};		// Multiplex data

	// DEMODULATOR
	//
	aligned_ptr<RadioReceiver>		m_receiver;			// RadioReceiver instance
	RingBuffer<uint8_t>				m_ringbuffer;		// I/Q sample ring buffer

	// EVENT QUEUE
	//
	event_queue_t					m_events;			// queue<> of worker events
	mutable std::mutex				m_eventslock;		// Synchronization object
	std::condition_variable			m_eventscv;			// Event condition variable
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DABMUXSCANNER_H_
