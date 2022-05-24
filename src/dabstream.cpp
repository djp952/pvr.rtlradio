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
#include "dabstream.h"

#include "string_exception.h"

#pragma warning(push, 4)

// dabstream::DEFAULT_AUDIO_RATE
//
// The default audio output sample rate
int const dabstream::DEFAULT_AUDIO_RATE = 48000;

// dabstream::MAX_PACKET_QUEUE
//
// Maximum number of queued demux packets
size_t const dabstream::MAX_PACKET_QUEUE = 200;		// ~5 seconds @ 24ms; 12 seconds @ 60ms

// dabstream::RING_BUFFER_SIZE
//
// Input ring buffer size
size_t const dabstream::RING_BUFFER_SIZE = (256 KiB);

// dabstream::SAMPLE_RATE
//
// Fixed device sample rate required for DAB
uint32_t const dabstream::SAMPLE_RATE = 2048000;

// dabstream::STREAM_ID_AUDIOBASE
//
// Base stream identifier for the audio output stream
int const dabstream::STREAM_ID_AUDIOBASE = 1;

// dabstream::STREAM_ID_ID3TAG
//
// Stream identifier for the ID3v2 tag output stream
int const dabstream::STREAM_ID_ID3TAG = 0;

//---------------------------------------------------------------------------
// dabstream Constructor (private)
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tuner device properties
//	channelprops	- Channel properties
//	dabprops		- DAB/DAB+ digital signal processor properties

dabstream::dabstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	struct channelprops const& channelprops, struct dabprops const& dabprops) : m_device(std::move(device)),
	m_subchannel((channelprops.subchannel == 0) ? 1 : channelprops.subchannel), m_pcmgain(powf(10.0f, dabprops.outputgain / 10.0f))
{
	// Allocate the input ring buffer
	m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[RING_BUFFER_SIZE]);
	if(!m_buffer) throw std::bad_alloc();

	// Initialize the RTL-SDR device instance
	m_device->set_frequency_correction(tunerprops.freqcorrection + channelprops.freqcorrection);
	m_device->set_sample_rate(SAMPLE_RATE);
	m_device->set_center_frequency(channelprops.frequency);

	// Adjust the device gain as specified by the channel properties
	m_device->set_automatic_gain_control(channelprops.autogain);
	if(channelprops.autogain == false) m_device->set_gain(channelprops.manualgain);

	// Construct and initialize the demodulator instance
	RadioControllerInterface& controllerinterface = *static_cast<RadioControllerInterface*>(this);
	InputInterface& inputinterface = *static_cast<InputInterface*>(this);
	RadioReceiverOptions options = {};
	options.disableCoarseCorrector = true;
	m_receiver = make_aligned<RadioReceiver>(controllerinterface, inputinterface, options, 1);

	// Create the worker thread
	scalar_condition<bool> started{ false };
	m_worker = std::thread(&dabstream::worker, this, std::ref(started));
	started.wait_until_equals(true);
}

//---------------------------------------------------------------------------
// dabstream Destructor

dabstream::~dabstream()
{
	close();
}

//---------------------------------------------------------------------------
// dabstream::canseek
//
// Gets a flag indicating if the stream allows seek operations
//
// Arguments:
//
//	NONE

bool dabstream::canseek(void) const
{
	return false;
}

//---------------------------------------------------------------------------
// dabstream::close
//
// Closes the stream
//
// Arguments:
//
//	NONE

void dabstream::close(void)
{
	m_stop = true;
	if(m_worker.joinable()) m_worker.join();

	if(m_receiver) m_receiver->stop();
	m_receiver.reset();

	if(m_device) m_device->cancel_async();
	m_device.reset();
}

//---------------------------------------------------------------------------
// dabstream::create (static)
//
// Factory method, creates a new dabstream instance
//
// Arguments:
//
//	device			- RTL-SDR device instance
//	tunerprops		- Tunder device properties
//	channelprops	- Channel properties
//	dabprops		- DAB/DAB+ digital signal processor properties

