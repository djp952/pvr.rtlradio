//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __ADDON_H_
#define __ADDON_H_
#pragma once

#include <kodi/addon-instance/PVR.h>
#include <memory>
#include <mutex>

#include "database.h"
#include "props.h"
#include "pvrstream.h"
#include "pvrtypes.h"
#include "rtldevice.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class addon
//
// Implements the PVR addon instance

class ATTR_DLL_LOCAL addon : public kodi::addon::CAddonBase, public kodi::addon::CInstancePVRClient
{
public:

	// Instance Constructor
	//
	addon();

	// Destructor
	//
	virtual ~addon() override;

	//-------------------------------------------------------------------------
	// CAddonBase
	//-------------------------------------------------------------------------

	// Create
	//
	// Initializes a new addon class instance
	ADDON_STATUS Create(void) override;

	// SetSetting
	//
	// Notifies the addon that a setting has been changed
	ADDON_STATUS SetSetting(std::string const& settingName, kodi::addon::CSettingValue const& settingValue) override;

	//-------------------------------------------------------------------------
	// CInstancePVRClient
	//-------------------------------------------------------------------------

	// CallSettingsMenuHook
	//
	// Call one of the settings related menu hooks
	PVR_ERROR CallSettingsMenuHook(kodi::addon::PVRMenuhook const& menuhook) override;

	// CanSeekStream
	//
	// Check if the backend supports seeking for the currently playing stream
	bool CanSeekStream(void) override;

	// CloseLiveStream
	//
	// Close an open live stream
	void CloseLiveStream(void) override;

	// DeleteChannel
	//
	// Deletes a channel from the backend
	PVR_ERROR DeleteChannel(kodi::addon::PVRChannel const& channel) override;

	// DemuxAbort
	//
	// Abort the demultiplexer thread in the add-on
	void DemuxAbort(void) override;

	// DemuxFlush
	//
	// Flush all data that's currently in the demultiplexer buffer
	void DemuxFlush(void) override;

	// DemuxRead
	//
	// Read the next packet from the demultiplexer
	DEMUX_PACKET* DemuxRead(void) override;

	// DemuxReset
	//
	// Reset the demultiplexer in the add-on
	void DemuxReset(void) override;

	// GetBackendName
	//
	// Get the name reported by the backend that will be displayed in the UI
	PVR_ERROR GetBackendName(std::string& name) override;

	// GetBackendVersion
	//
	// Get the version string reported by the backend that will be displayed in the UI
	PVR_ERROR GetBackendVersion(std::string& version) override;

	// GetCapabilities
	//
	// Get the list of features that this add-on provides
	PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;

	// GetChannelGroupsAmount
	//
	// Get the total amount of channel groups on the backend
	PVR_ERROR GetChannelGroupsAmount(int& amount) override;

	// GetChannelGroupMembers
	//
	// Request the list of all group members of a group from the backend
	PVR_ERROR GetChannelGroupMembers(kodi::addon::PVRChannelGroup const& group, kodi::addon::PVRChannelGroupMembersResultSet& results) override;
	
	// GetChannelGroups
	//
	// Request the list of all channel groups from the backend
	PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
		
	// GetChannels
	//
	// Request the list of all channels from the backend
	PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
		
	// GetChannelsAmount
	//
	// Gets the total amount of channels on the backend
	PVR_ERROR GetChannelsAmount(int& amount) override;

	// GetChannelStreamProperties
	//
	// Get the stream properties for a channel from the backend
	PVR_ERROR GetChannelStreamProperties(kodi::addon::PVRChannel const& channel, std::vector<kodi::addon::PVRStreamProperty>& properties) override;

