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
#include "database.h"

#include <stdexcept>

#include "dbtypes.h"
#include "sqlite_exception.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------------

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, const char* value);
static void bind_parameter(sqlite3_stmt* statement, int& paramindex, unsigned int value);
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
// add_channel
//
// Adds a new channel to the database
//
// Arguments:
//
//	instance		- Database instance
//	channelprops	- Channel properties

bool add_channel(sqlite3* instance, struct channelprops const& channelprops)
{
	// frequency | modulation | name | autogain | manualgain | freqcorrection | logourl
	return execute_non_query(instance, "replace into channel values(?1, ?2, ?3, ?4, ?5, ?6, ?7)",
		channelprops.frequency, static_cast<int>(channelprops.modulation), channelprops.name.c_str(),
		(channelprops.autogain) ? 1 : 0, channelprops.manualgain, channelprops.freqcorrection, channelprops.logourl.c_str()) > 0;
}

//---------------------------------------------------------------------------
// add_channel
//
// Adds a new channel to the database
//
// Arguments:
//
//	instance		- Database instance
//	channelprops	- Channel properties
//	subchannelprops	- Subchannel properties

bool add_channel(sqlite3* instance, struct channelprops const& channelprops, std::vector<struct subchannelprops> const& subchannelprops)
{
	bool						result = false;					// Result code to return to caller

	try {

		// Clone the subchannel table schema into a temporary table
		execute_non_query(instance, "drop table if exists subchannel_temp");
		execute_non_query(instance, "create temp table subchannel_temp as select * from subchannel limit 0");

		// frequency | number | modulation | name | logourl
		for(auto const& it : subchannelprops) {

			execute_non_query(instance, "insert into subchannel_temp values(?1, ?2, ?3, ?4, ?5)", channelprops.frequency,
				it.number, static_cast<int>(channelprops.modulation), it.name.c_str(), channelprops.logourl.c_str());
		}

		// This requires a multi-step operation; start a transaction
		execute_non_query(instance, "begin immediate transaction");

		try {

			// frequency | modulation | name | autogain | manualgain | freqcorrection | logourl
			result = execute_non_query(instance, "replace into channel values(?1, ?2, ?3, ?4, ?5, ?6, ?7)",
				channelprops.frequency, static_cast<int>(channelprops.modulation), channelprops.name.c_str(),
				(channelprops.autogain) ? 1 : 0, channelprops.manualgain, channelprops.freqcorrection, channelprops.logourl.c_str()) > 0;

			if(result) {

				// Remove subchannels that no longer exist for this channel
				execute_non_query(instance, "delete from subchannel where frequency = ?1 and modulation = ?2 "
					"and number not in (select number from subchannel_temp)", channelprops.frequency, static_cast<int>(channelprops.modulation));

				// Use an upsert operation to insert/update the remaining subchannels, only replace the name if necessary
				execute_non_query(instance, "insert into subchannel select * from subchannel_temp where true "
					"on conflict(frequency, modulation, number) do update set name=excluded.name");
			}

			// Commit the database transaction
			execute_non_query(instance, "commit transaction");
		}

		// Rollback the transaction on any exception
		catch(...) { try_execute_non_query(instance, "rollback transaction"); throw; }

		// Drop the temporary table
		execute_non_query(instance, "drop table subchannel_temp");
	}

	// Drop the temporary table on any exception
	catch(...) { execute_non_query(instance, "drop table subchannel_temp"); throw; }

	return result;
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

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, int value)
{
	int result = sqlite3_bind_int(statement, paramindex++, value);
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

static void bind_parameter(sqlite3_stmt* statement, int& paramindex, unsigned int value)
{
	int result = sqlite3_bind_int64(statement, paramindex++, static_cast<int64_t>(value));
	if(result != SQLITE_OK) throw sqlite_exception(result);
}

//---------------------------------------------------------------------------
// channel_exists
//
// Determines if a channel exists in the database
//
// Arguments:
//
//	instance		- Database instance
//	channelprops	- Channel properties

bool channel_exists(sqlite3* instance, struct channelprops const& channelprops)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	return execute_scalar_int(instance, "select exists(select * from channel where frequency = ?1 and modulation = ?2)",
		channelprops.frequency, static_cast<int>(channelprops.modulation)) == 1;
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
//	frequency	- Frequency of the channel to be deleted
//	modulation	- Modulation of the channel to be deleted

void delete_channel(sqlite3* instance, uint32_t frequency, enum modulation modulation)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	// This requires a multi-step operation; start a transaction
	execute_non_query(instance, "begin immediate transaction");

	try {

		// Remove subchannels prior to removing the channel
		execute_non_query(instance, "delete from subchannel where frequency = ?1 and modulation = ?2",
			frequency, static_cast<int>(modulation));

		// Remove the channel
		execute_non_query(instance, "delete from channel where frequency = ?1 and modulation = ?2",
			frequency, static_cast<int>(modulation));

		// Commit the database transaction
		execute_non_query(instance, "commit transaction");
	}

	// Rollback the transaction on any exception
	catch(...) { try_execute_non_query(instance, "rollback transaction"); throw; }
}