std::unique_ptr<dabstream> dabstream::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
	struct channelprops const& channelprops, struct dabprops const& dabprops)
{
	return std::unique_ptr<dabstream>(new dabstream(std::move(device), tunerprops, channelprops, dabprops));
}

//---------------------------------------------------------------------------
// dabstream::demuxabort
//
// Aborts the demultiplexer
//
// Arguments:
//
//	NONE

void dabstream::demuxabort(void)
{
}

//---------------------------------------------------------------------------
// dabstream::demuxflush
//
// Flushes the demultiplexer
//
// Arguments:
//
//	NONE

void dabstream::demuxflush(void)
{
}

//---------------------------------------------------------------------------
// dabstream::demuxread
//
// Reads the next packet from the demultiplexer
//
// Arguments:
//
//	allocator		- DemuxPacket allocation function

DEMUX_PACKET* dabstream::demuxread(std::function<DEMUX_PACKET* (int)> const& allocator)
{
	std::unique_lock<std::mutex> lock(m_queuelock);

	// Wait up to 50ms for there to be a packet available for processing
	if(!m_queuecv.wait_for(lock, std::chrono::milliseconds(50), [&]() -> bool { return ((m_queue.size() > 0) || m_stopped.load() == true); }))
		return allocator(0);

	// If the worker thread was stopped, check for and re-throw any exception that occurred,
	// otherwise assume it was stopped normally and return an empty demultiplexer packet
	if(m_stopped.load() == true) {

		if(m_worker_exception) std::rethrow_exception(m_worker_exception);
		else return allocator(0);
	}

	// Pop off the topmost object from the queue<> and release the lock
	std::unique_ptr<demux_packet_t> packet(std::move(m_queue.front()));
	m_queue.pop();
	lock.unlock();

	// The packet queue should never have a null packet in it
	assert(packet);
	if(!packet) return allocator(0);

	// Allocate and initialize the DEMUX_PACKET
	DEMUX_PACKET* demuxpacket = allocator(packet->size);
	if(demuxpacket != nullptr) {

		demuxpacket->iStreamId = packet->streamid;
		demuxpacket->iSize = packet->size;
		demuxpacket->duration = packet->duration;
		demuxpacket->dts = packet->dts;
		demuxpacket->pts = packet->pts;
		if(packet->size > 0) memcpy(demuxpacket->pData, packet->data.get(), packet->size);
	}

	return demuxpacket;
}

//---------------------------------------------------------------------------
// dabstream::demuxreset
//
// Resets the demultiplexer
//
// Arguments:
//
//	NONE

void dabstream::demuxreset(void)
{
}

//---------------------------------------------------------------------------
// dabstream::devicename
//
// Gets the device name associated with the stream
//
// Arguments:
//
//	NONE

std::string dabstream::devicename(void) const
{
	return std::string(m_device->get_device_name());
}

//---------------------------------------------------------------------------
// dabstream::enumproperties
//
// Enumerates the stream properties
//
// Arguments:
//
//	callback		- Callback to invoke for each stream

void dabstream::enumproperties(std::function<void(struct streamprops const& props)> const& callback)
{
	// AUDIO STREAM
	//
	streamprops audio = {};
	audio.codec = "pcm_s16le";
	audio.pid = m_audioid.load();
	audio.channels = 2;
	audio.samplerate = m_audiorate.load();
	audio.bitspersample = 16;
	callback(audio);
}

//---------------------------------------------------------------------------
// dabstream::length
//
// Gets the length of the stream; or -1 if stream is real-time
//
// Arguments:
//
//	NONE