	// GetEPGForChannel
	//
	// Request the EPG for a channel from the backend
	PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) override;
		
	// GetSignalStatus
	//
	// Get the signal status of the stream that's currently open
	PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override;

	// GetStreamProperties
	//
	// Get the stream properties of the stream that's currently being read
	PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperties>& properties) override;
		
	// GetConnectionString
	//
	// Gets the connection string reported by the backend
	PVR_ERROR GetConnectionString(std::string& connection) override;
		
	// IsRealTimeStream
	//
	// Check for real-time streaming
	bool IsRealTimeStream(void) override;

	// LengthLiveStream
	//
	// Obtain the length of a live stream
	int64_t LengthLiveStream(void) override;

	// OpenDialogChannelAdd
	//
	// Show the dialog to add a channel on the backend
	PVR_ERROR OpenDialogChannelAdd(kodi::addon::PVRChannel const& channel) override;

	// OpenDialogChannelScan
	//
	// Show the channel scan dialog
	PVR_ERROR OpenDialogChannelScan(void) override;

	// OpenDialogChannelSettings
	//
	// Show the channel settings dialog
	PVR_ERROR OpenDialogChannelSettings(kodi::addon::PVRChannel const& channel) override;

	// OpenLiveStream
	//
	// Open a live stream on the backend
	bool OpenLiveStream(kodi::addon::PVRChannel const& channel) override;
		
	// ReadLiveStream
	//
	// Read from an open live stream
	int ReadLiveStream(unsigned char* buffer, unsigned int size) override;

	// RenameChannel
	//
	// Renames a channel on the backend
	PVR_ERROR RenameChannel(kodi::addon::PVRChannel const& channel) override;

	// SeekLiveStream
	//
	// Seek in a live stream on a backend that supports timeshifting
	int64_t SeekLiveStream(int64_t position, int whence) override;

private:

	addon(addon const&)=delete;
	addon& operator=(addon const&)=delete;

	//-------------------------------------------------------------------------
	// Private Member Functions

	// Destroy
	//
	// Uninitializes/unloads the addon instance
	void Destroy(void) noexcept;

	// Channel Add Helpers
	//
	bool channeladd_dab(struct settings const& settings, struct channelprops& channelprops) const;
	bool channeladd_fm(struct settings const& settings, struct channelprops& channelprops) const;
	bool channeladd_hd(struct settings const& settings, struct channelprops& channelprops) const;
	bool channeladd_wx(struct settings const& settings, struct channelprops& channelprops) const;

	// Device Helpers
	//
	std::unique_ptr<rtldevice> create_device(struct settings const& settings) const;

	// Exception Helpers
	//
	void handle_generalexception(char const* function);
	template<typename _result> _result handle_generalexception(char const* function, _result result);
	void handle_stdexception(char const* function, std::exception const& ex);
	template<typename _result> _result handle_stdexception(char const* function, std::exception const& ex, _result result);

	// Log Helpers
	//
	template<typename... _args> void log_debug(_args&&... args) const;
	template<typename... _args> void log_error(_args&&... args) const;
	template<typename... _args> void log_info(_args&&... args) const;
	template<typename... _args> void log_message(ADDON_LOG level, _args&&... args) const;
	template<typename... _args> void log_warning(_args&&... args) const;

	// Menu Hook Helpers
	//
	void menuhook_clearchannels(void);
	void menuhook_exportchannels(void);
	void menuhook_importchannels(void);

	// Regional Helpers
	//
	bool is_region_northamerica(struct settings const& settings) const;
	void update_regioncode(enum regioncode code) const;

	// Settings Helpers
	//
	struct settings copy_settings(void) const;
	static std::string device_connection_to_string(enum device_connection connection);
	static std::string downsample_quality_to_string(enum downsample_quality quality);
	static std::string regioncode_to_string(enum regioncode code);

	//-------------------------------------------------------------------------
	// Member Variables

	std::shared_ptr<connectionpool>	m_connpool;				// Database connection pool
	std::unique_ptr<pvrstream>		m_pvrstream;			// Active PVR stream instance
	mutable std::mutex				m_pvrstream_lock;		// Synchronization object
	struct settings					m_settings;				// Custom addon settings
	mutable std::recursive_mutex	m_settings_lock;		// Synchronization object
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __ADDON_H_
