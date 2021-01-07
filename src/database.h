//---------------------------------------------------------------------------
// Copyright (c) 2020-2021 Michael G. Brehm
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
void delete_channel(sqlite3* instance, unsigned int id);

// enumerate_channels
//
// Enumerates all available channels
void enumerate_channels(sqlite3* instance, enumerate_channels_callback const& callback);

// enumerate_fmradio_channels
//
// Enumerates FM Radio channels
void enumerate_fmradio_channels(sqlite3* instance, enumerate_channels_callback const& callback);

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
bool get_channel_properties(sqlite3* instance, unsigned int id, struct channelprops& channelprops);

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
void rename_channel(sqlite3* instance, unsigned int id, char const* newname);

// update_channel_properties
//
// Gets the tuning properties of a channel from the database
bool update_channel_properties(sqlite3* instance, unsigned int id, struct channelprops const& channelprops);

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DATABASE_H_
