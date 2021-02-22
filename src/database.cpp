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

#include "stdafx.h"
#include "database.h"

#include <stdexcept>

#include "dbtypes.h"
#include "sqlite_exception.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------------

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, const char* value);
static void bind_parameter(sqlite3_stmt* statement, int& paramindex, uint32_t value);
template<typename... _parameters> static int execute_non_query(sqlite3* instance, char const* sql, _parameters&&... parameters);
template<typename... _parameters> static int execute_scalar_int(sqlite3* instance, char const* sql, _parameters&&... parameters);
template<typename... _parameters> static std::string execute_scalar_string(sqlite3* instance, char const* sql, _parameters&&... parameters);

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
// bind_parameter (local)
//
// Used by execute_non_query to bind a string parameter
//
// Arguments:
//
//	statement		- SQL statement instance
//	paramindex		- Index of the parameter to bind; will be incremented
//	value			- Value to bind as the parameter

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, char const* value)
{
	int					result;				// Result from binding operation

	// If a null string pointer was provided, bind it as NULL instead of TEXT
	if(value == nullptr) result = sqlite3_bind_null(statement, paramindex++);
	else result = sqlite3_bind_text(statement, paramindex++, value, -1, SQLITE_STATIC);

	if(result != SQLITE_OK) throw sqlite_exception(result);
}

//---------------------------------------------------------------------------
// bind_parameter (local)
//
// Used by execute_non_query to bind an integer parameter
//
// Arguments:
//
//	statement		- SQL statement instance
//	paramindex		- Index of the parameter to bind; will be incremented
//	value			- Value to bind as the parameter

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, uint32_t value)
{
	int result = sqlite3_bind_int(statement, paramindex++, static_cast<int32_t>(value));
	if(result != SQLITE_OK) throw sqlite_exception(result);
}

//---------------------------------------------------------------------------
// clear_channels
//
// Clears all channels from the database
//
// Arguments:
//
//	instance		- Database instance

void clear_channels(sqlite3* instance)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	execute_non_query(instance, "delete from channel");
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
// delete_channel
//
// Deletes a channel from the database
//
// Arguments:
//
//	instance	- Database instance
//	id			- ID of the channel to be deleted

void delete_channel(sqlite3* instance, unsigned int id)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	execute_non_query(instance, "delete from channel where frequency = ?1 and subchannel = ?2", (id / 10) * 100000, id % 10);
}

//---------------------------------------------------------------------------
// enumerate_channels
//
// Enumerates the available channels
//
// Arguments:
//
//	instance	- Database instance
//	callback	- Callback function