long long dabstream::length(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// dabstream::muxname
//
// Gets the mux name associated with the stream
//
// Arguments:
//
//	NONE

std::string dabstream::muxname(void) const
{
	return "";
}

//---------------------------------------------------------------------------
// dabstream::position
//
// Gets the current position of the stream
//
// Arguments:
//
//	NONE

long long dabstream::position(void) const
{
	return -1;
}

//---------------------------------------------------------------------------
// dabstream::read
//
// Reads data from the live stream
//
// Arguments:
//
//	buffer		- Buffer to receive the live stream data
//	count		- Size of the destination buffer in bytes

size_t dabstream::read(uint8_t* /*buffer*/, size_t /*count*/)
{
	return 0;
}

//---------------------------------------------------------------------------
// dabstream::realtime
//
// Gets a flag indicating if the stream is real-time
//
// Arguments:
//
//	NONE

bool dabstream::realtime(void) const
{
	return true;
}

//---------------------------------------------------------------------------
// dabstream::seek
//
// Sets the stream pointer to a specific position
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

long long dabstream::seek(long long /*position*/, int /*whence*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// dabstream::servicename
//
// Gets the service name associated with the stream
//
// Arguments:
//
//	NONE

std::string dabstream::servicename(void) const
{
	return "";				// TODO
}

//---------------------------------------------------------------------------
// dabstream::signalquality
//
// Gets the signal quality as percentages
//
// Arguments:
//
//	NONE

void dabstream::signalquality(int& quality, int& snr) const
{
	quality = snr = 0;		// TODO
}

//---------------------------------------------------------------------------
// dabstream::worker (private)
//
// Worker thread procedure used to transfer and process data
//
// Arguments:
//
//	started		- Condition variable to set when thread has started

void dabstream::worker(scalar_condition<bool>& started)
{
	std::vector<Service>	servicelist;			// vector<> of current services
	bool					foundsub = false;		// Flag indicating the desired subchannel was found

	assert(m_device);
	assert(m_receiver);

	m_receiver->restart(false);
	started = true;

	try {

		// Loop to process queued events until the worker is signaled to stop
		while(m_stop.test(true) == false) {

			// Check for and process all new events; hold the lock on the queue until
			// every queued event has been processed
			std::unique_lock<std::mutex> lock(m_eventslock);
			m_eventscv.wait_for(lock, std::chrono::milliseconds(50), [&]() -> bool {

				// No events
				if(m_events.empty()) return true;

				// The threading model is a bit weird here; the callback that queued a new
				// event needs to be free to continue execution otherwise the DSP may deadlock 
				// while we process that event. Combat this by swapping the queue<> with a new
				// one, release the lock, then go ahead and process each of the queued events

				event_queue_t events;							// Empty event queue<>
				m_events.swap(events);							// Swap with existing queue<>
				lock.unlock();									// Release the queue<> lock

				while(!events.empty()) {

					eventid_t eventid = events.front();			// eventid_t
					events.pop();								// Remove from queue<>

					switch(eventid) {

						// InputFailure
						//
						// Something has gone wrong with the input stream
						case eventid_t::InputFailure:

							throw string_exception("Input Failure");		// TODO: message
							break;

						// ServiceDetected
						//
						// A new service has been detected
						case eventid_t::ServiceDetected:

							if(foundsub) break;				// Subchannel has already been found; ignore

							// Determine if the desired subchannel is now present in the decoded services
							servicelist = m_receiver->getServiceList();
							for (auto const& service : servicelist) {
								for (auto const& component : m_receiver->getComponents(service)) {

									if(component.subchannelId == static_cast<int16_t>(m_subchannel)) {

										// The desired subchannel has been found; begin audio playback
										ProgrammeHandlerInterface& phi = *static_cast<ProgrammeHandlerInterface*>(this);
										m_receiver->playSingleProgramme(phi, {}, service);

										foundsub = true;					//  Stop processing service events
									}
								}
							}
							break;
					}
				}

				return true;
			});
		}
	}

	catch(...) { m_worker_exception = std::current_exception(); }

	m_stopped.store(true);					// Worker thread is now stopped
	m_queuecv.notify_all();					// Unblock any demux queue waiters
}

//---------------------------------------------------------------------------
// dabstream::getSamples (InputInterface)
//
// Reads the specified number of samples from the input device
//
// Arguments:
//
//	buffer		- Buffer to receive the input samples
//	size		- Number of samples to read

int32_t dabstream::getSamples(DSPCOMPLEX* buffer, int32_t size)
{
	int32_t numsamples = std::min(size, getSamplesToRead());
	for(int32_t index = 0; index < numsamples; index++) {

		// Scale the input data from [0,255] to [-1,1] for the demodulator
		buffer[index] = DSPCOMPLEX(
			(static_cast<float>(m_buffer[m_tail]) - 128.0f) / 128.0f,		// real
			(static_cast<float>(m_buffer[m_tail + 1]) - 128.0f) / 128.0f	// imaginary
		);

		m_tail += 2;
		if(m_tail >= RING_BUFFER_SIZE) m_tail = 0;
	}

	return numsamples;
}

//---------------------------------------------------------------------------
// dabstream::getSamplesToRead (InputInterface)
//
// Gets the number of input samples that are available to read from input
//
// Arguments:
//
//	NONE

int32_t dabstream::getSamplesToRead(void)
{
	size_t const READ_RATE = (SAMPLE_RATE / 100);			// .01 seconds worth of samples

	assert(m_device);

	// Determine how much data is available to read from the ring buffer, and if it has fallen
	// below the threshold read more input data from the device
	size_t available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
	if(available < READ_RATE) {

		// Read another set of data from the input device
		size_t cb = READ_RATE;
		while(cb) {

			// If the head is behind the tail linearly, take the data between them otherwise
			// take the data between the end of the buffer and the head
			size_t chunk = (m_head < m_tail) ? std::min(cb, m_tail - m_head) : std::min(cb, RING_BUFFER_SIZE - m_head);
			size_t read = m_device->read(&m_buffer[m_head], chunk);

			// If no data was read from the device, something is wrong ...
			if(read == 0) { m_streamok.store(false); return 0; }

			m_head += read;					// Increment the head position
			cb -= read;						// Decrement remaining bytes

			// If the head has reached the end of the buffer, reset it back to zero
			if(m_head >= RING_BUFFER_SIZE) m_head = 0;
		}

		available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
	}

	return static_cast<int32_t>(available / 2);
}

//---------------------------------------------------------------------------
// dabstream::is_ok (InputInterface)
//
// Determines if the input is still "OK"
//
// Arguments:
//
//	NONE

bool dabstream::is_ok(void)
{
	return m_streamok.load();
}

//---------------------------------------------------------------------------
// dabstream::restart (InputInterface)
//
// Restarts the input
//
// Arguments:
//
//	NONE

bool dabstream::restart(void)
{
	assert(m_device);

	m_streamok.store(true);
	m_device->begin_stream();

	return true;
}

//---------------------------------------------------------------------------
// dabstream::onNewAudio (ProgrammeHandlerInterface)
//
// Invoked when a new packet of audio data has been decoded
//
// Arguments:
//
//	audioData		- vector<> of stereo PCM audio data
//	sampleRate		- Sample rate of the audio data (subject to change)
//	mode			- Information about the audio encoding

void dabstream::onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, std::string const& /*mode*/)
{
	if(audioData.size() == 0) return;

	// Allocate the uint8_t buffer to hold the PCM audio data
	size_t pcmsize = audioData.size() * sizeof(int16_t);
	std::unique_ptr<uint8_t[]> pcm(new uint8_t[pcmsize]);

	// Copy the audio data into the buffer while applying the specified PCM output gain 
	int16_t* pcmdata = reinterpret_cast<int16_t*>(pcm.get());
	for(size_t index = 0; index < audioData.size(); index++)
		pcmdata[index] = static_cast<int16_t>(audioData[index] * m_pcmgain);

	std::unique_lock<std::mutex> lock(m_queuelock);

	// Detect and handle a change in the audio output sample rate
	if(sampleRate != m_audiorate) {

		m_audioid.fetch_add(1);				// Increment the audio stream id
		m_audiorate.store(sampleRate);		// Change the sample rate

		// Queue a DEMUX_SPECIALID_STREAMCHANGE packet to inform of the stream change
		std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
		packet->streamid = DEMUX_SPECIALID_STREAMCHANGE;
		m_queue.emplace(std::move(packet));
	}

	// If the queue size has exceeded the maximum, the packets aren't being
	// processed quickly enough by the demux read function
	if(m_queue.size() >= MAX_PACKET_QUEUE) {

		m_queue = demux_queue_t();		// Replace the queue<>

		// Queue a DEMUX_SPECIALID_STREAMCHANGE packet into the new queue
		std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
		packet->streamid = DEMUX_SPECIALID_STREAMCHANGE;
		m_queue.emplace(std::move(packet));

		m_dts = STREAM_TIME_BASE;		// Reset DTS back to base time
	}

	// Generate and queue the demux audio packet
	std::unique_ptr<demux_packet_t> packet = std::make_unique<demux_packet_t>();
	packet->streamid = m_audioid.load();
	packet->size = static_cast<int>(pcmsize);
	packet->duration = (audioData.size() / 2.0 / static_cast<double>(sampleRate)) * STREAM_TIME_BASE;
	packet->dts = packet->pts = m_dts;
	packet->data = std::move(pcm);

	m_dts += packet->duration;

	m_queue.emplace(std::move(packet));
	m_queuecv.notify_all();
}

//---------------------------------------------------------------------------
// dabstream::onNewDynamicLabel (ProgrammeHandlerInterface)
//
// Invoked when a new dynamic label has been decoded
//
// Arguments:
//
//	label		- The new dynamic label (UTF-8)

void dabstream::onNewDynamicLabel(std::string const& /*label*/)
{
	// TODO
}

//---------------------------------------------------------------------------
// dabstream::onMOT (ProgrammeHandlerInterface)
//
// Invoked when a new dynamic label has been decoded
//
// Arguments:
//
//	mot_file		- The new slide data

void dabstream::onMOT(mot_file_t const& /*mot_file*/)
{
	// Content sub type 0x01 = JPEG
	// Content sub type 0x03 = PNG

	//
	// TODO: I need a sample that contains a MOT frame
	//
}

//---------------------------------------------------------------------------
// dabstream::onFrequencyCorrectorChange (RadioControllerInterface)
//
// Invoked when the frequency correction has been changed
//
// Arguments:
//
//	fine			- Fine frequency correction value
//	coarse			- Coarse frequency correction value

void dabstream::onFrequencyCorrectorChange(int /*fine*/, int /*coarse*/)
{
	// TODO - This can be applied to the device real-time?
}

//---------------------------------------------------------------------------
// dabstream::onInputFailure (RadioControllerInterface)
//
// Invoked when the receiver has shut down to an input failure
//
// Arguments:
//
//	NONE

void dabstream::onInputFailure(void)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(eventid_t::InputFailure);
	m_eventscv.notify_all();
}

