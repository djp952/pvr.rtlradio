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

#ifndef __DBTYPES_H_
#define __DBTYPES_H_
#pragma once

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------------------

// DATABASE_CONNECTIONPOOL_SIZE
//
// Specifies the default size of the database connection pool
static size_t const DATABASE_CONNECTIONPOOL_SIZE = 3;

//---------------------------------------------------------------------------
// DATA TYPES
//---------------------------------------------------------------------------

// channelid_t
//
// Unique identifier for a channel
union channelid_t {

	struct {

		// FFFFFFFFFFFFFFFFFFFF SSSSSSSS MMMM (little endian)
		//
		unsigned int	modulation : 4;		// Modulation (0-15)
		unsigned int	subchannel : 8;		// Subchannel (0-255)
		unsigned int	frequency : 20;		// Frequency in KHz (0-1048575)

	} parts;

	unsigned int value;						// Complete channel id
};

static_assert(sizeof(channelid_t) == sizeof(uint32_t), "channelid_t union must be same size as a uint32_t");

// channelid
//
// Helper class to work with the channelid_t union data type
class channelid
{
public:

	channelid(unsigned int channelid) { m_channelid.value = channelid; }

	channelid(int frequency, enum modulation modulation)
	{
		m_channelid.parts.frequency = static_cast<unsigned int>(frequency / 1000);
		m_channelid.parts.subchannel = 0;
		m_channelid.parts.modulation = static_cast<unsigned int>(modulation);
	}

	channelid(int frequency, int subchannel, enum modulation modulation)
	{
		m_channelid.parts.frequency = static_cast<unsigned int>(frequency / 1000);
		m_channelid.parts.subchannel = static_cast<unsigned int>(subchannel);
		m_channelid.parts.modulation = static_cast<unsigned int>(modulation);
	}

	int frequency(void) const { return static_cast<int>(m_channelid.parts.frequency) * 1000; }

	unsigned int id(void) const { return m_channelid.value; }

	enum modulation modulation(void) const { return static_cast<enum modulation>(m_channelid.parts.modulation); }

	int subchannel(void) const { return static_cast<int>(m_channelid.parts.subchannel); }

private:

	channelid_t			m_channelid;		// Underlying channel identifier
};

// channel
//
// Information about a single channel enumerated from the database
struct channel {
	
	unsigned int	id;
	unsigned int	channel;
	unsigned int	subchannel;
	char const*		name;
	char const*		logourl;
};

// namedchannel
//
// Information about a single named channel enumerated from the database
struct namedchannel {

	uint32_t		frequency;
	char const*		name;
};

// rawfile
//
// Information about a single raw file enumerated from the database
struct rawfile {

	char const*		path;
	char const*		name;
	uint32_t		samplerate;
};

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DBTYPES_H_