//---------------------------------------------------------------------------
// delete_subchannel
//
// Deletes a subchannel from the database
//
// Arguments:
//
//	instance	- Database instance
//	frequency	- Frequency of the channel to be deleted
//	modulation	- Modulation of the channel to be deleted
//	number		- Subchannel number to be deleted

void delete_subchannel(sqlite3* instance, uint32_t frequency, enum modulation modulation, uint32_t number)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	// This requires a multi-step operation; start a transaction
	execute_non_query(instance, "begin immediate transaction");

	try {

		// Remove the specified subchannel
		execute_non_query(instance, "delete from subchannel where frequency = ?1 and number = ?2 and modulation = ?3",
			frequency, number, static_cast<int>(modulation));

		// If there are no more subchannels for this channel, delete the parent channel as well
		if(execute_scalar_int(instance, "select count(number) from subchannel where frequency = ?1 and modulation = ?2",
			frequency, static_cast<int>(modulation)) == 0) {

			execute_non_query(instance, "delete from channel where frequency = ?1 and modulation = ?2",
				frequency, static_cast<int>(modulation));
		}

		// Commit the database transaction
		execute_non_query(instance, "commit transaction");
	}

	// Rollback the transaction on any exception
	catch(...) { try_execute_non_query(instance, "rollback transaction"); throw; }
}

//---------------------------------------------------------------------------
// enumerate_dabradio_channels
//
// Enumerates DAB channels
//
// Arguments:
//
//	instance	- Database instance
//	callback	- Callback function

