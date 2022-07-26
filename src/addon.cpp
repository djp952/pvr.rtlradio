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

#include "addon.h"

#include <assert.h>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <kodi/gui/dialogs/FileBrowser.h>
#include <kodi/gui/dialogs/OK.h>
#include <kodi/gui/dialogs/Select.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <utility>
#include <vector>
#include <version.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "channeladd.h"
#include "channelsettings.h"
#include "dbtypes.h"
#include "filedevice.h"
#include "dabstream.h"
#include "fmstream.h"
#include "hdstream.h"
#include "string_exception.h"
#include "sqlite_exception.h"
#include "tcpdevice.h"
#include "usbdevice.h"
#include "wxstream.h"

#pragma warning(push, 4)

// Addon Entry Points 
//
ADDONCREATOR(addon)

//---------------------------------------------------------------------------
// addon Instance Constructor
//
// Arguments:
//
//	NONE

addon::addon() : m_settings{}
{
}

//---------------------------------------------------------------------------
// addon Destructor

addon::~addon()
{
	// There is no corresponding "Destroy" method in CAddonBase, only the class
	// destructor will be invoked; to keep the implementation pieces near each
	// other, perform the tear-down in a helper function
	Destroy();
}

//---------------------------------------------------------------------------
// addon::channeladd_dab (private)
//
// Performs the channel add operation for DAB
//
// Arguments:
//
//	settings		- Current addon settings
//	channelprops	- Channel properties to be populated on success