//---------------------------------------------------------------------------
// dabstream::onServiceDetected (RadioControllerInterface)
//
// Invoked when a new service was detected
//
// Arguments:
//
//	sId			- New service identifier

void dabstream::onServiceDetected(uint32_t /*sId*/)
{
	std::unique_lock<std::mutex> lock(m_eventslock);
	m_events.emplace(eventid_t::ServiceDetected);
	m_eventscv.notify_all();
}

//---------------------------------------------------------------------------
// dabstream::onSetEnsembleLabel (RadioControllerInterface)
//
// Invoked when the ensemble label has changed
//
// Arguments:
//
//	label		- New ensemble label

void dabstream::onSetEnsembleLabel(DabLabel& /*label*/)
{
	//
	// TODO: This can probably be used to automatically generate
	// a channel group
	//
}

//---------------------------------------------------------------------------
// dabstream::onSNR (RadioControllerInterface)
//
// Invoked when the Signal-to-Nosie Radio has been calculated
//
// Arguments:
//
//	snr			- Signal-to-Noise Ratio, in dB

void dabstream::onSNR(float /*snr*/)
{
	//
	// TODO: Figure out what an acceptable SNR is for DAB and provide
	// the necessary event to change the signal quality metric
	//
}

//---------------------------------------------------------------------------
// dabstream::onSyncChange (RadioControllerInterface)
//
// Invoked when signal synchronization was acquired or lost
//
// Arguments:
//
//	isSync		- Synchronization flag

void dabstream::onSyncChange(bool /*isSync*/)
{
	//
	// TODO: This might need to STREAMCHANGE, clear the demux queue,
	// silence the audio, and maybe throw up a banner to the user
	//
}

//---------------------------------------------------------------------------

#pragma warning(pop)