void enumerate_dabradio_channels(sqlite3* instance, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	//
	// DAB Band III channel numbers are arbitrarily assigned in the range of 301 (5A) through 338 (13F) based
	// on the content of the (static) namedchannel table
	//

	// frequency | channelnumber | subchannelnumber | name | logourl
	auto sql = "select channel.frequency as frequency, namedchannel.number as channelnumber, ifnull(subchannel.number, 0) as subchannelnumber, "
		"ifnull(subchannel.name, channel.name) as name, ifnull(subchannel.logourl, channel.logourl) as logourl "
		"from channel inner join namedchannel on channel.frequency = namedchannel.frequency and channel.modulation = namedchannel.modulation "
		"left outer join subchannel on channel.frequency = subchannel.frequency and channel.modulation = subchannel.modulation "
		"where channel.modulation = 2 order by channelnumber, subchannelnumber asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			channelid channelid(sqlite3_column_int(statement, 0), sqlite3_column_int(statement, 2), modulation::dab);

			item.id = channelid.id();
			item.channel = sqlite3_column_int(statement, 1);
			item.subchannel = sqlite3_column_int(statement, 2);
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.logourl = reinterpret_cast<char const*>(sqlite3_column_text(statement, 4));

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
//	instance		- Database instance
//	prependnumber	- Flag to prepend the channel number to the name
//	callback		- Callback function

void enumerate_fmradio_channels(sqlite3* instance, bool prependnumber, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	//
	// FM Radio channel/subchannel numbers are assigned in the range of 87.5 through 108.0 based on the 
	// channel frequency in Megahertz
	//

	// frequency | channelnumber | subchannelnumber | name | logourl
	auto sql = "select channel.frequency as frequency, channel.frequency / 1000000 as channelnumber, "
		"(channel.frequency % 1000000) / 100000 as subchannelnumber, "
		"case ?1 when 0 then channel.name else cast(channel.frequency / 1000000 as text) || '.' || cast((channel.frequency % 1000000) / 100000 as text) || ' ' || channel.name end as name, "
		"channel.logourl as logourl from channel where channel.modulation = 0 order by channelnumber, subchannelnumber asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int(statement, 1, (prependnumber) ? 1 : 0);
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			channelid channelid(sqlite3_column_int(statement, 0), modulation::fm);

			item.id = channelid.id();
			item.channel = sqlite3_column_int(statement, 1);
			item.subchannel = sqlite3_column_int(statement, 2);
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.logourl = reinterpret_cast<char const*>(sqlite3_column_text(statement, 4));

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// enumerate_hdradio_channels
//
// Enumerates HD Radio channels
//
// Arguments:
//
//	instance		- Database instance
//	prependnumber	- Flag to prepend the channel number to the name
//	callback		- Callback function

void enumerate_hdradio_channels(sqlite3* instance, bool prependnumber, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	//
	// HD Radio channel numbers are assigned in the range of 200-300 based on
	// https://en.wikipedia.org/wiki/List_of_channel_numbers_assigned_to_FM_frequencies_in_North_America
	// 

	// frequency | channelnumber | subchannelnumber | name | logourl
	auto sql = "select channel.frequency as frequency, (((channel.frequency / 100000) - 879) / 2) + 200 as channelnumber, "
		"ifnull(subchannel.number, 0) as subchannelnumber, "
		"case ?1 when 0 then '' else cast(channel.frequency / 1000000 as text) || '.' || cast((channel.frequency % 1000000) / 100000 as text) || ' ' end || "
		"  channel.name || iif(subchannel.name is null, '', ' ' || subchannel.name) as name, "
		"ifnull(subchannel.logourl, channel.logourl) as logourl "
		"from channel left outer join subchannel on channel.frequency = subchannel.frequency and channel.modulation = subchannel.modulation "
		"where channel.modulation = 1 order by channelnumber, subchannelnumber asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int(statement, 1, (prependnumber) ? 1 : 0);
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			channelid channelid(sqlite3_column_int(statement, 0), sqlite3_column_int(statement, 2), modulation::hd);

			item.id = channelid.id();
			item.channel = sqlite3_column_int(statement, 1);
			item.subchannel = sqlite3_column_int(statement, 2);
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.logourl = reinterpret_cast<char const*>(sqlite3_column_text(statement, 4));

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// enumerate_namedchannels
//
// Enumerates the named channels for a specific modulation
//
// Arguments:
//
//	instance	- Database instance
//	modulation	- Modulation type
//	callback	- Callback function

void enumerate_namedchannels(sqlite3* instance, enum modulation modulation, enumerate_namedchannels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	// frequency | name
	auto sql = "select frequency as frequency, name as name from namedchannel where modulation = ?1 order by number asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int(statement, 1, static_cast<int>(modulation));
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct namedchannel item = {};

			item.frequency = static_cast<uint32_t>(sqlite3_column_int64(statement, 0));
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 1));

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// enumerate_rawfiles
//
// Enumerates available raw files registered in the database
//
// Arguments:
//
//	instance	- Database instance
//	callback	- Callback function

void enumerate_rawfiles(sqlite3* instance, enumerate_rawfiles_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	// path | name | samplerate
	auto sql = "select path as path, name || ' (' || cast(samplerate as text) || ')' as name, samplerate as samplerate from rawfile order by name, samplerate asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct rawfile item = {};

			item.path = reinterpret_cast<char const*>(sqlite3_column_text(statement, 0));
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 1));
			item.samplerate = static_cast<uint32_t>(sqlite3_column_int64(statement, 2));

			callback(item);						// Invoke caller-supplied callback
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }
}

//---------------------------------------------------------------------------
// enumerate_wxradio_channels
//
// Enumerates Weather Radio channels
//
// Arguments:
//
//	instance	- Database instance
//	callback	- Callback function