bool addon::channeladd_dab(struct settings const& settings, struct channelprops& channelprops) const
{
	std::vector<std::string>				channelnames;			// Channel names
	std::vector<std::string>				channellabels;			// Channel labels
	std::vector<uint32_t>					channelfrequencies;		// Channel frequencies
	std::vector<struct subchannelprops>		subchannelprops;		// Existing subchannels


	// Pull a database handle out of the connection pool
	connectionpool::handle dbhandle(m_connpool);

	// Enumerate the named channels available for the specified modulation (DAB)
	enumerate_namedchannels(dbhandle, modulation::dab, [&](struct namedchannel const& item) -> void {

		if((item.frequency > 0) && (item.name != nullptr)) {

			// Append the frequency of the channel in megahertz (xxx.xxx format) to the channel label
			char label[256]{};
			unsigned int mhz = item.frequency / 1000000;
			unsigned int khz = (item.frequency % 1000000) / 1000;
			snprintf(label, std::extent<decltype(label)>::value, "%s (%u.%u MHz)", item.name, mhz, khz);

			channelnames.emplace_back(item.name);
			channellabels.emplace_back(label);
			channelfrequencies.emplace_back(item.frequency);
		}
	});

	assert((channelnames.size() == channellabels.size()) && (channelnames.size() == channelfrequencies.size()));
	if(channelnames.size() == 0) throw string_exception("No DAB ensembles were enumerated from the database");

	// The user has to select what DAB ensemble will be added from the hard-coded options in the database
	int selected = kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30418), channellabels);
	if(selected < 0) return false;

	// Initialize enough properties for the settings dialog to work
	channelprops.frequency = channelfrequencies[selected];
	channelprops.modulation = modulation::dab;
	channelprops.name = kodi::addon::GetLocalizedString(30322).append(" ").append(channelnames[selected]);

	// If the channel already exists in the database, get the previously set properties and subchannels
	bool exists = channel_exists(dbhandle, channelprops);
	if(exists) get_channel_properties(dbhandle, channelprops.frequency, channelprops.modulation, channelprops, subchannelprops);

	// Set up the tuner device properties
	struct tunerprops tunerprops = {};
	tunerprops.freqcorrection = settings.device_frequency_correction;

	// Create and initialize a channel settings dialog instance to allow the user to fine-tune the channel
	std::unique_ptr<channelsettings> settingsdialog = channelsettings::create(create_device(settings), tunerprops, channelprops, true);
	settingsdialog->DoModal();

	if(settingsdialog->get_dialog_result()) {

		std::vector<struct subchannelprops>	subchannels;		// Subchannel information

		// Retrieve the updated channel and subchannel properties from the dialog box
		settingsdialog->get_channel_properties(channelprops);
		settingsdialog->get_subchannel_properties(subchannels);

		// Prompt the user to select what subchannels they want to add for this channel, assuming all of them
		if(subchannels.size() > 0) {
			
			std::vector<kodi::gui::dialogs::SSelectionEntry> entries;
			for(auto const& subchannel : subchannels) {

				// GCC 4.9 doesn't allow a push_back() on SSelectionEntry with a braced initializer list, do this the verbose way
				kodi::gui::dialogs::SSelectionEntry entry = {};
				entry.id = std::to_string(subchannel.number);
				entry.name = std::string(entry.id).append(" ").append(subchannel.name);

				// Pre-select the subchannel if there were no prior subchannels defined or if it matches a previously defined subchannel
				entry.selected = ((subchannelprops.size() == 0) || 
					(std::find_if(subchannelprops.begin(), subchannelprops.end(), [&](auto const& val) -> bool { return val.number == subchannel.number; }) != subchannelprops.end()));

				entries.emplace_back(std::move(entry));
			}

			// TODO: use the existing dialog box for now; this needs a custom replacement. It seems to always toggle
			// the first item to selected == false and has no means to determine a 'cancel'
			if(!kodi::gui::dialogs::Select::ShowMultiSelect(kodi::addon::GetLocalizedString(30320), entries)) return false;
			entries[0].selected = true;

			// Remove any subchannels that were de-selected by the user from the vector<> of subchannels
			for(auto const& entry : entries) {

				if(entry.selected == false) {

					auto found = std::find_if(subchannels.begin(), subchannels.end(), [&](auto const& val) -> bool { return std::to_string(val.number) == entry.id; });
					if(found != subchannels.end()) subchannels.erase(found);
				}
			}
		}

		// Add or update the channel/subchannels in the database
		if(!exists) add_channel(dbhandle, channelprops, subchannels);
		else update_channel(dbhandle, channelprops, subchannels);

		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
// addon::channeladd_fm (private)
//
// Performs the channel add operation for FM Radio
//
// Arguments:
//
//	settings		- Current addon settings
//	channelprops	- Channel properties to be populated on success

bool addon::channeladd_fm(struct settings const& settings, struct channelprops& channelprops) const
{
	// Create and initialize the frequency input dialog box
	std::unique_ptr<channeladd> adddialog = channeladd::create(modulation::fm);
	adddialog->DoModal();

	// If the dialog was successful add the channel to the database
	if(adddialog->get_dialog_result()) {

		// Retrieve the new channel properties from the dialog box
		adddialog->get_channel_properties(channelprops);
		assert(channelprops.modulation == modulation::fm);

		// Pull a database handle out of the connection pool
		connectionpool::handle dbhandle(m_connpool);

		// If the channel already exists in the database, get the previously set properties
		bool exists = channel_exists(dbhandle, channelprops);
		if(exists) get_channel_properties(dbhandle, channelprops.frequency, channelprops.modulation, channelprops);

		// Set up the tuner device properties
		struct tunerprops tunerprops = {};
		tunerprops.freqcorrection = settings.device_frequency_correction;

		// Create and initialize the dialog box against a new signal meter instance
		std::unique_ptr<channelsettings> settingsdialog = channelsettings::create(create_device(settings), tunerprops, channelprops, true);
		settingsdialog->DoModal();

		if(settingsdialog->get_dialog_result()) {

			// Retrieve the updated channel properties from the dialog box
			settingsdialog->get_channel_properties(channelprops);
			
			// Add or update the channel in the database
			if(!exists) add_channel(dbhandle, channelprops);
			else update_channel(dbhandle, channelprops);
		}

		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
// addon::channeladd_hd (private)
//
// Performs the channel add operation for HD Radio
//
// Arguments:
//
//	settings		- Current addon settings
//	channelprops	- Channel properties to be populated on success

bool addon::channeladd_hd(struct settings const& settings, struct channelprops& channelprops) const
{
	std::vector<struct subchannelprops>		subchannelprops;		// Existing subchannels

	// Create and initialize the frequency input dialog box
	std::unique_ptr<channeladd> adddialog = channeladd::create(modulation::hd);
	adddialog->DoModal();

	// If the dialog was successful add the channel to the database
	if(adddialog->get_dialog_result()) {

		// Retrieve the new channel properties from the dialog box
		adddialog->get_channel_properties(channelprops);
		assert(channelprops.modulation == modulation::hd);

		// For HD Radio, change "New channel" to "New multiplex"
		channelprops.name = kodi::addon::GetLocalizedString(30321);

		// Pull a database handle out of the connection pool
		connectionpool::handle dbhandle(m_connpool);

		// If the channel already exists in the database, get the previously set properties
		bool exists = channel_exists(dbhandle, channelprops);
		if(exists) get_channel_properties(dbhandle, channelprops.frequency, channelprops.modulation, channelprops, subchannelprops);

		// Set up the tuner device properties
		struct tunerprops tunerprops = {};
		tunerprops.freqcorrection = settings.device_frequency_correction;

		// Create and initialize a channel settings dialog instance to allow the user to fine-tune the channel
		std::unique_ptr<channelsettings> settingsdialog = channelsettings::create(create_device(settings), tunerprops, channelprops, true);
		settingsdialog->DoModal();

		if(settingsdialog->get_dialog_result()) {

			std::vector<struct subchannelprops>	subchannels;		// Subchannel information

			// Retrieve the updated channel and subchannel properties from the dialog box
			settingsdialog->get_channel_properties(channelprops);
			settingsdialog->get_subchannel_properties(subchannels);

			// Prompt the user to select what subchannels they want to add for this channel, assuming all of them.
			// For HD Radio, if there is only one audio stream subchannel, bypass the selection process
			if(subchannels.size() > 1) {

				std::vector<kodi::gui::dialogs::SSelectionEntry> entries;
				for(auto const& subchannel : subchannels) {

					// GCC 4.9 doesn't allow a push_back() on SSelectionEntry with a braced initializer list, do this the verbose way
					kodi::gui::dialogs::SSelectionEntry entry = {};
					entry.id = std::to_string(subchannel.number);
					entry.name = subchannel.name;

					// Pre-select the subchannel if there were no prior subchannels defined or if it matches a previously defined subchannel
					entry.selected = ((subchannelprops.size() == 0) ||
						(std::find_if(subchannelprops.begin(), subchannelprops.end(), [&](auto const& val) -> bool { return val.number == subchannel.number; }) != subchannelprops.end()));

					entries.emplace_back(std::move(entry));
				}

				// TODO: use the existing dialog box for now; this needs a custom replacement. It seems to always toggle
				// the first item to selected == false and has no means to determine a 'cancel'
				if(!kodi::gui::dialogs::Select::ShowMultiSelect(kodi::addon::GetLocalizedString(30319), entries)) return false;
				entries[0].selected = true;

				// Remove any subchannels that were de-selected by the user from the vector<> of subchannels
				for(auto const& entry : entries) {

					if(entry.selected == false) {

						auto found = std::find_if(subchannels.begin(), subchannels.end(), [&](auto const& val) -> bool { return std::to_string(val.number) == entry.id; });
						if(found != subchannels.end()) subchannels.erase(found);
					}
				}
			}

			// Add or update the channel/subchannels in the database
			if(!exists) add_channel(dbhandle, channelprops, subchannels);
			else update_channel(dbhandle, channelprops, subchannels);

			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------
// addon::channeladd_wx (private)
//
// Performs the channel add operation for Weather Radio
//
// Arguments:
//
//	settings		- Current addon settings
//	channelprops	- Channel properties to be populated on success

bool addon::channeladd_wx(struct settings const& settings, struct channelprops& channelprops) const
{
	std::vector<std::string>	channelnames;			// Channel names
	std::vector<std::string>	channellabels;			// Channel labels
	std::vector<uint32_t>		channelfrequencies;		// Channel frequencies

	// Pull a database handle out of the connection pool
	connectionpool::handle dbhandle(m_connpool);

	// Enumerate the named channels available for the specified modulation (WX)
	enumerate_namedchannels(dbhandle, modulation::wx, [&](struct namedchannel const& item) -> void {

		if((item.frequency > 0) && (item.name != nullptr)) {

			// Append the frequency of the channel in megahertz (xxx.xxx format) to the channel name
			char label[256]{};
			unsigned int mhz = item.frequency / 1000000;
			unsigned int khz = (item.frequency % 1000000) / 1000;
			snprintf(label, std::extent<decltype(label)>::value, "%s (%u.%u MHz)", item.name, mhz, khz);

			channelnames.emplace_back(item.name);
			channellabels.emplace_back(label);
			channelfrequencies.emplace_back(item.frequency);
		}
	});

	assert((channelnames.size() == channellabels.size()) && (channelnames.size() == channelfrequencies.size()));
	if(channelnames.size() == 0) throw string_exception("No Weather Radio channels were enumerated from the database");

	// The user has to select what Weather Radio channel will be added from the hard-coded options in the database
	int selected = kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30428), channellabels);
	if(selected < 0) return false;

	// Initialize enough properties for the settings dialog to work
	channelprops.frequency = channelfrequencies[selected];
	channelprops.modulation = modulation::wx;
	channelprops.name = channelnames[selected];

	// If the channel already exists in the database, get the previously set properties
	bool exists = channel_exists(dbhandle, channelprops);
	if(exists) get_channel_properties(dbhandle, channelprops.frequency, channelprops.modulation, channelprops);

	// Set up the tuner device properties
	struct tunerprops tunerprops = {};
	tunerprops.freqcorrection = settings.device_frequency_correction;

	// Create and initialize a channel settings dialog instance to allow the user to fine-tune the channel
	std::unique_ptr<channelsettings> settingsdialog = channelsettings::create(create_device(settings), tunerprops, channelprops, true);
	settingsdialog->DoModal();

	if(settingsdialog->get_dialog_result() == true) {

		// Retrieve the updated channel properties from the dialog box
		settingsdialog->get_channel_properties(channelprops);
				
		// Add or update the channel in the database
		if(!exists) add_channel(dbhandle, channelprops);
		else update_channel(dbhandle, channelprops);

		return true;
	}

	return false;
}

//---------------------------------------------------------------------------
// addon::copy_settings (private, inline)
//
// Atomically creates a copy of the member addon_settings structure
//
// Arguments:
//
//	NONE

inline struct settings addon::copy_settings(void) const
{
	std::unique_lock<std::recursive_mutex> settings_lock(m_settings_lock);
	return m_settings;
}

//---------------------------------------------------------------------------
// addon::create_device (private)
//
// Creates the RTL-SDR device instance
//
// Arguments:
//
//	settings		- Current addon settings structure

std::unique_ptr<rtldevice> addon::create_device(struct settings const& settings) const
{
	// Pull a database handle out of the connection pool
	connectionpool::handle dbhandle(m_connpool);

	// File device
	if(has_rawfiles(dbhandle)) {

		std::vector<std::string>						names;		// File names
		std::vector<std::pair<std::string, uint32_t>>	files;		// File paths and sample rates

		// Enumerate the available raw files registered in the database
		enumerate_rawfiles(dbhandle, [&](struct rawfile const& item) -> void {

			if((item.path != nullptr) && (item.name != nullptr) && (item.samplerate > 0)) {

				names.emplace_back(std::string(item.name));
				files.emplace_back(std::string(item.path), item.samplerate);
			}
		});

		// Prompt to select from among the available files, or cancel the operation
		int selected = kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30412), names, -1, 0);
		if(selected >= 0) {

			auto const& item = files[selected];
			return filedevice::create(item.first.c_str(), item.second);
		}
	}

	// USB device
	if(settings.device_connection == device_connection::usb)
		return usbdevice::create(settings.device_connection_usb_index);

	// Network device
	if(settings.device_connection == device_connection::rtltcp)
		return tcpdevice::create(settings.device_connection_tcp_host.c_str(), static_cast<uint16_t>(settings.device_connection_tcp_port));

	// Unknown device type
	throw string_exception("invalid device_connection type specified");
}

//---------------------------------------------------------------------------
// addon::downsample_quality_to_string (private, static)
//
// Converts a downsample_quality enumeration value into a string
//
// Arguments:
//
//	quality			- Downsample quality to convert into a string

std::string addon::downsample_quality_to_string(enum downsample_quality quality)
{
	switch(quality) {

		case downsample_quality::fast: return kodi::addon::GetLocalizedString(30216);
		case downsample_quality::standard: return kodi::addon::GetLocalizedString(30217);
		case downsample_quality::maximum: return kodi::addon::GetLocalizedString(30218);
	}

	return "Unknown";
}

//---------------------------------------------------------------------------
// addon::device_connection_to_string (private, static)
//
// Converts a device_connection enumeration value into a string
//
// Arguments:
//
//	connection		- Connection type to convert into a string

std::string addon::device_connection_to_string(enum device_connection connection)
{
	switch(connection) {

		case device_connection::usb: return kodi::addon::GetLocalizedString(30200);
		case device_connection::rtltcp: return kodi::addon::GetLocalizedString(30201);
	}

	return "Unknown";
}

//---------------------------------------------------------------------------
// addon::is_region_northamerica (private)
//
// Determines if the currently set region is North America
//
// Arguments:
//
//	settings		- Reference to the current addon settings

bool addon::is_region_northamerica(struct settings const& settings) const
{
	// If a region code has not been set, try to determine if the RTL-SDR device is
	// being operated in North America based on the ISO language code
	if(settings.region_regioncode == regioncode::notset) {

		std::string language = kodi::GetLanguage(LANG_FMT_ISO_639_1, true);

		// Only North American countries use the RBDS standard
		if(language.find("-us") != std::string::npos) return true;
		else if(language.find("-ca") != std::string::npos) return true;
		else if(language.find("-mx") != std::string::npos) return true;
		else return false;
	}

	return (settings.region_regioncode == regioncode::northamerica);
}

//---------------------------------------------------------------------------
// addon::handle_generalexception (private)
//
// Handler for thrown generic exceptions
//
// Arguments:
//
//	function		- Name of the function where the exception was thrown

void addon::handle_generalexception(char const* function)
{
	log_error(function, " failed due to an exception");
}

//---------------------------------------------------------------------------
// addon::handle_generalexception (private)
//
// Handler for thrown generic exceptions
//
// Arguments:
//
//	function		- Name of the function where the exception was thrown
//	result			- Result code to return

template<typename _result>
_result addon::handle_generalexception(char const* function, _result result)
{
	handle_generalexception(function);
	return result;
}

//---------------------------------------------------------------------------
// addon::handle_stdexception (private)
//
// Handler for thrown std::exceptions
//
// Arguments:
//
//	function		- Name of the function where the exception was thrown
//	exception		- std::exception that was thrown

void addon::handle_stdexception(char const* function, std::exception const& ex)
{
	log_error(function, " failed due to an exception: ", ex.what());
}

//---------------------------------------------------------------------------
// addon::handle_stdexception (private)
//
// Handler for thrown std::exceptions
//
// Arguments:
//
//	function		- Name of the function where the exception was thrown
//	exception		- std::exception that was thrown
//	result			- Result code to return

template<typename _result>
_result addon::handle_stdexception(char const* function, std::exception const& ex, _result result)
{
	handle_stdexception(function, ex);
	return result;
}

//---------------------------------------------------------------------------
// addon::log_debug (private)
//
// Variadic method of writing a LOG_DEBUG entry into the Kodi application log
//
// Arguments:
//
//	args	- Variadic argument list

template<typename... _args>
void addon::log_debug(_args&&... args) const
{
	log_message(ADDON_LOG::ADDON_LOG_DEBUG, std::forward<_args>(args)...);
}

//---------------------------------------------------------------------------
// addon::log_error (private)
//
// Variadic method of writing a LOG_ERROR entry into the Kodi application log
//
// Arguments:
//
//	args	- Variadic argument list

template<typename... _args>
void addon::log_error(_args&&... args) const
{
	log_message(ADDON_LOG::ADDON_LOG_ERROR, std::forward<_args>(args)...);
}

//---------------------------------------------------------------------------
// addon::log_info (private)
//
// Variadic method of writing a LOG_INFO entry into the Kodi application log
//
// Arguments:
//
//	args	- Variadic argument list

template<typename... _args>
void addon::log_info(_args&&... args) const
{
	log_message(ADDON_LOG::ADDON_LOG_INFO, std::forward<_args>(args)...);
}

//---------------------------------------------------------------------------
// addon::log_message (private)
//
// Variadic method of writing a log entry into the Kodi application log
//
// Arguments:
//
//	args	- Variadic argument list

template<typename... _args>
void addon::log_message(ADDON_LOG level, _args&&... args) const
{
	std::ostringstream stream;
	int unpack[] = { 0, (static_cast<void>(stream << args), 0) ... };
	(void)unpack;

	kodi::Log(level, stream.str().c_str());

	// Write ADDON_LOG_ERROR level messages to an appropriate secondary log mechanism
	if(level == ADDON_LOG::ADDON_LOG_ERROR) {

#if defined(_WINDOWS) || defined(WINAPI_FAMILY)
		std::string message = "ERROR: " + stream.str() + "\r\n";
		OutputDebugStringA(message.c_str());
#elif __ANDROID__
		__android_log_print(ANDROID_LOG_ERROR, VERSION_PRODUCTNAME_ANSI, "ERROR: %s\n", stream.str().c_str());
#else
		fprintf(stderr, "ERROR: %s\r\n", stream.str().c_str());
	#endif
	}
}

//---------------------------------------------------------------------------
// addon::log_warning (private)
//
// Variadic method of writing a LOG_WARNING entry into the Kodi application log
//
// Arguments:
//
//	args	- Variadic argument list

template<typename... _args>
void addon::log_warning(_args&&... args) const
{
	log_message(ADDON_LOG::ADDON_LOG_WARNING, std::forward<_args>(args)...);
}

//---------------------------------------------------------------------------
// addon::menuhook_clearchannels (private)
//
// Menu hook to delete all channels from the database
//
// Arguments:
//
//	NONE

void addon::menuhook_clearchannels(void)
{
	log_info(__func__, ": clearing channel data");

	try {

		// Clear the channel data from the database and inform the user if successful
		clear_channels(connectionpool::handle(m_connpool));
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30402), "Channel data successfully cleared");

		TriggerChannelGroupsUpdate();					// Trigger a channel group update in Kodi
	}

	catch(std::exception& ex) {

		// Log the error, inform the user that the operation failed, and re-throw the exception with this function name
		handle_stdexception(__func__, ex);
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30402), "An error occurred clearing the channel data:", "", ex.what());
		throw string_exception(__func__, ": ", ex.what());
	}

	catch(...) { handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// addon::menuhook_exportchannels (private)
//
// Menu hook to export the channel information from the database
//
// Arguments:
//
//	NONE

void addon::menuhook_exportchannels(void)
{
	std::string						folderpath;				// Export folder path

	// Prompt the user to locate the folder where the .json file will be exported ...
	if(kodi::gui::dialogs::FileBrowser::ShowAndGetDirectory("local|network|removable", kodi::addon::GetLocalizedString(30403), folderpath, true)) {

		try {

			rapidjson::Document		document;				// Resultant JSON document

			// Generate the output file name based on the selected path
			std::string filepath(folderpath);
			filepath.append("radiochannels.json");
			log_info(__func__, ": exporting channel data to file ", filepath.c_str());

			// Export the channels from the database into a JSON string
			std::string json = export_channels(connectionpool::handle(m_connpool));

			// Parse the JSON data so that it can be pretty printed for the user
			document.Parse(json.c_str());
			if(document.HasParseError()) throw string_exception("JSON parse error during export - ",
				rapidjson::GetParseError_En(document.GetParseError()));

			// Pretty print the JSON data
			rapidjson::StringBuffer sb;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
			document.Accept(writer);

			// Attempt to create the output file in the selected directory
			kodi::vfs::CFile jsonfile;
			if(!jsonfile.OpenFileForWrite(filepath, true)) throw string_exception("unable to open file ", filepath.c_str(), " for write access");

			// Write the pretty printed JSON data into the output file
			ssize_t written = jsonfile.Write(sb.GetString(), sb.GetSize());
			jsonfile.Close();

			// If the file wasn't written properly, throw an exception
			if(written != static_cast<ssize_t>(sb.GetSize()))
				throw string_exception("short write occurred generating file ", filepath.c_str());

			// Inform the user that the operation was successful
			kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30401), "Channels successfully exported to:", "", filepath.c_str());
		}

		catch(std::exception& ex) {

			// Log the error, inform the user that the operation failed, and re-throw the exception with this function name
			handle_stdexception(__func__, ex);
			kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30401), "An error occurred exporting the channel data:", "", ex.what());
			throw string_exception(__func__, ": ", ex.what());
		}
		
		catch(...) { handle_generalexception(__func__); }
	}
}

