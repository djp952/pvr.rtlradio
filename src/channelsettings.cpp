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
#include "channelsettings.h"

#include <cmath>
#include <kodi/General.h>
#include <kodi/gui/dialogs/FileBrowser.h>

#pragma warning(push, 4)

// Control Identifiers
//
static const int CONTROL_BUTTON_OK				= 100;
static const int CONTROL_BUTTON_CANCEL			= 101;
static const int CONTROL_EDIT_FREQUENCY			= 200;
static const int CONTROL_EDIT_CHANNELNAME		= 201;
static const int CONTROL_BUTTON_CHANNELICON		= 202;
static const int CONTROL_IMAGE_CHANNELICON		= 203;
static const int CONTROL_RADIO_AUTOMATICGAIN	= 204;
static const int CONTROL_SLIDER_MANUALGAIN		= 205;
static const int CONTROL_RENDER_SIGNALMETER		= 206;
static const int CONTROL_EDIT_METERGAIN			= 207;
static const int CONTROL_EDIT_METERPOWER		= 208;
static const int CONTROL_EDIT_METERSNR			= 209;

// channelsettings::FMRADIO_BANDWIDTH
//
// Bandwidth of an analog FM radio channel
uint32_t const channelsettings::FMRADIO_BANDWIDTH = (200 KHz);

// channelsettings::HDRADIO_BANDWIDTH
//
// Bandwidth of a Hybrid Digital (HD) FM radio channel
uint32_t const channelsettings::HDRADIO_BANDWIDTH = (400 KHz);

// channelsettings::WXRADIO_BANDWIDTH
//
// Bandwidth of a VHF weather radio channel
uint32_t const channelsettings::WXRADIO_BANDWIDTH = (10 KHz);

//---------------------------------------------------------------------------
// channelsettings Constructor (private)
//
// Arguments:
//
//	device			- Device instance
//	tunerprops		- Tuner properties
//	channelprops	- Channel properties

channelsettings::channelsettings(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, struct channelprops const& channelprops) 
	: kodi::gui::CWindow("channelsettings.xml", "skin.estuary", true), m_channelprops(channelprops)
{
	uint32_t				bandwidth = FMRADIO_BANDWIDTH;			// Default to wideband FM radio bandwidth

	assert(device);

	// Adjust the signal meter bandwidth based on the underlying channel type
	switch(get_channel_type(channelprops)) {

		case channeltype::fmradio: bandwidth = FMRADIO_BANDWIDTH; break;
		case channeltype::hdradio: bandwidth = HDRADIO_BANDWIDTH; break;
		case channeltype::wxradio: bandwidth = WXRADIO_BANDWIDTH; break;
	}

	// Create the signal meter instance with the specified device and tuner properties, set for a 500ms callback rate
	m_signalmeter = fmmeter::create(std::move(device), tunerprops, channelprops.frequency, bandwidth, 
		std::bind(&channelsettings::fm_meter_status, this, std::placeholders::_1), 500, 
		std::bind(&channelsettings::fm_meter_exception, this, std::placeholders::_1));

	// Get the vector<> of valid manual gain values for the attached device
	m_signalmeter->get_valid_manual_gains(m_manualgains);
}

//---------------------------------------------------------------------------
// channelsettings Destructor

channelsettings::~channelsettings()
{
	// Stop the signal meter
	m_signalmeter->stop();
}

//---------------------------------------------------------------------------
// channelsettings::create (static)
//
// Factory method, creates a new channelsettings instance
//
// Arguments:
//
//	device			- Device instance
//	tunerprops		- Tuner properties
//	channelprops	- Channel properties

std::unique_ptr<channelsettings> channelsettings::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
	struct channelprops const& channelprops)
{
	return std::unique_ptr<channelsettings>(new channelsettings(std::move(device), tunerprops, channelprops));
}

//---------------------------------------------------------------------------
// channelsettings::fm_meter_exception (private)
//
// Callback to handle an exception raised by the signal meter
//
// Arguments:
//
//	ex		- Exception that was raised by the signal meter