void enumerate_wxradio_channels(sqlite3* instance, enumerate_channels_callback const& callback)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	//
	// Weather Radio channel numbers are arbitrarily assigned in the range of 401 (WX1) through 407 (WX7) based
	// on the content of the (static) namedchannel table
	//

	// frequency | channelnumber | subchannelnumber | name | logourl
	auto sql = "select channel.frequency as frequency, namedchannel.number as channelnumber, 0 as subchannelnumber, "
		"channel.name as name, channel.logourl as logourl from channel inner join namedchannel on channel.frequency = namedchannel.frequency "
		"and channel.modulation = namedchannel.modulation where channel.modulation = 3 order by channelnumber, subchannelnumber asc";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct channel item = {};
			channelid channelid(sqlite3_column_int(statement, 0), modulation::wx);

			item.id = channelid.id();
			item.channel = sqlite3_column_int(statement, 1);
			item.subchannel = sqlite3_column_int(statement, 2);
			item.name = reinterpret_cast<char const*>(sqlite3_column_text(statement, 3));
			item.logourl = reinterpret_cast<char const*>(sqlite3_column_text(statement, 4));

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

	return execute_scalar_string(instance, "select json_group_array(json_object("
		"'frequency', frequency, 'modulation', case modulation when 0 then 'FM' when 1 then 'HD' when 2 then 'DAB' when 3 then 'WX' else 'FM' end, "
		"'name', name, 'autogain', autogain, 'manualgain', manualgain, 'freqcorrection', freqcorrection, 'logourl', logourl)) "
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

	// Use the same LEFT OUTER JOIN logic against subchannel to return an accurate count when a digital
	// channel does not have any defined subchannels and will return a .0 channel instance
	return execute_scalar_int(instance, "select count(*) from channel left outer join subchannel "
		"on channel.frequency = subchannel.frequency and channel.modulation = subchannel.modulation");
}

//---------------------------------------------------------------------------
// get_channel_properties
//
// Gets the tuning properties of a channel from the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	frequency		- Channel frequency
//	modulation		- Channel modulation
//	id				- Channel unique identifier
//	channelprops	- Structure to receive the channel properties

bool get_channel_properties(sqlite3* instance, uint32_t frequency, enum modulation modulation, struct channelprops& channelprops)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function
	bool						found = false;		// Flag if channel was found in database

	if(instance == nullptr) throw std::invalid_argument("instance");

	// name | autogain | manualgain | freqcorrection | logourl
	auto sql = "select name, autogain, manualgain, freqcorrection, logourl from channel where frequency = ?1 and modulation = ?2";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int64(statement, 1, static_cast<int64_t>(frequency));
		if(result == SQLITE_OK) sqlite3_bind_int(statement, 2, static_cast<int>(modulation));
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query; there should be one and only one row returned
		if(sqlite3_step(statement) == SQLITE_ROW) {

			channelprops.frequency = frequency;
			channelprops.modulation = modulation;

			unsigned char const* name = sqlite3_column_text(statement, 0);
			channelprops.name.assign((name == nullptr) ? "" : reinterpret_cast<char const*>(name));

			channelprops.autogain = (sqlite3_column_int(statement, 1) != 0);
			channelprops.manualgain = sqlite3_column_int(statement, 2);
			channelprops.freqcorrection = sqlite3_column_int(statement, 3);

			unsigned char const* logourl = sqlite3_column_text(statement, 4);
			channelprops.logourl.assign((logourl == nullptr) ? "" : reinterpret_cast<char const*>(logourl));

			found = true;						// Channel was found in the database
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }

	return found;
}

//---------------------------------------------------------------------------
// get_channel_properties
//
// Gets the tuning properties of a channel from the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	frequency		- Channel frequency
//	modulation		- Channel modulation
//	id				- Channel unique identifier
//	channelprops	- Structure to receive the channel properties
//	subchannelprops	- vector<> containing subchannel properties