//---------------------------------------------------------------------------
// addon::menuhook_importchannels (private)
//
// Menu hook to import channel information into the database
//
// Arguments:
//
//	NONE

void addon::menuhook_importchannels(void)
{
	std::string						filepath;				// Import file path
	std::string						json;					// Imported JSON data

	// Prompt the user to locate the .json file to be imported ...
	if(kodi::gui::dialogs::FileBrowser::ShowAndGetFile("local|network|removable", "*.json", kodi::addon::GetLocalizedString(30404), filepath)) {

		try {

			log_info(__func__, ": importing channel data from file ", filepath.c_str());

			// Ensure the file exists before trying to open it
			if(!kodi::vfs::FileExists(filepath, false)) throw string_exception("input file ", filepath.c_str(), " does not exist");

			// Attempt to open the specified input file
			kodi::vfs::CFile jsonfile;
			if(!jsonfile.OpenFile(filepath)) throw string_exception("unable to open file ", filepath.c_str(), " for read access");

			// Read in the input file in 1KiB chunks; it shouldn't be that big
			std::unique_ptr<char[]> buffer(new char[1 KiB]);
			ssize_t read = jsonfile.Read(&buffer[0], 1 KiB);
			while(read > 0) {

				json.append(&buffer[0], read);
				read = jsonfile.Read(&buffer[0], 1 KiB);
			}

			// Close the input file
			jsonfile.Close();

			// Only try to import channels from the file if something was actually in there ...
			if(json.length()) import_channels(connectionpool::handle(m_connpool), json.c_str());

			// Inform the user that the operation was successful
			kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30400), "Channels successfully imported from:", "", filepath.c_str());

			TriggerChannelGroupsUpdate();					// Trigger a channel group update in Kodi
		}

		catch(std::exception& ex) {

			// Log the error, inform the user that the operation failed, and re-throw the exception with this function name
			handle_stdexception(__func__, ex);
			kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30400), "An error occurred importing the channel data:", "", ex.what());
			throw string_exception(__func__, ": ", ex.what());
		}

		catch(...) { handle_generalexception(__func__); }
	}
}

//---------------------------------------------------------------------------
// addon::regioncode_to_string (private, static)
//
// Converts a regioncode enumeration value into a string
//
// Arguments:
//
//	code		- Region code value to be converted into a string

std::string addon::regioncode_to_string(enum regioncode code)
{
	switch(code) {

		case regioncode::notset: return kodi::addon::GetLocalizedString(30219);
		case regioncode::world: return kodi::addon::GetLocalizedString(30220);
		case regioncode::northamerica: return kodi::addon::GetLocalizedString(30221);
		case regioncode::europe: return kodi::addon::GetLocalizedString(30222);
	}

	return "Unknown";
}

//---------------------------------------------------------------------------
// addon::update_regioncode (private)
//
// Updates the addon region code
//
// Arguments:
//
//	code		- The updated region code

