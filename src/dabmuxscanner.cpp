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
#include "dabmuxscanner.h"

#include <algorithm>
#include <stdexcept>

#pragma warning(push, 4)

// dabmuxscanner::RING_BUFFER_SIZE
//
// Input ring buffer size
size_t const dabmuxscanner::RING_BUFFER_SIZE = (4 MiB);		// 1 second @ 2048000

// dabmuxscanner::SAMPLE_RATE
//
// Fixed device sample rate required for DAB
uint32_t const dabmuxscanner::SAMPLE_RATE = 2048000;

// trim (local)
//
// Trims an std::string instance
inline std::string trim(const std::string& str)
{
	auto left = std::find_if_not(str.begin(), str.end(), isspace);
	auto right = std::find_if_not(str.rbegin(), str.rend(), isspace);
	return (right.base() <= left) ? std::string() : std::string(left, right.base());
}

//---------------------------------------------------------------------------
// dabmuxscanner Constructor (private)
//
// Arguments:
//
//	samplerate		- Sample rate of the input data
//	callback		- Callback function to invoke on status change

dabmuxscanner::dabmuxscanner(uint32_t samplerate, callback const& callback) : m_callback(callback),
	m_ringbuffer(RING_BUFFER_SIZE)
{
	assert(samplerate == SAMPLE_RATE);
	if(samplerate != SAMPLE_RATE) throw std::invalid_argument("samplerate");

	// Construct and initialize the demodulator instance
	RadioControllerInterface& controllerinterface = *static_cast<RadioControllerInterface*>(this);
	InputInterface& inputinterface = *static_cast<InputInterface*>(this);
	RadioReceiverOptions options = {};
	options.disableCoarseCorrector = true;
	m_receiver = make_aligned<RadioReceiver>(controllerinterface, inputinterface, options, 1);
	m_receiver->restart(false);
}

//---------------------------------------------------------------------------
// dabmuxscanner Destructor

dabmuxscanner::~dabmuxscanner()
{
	if(m_receiver) m_receiver->stop();			// Stop receiver
	m_receiver.reset();							// Reset receiver instance
}

//---------------------------------------------------------------------------
// dabmuxscanner::create (static)
//
// Factory method, creates a new dabmuxscanner instance
//
// Arguments:
//
//	samplerate		- Sample rate of the input data
//	callback		- Callback function to invoke on status change

std::unique_ptr<dabmuxscanner> dabmuxscanner::create(uint32_t samplerate, callback const& callback)
{
	return std::unique_ptr<dabmuxscanner>(new dabmuxscanner(samplerate, callback));
}

//---------------------------------------------------------------------------
// dabmuxscanner::inputsamples
//
// Pipes input samples into the muliplex scanner
//
// Arguments:
//
//	samples		- Pointer to the input samples
//	length		- Length of the input samples, in bytes

void dabmuxscanner::inputsamples(uint8_t const* samples, size_t length)
{
	bool			invokecallback = false;			// Flag to invoke the callback

	assert(length <= std::numeric_limits<int32_t>::max());
	m_ringbuffer.putDataIntoBuffer(samples, static_cast<int32_t>(length));

	// Check for and process any new events
	std::unique_lock<std::mutex> eventslock(m_eventslock);
	if(m_events.empty() == false) {

		// The threading model is a bit weird here; the callback that queued a new
		// event needs to be free to continue execution otherwise the DSP may deadlock 
		// while we process that event. Combat this by swapping the queue<> with a new
		// one, release the lock, then go ahead and process each of the queued events

		event_queue_t events;							// Empty event queue<>
		m_events.swap(events);							// Swap with existing queue<>
		eventslock.unlock();							// Release the queue<> lock

		while(!events.empty()) {

			event_t event = events.front();				// event_t
			events.pop();								// Remove from queue<>

			// Sync
			//
			// Signal synchronization/lock has been achieved
			if(event.eventid == eventid_t::Sync) {

				if(m_muxdata.sync == false) {

					m_muxdata.sync = true;
					invokecallback = true;
				}
			}

			// LostSync
			//
			// Signal synchronization/lock has been lost
			else if(event.eventid == eventid_t::LostSync) {

				if(m_muxdata.sync == true) {

					m_muxdata.sync = false;
					invokecallback = true;
				}
			}

			// ServiceDetected
			//
			// A new service has been detected
			else if(event.eventid == eventid_t::ServiceDetected) {

				// Iterate over each detected component within the service
				for(auto const& component : m_receiver->getComponents(m_receiver->getService(event.serviceid))) {

					// We only care about audio components; the presense of an SCId and/or a 
					// Packet address *appears* to indicate a data-only component
					if((component.SCId == 0) && (component.packetAddress == 0)) {

						// Check to see if we already have this subchannel in the mux data
						auto found = std::find_if(m_muxdata.subchannels.begin(), m_muxdata.subchannels.end(),
							[&](auto const& val) -> bool { return val.number == static_cast<uint32_t>(component.subchannelId); });

						// New subchannel detected; the label (name) will come later via SetServiceLabel
						if(found == m_muxdata.subchannels.end()) {

							m_muxdata.subchannels.push_back({ static_cast<uint32_t>(component.subchannelId), std::string() });
							invokecallback = true;
						}
					}
				}
			}

			// SetEnsembleLabel
			//
			// A new ensemble label has been received
			else if(event.eventid == eventid_t::SetEnsembleLabel) {

				std::string label = trim(m_receiver->getEnsembleLabel().utf8_label());
				if(label != m_muxdata.name) {

					m_muxdata.name = label;
					invokecallback = true;
				}
			}

			// SetServiceLabel
			//
			// A new service label has been received
			else if(event.eventid == eventid_t::SetServiceLabel) {

				// Iterate over each detected component within the service
				Service service = m_receiver->getService(event.serviceid);
				for(auto const& component : m_receiver->getComponents(service)) {

					// Update the label for any subchannels associated with the component
					for(auto& it : m_muxdata.subchannels) {

						if(it.number == static_cast<uint32_t>(component.subchannelId)) {

							std::string label = trim(service.serviceLabel.utf8_label());
							if(label != it.name) {

								it.name = label;
								invokecallback = true;
							}
						}
					}
				}
			}
		}
	}

	// If anything about the multiplex has changed, invoke the callback
	if(invokecallback) m_callback(m_muxdata);
}