bool get_channel_properties(sqlite3* instance, uint32_t frequency, enum modulation modulation, struct channelprops& channelprops,
	std::vector<struct subchannelprops>& subchannelprops)
{
	sqlite3_stmt*				statement;			// SQL statement to execute
	int							result;				// Result from SQLite function

	if(instance == nullptr) throw std::invalid_argument("instance");

	// Get the parent channel properties before enumerating the subchannels
	if(!get_channel_properties(instance, frequency, modulation, channelprops)) return false;

	subchannelprops.clear();						// Reset the vector<>

	// number | name | logourl
	auto sql = "select number, name, logourl from subchannel where frequency = ?1 and modulation = ?2 order by number";

	result = sqlite3_prepare_v2(instance, sql, -1, &statement, nullptr);
	if(result != SQLITE_OK) throw sqlite_exception(result, sqlite3_errmsg(instance));

	try {

		// Bind the query parameters
		result = sqlite3_bind_int64(statement, 1, static_cast<int64_t>(frequency));
		if(result == SQLITE_OK) sqlite3_bind_int(statement, 2, static_cast<int>(modulation));
		if(result != SQLITE_OK) throw sqlite_exception(result);

		// Execute the query and iterate over all returned rows
		while(sqlite3_step(statement) == SQLITE_ROW) {

			struct subchannelprops subchannel = {};

			subchannel.number = sqlite3_column_int(statement, 0);

			unsigned char const* name = sqlite3_column_text(statement, 1);
			subchannel.name.assign((name == nullptr) ? "" : reinterpret_cast<char const*>(name));

			unsigned char const* logourl = sqlite3_column_text(statement, 2);
			subchannel.logourl.assign((logourl == nullptr) ? "" : reinterpret_cast<char const*>(logourl));

			subchannelprops.emplace_back(std::move(subchannel));
		}

		sqlite3_finalize(statement);			// Finalize the SQLite statement
	}

	catch(...) { sqlite3_finalize(statement); throw; }

	return true;
}

//---------------------------------------------------------------------------
// has_rawfiles
//
// Gets a flag indicating if there are raw input files available to use
//
// Arguments:
//
//	instance	- Database instance