void addon::update_regioncode(enum regioncode code) const
{
	std::string region = regioncode_to_string(code);

	// NORTH AMERICA
	//
	if(code == regioncode::northamerica) {

		kodi::addon::SetSettingBoolean("fmradio_enable", true);
		log_info(__func__, ": setting fmradio_enable systemically changed to true for region ", region);

		kodi::addon::SetSettingBoolean("hdradio_enable", true);
		log_info(__func__, ": setting hdradio_enable systemically changed to true for region ", region);

		kodi::addon::SetSettingBoolean("dabradio_enable", false);
		log_info(__func__, ": setting dabradio_enable systemically changed to false for region ", region);

		kodi::addon::SetSettingBoolean("wxradio_enable", true);
		log_info(__func__, ": setting wxradio_enable systemically changed to true for region ", region);
	}

	// EUROPE/AUSTRALIA
	//
	else if(code == regioncode::europe) {

		kodi::addon::SetSettingBoolean("fmradio_enable", true);
		log_info(__func__, ": setting fmradio_enable systemically changed to true for region ", region);

		kodi::addon::SetSettingBoolean("hdradio_enable", false);
		log_info(__func__, ": setting hdradio_enable systemically changed to false for region ", region);

		kodi::addon::SetSettingBoolean("dabradio_enable", true);
		log_info(__func__, ": setting dabradio_enable systemically changed to true for region ", region);

		kodi::addon::SetSettingBoolean("wxradio_enable", false);
		log_info(__func__, ": setting wxradio_enable systemically changed to false for region ", region);
	}

	// WORLD
	//
	else {

		kodi::addon::SetSettingBoolean("fmradio_enable", true);
		log_info(__func__, ": setting fmradio_enable systemically changed to true for region ", region);

		kodi::addon::SetSettingBoolean("hdradio_enable", false);
		log_info(__func__, ": setting hdradio_enable systemically changed to false for region ", region);

		kodi::addon::SetSettingBoolean("dabradio_enable", false);
		log_info(__func__, ": setting dabradio_enable systemically changed to false for region ", region);

		kodi::addon::SetSettingBoolean("wxradio_enable", false);
		log_info(__func__, ": setting wxradio_enable systemically changed to false for region ", region);
	}
}

//---------------------------------------------------------------------------
// CADDONBASE IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// addon::Create (CAddonBase)
//
// Initializes the addon instance
//
// Arguments:
//
//	NONE

ADDON_STATUS addon::Create(void)
{
	try {

#ifdef _WINDOWS
		// On Windows, initialize winsock in case broadcast discovery is used; WSAStartup is
		// reference-counted so if it has already been called this won't hurt anything
		WSADATA wsaData;
		int wsaresult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if(wsaresult != 0) throw string_exception(__func__, ": WSAStartup failed with error code ", wsaresult);
#endif

		// Initialize SQLite
		int result = sqlite3_initialize();
		if(result != SQLITE_OK) throw sqlite_exception(result, "sqlite3_initialize() failed");

		// Throw a banner out to the Kodi log indicating that the add-on is being loaded
		log_info(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " loading");

		try {

			// The user data path doesn't always exist when an addon has been installed
			if(!kodi::vfs::DirectoryExists(UserPath())) {

				log_info(__func__, ": user data directory ", UserPath().c_str(), " does not exist");
				if(!kodi::vfs::CreateDirectory(UserPath())) throw string_exception(__func__, ": unable to create addon user data directory");
				log_info(__func__, ": user data directory ", UserPath().c_str(), " created");
			}

			// Load the device settings
			m_settings.device_connection = kodi::addon::GetSettingEnum("device_connection", device_connection::usb);
			m_settings.device_connection_usb_index = kodi::addon::GetSettingInt("device_connection_usb_index", 0);
			m_settings.device_connection_tcp_host = kodi::addon::GetSettingString("device_connection_tcp_host");
			m_settings.device_connection_tcp_port = kodi::addon::GetSettingInt("device_connection_tcp_port", 1234);
			m_settings.device_frequency_correction = kodi::addon::GetSettingInt("device_frequency_correction", 0);

			// Load the region settings
			m_settings.region_regioncode = kodi::addon::GetSettingEnum("region_regioncode", regioncode::notset);

			// Load the FM Radio settings
			m_settings.fmradio_enable_rds = kodi::addon::GetSettingBoolean("fmradio_enable_rds", true);
			m_settings.fmradio_prepend_channel_numbers = kodi::addon::GetSettingBoolean("fmradio_prepend_channel_numbers", false);
			m_settings.fmradio_sample_rate = kodi::addon::GetSettingInt("fmradio_sample_rate", (1600 KHz));
			m_settings.fmradio_downsample_quality = kodi::addon::GetSettingEnum("fmradio_downsample_quality", downsample_quality::standard);
			m_settings.fmradio_output_samplerate = kodi::addon::GetSettingInt("fmradio_output_samplerate", 48000);
			m_settings.fmradio_output_gain = kodi::addon::GetSettingFloat("fmradio_output_gain", -3.0f);

			// Load the HD Radio settings
			m_settings.hdradio_enable = kodi::addon::GetSettingBoolean("hdradio_enable", false);
			m_settings.hdradio_prepend_channel_numbers = kodi::addon::GetSettingBoolean("hdradio_prepend_channel_numbers", false);
			m_settings.hdradio_output_gain = kodi::addon::GetSettingFloat("hdradio_output_gain", -3.0f);

			// Load the DAB settings
			m_settings.dabradio_enable = kodi::addon::GetSettingBoolean("dabradio_enable", false);
			m_settings.dabradio_output_gain = kodi::addon::GetSettingFloat("dabradio_output_gain", -3.0f);

			// Load the Weather Radio settings
			m_settings.wxradio_enable = kodi::addon::GetSettingBoolean("wxradio_enable", false);
			m_settings.wxradio_sample_rate = kodi::addon::GetSettingInt("wxradio_sample_rate", (1600 KHz));
			m_settings.wxradio_output_samplerate = kodi::addon::GetSettingInt("wxradio_output_samplerate", 48000);
			m_settings.wxradio_output_gain = kodi::addon::GetSettingFloat("wxradio_output_gain", -3.0f);

			// Log the setting values
			log_info(__func__, ": m_settings.dabradio_enable                   = ", m_settings.dabradio_enable);
			log_info(__func__, ": m_settings.dabradio_output_gain              = ", m_settings.dabradio_output_gain);
			log_info(__func__, ": m_settings.device_connection                 = ", device_connection_to_string(m_settings.device_connection));
			log_info(__func__, ": m_settings.device_connection_tcp_host        = ", m_settings.device_connection_tcp_host);
			log_info(__func__, ": m_settings.device_connection_tcp_port        = ", m_settings.device_connection_tcp_port);
			log_info(__func__, ": m_settings.device_connection_usb_index       = ", m_settings.device_connection_usb_index);
			log_info(__func__, ": m_settings.device_frequency_correction       = ", m_settings.device_frequency_correction);
			log_info(__func__, ": m_settings.fmradio_downsample_quality        = ", downsample_quality_to_string(m_settings.fmradio_downsample_quality));
			log_info(__func__, ": m_settings.fmradio_enable_rds                = ", m_settings.fmradio_enable_rds);
			log_info(__func__, ": m_settings.fmradio_prepend_channel_numbers   = ", m_settings.fmradio_prepend_channel_numbers);
			log_info(__func__, ": m_settings.fmradio_output_gain               = ", m_settings.fmradio_output_gain);
			log_info(__func__, ": m_settings.fmradio_output_samplerate         = ", m_settings.fmradio_output_samplerate);
			log_info(__func__, ": m_settings.fmradio_sample_rate               = ", m_settings.fmradio_sample_rate);
			log_info(__func__, ": m_settings.hdradio_enable                    = ", m_settings.hdradio_enable);
			log_info(__func__, ": m_settings.hdradio_output_gain               = ", m_settings.hdradio_output_gain);
			log_info(__func__, ": m_settings.hdradio_prepend_channel_numbers   = ", m_settings.hdradio_prepend_channel_numbers);
			log_info(__func__, ": m_settings.region_regioncode                 = ", regioncode_to_string(m_settings.region_regioncode));
			log_info(__func__, ": m_settings.wxradio_enable                    = ", m_settings.wxradio_enable);
			log_info(__func__, ": m_settings.wxradio_output_gain               = ", m_settings.wxradio_output_gain);
			log_info(__func__, ": m_settings.wxradio_output_samplerate         = ", m_settings.wxradio_output_samplerate);
			log_info(__func__, ": m_settings.wxradio_sample_rate               = ", m_settings.wxradio_sample_rate);

			// Register the PVR_MENUHOOK_SETTING category menu hooks
			AddMenuHook(kodi::addon::PVRMenuhook(MENUHOOK_SETTING_IMPORTCHANNELS, 30400, PVR_MENUHOOK_SETTING));
			AddMenuHook(kodi::addon::PVRMenuhook(MENUHOOK_SETTING_EXPORTCHANNELS, 30401, PVR_MENUHOOK_SETTING));
			AddMenuHook(kodi::addon::PVRMenuhook(MENUHOOK_SETTING_CLEARCHANNELS, 30402, PVR_MENUHOOK_SETTING));

			// Generate the local file system and URL-based file names for the channels database
			std::string databasefile = UserPath() + "/channels.db";
			std::string databasefileuri = "file:///" + databasefile;

			// Create the global database connection pool instance
			try { m_connpool = std::make_shared<connectionpool>(databasefileuri.c_str(), DATABASE_CONNECTIONPOOL_SIZE, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI); } 
			catch(sqlite_exception const& dbex) {

				log_error(__func__, ": unable to create/open the channels database ", databasefile, " - ", dbex.what());
				throw;
			}

			// If the user has not specified a region code, attempt to get them to do it during startup
			if(m_settings.region_regioncode == regioncode::notset) {

				std::vector<enum regioncode>	regioncodes;		// Region codes
				std::vector<std::string>		regionlabels;		// Region labels

				// NORTH AMERICA (FM / HD / WX)
				//
				regioncodes.emplace_back(regioncode::northamerica);
				regionlabels.emplace_back(kodi::addon::GetLocalizedString(30317));

				// EUROPE/AUSTRALIA (FM / DAB)
				//
				regioncodes.emplace_back(regioncode::europe);
				regionlabels.emplace_back(kodi::addon::GetLocalizedString(30318));

				// WORLD (FM)
				//
				regioncodes.emplace_back(regioncode::world);
				regionlabels.emplace_back(kodi::addon::GetLocalizedString(30316));

				assert(regioncodes.size() == regionlabels.size());

				// Prompt the user; if they cancel the operation the region will remain as "not set"
				// and default to "world" for things like FM Radio RDS vs RBDS
				result = kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30315), regionlabels);
				if(result >= 0) {

					kodi::addon::SetSettingEnum<enum regioncode>("region_regioncode", regioncodes[result]);
					update_regioncode(regioncodes[result]);
				}
			}
		}

		catch(std::exception& ex) { handle_stdexception(__func__, ex); throw; }
		catch(...) { handle_generalexception(__func__); throw; }
	}

	// Anything that escapes above can't be logged at this point, just return ADDON_STATUS_PERMANENT_FAILURE
	catch(...) { return ADDON_STATUS::ADDON_STATUS_PERMANENT_FAILURE; }

	// Throw a simple banner out to the Kodi log indicating that the add-on has been loaded
	log_info(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " loaded");

	return ADDON_STATUS::ADDON_STATUS_OK;
}

