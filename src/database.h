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

#ifndef __DATABASE_H_
#define __DATABASE_H_
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <sqlite3.h>
#include <string>
#include <vector>

#include "props.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// DATA TYPES
//---------------------------------------------------------------------------

// enumerate_channels_callback
//
// Callback function passed to enumerate_channels
using enumerate_channels_callback = std::function<void(struct channel const& channel)>;

// enumerate_namedchannels_callback
//
// Callback function passed to enumerate_namedchannels
using enumerate_namedchannels_callback = std::function<void(struct namedchannel const& namedchannel)>;

// enumerate_rawfiles_callback
//
// Callback function passed to enumerate_rawfiles
using enumerate_rawfiles_callback = std::function<void(struct rawfile const& rawfile)>;

//---------------------------------------------------------------------------
// connectionpool
//
// Implements a connection pool for the SQLite database connections

class connectionpool
{
public:

	// Instance Constructor
	//
	connectionpool(char const* connstr, size_t poolsize, int flags);

	// Destructor
	//
	~connectionpool();

	//-----------------------------------------------------------------------
	// Member Functions

	// acquire
	//
	// Acquires a connection from the pool, creating a new one as necessary
	sqlite3* acquire(void);

	// release
	//
	// Releases a previously acquired connection back into the pool
	void release(sqlite3* handle);

	//-----------------------------------------------------------------------
	// Type Declarations

	// handle
	//
	// RAII class to acquire and release connections from the pool
	class handle
	{
	public:

		// Constructor / Destructor
		//
		handle(std::shared_ptr<connectionpool> const& pool) : m_pool(pool), m_handle(pool->acquire()) { }
		~handle() { m_pool->release(m_handle); }

		// sqlite3* type conversion operator
		//
		operator sqlite3*(void) const { return m_handle; }

	private:

		handle(handle const&) = delete;
		handle& operator=(handle const&) = delete;

		// m_pool
		//
		// Shared pointer to the parent connection pool
		std::shared_ptr<connectionpool> const m_pool;

		// m_handle
		//
		// SQLite handle acquired from the pool
		sqlite3* m_handle;
	};

private:

	connectionpool(connectionpool const&) = delete;
	connectionpool& operator=(connectionpool const&) = delete;

	//-----------------------------------------------------------------------
	// Member Variables

	std::string	const			m_connstr;			// Connection string
	int	const					m_flags;			// Connection flags
	std::vector<sqlite3*>		m_connections;		// All active connections
	std::queue<sqlite3*>		m_queue;			// Queue of unused connection
	mutable std::mutex			m_lock;				// Synchronization object
};

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------------

// add_channel
//
// Adds a new channel to the database
bool add_channel(sqlite3* instance, struct channelprops const& channelprops);
bool add_channel(sqlite3* instance, struct channelprops const& channelprops, std::vector<struct subchannelprops> const& subchannelprops);

// channel_exists
//
// Determines if a channel exists in the database
bool channel_exists(sqlite3* instance, struct channelprops const& channelprops);

// clear_channels
//
// Clears all channels from the database
void clear_channels(sqlite3* instance);

// close_database
//
// Creates a SQLite database instance handle
void close_database(sqlite3* instance);

// delete_channel
//
// Deletes a channel from the database
void delete_channel(sqlite3* instance, uint32_t frequency, enum modulation modulation);

// delete_channel
//
// Deletes a channel from the database
void delete_channel(sqlite3* instance, uint32_t frequency, enum modulation modulation);

// delete_subchannel
//
// Deletes a subchannel from the database
void delete_subchannel(sqlite3* instance, uint32_t frequency, enum modulation modulation, uint32_t number);

// enumerate_dabradio_channels
//
// Enumerates DAB channels
void enumerate_dabradio_channels(sqlite3* instance, enumerate_channels_callback const& callback);

// enumerate_fmradio_channels
//
// Enumerates FM Radio channels
void enumerate_fmradio_channels(sqlite3* instance, bool prependnumber, enumerate_channels_callback const& callback);

// enumerate_hdradio_channels
//
// Enumerates HD Radio channels
void enumerate_hdradio_channels(sqlite3* instance, bool prependnumber, enumerate_channels_callback const& callback);

// enumerate_namedchannels
//
// Enumerates the named channels for a specific modulation
void enumerate_namedchannels(sqlite3* instance, enum modulation modulation, enumerate_namedchannels_callback const& callback);

// enumerate_rawfiles
//
// Enumerates available raw files registered in the database
void enumerate_rawfiles(sqlite3* instance, enumerate_rawfiles_callback const& callback);

// enumerate_wxradio_channels
//
// Enumerates Weather Radio channels
void enumerate_wxradio_channels(sqlite3* instance, enumerate_channels_callback const& callback);

// export_channels
//
// Exports the channels into a JSON string
std::string export_channels(sqlite3* instance);

// get_channel_count
//
// Gets the number of available channels in the database
int get_channel_count(sqlite3* instance);

// get_channel_properties
//
// Gets the tuning properties of a channel from the database
bool get_channel_properties(sqlite3* instance, uint32_t frequency, enum modulation modulation, struct channelprops& channelprops);
bool get_channel_properties(sqlite3* instance, uint32_t frequency, enum modulation modulation, struct channelprops& channelprops,
	std::vector<struct subchannelprops>& subchannelprops);

// has_rawfiles
//
// Gets a flag indicating if there are raw input files available to use
bool has_rawfiles(sqlite3* instance);

// import_channels
//
// Imports channels from a JSON string
void import_channels(sqlite3* instance, char const* json);

// open_database
//
// Opens a handle to the backend SQLite database
sqlite3* open_database(char const* connstring, int flags);
sqlite3* open_database(char const* connstring, int flags, bool initialize);

// rename_channel
//
// Renames a channel in the database
void rename_channel(sqlite3* instance, uint32_t frequency, enum modulation modulation, char const* newname);

// try_execute_non_query
//
// executes a non-query against the database but eats any exceptions
bool try_execute_non_query(sqlite3* instance, char const* sql);

// update_channel
//
// Updates the tuning properties of a channel in the database
bool update_channel(sqlite3* instance, struct channelprops const& channelprops);
bool update_channel(sqlite3* instance, struct channelprops const& channelprops, std::vector<struct subchannelprops> const& subchannelprops);

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DATABASE_H_