bool has_rawfiles(sqlite3* instance)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	return execute_scalar_int(instance, "select exists(select path from rawfile)") != 0;
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

	//
	// TODO: Add a 'channel' element that can replace frequency/modulation for named channels.
	// The JSON will likely need to be put into a temp table anyway to deal with subchannels
	// so this entire operation will likely need to be redone regardless
	//

	// Massage the input as much as possible, only the frequency is actually required,
	// the rest can be defaulted if not present. Also watch out for duplicates and the frequency range
	execute_non_query(instance, "replace into channel "
		"select cast(json_extract(entry.value, '$.frequency') as integer) as frequency, "
		"case upper(cast(ifnull(json_extract(entry.value, '$.modulation'), '') as text)) "
		"  when 'FM' then 0 "
		"  when 'FMRADIO' then 0 "
		"  when 'HD' then 1 "
		"  when 'HDRADIO' then 1 "
		"  when 'DAB' then 2 "
		"  when 'DAB+' then 2 "
		"  when 'WX' then 3 "
		"  when 'WEATHER' then 3 "
		"  else case "
		"    when cast(json_extract(entry.value, '$.frequency') as integer) between 174928000 and 239200000 then 2 "		// DAB
		"    when cast(json_extract(entry.value, '$.frequency') as integer) between 162400000 and 162550000 then 3 "		// WX
		"    else 0 end "																									// FM
		"  end as modulation, "
		"cast(ifnull(json_extract(entry.value, '$.name'), '') as text) as name, "
		"cast(ifnull(json_extract(entry.value, '$.autogain'), 0) as integer) as autogain, "
		"cast(ifnull(json_extract(entry.value, '$.manualgain'), 0) as integer) as manualgain, "
		"cast(ifnull(json_extract(entry.value, '$.freqcorrection'), 0) as integer) as freqcorrection, "
		"json_extract(entry.value, '$.logourl') as logourl "	// <-- this one allows nulls
		"from json_each(?1) as entry "
		"where frequency is not null and "
		"  ((frequency between 87500000 and 108000000) or "		// FM / HD
		"  (frequency between 174928000 and 239200000) or "		// DAB
		"  (frequency between 162400000 and 162550000)) "		// WX
		"  and modulation between 0 and 3 "
		"group by frequency, modulation", json);

	// Remove any FM channels that are outside the frequency range
	execute_non_query(instance, "delete from channel where modulation = 0 and "
		"frequency not between 87500000 and 108000000");

	// Remove any HD Radio channels that are outside the frequency range
	execute_non_query(instance, "delete from channel where modulation = 0 and "
		"(frequency not between 87900000 and 107900000 or (frequency / 100000) % 2 = 0)");

	// Remove any DAB channels that don't match an entry in namedchannel
	execute_non_query(instance, "delete from channel where modulation = 2 and "
		"frequency not in(select frequency from namedchannel where modulation = 2)");

	// Remove any Weather Radio channels that don't match an entry in namedchannel
	execute_non_query(instance, "delete from channel where modulation = 3 and "
		"frequency not in(select frequency from namedchannel where modulation = 3)");
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

			// SCHEMA VERSION 0 -> VERSION 1
			//
			if(dbversion == 0) {

				// table: channel
				//
				// frequency(pk) | subchannel(pk) | hidden | name | autogain | manualgain | freqcorrection | logourl
				execute_non_query(instance, "drop table if exists channel");
				execute_non_query(instance, "create table channel(frequency integer not null, subchannel integer not null, "
					"hidden integer not null, name text not null, autogain integer not null, manualgain integer not null, freqcorrection integer not null, "
					"logourl text null, primary key(frequency, subchannel))");

				execute_non_query(instance, "pragma user_version = 1");
				dbversion = 1;
			}

			// SCHEMA VERSION 1 -> VERSION 2
			//
			if(dbversion == 1) {

				// table: channel_v1
				//
				// frequency(pk) | subchannel(pk) | hidden | name | autogain | manualgain | logourl
				execute_non_query(instance, "alter table channel rename to channel_v1");

				// table: channel
				//
				// frequency(pk) | subchannel(pk) | modulation (pk) | hidden | name | autogain | manualgain | freqcorrection | logourl
				execute_non_query(instance, "create table channel(frequency integer not null, subchannel integer not null, modulation integer not null, "
					"hidden integer not null, name text not null, autogain integer not null, manualgain integer not null, freqcorrection integer not null, "
					"logourl text null, primary key(frequency, subchannel, modulation))");

				// Version 1 modulation can be gleaned from the frequency, anything between 162.400MHz and 162.550MHz is WX (3), anything else is FM (0)
				execute_non_query(instance, "insert into channel select v1.frequency, v1.subchannel, case when (v1.frequency >= 162400000 and v1.frequency <= 162550000) then 3 else 0 end, "
					"v1.hidden, v1.name, v1.autogain, v1.manualgain, 0, v1.logourl from channel_v1 as v1");

				execute_non_query(instance, "drop table channel_v1");
				execute_non_query(instance, "pragma user_version = 2");
				dbversion = 2;
			}

			// SCHEMA VERSION 2 -> VERSION 3
			//
			if(dbversion == 2) {

				// table: channel_v2
				//
				// frequency(pk) | subchannel(pk) | modulation (pk) | hidden | name | autogain | manualgain | freqcorrection | logourl
				execute_non_query(instance, "alter table channel rename to channel_v2");

				// table: channel
				//
				// frequency(pk) | modulation (pk) | name | autogain | manualgain | freqcorrection | logourl
				execute_non_query(instance, "create table channel(frequency integer not null, modulation integer not null, "
					"name text not null, autogain integer not null, manualgain integer not null, freqcorrection integer not null, "
					"logourl text null, primary key(frequency, modulation))");

				// There shouldn't be any channels with a subchannel number in the previous version of the table, but clean them
				// out during the upgrade just in case to avoid possible primary key violation on the reload
				execute_non_query(instance, "delete from channel_v2 where subchannel != 0");
				execute_non_query(instance, "insert into channel select v2.frequency, v2.modulation, v2.name, v2.autogain, v2.manualgain, "
					"v2.freqcorrection, v2.logourl from channel_v2 as v2");

				// table: subchannel
				//
				// frequency(pk) | subchannel(pk) | modulation (pk) | name
				execute_non_query(instance, "drop table if exists subchannel");
				execute_non_query(instance, "create table subchannel(frequency integer not null, number integer not null, modulation integer not null, "
					"name text not null, logourl null, primary key(frequency, number, modulation))");
				
				execute_non_query(instance, "drop table channel_v2");

				// table: rawfile
				//
				// path(pk) | name | samplerate
				execute_non_query(instance, "drop table if exists rawfile");
				execute_non_query(instance, "create table rawfile(path text not null, name text not null, samplerate integer not null, primary key(path))");

				// table: namedchannel
				//
				// frequency(pk) | modulation(pk) | name | number
				execute_non_query(instance, "drop table if exists namedchannel");
				execute_non_query(instance, "create table namedchannel(frequency integer not null, modulation integer not null, "
					"name text not null, number not null, primary key(frequency, modulation))");

				// DAB Band III
				//
				execute_non_query(instance, "insert into namedchannel values(174928000, 2, '5A',  301)");
				execute_non_query(instance, "insert into namedchannel values(176640000, 2, '5B',  302)");
				execute_non_query(instance, "insert into namedchannel values(178352000, 2, '5C',  303)");
				execute_non_query(instance, "insert into namedchannel values(180064000, 2, '5D',  304)");
				execute_non_query(instance, "insert into namedchannel values(181936000, 2, '6A',  305)");
				execute_non_query(instance, "insert into namedchannel values(183648000, 2, '6B',  306)");
				execute_non_query(instance, "insert into namedchannel values(185360000, 2, '6C',  307)");
				execute_non_query(instance, "insert into namedchannel values(187072000, 2, '6D',  308)");
				execute_non_query(instance, "insert into namedchannel values(188928000, 2, '7A',  309)");
				execute_non_query(instance, "insert into namedchannel values(190640000, 2, '7B',  310)");
				execute_non_query(instance, "insert into namedchannel values(192352000, 2, '7C',  311)");
				execute_non_query(instance, "insert into namedchannel values(194064000, 2, '7D',  312)");
				execute_non_query(instance, "insert into namedchannel values(195936000, 2, '8A',  313)");
				execute_non_query(instance, "insert into namedchannel values(197648000, 2, '8B',  314)");
				execute_non_query(instance, "insert into namedchannel values(199360000, 2, '8C',  315)");
				execute_non_query(instance, "insert into namedchannel values(201072000, 2, '8D',  316)");
				execute_non_query(instance, "insert into namedchannel values(202928000, 2, '9A',  317)");
				execute_non_query(instance, "insert into namedchannel values(204640000, 2, '9B',  318)");
				execute_non_query(instance, "insert into namedchannel values(206352000, 2, '9C',  319)");
				execute_non_query(instance, "insert into namedchannel values(208064000, 2, '9D',  320)");
				execute_non_query(instance, "insert into namedchannel values(209936000, 2, '10A', 321)");
				execute_non_query(instance, "insert into namedchannel values(211648000, 2, '10B', 322)");
				execute_non_query(instance, "insert into namedchannel values(213360000, 2, '10C', 323)");
				execute_non_query(instance, "insert into namedchannel values(215072000, 2, '10D', 324)");
				execute_non_query(instance, "insert into namedchannel values(216928000, 2, '11A', 325)");
				execute_non_query(instance, "insert into namedchannel values(218640000, 2, '11B', 326)");
				execute_non_query(instance, "insert into namedchannel values(220352000, 2, '11C', 327)");
				execute_non_query(instance, "insert into namedchannel values(222064000, 2, '11D', 328)");
				execute_non_query(instance, "insert into namedchannel values(223936000, 2, '12A', 329)");
				execute_non_query(instance, "insert into namedchannel values(225648000, 2, '12B', 330)");
				execute_non_query(instance, "insert into namedchannel values(227360000, 2, '12C', 331)");
				execute_non_query(instance, "insert into namedchannel values(229072000, 2, '12D', 332)");
				execute_non_query(instance, "insert into namedchannel values(230784000, 2, '13A', 333)");
				execute_non_query(instance, "insert into namedchannel values(232496000, 2, '13B', 334)");
				execute_non_query(instance, "insert into namedchannel values(234208000, 2, '13C', 335)");
				execute_non_query(instance, "insert into namedchannel values(235776000, 2, '13D', 336)");
				execute_non_query(instance, "insert into namedchannel values(237488000, 2, '13E', 337)");
				execute_non_query(instance, "insert into namedchannel values(239200000, 2, '13F', 338)");

				// Weather Radio
				//
				execute_non_query(instance, "insert into namedchannel values(162400000, 3, 'WX2', 402)");
				execute_non_query(instance, "insert into namedchannel values(162425000, 3, 'WX4', 404)");
				execute_non_query(instance, "insert into namedchannel values(162450000, 3, 'WX5', 405)");
				execute_non_query(instance, "insert into namedchannel values(162475000, 3, 'WX3', 403)");
				execute_non_query(instance, "insert into namedchannel values(162500000, 3, 'WX6', 406)");
				execute_non_query(instance, "insert into namedchannel values(162525000, 3, 'WX7', 407)");
				execute_non_query(instance, "insert into namedchannel values(162550000, 3, 'WX1', 401)");

				// Remove any Weather Radio channels that don't match an entry in namedchannel
				execute_non_query(instance, "delete from channel where modulation = 3 and "
					"frequency not in(select frequency from namedchannel where modulation = 3)");

				execute_non_query(instance, "pragma user_version = 3");
				dbversion = 3;
			}
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
//	frequency	- Frequency of the channel to be renamed
//	modulation	- Modulation of the channel to be renamed
//	newname		- New name to assign to the channel

