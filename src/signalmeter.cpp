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
#include "signalmeter.h"

#include <cmath>
#include <string.h>

#include "align.h"
#include "string_exception.h"

#pragma warning(push, 4)

// signalmeter::DEFAULT_FFT_SIZE
//
// Default FFT size (bins)
size_t const signalmeter::DEFAULT_FFT_SIZE = 512;

// signalmeter::RING_BUFFER_SIZE
//
// Input ring buffer size
size_t const signalmeter::RING_BUFFER_SIZE = (4 MiB);		// 1s @ 2048000

//---------------------------------------------------------------------------
// signalmeter Constructor (private)
//
// Arguments:
//
//	signalprops		- Signal properties
//	plotprops		- Signal plot properties
//	rate			- Signal status rate in milliseconds
//	onstatus		- Signal status callback function

signalmeter::signalmeter(struct signalprops const& signalprops, struct signalplotprops const& plotprops, 
	uint32_t rate, status_callback const& onstatus) : m_signalprops(signalprops), m_plotprops(plotprops), 
	m_onstatus(onstatus)
{
	// Make sure the ring buffer is going to be big enough for the requested rate
	size_t bufferrequired = static_cast<size_t>((signalprops.samplerate * 2) * (static_cast<float>(rate) / 1000.0f));
	if(bufferrequired > RING_BUFFER_SIZE) throw std::invalid_argument("rate");

	// Allocate the input ring buffer
	m_buffer = std::unique_ptr<uint8_t[]>(new uint8_t[RING_BUFFER_SIZE]);
	if(!m_buffer) throw std::bad_alloc();

	// The FFT size (bins) needs to be a power of two equal to or larger than the signal plot
	// width; start at 512 and increase until we find that value
	while(m_plotprops.width > m_fftsize) m_fftsize <<= 1;

	// Calculate the number of bytes required in the input buffer to process and report
	// updated signal statistics at (apporoximately) the requested rate in milliseconds
	size_t bytespersecond = m_signalprops.samplerate * 2;
	m_fftminbytes = align::down(static_cast<size_t>(bytespersecond * (static_cast<float>(rate) / 1000.0f)),
		static_cast<unsigned int>(m_fftsize));

	// Initialize the finite impulse response filter
	m_fir.SetupParameters(static_cast<TYPEREAL>(m_signalprops.lowcut), static_cast<TYPEREAL>(m_signalprops.highcut), 
		-static_cast<TYPEREAL>(m_signalprops.offset), m_signalprops.samplerate);

	// Initialize the fast fourier transform instance
	m_fft.SetFFTParams(static_cast<int>(m_fftsize), false, 0.0, m_signalprops.samplerate);
	m_fft.SetFFTAve(50);
}

//---------------------------------------------------------------------------
// signalmeter::create (static)
//
// Factory method, creates a new signalmeter instance
//
// Arguments:
//
//	signalprops		- Signal properties
//	plotprops		- Signal plot properties
//	rate			- Signal status rate in milliseconds
//	onstatus		- Signal status callback function

std::unique_ptr<signalmeter> signalmeter::create(struct signalprops const& signalprops, struct signalplotprops const& plotprops,
	uint32_t rate, status_callback const& onstatus)
{
	return std::unique_ptr<signalmeter>(new signalmeter(signalprops, plotprops, rate, onstatus));
}

//---------------------------------------------------------------------------
// signalmeter::inputsamples
//
// Pipes input samples into the signal meter instance
//
// Arguments:
//
//	samples		- Pointer to the raw 8-bit I/Q samples from the device
//	length		- Length of the input data in bytes

void signalmeter::inputsamples(uint8_t const* samples, size_t length)
{
	size_t		byteswritten = 0;					// Bytes written into ring buffer

	if((samples == nullptr) || (length == 0)) return;

	// Ensure there is enough space in the ring buffer to satisfy the operation
	size_t available = (m_head < m_tail) ? m_tail - m_head : (RING_BUFFER_SIZE - m_head) + m_tail;
	if(length > available) throw string_exception("Insufficient ring buffer space to accomodate input");

	// Write the input data into the ring buffer
	size_t cb = length;
	while(cb > 0) {

		// If the head is behind the tail linearly, take the data between them otherwise
		// take the data between the end of the buffer and the head
		size_t chunk = (m_head < m_tail) ? std::min(cb, m_tail - m_head) : std::min(cb, RING_BUFFER_SIZE - m_head);
		memcpy(&m_buffer[m_head], &samples[byteswritten], chunk);

		m_head += chunk;				// Increment the head position
		byteswritten += chunk;			// Increment number of bytes written
		cb -= chunk;					// Decrement remaining bytes

		// If the head has reached the end of the buffer, reset it back to zero
		if(m_head >= RING_BUFFER_SIZE) m_head = 0;
	}

	assert(byteswritten == length);		// Verify all bytes were written
	processsamples();					// Process the available input samples
}