void channelsettings::fm_meter_exception(std::exception const& ex)
{
	kodi::QueueFormattedNotification(QueueMsg::QUEUE_ERROR, "Unable to read from device: %s", ex.what());
}

//---------------------------------------------------------------------------
// channelsettings::fm_meter_status (private)
//
// Updates the state of the signal meter
//
// Arguments:
//
//	status		- Updated signal status from the signal meter

void channelsettings::fm_meter_status(struct fmmeter::signal_status const& status)
{
	char strbuf[64] = {};						// snprintf() text buffer

	// Signal Strength
	//
	if(!std::isnan(status.power)) {

		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%.1F dB", status.power);
		m_edit_signalpower->SetText(strbuf);
	}
	else m_edit_signalpower->SetText("N/A");

	// Signal-to-noise
	//
	if(!std::isnan(status.snr)) {
		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%d dB", static_cast<int>(status.snr));
		m_edit_signalsnr->SetText(strbuf);
	}
	else m_edit_signalsnr->SetText("N/A");
}

//---------------------------------------------------------------------------
// channelsettings::gain_to_percent (private)
//
// Converts a manual gain value into a percentage
//
// Arguments:
//
//	gain		- Gain to convert

int channelsettings::gain_to_percent(int gain) const
{
	if(m_manualgains.empty()) return 0;

	// Convert the gain into something that's valid for the tuner
	gain = nearest_valid_gain(gain);

	// Use the index within the gain table to generate the percentage
	for(size_t index = 0; index < m_manualgains.size(); index++) {

		if(gain == m_manualgains[index]) return static_cast<int>((index * 100) / (m_manualgains.size() - 1));
	}

	return 0;
}

//---------------------------------------------------------------------------
// channelsettings::get_channel_properties
//
// Gets the updated channel properties from the dialog box
//
// Arguments:
//
//	channelprops	- Structure to receive the updated channel properties

void channelsettings::get_channel_properties(struct channelprops& channelprops) const
{
	channelprops = m_channelprops;
}

//---------------------------------------------------------------------------
// channelsettings::get_dialog_result
//
// Gets the result code from the dialog box
//
// Arguments:
//
//	NONE

bool channelsettings::get_dialog_result(void) const
{
	return m_result;
}

//---------------------------------------------------------------------------
// channelsettings::nearest_valid_gain (private)
//
// Gets the closest valid value for a manual gain setting
//
// Arguments:
//
//	gain		- Gain to adjust

int channelsettings::nearest_valid_gain(int gain) const
{
	if(m_manualgains.empty()) return 0;

	// Select the gain value that's closest to what has been requested
	int nearest = m_manualgains[0];
	for(size_t index = 0; index < m_manualgains.size(); index++) {

		if(std::abs(gain - m_manualgains[index]) < std::abs(gain - nearest)) nearest = m_manualgains[index];
	}

	return nearest;
}

//---------------------------------------------------------------------------
// channelsettings::percent_to_gain (private)
//
// Converts a percentage into a manual gain value
//
// Arguments:
//
//	percent		- Percentage to convert

int channelsettings::percent_to_gain(int percent) const
{
	if(m_manualgains.empty()) return 0;

	if(percent == 0) return m_manualgains.front();
	else if(percent == 100) return m_manualgains.back();

	return m_manualgains[(percent * m_manualgains.size()) / 100];
}

//---------------------------------------------------------------------------
// channelsettings::update_gain (private)
//
// Updates the state of the gain control
//
// Arguments:
//
//	NONE

void channelsettings::update_gain(void)
{
	char strbuf[64] = {};						// snprintf() text buffer

	if(!m_channelprops.autogain) {

		// Convert the gain value from tenths of a decibel into XX.X dB format
		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%.1f dB", m_channelprops.manualgain / 10.0);
		m_edit_signalgain->SetText(strbuf);
	}

	else m_edit_signalgain->SetText("Auto");
}

//---------------------------------------------------------------------------
// CWINDOW IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// channelsettings::OnAction (CWindow)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	actionId	- The action id to perform

bool channelsettings::OnAction(ADDON_ACTION actionId)
{
	return kodi::gui::CWindow::OnAction(actionId);
}