//---------------------------------------------------------------------------
// addon::Destroy (private)
//
// Uninitializes/unloads the addon instance
//
// Arguments:
//
//	NONE

void addon::Destroy(void) noexcept
{
	try {

		// Throw a message out to the Kodi log indicating that the add-on is being unloaded
		log_info(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " unloading");

		m_pvrstream.reset();					// Destroy any active stream instance

		// Check for more than just the global connection pool reference during shutdown
		long poolrefs = m_connpool.use_count();
		if(poolrefs != 1) log_warning(__func__, ": m_connpool.use_count = ", m_connpool.use_count());
		m_connpool.reset();

		sqlite3_shutdown();						// Clean up SQLite

#ifdef _WINDOWS
		WSACleanup();							// Release winsock reference
#endif

		// Send a notice out to the Kodi log as late as possible and destroy the addon callbacks
		log_info(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " unloaded");
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// addon::SetSetting (CAddonBase)
//
// Notifies the addon that a setting has been changed
//
// Arguments:
//

ADDON_STATUS addon::SetSetting(std::string const& settingName, kodi::addon::CSettingValue const& settingValue)
{
	// Changing settings may be recursive operation, use recursive_lock
	std::unique_lock<std::recursive_mutex> settings_lock(m_settings_lock);

	// For comparison purposes
	struct settings previous = m_settings;

	// device_connection
	//
	if(settingName == "device_connection") {

		enum device_connection value = settingValue.GetEnum<enum device_connection>();
		if(value != m_settings.device_connection) {

			m_settings.device_connection = value;
			log_info(__func__, ": setting device_connection changed to ", device_connection_to_string(value).c_str());
		}
	}

	// device_connection_usb_index
	//
	else if(settingName == "device_connection_usb_index") {

		int nvalue = settingValue.GetInt();
		if(nvalue != static_cast<int>(m_settings.device_connection_usb_index)) {

			m_settings.device_connection_usb_index = nvalue;
			log_info(__func__, ": setting device_connection_usb_index changed to ", m_settings.device_connection_usb_index);
		}
	}

	// device_connection_tcp_host
	//
	else if(settingName == "device_connection_tcp_host") {

		std::string strvalue = settingValue.GetString();
		if(strvalue != m_settings.device_connection_tcp_host) {

			m_settings.device_connection_tcp_host = strvalue;
			log_info(__func__, ": setting device_connection_tcp_host changed to ", strvalue.c_str());
		}
	}

	// device_connection_tcp_port
	//
	else if(settingName == "device_connection_tcp_port") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.device_connection_tcp_port) {

			m_settings.device_connection_tcp_port = nvalue;
			log_info(__func__, ": setting device_connection_tcp_port changed to ", m_settings.device_connection_tcp_port);
		}
	}

	// device_frequency_correction
	//
	else if(settingName == "device_frequency_correction") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.device_frequency_correction) {

			m_settings.device_frequency_correction = nvalue;
			log_info(__func__, ": setting device_frequency_correction changed to ", m_settings.device_frequency_correction, "PPM");
		}
	}

	// fmradio_enable_rds
	//
	else if(settingName == "fmradio_enable_rds") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.fmradio_enable_rds) {

			m_settings.fmradio_enable_rds = bvalue;
			log_info(__func__, ": setting fmradio_enable_rds changed to ", bvalue);
		}
	}

	// fmradio_prepend_channel_numbers
	//
	else if(settingName == "fmradio_prepend_channel_numbers") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.fmradio_prepend_channel_numbers) {

			m_settings.fmradio_prepend_channel_numbers = bvalue;
			log_info(__func__, ": setting fmradio_prepend_channel_numbers changed to ", bvalue);

			// Trigger an update to refresh the channel names
			TriggerChannelUpdate();
		}
	}

	// fmradio_sample_rate
	//
	else if(settingName == "fmradio_sample_rate") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.fmradio_sample_rate) {

			m_settings.fmradio_sample_rate = nvalue;
			log_info(__func__, ": setting fmradio_sample_rate changed to ", m_settings.fmradio_sample_rate, "Hz");
		}
	}

	// fmradio_downsample_quality
	//
	else if(settingName == "fmradio_downsample_quality") {

		enum downsample_quality value = settingValue.GetEnum<enum downsample_quality>();
		if(value != m_settings.fmradio_downsample_quality) {

			m_settings.fmradio_downsample_quality = value;
			log_info(__func__, ": setting fmradio_downsample_quality changed to ", downsample_quality_to_string(value).c_str());
		}
	}

	// fmradio_output_samplerate
	//
	else if(settingName == "fmradio_output_samplerate") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.fmradio_output_samplerate) {

			m_settings.fmradio_output_samplerate = nvalue;
			log_info(__func__, ": setting fmradio_output_samplerate changed to ", nvalue, "Hz");
		}
	}

	// fmradio_output_gain
	//
	else if(settingName == "fmradio_output_gain") {

		float fvalue = settingValue.GetFloat();
		if(fvalue != m_settings.fmradio_output_gain) {

			m_settings.fmradio_output_gain = fvalue;
			log_info(__func__, ": setting fmradio_output_gain changed to ", fvalue, "dB");
		}
	}

	// hdradio_enable
	// 
	else if(settingName == "hdradio_enable") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.hdradio_enable) {

			m_settings.hdradio_enable = bvalue;
			log_info(__func__, ": setting hdradio_enable changed to ", bvalue);
			
			// Trigger an update to refresh the channel groups
			TriggerChannelGroupsUpdate();
		}
	}

	// hdradio_prepend_channel_numbers
	//
	else if(settingName == "hdradio_prepend_channel_numbers") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.hdradio_prepend_channel_numbers) {

			m_settings.hdradio_prepend_channel_numbers = bvalue;
			log_info(__func__, ": setting hdradio_prepend_channel_numbers changed to ", bvalue);

			// Trigger an update to refresh the channel names
			TriggerChannelUpdate();
		}
	}

	// hdradio_output_gain
	//
	else if(settingName == "hdradio_output_gain") {

		float fvalue = settingValue.GetFloat();
		if(fvalue != m_settings.hdradio_output_gain) {

			m_settings.hdradio_output_gain = fvalue;
			log_info(__func__, ": setting hdradio_output_gain changed to ", fvalue, "dB");
		}
	}

	// dabradio_enable
	//
	else if(settingName == "dabradio_enable") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.dabradio_enable) {

			m_settings.dabradio_enable = bvalue;
			log_info(__func__, ": setting dabradio_enable changed to ", bvalue);

			// Trigger an update to refresh the channel groups
			TriggerChannelGroupsUpdate();
		}
	}

	// dabradio_output_gain
	//
	else if(settingName == "dabradio_output_gain") {

		float fvalue = settingValue.GetFloat();
		if(fvalue != m_settings.dabradio_output_gain) {

			m_settings.dabradio_output_gain = fvalue;
			log_info(__func__, ": setting dabradio_output_gain changed to ", fvalue, "dB");
		}
	}

	// region_regioncode
	//
	if(settingName == "region_regioncode") {

		enum regioncode value = settingValue.GetEnum<enum regioncode>();
		if(value != m_settings.region_regioncode) {

			m_settings.region_regioncode = value;
			log_info(__func__, ": setting region_regioncode changed to ", regioncode_to_string(value).c_str());

			// Update the region code (warning: recursive)
			update_regioncode(m_settings.region_regioncode);
		}
	}

	// wxradio_enable
	//
	else if(settingName == "wxradio_enable") {

		bool bvalue = settingValue.GetBoolean();
		if(bvalue != m_settings.wxradio_enable) {

			m_settings.wxradio_enable = bvalue;
			log_info(__func__, ": setting wxradio_enable changed to ", bvalue);
			
			// Trigger an update to refresh the channel groups
			TriggerChannelGroupsUpdate();
		}
	}

	// wxradio_sample_rate
	//
	else if(settingName == "wxradio_sample_rate") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.wxradio_sample_rate) {

			m_settings.wxradio_sample_rate = nvalue;
			log_info(__func__, ": setting wxradio_sample_rate changed to ", m_settings.wxradio_sample_rate, "Hz");
		}
	}

	// wxradio_output_samplerate
	//
	else if(settingName == "wxradio_output_samplerate") {

		int nvalue = settingValue.GetInt();
		if(nvalue != m_settings.wxradio_output_samplerate) {

			m_settings.wxradio_output_samplerate = nvalue;
			log_info(__func__, ": setting wxradio_output_samplerate changed to ", nvalue, "Hz");
		}
	}

	// wxradio_output_gain
	//
	else if(settingName == "wxradio_output_gain") {

		float fvalue = settingValue.GetFloat();
		if(fvalue != m_settings.wxradio_output_gain) {

			m_settings.wxradio_output_gain = fvalue;
			log_info(__func__, ": setting wxradio_output_gain changed to ", fvalue, "dB");
		}
	}

	return ADDON_STATUS::ADDON_STATUS_OK;
}