void rename_channel(sqlite3* instance, uint32_t frequency, enum modulation modulation, char const* newname)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	execute_non_query(instance, "update channel set name = ?1 where frequency = ?2 and modulation = ?3",
		(newname == nullptr) ? "" : newname, frequency, static_cast<int>(modulation));
}

//---------------------------------------------------------------------------
// try_execute_non_query
//
// Executes a non-query against the database and eats any exceptions
//
// Arguments:
//
//	instance	- Database instance
//	sql			- SQL non-query to execute

bool try_execute_non_query(sqlite3* instance, char const* sql)
{
	try { execute_non_query(instance, sql); }
	catch(...) { return false; }

	return true;
}

//---------------------------------------------------------------------------
// update_channel
//
// Updates the tuning properties of a channel in the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	channelprops	- Structure containing the updated channel properties

bool update_channel(sqlite3* instance, struct channelprops const& channelprops)
{
	if(instance == nullptr) throw std::invalid_argument("instance");

	return execute_non_query(instance, "update channel set name = ?1, autogain = ?2, manualgain = ?3, freqcorrection = ?4, logourl = ?5 "
		"where frequency = ?6 and modulation = ?7", channelprops.name.c_str(), (channelprops.autogain) ? 1 : 0,
		channelprops.manualgain, channelprops.freqcorrection, channelprops.logourl.c_str(), channelprops.frequency,
		static_cast<int>(channelprops.modulation)) > 0;
}

