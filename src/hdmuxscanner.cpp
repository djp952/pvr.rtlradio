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
#include "hdmuxscanner.h"

#include <algorithm>
#include <stdexcept>

#pragma warning(push, 4)

// hdmuxscanner::SAMPLE_RATE
//
// Fixed device sample rate required for HD Radio
uint32_t const hdmuxscanner::SAMPLE_RATE = 1488375;

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
// hdmuxscanner Constructor (private)
//
// Arguments:
//
//	samplerate		- Sample rate of the input data
//	frequency		- Input data frequency in Hz
//	callback		- Callback function to invoke on status change

hdmuxscanner::hdmuxscanner(uint32_t samplerate, uint32_t frequency, callback const& callback) : m_callback(callback)
{
	assert(samplerate == SAMPLE_RATE);
	if(samplerate != SAMPLE_RATE) throw std::invalid_argument("samplerate");

	assert((frequency >= 87900000) && (frequency <= 107900000));
	if((frequency < 87900000) || (frequency > 107900000)) throw std::invalid_argument("frequency");

	// Initialize the HD Radio demodulator
	nrsc5_open_pipe(&m_nrsc5);
	nrsc5_set_mode(m_nrsc5, NRSC5_MODE_FM);
	nrsc5_set_callback(m_nrsc5, nrsc5_callback, this);
}

//---------------------------------------------------------------------------
// hdmuxscanner Destructor

hdmuxscanner::~hdmuxscanner()
{
	nrsc5_close(m_nrsc5);						// Close NRSC5
	m_nrsc5 = nullptr;							// Reset NRSC5 API handle
}

//---------------------------------------------------------------------------
// hdmuxscanner::create (static)
//
// Factory method, creates a new hdmuxscanner instance
//
// Arguments:
//
//	samplerate		- Sample rate of the input data
//	frequency		- Input data frequency in Hz
//	callback		- Callback function to invoke on status change

std::unique_ptr<hdmuxscanner> hdmuxscanner::create(uint32_t samplerate, uint32_t frequency, callback const& callback)
{
	return std::unique_ptr<hdmuxscanner>(new hdmuxscanner(samplerate, frequency, callback));
}

//---------------------------------------------------------------------------
// hdmuxscanner::inputsamples
//
// Pipes input samples into the muliplex scanner
//
// Arguments:
//
//	samples		- Pointer to the input samples
//	length		- Length of the input samples, in bytes

void hdmuxscanner::inputsamples(uint8_t const* samples, size_t length)
{
	// Pipe the samples into NRSC5, it will invoke the necessary callback(s)
	nrsc5_pipe_samples_cu8(m_nrsc5, const_cast<uint8_t*>(samples), static_cast<unsigned int>(length));
}

//---------------------------------------------------------------------------
// hdmuxscanner::nrsc5_callback (private, static)
//
// NRSC5 library event callback function
//
// Arguments:
//
//	event	- NRSC5 event being raised
//	arg		- Implementation-specific context pointer

void hdmuxscanner::nrsc5_callback(nrsc5_event_t const* event, void* arg)
{
	assert(arg != nullptr);
	reinterpret_cast<hdmuxscanner*>(arg)->nrsc5_callback(event);
}

//---------------------------------------------------------------------------
// hdmuxscanner::nrsc5_callback (private)
//
// NRSC5 library event callback function
//
// Arguments:
//
//	event	- NRSC5 event being raised

void hdmuxscanner::nrsc5_callback(nrsc5_event_t const* event)
{
	// NRSC5_EVENT_SYNC
	//
	// The HD Radio signal has been synchronized (locked)
	if(event->event == NRSC5_EVENT_SYNC) {

		if(m_muxdata.sync == false) {

			m_muxdata.sync = true;
			m_callback(m_muxdata);
		}
	}

	// NRSC5_EVENT_LOST_SYNC
	//
	// The HD Radio signal has been lost
	else if(event->event == NRSC5_EVENT_LOST_SYNC) {

		if(m_muxdata.sync == true) {

			m_muxdata.sync = false;
			m_callback(m_muxdata);
		}
	}

	// NRSC5_EVENT_SIG
	//
	// Station Information Guide (SIG) records have been decoded
	else if(event->event == NRSC5_EVENT_SIG) {

		bool invokecallback = false;

		nrsc5_sig_service_t* service = event->sig.services;
		while(service != nullptr) {

			// We only care about SERVICE_AUDIO subchannels here
			if(service->type == NRSC5_SIG_SERVICE_AUDIO) {

				assert(service->number > 0);			// Should never happen

				// HD Radio subchannels should always be "HDx"
				char name[256] = {};
				snprintf(name, std::extent<decltype(name)>::value, "HD%u", service->number);
				std::string servicename(name);

				auto found = std::find_if(m_muxdata.subchannels.begin(), m_muxdata.subchannels.end(),
					[&](auto const& val) -> bool { return val.number == service->number; });

				// Existing subchannel, check for a change to the name
				if(found != m_muxdata.subchannels.end()) {

					if(servicename != found->name) {

						found->name = servicename;
						invokecallback = true;
					}
				}

				// New subchannel, add to the vector<> of subchannels
				else {

					m_muxdata.subchannels.push_back({ service->number, servicename });
					invokecallback = true;
				}
			}

			service = service->next;
		}

		if(invokecallback) m_callback(m_muxdata);
	}

	// NRSC5_EVENT_SIS
	//
	// Station Information Service (SIS) data has been decoded
	else if(event->event == NRSC5_EVENT_SIS) {

		std::string name = trim((event->sis.name != nullptr) ? event->sis.name : "");
		if(name != m_muxdata.name) {

			m_muxdata.name = name;
			m_callback(m_muxdata);
		}
	}
}

//---------------------------------------------------------------------------

#pragma warning(pop)