void enumerate_channels(sqlite3* instance, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	// id | channel | subchannel | name | hidden
	auto sql = "select ((frequency / 100000) * 10) + subchannel as id, (frequency / 1000000) as channel, "
		"(frequency % 1000000) / 100000 as subchannel, name as name, hidden as hidden, logourl as logourl "
		"from channel order by id asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			item.id = static_cast<unsigned int>(sqlite3_column_int(statement, 0));
			item.channel = static_cast<unsigned int>(sqlite3_column_int(statement, 1));
			item.subchannel = static_cast<unsigned int>(sqlite3_column_int(statement, 2));
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.hidden = (sqlite3_column_int(statement, 4) != 0);
			item.logourl = reinterpret_cast<char const*>(sqlite3_column_text(statement, 5));

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// enumerate_fmradio_channels
//
// Enumerates FM Radio channels
//
// Arguments:
//
//	instance	- Database instance
//	callback	- Callback function

void enumerate_fmradio_channels(sqlite3* instance, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	// id | channel | subchannel | name | hidden
	auto sql = "select ((frequency / 100000) * 10) + subchannel as id, (frequency / 1000000) as channel, "
		"(frequency % 1000000) / 100000 as sub, name as name, hidden as hidden from channel "
		"where channel.subchannel = 0 order by id asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			item.id = static_cast<unsigned int>(sqlite3_column_int(statement, 0));
			item.channel = static_cast<unsigned int>(sqlite3_column_int(statement, 1));
			item.subchannel = static_cast<unsigned int>(sqlite3_column_int(statement, 2));
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.hidden = (sqlite3_column_int(statement, 4) != 0);

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
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
// execute_scalar_int (local)
//
// Executes a database query and returns a scalar integer result
//
// Arguments:
//
//	instance		- Database instance
//	sql				- SQL query to execute
//	parameters		- Parameters to be bound to the query

template<typename... _parameters>
static int execute_scalar_int(sqlite3* instance, char const* sql, _parameters&&... parameters)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							paramindex = 1;		// Bound parameter index value
	int							value = 0;			// Result from the scalar function

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

		// Execute the query; only the first row returned will be used
		result = sqlite3_step(statement);

		if(result == SQLITE_ROW) value = sqlite3_column_int(statement, 0);
		else if(result != SQLITE_DONE) throw sqlite_exception(result, sqlite3_errmsg(instance));

		// Finalize the statement
		sqlite3_finalize(statement);

		// Return the resultant value from the scalar query
		return value;
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// execute_scalar_string (local)
//
// Executes a database query and returns a scalar string result
//
// Arguments:
//
//	instance		- Database instance
//	sql				- SQL query to execute
//	parameters		- Parameters to be bound to the query

template<typename... _parameters>
static std::string execute_scalar_string(sqlite3* instance, char const* sql, _parameters&&... parameters)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							paramindex = 1;		// Bound parameter index value
	std::string					value;				// Result from the scalar function

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

		// Execute the query; only the first row returned will be used
		result = sqlite3_step(statement);

		if(result == SQLITE_ROW) {

			char const* ptr = reinterpret_cast<char const*>(sqlite3_column_text(statement, 0));
			if(ptr != nullptr) value.assign(ptr);
		}
		else if(result != SQLITE_DONE) throw sqlite_exception(result, sqlite3_errmsg(instance));

		// Finalize the statement
		sqlite3_finalize(statement);

		// Return the resultant value from the scalar query
		return value;
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// export_channels
//
// Exports the channels into a JSON file
//
// Arguments:
//
//	instance	- SQLite database instance
//	path		- Path to the output file to generate

std::string export_channels(sqlite3* instance)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	return execute_scalar_string(instance, "select json_group_array("
		"json_object('frequency', frequency, 'subchannel', subchannel, 'hidden', hidden, "
		"'name', name, 'autogain', autogain, 'manualgain', manualgain, 'logourl', logourl)) "
		"from channel");
}

//---------------------------------------------------------------------------
// get_channel_count
//
// Gets the number of available channels in the database
//
// Arguments:
//
//	instance	- SQLite database instance

int get_channel_count(sqlite3* instance)
{
	if(instance == nullptr) return 0;

	return execute_scalar_int(instance, "select count(*) from channel");
}

//---------------------------------------------------------------------------
// get_channel_properties
//
// Gets the tuning properties of a channel from the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	id				- Channel unique identifier
//	channelprops	- Structure to receive the channel properties

bool get_channel_properties(sqlite3* instance, unsigned int id, struct channelprops& channelprops)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function
	bool						found = false;		// Flag if channel was found in database

	if(instance == nullptr) throw std::invalid_argument("instance");

	// frequency | subchannel | name | autogain | manualgain | logourl
	auto sql = "select frequency, subchannel, name, autogain, manualgain, logourl from channel "
		"where frequency = ?1 and subchannel = ?2";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int(statement, 1, (id / 10) * 100000);
		if(result == SQLITE_OK) sqlite3_bind_int(statement, 2, id % 10);
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query; there should be one and only one row returned
		if(sqlite3_step(statement) == SQLITE_ROW) {

			channelprops.frequency = static_cast<uint32_t>(sqlite3_column_int(statement, 0));
			channelprops.subchannel = static_cast<uint32_t>(sqlite3_column_int(statement, 1));

			unsigned char const* name = sqlite3_column_text(statement, 2);
			channelprops.name.assign((name == nullptr) ? "" : reinterpret_cast<char const*>(name));

			channelprops.autogain = (sqlite3_column_int(statement, 3) != 0);
			channelprops.manualgain = sqlite3_column_int(statement, 4);

			unsigned char const* logourl = sqlite3_column_text(statement, 5);
			channelprops.logourl.assign((logourl == nullptr) ? "" : reinterpret_cast<char const*>(logourl));

			found = true;						// Channel was found in the database
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }

	return found;
}