//---------------------------------------------------------------------------
// update_channel
//
// Updates the tuning properties of a channel in the database
//
// Arguments:
//
//	instance		- SQLite database instance
//	channelprops	- Structure containing the updated channel properties
//	subchannelprops	- vector<> containing updated subchannel properties

bool update_channel(sqlite3* instance, struct channelprops const& channelprops, std::vector<struct subchannelprops> const& subchannelprops)
{
	bool						result = false;					// Result code to return to caller

	try {

		// Clone the subchannel table schema into a temporary table
		execute_non_query(instance, "drop table if exists subchannel_temp");
		execute_non_query(instance, "create temp table subchannel_temp as select * from subchannel limit 0");

		// frequency | subchannel | modulation | name | logourl
		for(auto const& it : subchannelprops) {

			execute_non_query(instance, "insert into subchannel_temp values(?1, ?2, ?3, ?4, ?5)", channelprops.frequency,
				it.number, static_cast<int>(channelprops.modulation), it.name.c_str(), channelprops.logourl.c_str());
		}

		// This requires a multi-step operation; start a transaction
		execute_non_query(instance, "begin immediate transaction");

		try {

			// Update the base channel properties
			result = execute_non_query(instance, "update channel set name = ?1, autogain = ?2, manualgain = ?3, freqcorrection = ?4, logourl = ?5 "
				"where frequency = ?6 and modulation = ?7", channelprops.name.c_str(), (channelprops.autogain) ? 1 : 0,
				channelprops.manualgain, channelprops.freqcorrection, channelprops.logourl.c_str(), channelprops.frequency,
				static_cast<int>(channelprops.modulation)) > 0;

			if(result) {

				// Remove subchannels that no longer exist for this channel
				execute_non_query(instance, "delete from subchannel where frequency = ?1 and modulation = ?2 "
					"and number not in (select number from subchannel_temp)", channelprops.frequency, static_cast<int>(channelprops.modulation));

				// Use an upsert operation to insert/update the remaining subchannels, only replace the name if necessary
				execute_non_query(instance, "insert into subchannel select * from subchannel_temp where true "
					"on conflict(frequency, modulation, number) do update set name=excluded.name");
			}

			// Commit the database transaction
			execute_non_query(instance, "commit transaction");
		}

		// Rollback the transaction on any exception
		catch(...) { try_execute_non_query(instance, "rollback transaction"); throw; }

		// Drop the temporary table
		execute_non_query(instance, "drop table subchannel_temp");
	}

	// Drop the temporary table on any exception
	catch(...) { execute_non_query(instance, "drop table subchannel_temp"); throw; }

	return result;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