//---------------------------------------------------------------------------
// signalmeter::processsamples (private)
//
// Processes the available input samples
//
// Arguments:
//
//	NONE

void signalmeter::processsamples(void)
{
	// Allocate a heap buffer to store the converted I/Q samples for the FFT
	std::unique_ptr<TYPECPX[]> samples(new TYPECPX[m_fftsize]);
	if(!samples) throw std::bad_alloc();

	assert((m_fftminbytes % m_fftsize) == 0);

	// Determine how much data is available in the ring buffer for processing as I/Q samples
	size_t available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
	while(available >= m_fftminbytes) {

		// Push all of the input samples into the FFT
		for(size_t iterations = 0; iterations < (available / m_fftsize); iterations++) {

			// Convert the raw 8-bit I/Q samples into scaled complex I/Q samples
			for(size_t index = 0; index < m_fftsize; index++) {

				// The FFT expects the I/Q samples in the range of -32767.0 through +32767.0
				// (32767.0 / 127.5) = 256.9960784313725
				samples[index] = {

				#ifdef FMDSP_USE_DOUBLE_PRECISION
					(static_cast<TYPEREAL>(m_buffer[m_tail]) - 127.5) * 256.9960784313725,			// I
					(static_cast<TYPEREAL>(m_buffer[m_tail + 1]) - 127.5) * 256.9960784313725,		// Q
				#else
					(static_cast<TYPEREAL>(m_buffer[m_tail]) - 127.5f) * 256.9960784313725f,		// I
					(static_cast<TYPEREAL>(m_buffer[m_tail + 1]) - 127.5f) * 256.9960784313725f,	// Q
				#endif
				};

				m_tail += 2;
				if(m_tail >= RING_BUFFER_SIZE) m_tail = 0;
			}

			size_t numsamples = m_fftsize;

			// If specified, filter out everything but the desired bandwidth
			if(m_signalprops.filter) {

				numsamples = m_fir.ProcessData(static_cast<int>(m_fftsize), &samples[0], &samples[0]);
				assert(numsamples == m_fftsize);
			}

			// Push the current set of samples through the fast fourier transform
			m_fft.PutInDisplayFFT(static_cast<qint32>(numsamples), &samples[0]);

			// Recalculate the amount of available data in the ring buffer after the read operation
			available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
		}

		// Convert the FFT into an integer-based signal plot
		std::unique_ptr<int[]> plot(new int[m_plotprops.width + 1]);
		bool overload = m_fft.GetScreenIntegerFFTData(static_cast<qint32>(m_plotprops.height), static_cast<qint32>(m_plotprops.width),
			static_cast<TYPEREAL>(m_plotprops.maxdb), static_cast<TYPEREAL>(m_plotprops.mindb),
			-(static_cast<int32_t>(m_signalprops.bandwidth) / 2) - m_signalprops.offset,
			(static_cast<int32_t>(m_signalprops.bandwidth) / 2) - m_signalprops.offset, &plot[0]);

		// Determine how many Hertz are represented by each measurement in the signal plot and the center point
		float hzper = static_cast<float>(m_plotprops.width) / static_cast<float>(m_signalprops.bandwidth);
		int32_t center = static_cast<int>(m_plotprops.width) / 2;

		// Initialize a new signal_status structure with the proper signal type for the low/high cut indexes
		struct signal_status status = {};

		// Power is measured at the center frequency and smoothed
		float power = ((m_plotprops.mindb - m_plotprops.maxdb) * (static_cast<float>(plot[center]) / 
			static_cast<float>(m_plotprops.height))) + m_plotprops.maxdb;
		status.power = m_avgpower = (std::isnan(m_avgpower)) ? power : 0.85f * m_avgpower + 0.15f * power;

		// Noise is measured at the low and high cuts, averaged, and smoothed
		status.lowcut = std::max(0, center + static_cast<int32_t>(m_signalprops.lowcut * hzper));
		status.highcut = std::min(static_cast<int32_t>(m_plotprops.width - 1), center + static_cast<int32_t>(m_signalprops.highcut * hzper));
		float noise = ((m_plotprops.mindb - m_plotprops.maxdb) * (static_cast<float>((plot[status.lowcut] + plot[status.highcut]) / 2.0f) /
			static_cast<float>(m_plotprops.height))) + m_plotprops.maxdb;
		status.noise = m_avgnoise = (std::isnan(m_avgnoise)) ? noise : 0.85f * m_avgnoise + 0.15f * noise;

		status.snr = m_avgpower + -m_avgnoise;				// SNR in dB
		status.overload = overload;							// Overload flag
		status.plotsize = m_plotprops.width;				// Plot width
		status.plotdata = &plot[0];							// Plot data

		// Invoke the callback to report the updated metrics and signal plot
		m_onstatus(status);

		// Recalculate the amount of available data in the ring buffer
		available = (m_tail > m_head) ? (RING_BUFFER_SIZE - m_tail) + m_head : m_head - m_tail;
	}
}

//---------------------------------------------------------------------------

#pragma warning(pop)
