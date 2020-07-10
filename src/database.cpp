//---------------------------------------------------------------------------
// Copyright (c) 2016-2020 Michael G. Brehm
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
#include "database.h"

#include <stdexcept>

#include "dbtypes.h"
#include "sqlite_exception.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------------

template<typename... _parameters> 
static int execute_non_query(sqlite3* instance, char const* sql, _parameters&&... parameters);

//---------------------------------------------------------------------------
// CONNECTIONPOOL IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// connectionpool Constructor
//
// Arguments:
//
//	connstring		- Database connection string
//	poolsize		- Initial connection pool size
//	flags			- Database connection flags

connectionpool::connectionpool(char const* connstring, size_t poolsize, int flags) :
	m_connstr((connstring) ? connstring : ""), m_flags(flags)
{
	sqlite3*		handle = nullptr;		// Initial database connection

	if(connstring == nullptr) throw std::invalid_argument("connstring");

	// Create and pool an initial connection to initialize the database
	handle = open_database(m_connstr.c_str(), m_flags, true);
	m_connections.push_back(handle);
	m_queue.push(handle);

	// Create and pool the requested number of additional connections
	try {

		for(size_t index = 1; index < poolsize; index++) {

			handle = open_database(m_connstr.c_str(), m_flags, false);
			m_connections.push_back(handle);
			m_queue.push(handle);
		}
	}

	catch(...) {

		// Clear the connection cache and destroy all created connections
		while(!m_queue.empty()) m_queue.pop();
		for(auto const& iterator : m_connections) close_database(iterator);

		throw;
	}
}

//---------------------------------------------------------------------------
// connectionpool Destructor

connectionpool::~connectionpool()
{
	// Close all of the connections that were created in the pool
	for(auto const& iterator : m_connections) close_database(iterator);
}

//---------------------------------------------------------------------------
// connectionpool::acquire
//
// Acquires a database connection, opening a new one if necessary
//
// Arguments:
//
//	NONE

sqlite3* connectionpool::acquire(void)
{
	sqlite3* handle = nullptr;				// Handle to return to the caller

	std::unique_lock<std::mutex> lock(m_lock);

	if(m_queue.empty()) {

		// No connections are available, open a new one using the same flags
		handle = open_database(m_connstr.c_str(), m_flags, false);
		m_connections.push_back(handle);
	}

	// At least one connection is available for reuse
	else { handle = m_queue.front(); m_queue.pop(); }

	return handle;
}

//---------------------------------------------------------------------------
// connectionpool::release
//
// Releases a database handle acquired from the pool
//
// Arguments:
//
//	handle		- Handle to be releases

void connectionpool::release(sqlite3* handle)
{
	std::unique_lock<std::mutex> lock(m_lock);

	if(handle == nullptr) throw std::invalid_argument("handle");

	m_queue.push(handle);
}

//---------------------------------------------------------------------------
// close_database
//
// Closes a SQLite database handle
//
// Arguments:
//
//	instance	- Database instance handle to be closed

void close_database(sqlite3* instance)
{
	if(instance) sqlite3_close(instance);
}

//---------------------------------------------------------------------------
// execute_non_query (local)
//
// Executes a database query and returns the number of rows affected
//
// Arguments:
//
//	instance		- Database instance
//	sql				- SQL query to execute
//	parameters		- Parameters to be bound to the query

template<typename... _parameters>
static int execute_non_query(sqlite3* instance, char const* sql, _parameters&&... parameters)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							paramindex = 1;		// Bound parameter index value

	if(instance == nullptr) throw std::invalid_argument("instance");
	if(sql == nullptr) throw std::invalid_argument("sql");

	// Suppress unreferenced local variable warning when there are no parameters to bind
	(void)paramindex;

	// Prepare the statement
	int result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the provided query parameter(s) by unpacking the parameter pack
		int unpack[] = { 0, (static_cast<void>(bind_parameter(statement, paramindex, parameters)), 0) ... };
		(void)unpack;

		// Execute the query; ignore any rows that are returned
		do result = sqlite3_step(statement);
		while(result == SQLITE_ROW);

		// The final result from sqlite3_step should be SQLITE_DONE
		if(result != SQLITE_DONE) throw sqlite_exception(result, sqlite3_errmsg(instance));

		// Finalize the statement
		sqlite3_finalize(statement);

		// Return the number of changes made by the statement
		return sqlite3_changes(instance);
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// open_database
//
// Opens the SQLite database instance
//
// Arguments:
//
//	connstring		- Database connection string
//	flags			- Database open flags (see sqlite3_open_v2)

sqlite3* open_database(char const* connstring, int flags)
{
	return open_database(connstring, flags, false);
}

//---------------------------------------------------------------------------
// open_database
//
// Opens the SQLite database instance
//
// Arguments:
//
//	connstring		- Database connection string
//	flags			- Database open flags (see sqlite3_open_v2)
//	initialize		- Flag indicating database schema should be (re)initialized

sqlite3* open_database(char const* connstring, int flags, bool initialize)
{
	sqlite3*			instance = nullptr;			// SQLite database instance

	// Create the database using the provided connection string
	int result = sqlite3_open_v2(connstring, &instance, flags, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result);

	// set the connection to report extended error codes
	sqlite3_extended_result_codes(instance, -1);

	// set a busy_timeout handler for this connection
	sqlite3_busy_timeout(instance, 5000);

	try {

		// switch the database to write-ahead logging
		//
		execute_non_query(instance, "pragma journal_mode=wal");

		// Only execute schema creation steps if the database is being initialized; the caller needs
		// to ensure that this is set for only one connection otherwise locking issues can occur
		if(initialize) {

			// table: channel
			//
			// channelid(pk) | type | name | frequency | autogain | manualgain
			execute_non_query(instance, "create table if not exists channel(channelid integer primary key not null, type integer not null, name text not null, "
				"frequency integer not null, autogain integer not null, manualgain integer not null)");
		}
	}

	// Close the database instance on any thrown exceptions
	catch(...) { sqlite3_close(instance); throw; }

	return instance;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