//---------------------------------------------------------------------------
// import_channels
//
// Imports channels from a JSON string
//
// Arguments:
//
//	instance	- Database instance
//	json		- JSON data to be imported

void import_channels(sqlite3* instance, char const* json)
{
	if(instance == nullptr) throw std::invalid_argument("instance");
	if((json == nullptr) || (*json == '\0')) throw std::invalid_argument("instance");

	// Massage the input as much as possible, only the frequency and subchannel fields are actually required,
	// the rest can be defaulted if not present. Also watch out for duplicates and the frequency range
	execute_non_query(instance, "replace into channel "
		"select cast(json_extract(entry.value, '$.frequency') as integer) as frequency, "
		"cast(json_extract(entry.value, '$.subchannel') as integer) as subchannel, "
		"cast(ifnull(json_extract(entry.value, '$.hidden'), 0) as integer) as hidden, "
		"cast(ifnull(json_extract(entry.value, '$.name'), '') as text) as name, "
		"cast(ifnull(json_extract(entry.value, '$.autogain'), 1) as integer) as autogain, "
		"cast(ifnull(json_extract(entry.value, '$.manualgain'), 0) as integer) as manualgain, "
		"json_extract(entry.value, '$.logourl') as logourl "	// <-- this one allows nulls
		"from json_each(?1) as entry "
		"where frequency is not null and subchannel is not null and frequency between 87500000 and 107900000 "
		"group by frequency, subchannel", json);
}

//---------------------------------------------------------------------------
// open_database
//
// Opens the SQLite database instance
//
// Arguments:
//
//	connstring	- Database connection string
//	flags		- Database open flags (see sqlite3_open_v2)

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
//	connstring	- Database connection string
//	flags		- Database open flags (see sqlite3_open_v2)
//	initialize	- Flag indicating database schema should be (re)initialized

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

			// get the database schema version
			//
			int dbversion = execute_scalar_int(instance, "pragma user_version");

			// SCHEMA VERSION 0 - NEW DATABASE
			//
			if(dbversion == 0) {

				// table: channel
				//
				// frequency(pk) | subchannel(pk) | hidden | name | autogain | manualgain | logourl
				execute_non_query(instance, "drop table if exists channel");
				execute_non_query(instance, "create table channel(frequency integer not null, subchannel integer not null, "
					"hidden integer not null, name text not null, autogain integer not null, manualgain integer not null, logourl text null, "
					"primary key(frequency, subchannel))");

				execute_non_query(instance, "pragma user_version = 1");
			}

			// SCHEMA VERSION 1 - CURRENT SCHEMA
		}
	}

	// Close the database instance on any thrown exceptions
	catch(...) { sqlite3_close(instance); throw; }

	return instance;
}

//---------------------------------------------------------------------------
// rename_channel
//
// Renames a channel in the database
//
// Arguments:
//
//	instance	- Database instance
//	id			- ID of the channel to be renamed
//	newname		- New name to assign to the channel

void rename_channel(sqlite3* instance, unsigned int id, char const* newname)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	execute_non_query(instance, "update channel set name = ?1 where frequency = ?2 and subchannel = ?3",
		(newname == nullptr) ? "" : newname, (id / 10) * 100000, id % 10);
}

//---------------------------------------------------------------------------
// update_channel_properties
//
// Updates the tuning properties of a channel in the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	id				- Channel unique identifier
//	channelprops	- Structure containing the updated channel properties

bool update_channel_properties(sqlite3* instance, unsigned int id, struct channelprops const& channelprops)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	return execute_non_query(instance, "update channel set name = ?1, autogain = ?2, manualgain = ?3, logourl = ?4"
		"where frequency = ?5 and subchannel = ?6", channelprops.name.c_str(), (channelprops.autogain) ? 1 : 0,
		channelprops.manualgain, channelprops.logourl.c_str(), (id / 10) * 100000, id % 10) > 0;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