//---------------------------------------------------------------------------
// channelsettings::OnClick (CWindow)
//
// Receives click event notifications for a control
//
//	controlId	- GUI control identifier

bool channelsettings::OnClick(int controlId)
{
	switch(controlId) {

		case CONTROL_EDIT_CHANNELNAME:
			m_channelprops.name = m_edit_channelname->GetText();
			break;

		case CONTROL_BUTTON_CHANNELICON:
			kodi::gui::dialogs::FileBrowser::ShowAndGetImage("local|network|pictures", kodi::GetLocalizedString(30406), m_channelprops.logourl);
			m_image_channelicon->SetFileName(m_channelprops.logourl, false);
			return true;

		case CONTROL_RADIO_AUTOMATICGAIN:
			m_channelprops.autogain = m_radio_autogain->IsSelected();
			m_signalmeter->set_automatic_gain(m_channelprops.autogain);
			m_slider_manualgain->SetEnabled(!m_channelprops.autogain);
			update_gain();
			return true;

		case CONTROL_SLIDER_MANUALGAIN:
			m_channelprops.manualgain = percent_to_gain(static_cast<int>(m_slider_manualgain->GetPercentage()));
			m_signalmeter->set_manual_gain(m_channelprops.manualgain);
			update_gain();
			return true;

		case CONTROL_BUTTON_OK:
			m_result = true;
			Close();
			return true;

		case CONTROL_BUTTON_CANCEL:
			Close();
			return true;
	}

	return kodi::gui::CWindow::OnClick(controlId);
}

//---------------------------------------------------------------------------
// channelsettings::OnInit (CWindow)
//
// Called to initialize the window object
//
// Arguments:
//
//	NONE

bool channelsettings::OnInit(void)
{
	try {

		// Get references to all of the manipulable dialog controls
		m_edit_frequency = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_FREQUENCY));
		m_edit_channelname = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_CHANNELNAME));
		m_button_channelicon = std::unique_ptr<CButton>(new CButton(this, CONTROL_BUTTON_CHANNELICON));
		m_image_channelicon = std::unique_ptr<CImage>(new CImage(this, CONTROL_IMAGE_CHANNELICON));
		m_radio_autogain = std::unique_ptr<CRadioButton>(new CRadioButton(this, CONTROL_RADIO_AUTOMATICGAIN));
		m_slider_manualgain = std::unique_ptr<CSettingsSlider>(new CSettingsSlider(this, CONTROL_SLIDER_MANUALGAIN));
		m_render_signalmeter = std::unique_ptr<CRendering>(new CRendering(this, CONTROL_RENDER_SIGNALMETER));
		m_edit_signalgain = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERGAIN));
		m_edit_signalpower = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERPOWER));
		m_edit_signalsnr = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERSNR));

		// Set the channel frequency in XXX.X MHz format
		char freqstr[128];
		snprintf(freqstr, std::extent<decltype(freqstr)>::value, "%.1f", (m_channelprops.frequency / static_cast<double>(100000)) / 10.0);
		m_edit_frequency->SetText(freqstr);

		// Set the channel name and logo/icon
		m_edit_channelname->SetText(m_channelprops.name);
		m_image_channelicon->SetFileName(m_channelprops.logourl, false);

		// Adjust the manual gain value to match something that the tuner supports
		m_channelprops.manualgain = nearest_valid_gain(m_channelprops.manualgain);

		// Set the tuner gain parameters
		m_radio_autogain->SetSelected(m_channelprops.autogain);
		m_slider_manualgain->SetEnabled(!m_channelprops.autogain);
		m_slider_manualgain->SetPercentage(static_cast<float>(gain_to_percent(m_channelprops.manualgain)));
		update_gain();

		// Set the default text for the signal indicators
		m_edit_signalpower->SetText("N/A");
		m_edit_signalsnr->SetText("N/A");

		// Start the signal meter instance
		m_signalmeter->set_automatic_gain(m_channelprops.autogain);
		m_signalmeter->set_manual_gain(m_channelprops.manualgain);
		m_signalmeter->start();
	}

	catch(...) { return false; }

	return kodi::gui::CWindow::OnInit();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