//---------------------------------------------------------------------------
// CINSTANCEPVRCLIENT IMPLEMENTATION
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// addon::CallSettingsMenuHook (CInstancePVRClient)
//
// Call one of the settings related menu hooks
//
// Arguments:
//
//	menuhook	- The hook being invoked

PVR_ERROR addon::CallSettingsMenuHook(kodi::addon::PVRMenuhook const& menuhook)
{
	try {

		// Invoke the proper helper function to handle the settings menu hook implementation
		if(menuhook.GetHookId() == MENUHOOK_SETTING_IMPORTCHANNELS) menuhook_importchannels();
		else if(menuhook.GetHookId() == MENUHOOK_SETTING_EXPORTCHANNELS) menuhook_exportchannels();
		else if(menuhook.GetHookId() == MENUHOOK_SETTING_CLEARCHANNELS) menuhook_clearchannels();
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::CanSeekStream (CInstancePVRClient)
//
// Check if the backend supports seeking for the currently playing stream
//
// Arguments:
//
//	NONE

bool addon::CanSeekStream(void)
{
	try { return (m_pvrstream) ? m_pvrstream->canseek() : false; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, false); }
	catch(...) { return handle_generalexception(__func__, false); }
}

//-----------------------------------------------------------------------------
// addon::CloseLiveStream (CInstancePVRClient)
//
// Close an open live stream
//
// Arguments:
//
//	NONE

void addon::CloseLiveStream(void)
{
	// Prevent race condition with GetSignalStatus()
	std::unique_lock<std::mutex> lock(m_pvrstream_lock);

	try { m_pvrstream.reset(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); } 
	catch(...) { return handle_generalexception(__func__); }
}

//-----------------------------------------------------------------------------
// addon::DeleteChannel (CInstancePVRClient)
//
// Deletes a channel from the backend
//
// Arguments:
//
//	channel		- Channel to be deleted

PVR_ERROR addon::DeleteChannel(kodi::addon::PVRChannel const& channel)
{
	channelid channelid(channel.GetUniqueId());			// Convert UniqueID back into a channelid
	
	try {

		connectionpool::handle dbhandle(m_connpool);

		uint32_t const frequency = channelid.frequency();
		enum modulation const modulationtype = channelid.modulation();
		uint32_t const subchannel = channelid.subchannel();
	
		// For HD Radio and DAB, if the subchannel number is set only delete the subchannel
		if((modulationtype == modulation::hd || modulationtype == modulation::dab) && (subchannel > 0))
			delete_subchannel(dbhandle, channelid.frequency(), channelid.modulation(), channelid.subchannel());

		else delete_channel(dbhandle, frequency, modulationtype); 
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::DemuxAbort (CInstancePVRClient)
//
// Abort the demultiplexer thread in the add-on
//
// Arguments:
//
//	NONE

void addon::DemuxAbort(void)
{
	try { if(m_pvrstream) m_pvrstream->demuxabort(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//-----------------------------------------------------------------------------
// addon::DemuxFlush (CInstancePVRClient)
//
// Flush all data that's currently in the demultiplexer buffer
//
// Arguments:
//
//	NONE

void addon::DemuxFlush(void)
{
	try { if(m_pvrstream) m_pvrstream->demuxflush(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); } 
	catch(...) { return handle_generalexception(__func__); }
}

//-----------------------------------------------------------------------------
// addon::DemuxRead (CInstancePVRClient)
//
// Read the next packet from the demultiplexer
//
// Arguments:
//
//	NONE

DEMUX_PACKET* addon::DemuxRead(void)
{
	// Prevent race condition with GetSignalStatus()
	std::unique_lock<std::mutex> lock(m_pvrstream_lock);

	if(!m_pvrstream) return nullptr;

	try { 

		// Use an inline lambda to provide the stream an std::function to use to invoke AllocateDemuxPacket()
		DEMUX_PACKET* packet = m_pvrstream->demuxread([&](int size) -> DEMUX_PACKET* { return AllocateDemuxPacket(size); });

		// Log a warning if a stream change packet was detected; this means the application isn't keeping up with the device
		if((packet != nullptr) && (packet->iStreamId == DEMUX_SPECIALID_STREAMCHANGE))
			log_warning(__func__, ": stream buffer has been flushed; device sample rate may need to be reduced");

		return packet;
	}

	catch(std::exception& ex) {

		// Log the exception and alert the user of the failure with an error notification
		log_error(__func__, ": read operation failed with exception: ", ex.what());
		kodi::QueueFormattedNotification(QueueMsg::QUEUE_ERROR, "Unable to read from stream: %s", ex.what());

		m_pvrstream.reset();				// Close the stream
		return nullptr;						// Return a null demultiplexer packet
	}

	catch(...) { return handle_generalexception(__func__, nullptr); }
}

//-----------------------------------------------------------------------------
// addon::DemuxReset (CInstancePVRClient)
//
// Reset the demultiplexer in the add-on
//
// Arguments:
//
//	NONE

void addon::DemuxReset(void)
{
	try { if(m_pvrstream) m_pvrstream->demuxreset(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//-----------------------------------------------------------------------------
// addon::GetBackendName (CInstancePVRClient)
//
// Get the name reported by the backend that will be displayed in the UI
//
// Arguments:
//
//	name		- Backend name string to be initialized

PVR_ERROR addon::GetBackendName(std::string& name)
{
	name.assign(VERSION_PRODUCTNAME_ANSI);

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetBackendVersion (CInstancePVRClient)
//
// Get the version string reported by the backend that will be displayed in the UI
//
// Arguments:
//
//	version		- Backend version string to be initialized

PVR_ERROR addon::GetBackendVersion(std::string& version)
{
	version.assign(VERSION_VERSION3_ANSI);

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetCapabilities (CInstancePVRClient)
//
// Get the list of features that this add-on provides
//
// Arguments:
//
//	capabilities	- PVR capability attributes class

PVR_ERROR addon::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
	capabilities.SetSupportsRadio(true);
	capabilities.SetSupportsChannelGroups(true);
	capabilities.SetSupportsChannelSettings(true);
	capabilities.SetHandlesInputStream(true);
	capabilities.SetHandlesDemuxing(true);
	capabilities.SetSupportsEPG(true);

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannelGroupsAmount (CInstancePVRClient)
//
// Get the total amount of channel groups on the backend
//
// Arguments:
//
//	amount		- Set to the number of available channel groups

PVR_ERROR addon::GetChannelGroupsAmount(int& amount)
{
	amount = 4;				// "FM Radio", "HD Radio", "DAB", "Weather Radio"

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannelGroupMembers (CInstancePVRClient)
//
// Request the list of all group members of a group from the backend
//
// Arguments:
//
//	group		- Channel group for which to get the group members
//	results		- Channel group members result set to be loaded

PVR_ERROR addon::GetChannelGroupMembers(kodi::addon::PVRChannelGroup const& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
	// Only interested in radio channel groups
	if(!group.GetIsRadio()) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();

	// Select the proper enumerator for the channel group
	std::function<void(sqlite3*, enumerate_channels_callback)> enumerator = nullptr;
	
	if(group.GetGroupName() == kodi::addon::GetLocalizedString(30408))
		enumerator = std::bind(enumerate_fmradio_channels, std::placeholders::_1, settings.fmradio_prepend_channel_numbers, std::placeholders::_2);

	else if((group.GetGroupName() == kodi::addon::GetLocalizedString(30409)) && (settings.hdradio_enable)) 
		enumerator = std::bind(enumerate_hdradio_channels, std::placeholders::_1, settings.hdradio_prepend_channel_numbers, std::placeholders::_2);

	else if((group.GetGroupName() == kodi::addon::GetLocalizedString(30411)) && (settings.dabradio_enable))
		enumerator = enumerate_dabradio_channels;

	else if((group.GetGroupName() == kodi::addon::GetLocalizedString(30410)) && (settings.wxradio_enable))
		enumerator = enumerate_wxradio_channels;

	// If no enumerator was selected, there isn't any work to do here
	if(enumerator == nullptr) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	try {

		// Enumerate all of the channels in the specified group
		enumerator(connectionpool::handle(m_connpool), [&](struct channel const& channel) -> void {

			// Create and initialize a PVRChannelGroupMember instance for the enumerated channel
			kodi::addon::PVRChannelGroupMember member;
			member.SetGroupName(group.GetGroupName());
			member.SetChannelUniqueId(channel.id);
			member.SetChannelNumber(channel.channel);
			member.SetSubChannelNumber(channel.subchannel);

			results.Add(member);
		});
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannelGroups (CInstancePVRClient)
//
// Request the list of all channel groups from the backend
//
// Arguments:
//
//	radio		- True to get radio groups, false to get TV channel groups
//	results		- Channel groups result set to be loaded

PVR_ERROR addon::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
	kodi::addon::PVRChannelGroup	fmradio;			// FM Radio
	kodi::addon::PVRChannelGroup	hdradio;			// HD Radio
	kodi::addon::PVRChannelGroup	dabradio;			// DAB
	kodi::addon::PVRChannelGroup	wxradio;			// Weather Radio

	// The PVR only supports radio channel groups
	if(!radio) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	fmradio.SetGroupName(kodi::addon::GetLocalizedString(30408));
	fmradio.SetIsRadio(true);
	results.Add(fmradio);

	hdradio.SetGroupName(kodi::addon::GetLocalizedString(30409));
	hdradio.SetIsRadio(true);
	results.Add(hdradio);

	dabradio.SetGroupName(kodi::addon::GetLocalizedString(30411));
	dabradio.SetIsRadio(true);
	results.Add(dabradio);

	wxradio.SetGroupName(kodi::addon::GetLocalizedString(30410));
	wxradio.SetIsRadio(true);
	results.Add(wxradio);

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannels (CInstancePVRClient)
//
// Request the list of all channels from the backend
//
// Arguments:
//
//	radio		- True to get radio channels, false to get TV channels
//	results		- Channels result set to be loaded

PVR_ERROR addon::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
	// The PVR only supports radio channels
	if(!radio) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();

	try {

		auto callback = [&](struct channel const& item) -> void {

			kodi::addon::PVRChannel channel;

			channel.SetUniqueId(item.id);
			channel.SetIsRadio(true);
			channel.SetChannelNumber(item.channel);
			channel.SetSubChannelNumber(item.subchannel);
			if(item.name != nullptr) channel.SetChannelName(item.name);
			if(item.logourl != nullptr) channel.SetIconPath(item.logourl);

			results.Add(channel);
		};

		connectionpool::handle dbhandle(m_connpool);
		enumerate_fmradio_channels(dbhandle, settings.fmradio_prepend_channel_numbers, callback);
		if(settings.hdradio_enable) enumerate_hdradio_channels(dbhandle, settings.hdradio_prepend_channel_numbers, callback);
		if(settings.dabradio_enable) enumerate_dabradio_channels(dbhandle, callback);
		if(settings.wxradio_enable) enumerate_wxradio_channels(dbhandle, callback);
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannelsAmount (CInstancePVRClient)
//
// Gets the total amount of channels on the backend
//
// Arguments:
//
//	amount		- Set to the number of available channels

PVR_ERROR addon::GetChannelsAmount(int& amount)
{
	try { amount = get_channel_count(connectionpool::handle(m_connpool)); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetChannelStreamProperties (CInstancePVRClient)
//
// Get the stream properties for a channel from the backend
//
// Arguments:
//
//	channel		- channel to get the stream properties for
//	properties	- properties required to play the stream

PVR_ERROR addon::GetChannelStreamProperties(kodi::addon::PVRChannel const& /*channel*/, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
	properties.emplace_back(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");
	properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM_PLAYER, "audiodefaultplayer");

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetEPGForChannel (CInstancePVRClient)
//
// Request the EPG for a channel from the backend
//
// Arguments:
//
//	channelUid		- Channel identifier
//	start			- Start of the requested time frame
//	end				- End of the requested time frame
//	results			- EPG tag result set

PVR_ERROR addon::GetEPGForChannel(int /*channelUid*/, time_t /*start*/, time_t /*end*/, kodi::addon::PVREPGTagsResultSet& /*results*/)
{
	// This PVR doesn't support EPG, but if it doesn't claim that it does
	// the radio and TV channels get all mixed up ...
	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetSignalStatus (CInstancePVRClient)
//
// Get the signal status of the stream that's currently open
//
// Arguments:
//
//	channelUid		- Channel identifier
//	signalStatus	- Structure to set with the signal status information

PVR_ERROR addon::GetSignalStatus(int /*channelUid*/, kodi::addon::PVRSignalStatus& signalStatus)
{
	// Prevent race condition with functions that modify m_pvrstream
	std::unique_lock<std::mutex> lock(m_pvrstream_lock);

	// Kodi may call this function before the stream is open, avoid the error log
	if(!m_pvrstream) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	try {

		int quality = 0;			// Quality as a percentage
		int snr = 0;				// SNR as a percentage

		// Retrieve the quality metrics from the stream instance
		m_pvrstream->signalquality(quality, snr);

		signalStatus.SetAdapterName(m_pvrstream->devicename());
		signalStatus.SetAdapterStatus("Active");
		signalStatus.SetServiceName(m_pvrstream->servicename());
		signalStatus.SetProviderName("RTL-SDR");
		signalStatus.SetMuxName(m_pvrstream->muxname());

		signalStatus.SetSignal(quality * 655);		// Range: 0-65535
		signalStatus.SetSNR(snr * 655);				// Range: 0-65535
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetStreamProperties (CInstancePVRClient)
//
// Get the stream properties of the stream that's currently being read
//
// Arguments:
//
//	properties		- Stream properties to be set

PVR_ERROR addon::GetStreamProperties(std::vector<kodi::addon::PVRStreamProperties>& properties)
{
	if(!m_pvrstream) return PVR_ERROR::PVR_ERROR_FAILED;

	// Enumerate the stream properties as specified by the PVR stream instance
	m_pvrstream->enumproperties([&](struct streamprops const& props) -> void {

		kodi::addon::PVRCodec codec = GetCodecByName(props.codec);
		if(codec.GetCodecType() != PVR_CODEC_TYPE::PVR_CODEC_TYPE_UNKNOWN) {

			kodi::addon::PVRStreamProperties streamprops;

			streamprops.SetPID(props.pid);
			streamprops.SetCodecType(codec.GetCodecType());
			streamprops.SetCodecId(codec.GetCodecId());
			streamprops.SetChannels(props.channels);
			streamprops.SetSampleRate(props.samplerate);
			streamprops.SetBitsPerSample(props.bitspersample);
			streamprops.SetBitRate(props.samplerate * props.channels * props.bitspersample);

			properties.emplace_back(std::move(streamprops));
		}
	});

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::GetConnectionString (CInstancePVRClient)
//
// Gets the connection string reported by the backend
//
// Arguments:
//
//	connection	- Set to the connection string to report

PVR_ERROR addon::GetConnectionString(std::string& connection)
{
	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();

	// This property is fairly useless; just return the device connection type
	if(settings.device_connection == device_connection::usb) connection = "usb";
	else if(settings.device_connection == device_connection::rtltcp) connection = "network";
	else connection = "unknown";

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::IsRealTimeStream (CInstancePVRClient)
//
// Check for real-time streaming
//
// Arguments:
//
//	NONE

bool addon::IsRealTimeStream(void)
{
	try { return (m_pvrstream) ? m_pvrstream->realtime() : false; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, false); }
	catch(...) { return handle_generalexception(__func__, false); }
}

//-----------------------------------------------------------------------------
// addon::LengthLiveStream (CInstancePVRClient)
//
// Obtain the length of a live stream
//
// Arguments:
//
//	NONE

int64_t addon::LengthLiveStream(void)
{
	try { return (m_pvrstream) ? m_pvrstream->length() : -1; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//-----------------------------------------------------------------------------
// addon::OpenDialogChannelAdd (CInstancePVRClient)
//
// Show the dialog to add a channel on the backend
//
// Arguments:
//
//	channel		- The channel to add

PVR_ERROR addon::OpenDialogChannelAdd(kodi::addon::PVRChannel const& /*channel*/)
{
	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();
	
	// The user has the option to disable support for each type of channel
	std::vector<std::string> channeltypes;
	std::vector<enum modulation> modulationtypes;

	channeltypes.emplace_back(kodi::addon::GetLocalizedString(30414));
	modulationtypes.emplace_back(modulation::fm);

	if(settings.hdradio_enable) {

		channeltypes.emplace_back(kodi::addon::GetLocalizedString(30415));
		modulationtypes.emplace_back(modulation::hd);
	}

	if(settings.dabradio_enable) {

		channeltypes.emplace_back(kodi::addon::GetLocalizedString(30416));
		modulationtypes.emplace_back(modulation::dab);
	}

	if(settings.wxradio_enable) {

		channeltypes.emplace_back(kodi::addon::GetLocalizedString(30417));
		modulationtypes.emplace_back(modulation::wx);
	}

	assert(channeltypes.size() == modulationtypes.size());
	
	// If the user has no channel types enabled, just return without error
	if((channeltypes.size() == 0) || (modulationtypes.size() == 0)) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	// If more than one modulation type is available, prompt the user to select one
	enum modulation modulationtype = modulationtypes[0];
	if(modulationtypes.size() > 1) {

		int selected = kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30413), channeltypes);
		if(selected < 0) return PVR_ERROR::PVR_ERROR_NO_ERROR;

		modulationtype = modulationtypes[selected];
	}

	// TODO: see if there is a better way we can work with this
	if(m_pvrstream) {

		// TODO: This message is terrible
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30405), "Modifying PVR Radio channel settings requires "
			"exclusive access to the connected RTL-SDR tuner device.", "", "Active playback of PVR Radio streams must be stopped before continuing.");

		return PVR_ERROR::PVR_ERROR_NO_ERROR;
	}

	try {

		struct channelprops channelprops = {};			// New channel properties
		bool result = false;							// Result from channel add helper

		if(modulationtype == modulation::fm) result = channeladd_fm(settings, channelprops);
		else if(modulationtype == modulation::hd) result = channeladd_hd(settings, channelprops);
		else if(modulationtype == modulation::dab) result = channeladd_dab(settings, channelprops);
		else if(modulationtype == modulation::wx) result = channeladd_wx(settings, channelprops);

		if(result == false) return PVR_ERROR::PVR_ERROR_NO_ERROR;
	}

	catch(std::exception& ex) {

		// Log the error and inform the user that the operation failed, do not return an error code
		handle_stdexception(__func__, ex);
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30407), "An error occurred displaying the "
			"add channel dialog:", "", ex.what());
	}

	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::OpenDialogChannelScan (CInstancePVRClient)
//
// Show the channel scan dialog
//
// Arguments:
//
//	NONE

PVR_ERROR addon::OpenDialogChannelScan(void)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// addon::OpenDialogChannelSettings (CInstancePVRClient)
//
// Show the channel settings dialog
//
// Arguments:
//
//	channel		- The channel to show the dialog for

PVR_ERROR addon::OpenDialogChannelSettings(kodi::addon::PVRChannel const& channel)
{
	// Prevent manipulation of the PVR stream (OpenLiveStream/CloseLiveStream)
	std::unique_lock<std::mutex> lock(m_pvrstream_lock);

	// The channel settings dialog can't be shown when there is an active stream
	if(m_pvrstream) {

		// TODO: This message is terrible
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30405), "Modifying PVR Radio channel settings requires "
			"exclusive access to the connected RTL-SDR tuner device.", "", "Active playback of PVR Radio streams must be stopped before continuing.");

		return PVR_ERROR::PVR_ERROR_NO_ERROR;
	}

	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();

	try {

		// Set up the tuner device properties
		struct tunerprops tunerprops = {};
		tunerprops.freqcorrection = settings.device_frequency_correction;

		channelid channelid(channel.GetUniqueId());			// Convert UniqueID back into a channelid

		// Get the properties of the channel to be manipulated
		struct channelprops channelprops = {};
		if(!get_channel_properties(connectionpool::handle(m_connpool), channelid.frequency(), channelid.modulation(), channelprops))
			throw string_exception("Unable to retrieve properties for channel ", channel.GetChannelName().c_str());

		// Create and initialize the dialog box against a new signal meter instance
		std::unique_ptr<channelsettings> dialog = channelsettings::create(create_device(settings), tunerprops, channelprops, false);
		dialog->DoModal();

		if(dialog->get_dialog_result()) {

			// Retrieve the updated channel properties from the dialog box and persist them
			dialog->get_channel_properties(channelprops);
			update_channel(connectionpool::handle(m_connpool), channelprops);
		}
	}

	catch(std::exception& ex) {

		// Log the error and inform the user that the operation failed, do not return an error code
		handle_stdexception(__func__, ex);
		kodi::gui::dialogs::OK::ShowAndGetInput(kodi::addon::GetLocalizedString(30407), "An error occurred displaying the "
			"channel settings dialog:", "", ex.what());
	}

	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::OpenLiveStream (CInstancePVRClient)
//
// Open a live stream on the backend
//
// Arguments:
//
//	channel		- Channel of the live stream to be opened

bool addon::OpenLiveStream(kodi::addon::PVRChannel const& channel)
{
	// Prevent race condition with GetSignalStatus()
	std::unique_lock<std::mutex> lock(m_pvrstream_lock);

	// Create a copy of the current addon settings structure
	struct settings settings = copy_settings();

	try {

		// Set up the tuner device properties
		struct tunerprops tunerprops = {};
		tunerprops.freqcorrection = settings.device_frequency_correction;

		channelid channelid(channel.GetUniqueId());		// Convert UniqueID back into a channelid

		// Retrieve the tuning properties for the channel from the database
		struct channelprops channelprops = {};
		if(!get_channel_properties(connectionpool::handle(m_connpool), channelid.frequency(), channelid.modulation(), channelprops))
			throw string_exception("channel ", channel.GetUniqueId(), " (", channel.GetChannelName().c_str(), ") was not found in the database");

		// FM Radio
		//
		if(channelprops.modulation == modulation::fm) {

			// Set up the FM digital signal processor properties
			struct fmprops fmprops = {};
			fmprops.decoderds = settings.fmradio_enable_rds;
			fmprops.isnorthamerica = is_region_northamerica(settings);
			fmprops.samplerate = settings.fmradio_sample_rate;
			fmprops.downsamplequality = static_cast<int>(settings.fmradio_downsample_quality);
			fmprops.outputrate = settings.fmradio_output_samplerate;
			fmprops.outputgain = settings.fmradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_info(__func__, ": Creating fmstream for channel \"", channelprops.name, "\"");
			log_info(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_info(__func__, ": fmprops.decoderds = ", (fmprops.decoderds) ? "true" : "false");
			log_info(__func__, ": fmprops.isnorthamerica = ", (fmprops.isnorthamerica) ? "true" : "false");
			log_info(__func__, ": fmrops.samplerate = ", fmprops.samplerate, " Hz");
			log_info(__func__, ": fmprops.downsamplequality = ", downsample_quality_to_string(static_cast<enum downsample_quality>(fmprops.downsamplequality)));
			log_info(__func__, ": fmprops.outputgain = ", fmprops.outputgain, " dB");
			log_info(__func__, ": fmprops.outputrate = ", fmprops.outputrate, " Hz");
			log_info(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_info(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_info(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");
			log_info(__func__, ": channelprops.freqcorrection = ", channelprops.freqcorrection, " PPM");

			// Create the FM Radio stream
			m_pvrstream = fmstream::create(create_device(settings), tunerprops, channelprops, fmprops);
		}

		// HD Radio
		//
		else if(channelprops.modulation == modulation::hd) {

			// Set up the HD Radio digital signal processor properties
			struct hdprops hdprops = {};
			hdprops.outputgain = settings.hdradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_info(__func__, ": Creating hdstream for channel \"", channelprops.name, "\"");
			log_info(__func__, ": subchannel = ", channelid.subchannel());
			log_info(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_info(__func__, ": hdprops.outputgain = ", hdprops.outputgain, " dB");
			log_info(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_info(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_info(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");
			log_info(__func__, ": channelprops.freqcorrection = ", channelprops.freqcorrection, " PPM");

			// Create the HD Radio stream
			m_pvrstream = hdstream::create(create_device(settings), tunerprops, channelprops, hdprops, channelid.subchannel());
		}

		// DAB
		//
		else if(channelprops.modulation == modulation::dab) {

			// Set up the DAB digital signal processor properties
			struct dabprops dabprops = {};
			dabprops.outputgain = settings.dabradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_info(__func__, ": Creating dabstream for channel \"", channelprops.name, "\"");
			log_info(__func__, ": subchannel = ", channelid.subchannel());
			log_info(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_info(__func__, ": dabrops.outputgain = ", dabprops.outputgain, " dB");
			log_info(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_info(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_info(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");
			log_info(__func__, ": channelprops.freqcorrection = ", channelprops.freqcorrection, " PPM");

			// Create the DAB stream
			m_pvrstream = dabstream::create(create_device(settings), tunerprops, channelprops, dabprops, channelid.subchannel());
		}

		// Weather Radio
		//
		else if(channelprops.modulation == modulation::wx) {

			// Set up the FM digital signal processor properties
			struct wxprops wxprops = {};
			wxprops.samplerate = settings.wxradio_sample_rate;
			wxprops.outputrate = settings.wxradio_output_samplerate;
			wxprops.outputgain = settings.wxradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_info(__func__, ": Creating wxstream for channel \"", channelprops.name, "\"");
			log_info(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_info(__func__, ": wxprops.samplerate = ", wxprops.samplerate, " Hz");
			log_info(__func__, ": wxprops.outputgain = ", wxprops.outputgain, " dB");
			log_info(__func__, ": wxprops.outputrate = ", wxprops.outputrate, " Hz");
			log_info(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_info(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_info(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");
			log_info(__func__, ": channelprops.freqcorrection = ", channelprops.freqcorrection, " PPM");

			// Create the Weather Radio stream
			m_pvrstream = wxstream::create(create_device(settings), tunerprops, channelprops, wxprops);
		}

		else throw string_exception("channel ", channel.GetUniqueId(), " (", channel.GetChannelName().c_str(), ") has an unknown modulation type");
	}

	// Queue a notification for the user when a live stream cannot be opened, don't just silently log it
	catch(std::exception& ex) { 
		
		kodi::QueueFormattedNotification(QueueMsg::QUEUE_ERROR, "Live Stream creation failed (%s).", ex.what());
		return handle_stdexception(__func__, ex, false); 
	}

	catch(...) { return handle_generalexception(__func__, false); }

	return true;
}

//-----------------------------------------------------------------------------
// addon::ReadLiveStream (CInstancePVRClient)
//
// Read from an open live stream
//
// Arguments:
//
//	buffer		- The buffer to store the data in
//	size		- The number of bytes to read into the buffer

int addon::ReadLiveStream(unsigned char* /*buffer*/, unsigned int /*size*/)
{
	return -1;
}

//-----------------------------------------------------------------------------
// addon::RenameChannel (CInstancePVRClient)
//
// Renames a channel on the backend
//
// Arguments:
//
//	channel		- Channel to be renamed

PVR_ERROR addon::RenameChannel(kodi::addon::PVRChannel const& channel)
{
	channelid channelid(channel.GetUniqueId());			// Convert UniqueID back into a channelid

	try { rename_channel(connectionpool::handle(m_connpool), channelid.frequency(), channelid.modulation(), channel.GetChannelName().c_str()); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//-----------------------------------------------------------------------------
// addon::SeekLiveStream (CInstancePVRClient)
//
// Seek in a live stream on a backend that supports timeshifting
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

int64_t addon::SeekLiveStream(int64_t position, int whence)
{
	try { return (m_pvrstream) ? m_pvrstream->seek(position, whence) : -1; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//---------------------------------------------------------------------------

#pragma warning(pop)