//---------------------------------------------------------------------------
// dabmuxscanner::getSamples (InputInterface)
//
// Reads the specified number of samples from the input device
//
// Arguments:
//
//	buffer		- Buffer to receive the input samples
//	size		- Number of samples to read

int32_t dabmuxscanner::getSamples(DSPCOMPLEX* buffer, int32_t size)
{
	int32_t			numsamples = 0;			// Number of available samples in the buffer

	// Allocate a temporary buffer to pull the data out of the ring buffer
	std::unique_ptr<uint8_t[]> tempbuffer(new uint8_t[size * 2]);

	// Get the data from the ring buffer
	numsamples = m_ringbuffer.getDataFromBuffer(tempbuffer.get(), size * 2);

	// Scale the input data from [0,255] to [-1,1] for the demodulator
	for(int32_t index = 0; index < numsamples / 2; index++) {

		buffer[index] = DSPCOMPLEX(
			(static_cast<float>(tempbuffer[index * 2]) - 128.0f) / 128.0f,			// real
			(static_cast<float>(tempbuffer[(index * 2) + 1]) - 128.0f) / 128.0f		// imaginary
		);
	}

	return numsamples / 2;
}

//---------------------------------------------------------------------------
// dabmuxscanner::getSamplesToRead (InputInterface)
//
// Gets the number of input samples that are available to read from input
//
// Arguments:
//
//	NONE

int32_t dabmuxscanner::getSamplesToRead(void)
{
	return m_ringbuffer.GetRingBufferReadAvailable() / 2;
}

//---------------------------------------------------------------------------
// dabmuxscanner::is_ok (InputInterface)
//
// Determines if the input is still "OK"
//
// Arguments:
//
//	NONE

bool dabmuxscanner::is_ok(void)
{
	return true;
}

//---------------------------------------------------------------------------
// dabmuxscanner::restart (InputInterface)
//
// Restarts the input
//
// Arguments:
//
//	NONE

bool dabmuxscanner::restart(void)
{
	return true;
}

//---------------------------------------------------------------------------
// dabmuxscanner::onServiceDetected (RadioControllerInterface)
//
// Invoked when a new service was detected
//
// Arguments:
//
//	sId			- New service identifier

void dabmuxscanner::onServiceDetected(uint32_t sId)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(event_t{ eventid_t::ServiceDetected, sId });
}

//---------------------------------------------------------------------------
// dabmuxscanner::onSetEnsembleLabel (RadioControllerInterface)
//
// Invoked when the ensemble label has changed
//
// Arguments:
//
//	label		- New ensemble label

void dabmuxscanner::onSetEnsembleLabel(DabLabel& /*label*/)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(event_t{ eventid_t::SetEnsembleLabel, 0 });
}

//---------------------------------------------------------------------------
// dabmuxscanner::onSetServiceLabel (RadioControllerInterface)
//
// Invoked when a service label has changed
//
// Arguments:
//
//	sId				- Service identifier
//	label			- New service label

void dabmuxscanner::onSetServiceLabel(uint32_t sId, DabLabel& /*label*/)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(event_t{ eventid_t::SetServiceLabel, sId });
}

//---------------------------------------------------------------------------
// dabmuxscanner::onSyncChange (RadioControllerInterface)
//
// Invoked when signal synchronization was acquired or lost
//
// Arguments:
//
//	isSync		- Synchronization flag

void dabmuxscanner::onSyncChange(bool isSync)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(event_t{ (isSync) ? eventid_t::Sync : eventid_t::LostSync, 0 });
}

//---------------------------------------------------------------------------

#pragma warning(pop)
