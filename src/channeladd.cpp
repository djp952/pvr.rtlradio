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
#include "channeladd.h"

#include <assert.h>
#include <kodi/General.h>

#pragma warning(push, 4)

// Control Identifiers
//
static const int CONTROL_LABEL_HEADERLABEL	= 2;
static const int CONTROL_BUTTON_ADD			= 100;
static const int CONTROL_BUTTON_CLOSE		= 101;
static const int CONTROL_BUTTON_0			= 200;
static const int CONTROL_BUTTON_1			= 201;
static const int CONTROL_BUTTON_2			= 202;
static const int CONTROL_BUTTON_3			= 203;
static const int CONTROL_BUTTON_4			= 204;
static const int CONTROL_BUTTON_5			= 205;
static const int CONTROL_BUTTON_6			= 206;
static const int CONTROL_BUTTON_7			= 207;
static const int CONTROL_BUTTON_8			= 208;
static const int CONTROL_BUTTON_9			= 209;
static const int CONTROL_BUTTON_BACKSPACE	= 210;
static const int CONTROL_LABEL_INPUT		= 300;

//---------------------------------------------------------------------------
// channeladd Constructor (private)
//
// Arguments:
//
//	modulation		- Modulation of the channel being added

channeladd::channeladd(enum modulation modulation) : kodi::gui::CWindow("channeladd.xml", "skin.estuary", true),
	m_modulation(modulation)
{
}

//---------------------------------------------------------------------------
// channeladd::create (static)
//
// Factory method, creates a new channeladd instance
//
// Arguments:
//
//	modulation		- Modulation of the channel being added

std::unique_ptr<channeladd> channeladd::create(enum modulation modulation)
{
	return std::unique_ptr<channeladd>(new channeladd(modulation));
}

//---------------------------------------------------------------------------
// channeladd::format_input (private)
//
// Formats an input frequency string for display
//
// Arguments:
//
//	input		- Input to be formatted

std::string channeladd::format_input(std::string const& input) const
{
	std::string formatted(input);

	// 8x.x, 9x.x
	if((formatted.length() == 3) && ((formatted[0] == '8') || (formatted[0] == '9'))) formatted.insert(2, ".");

	// 1xx.x[xx]
	else if(formatted.length() >= 4) formatted.insert(3, ".");

	return formatted;
}

//---------------------------------------------------------------------------
// channeladd::get_channel_properties
//
// Gets the updated channel properties from the dialog box
//
// Arguments:
//
//	channelprops	- Structure to receive the updated channel properties

void channeladd::get_channel_properties(struct channelprops& channelprops) const
{
	channelprops = m_channelprops;
}

//---------------------------------------------------------------------------
// channeladd::get_dialog_result
//
// Gets the result code from the dialog box
//
// Arguments:
//
//	NONE

bool channeladd::get_dialog_result(void) const
{
	return m_result;
}

//---------------------------------------------------------------------------
// channeladd::get_frequency (private)
//
// Attempts to parse a formatted input string to get the frequency
//
// Arguments:
//
//	input		- Input string to be checked
//	frequency	- On success, contains the parsed frequency in Hertz

bool channeladd::get_frequency(std::string const& input, uint32_t& frequency) const
{
	uint32_t	mhz = 0;			// Megahertz component
	uint32_t	khz = 0;			// Kilohertz component

	frequency = 0;					// Initialize [out] reference

	// There should be two integer parts to the string, megahertz and kilohertz
	if(sscanf(input.c_str(), "%u.%u", &mhz, &khz) != 2) return false;

	// FM channels are specified as 100KHz, scale the value
	if(khz < 10) khz *= 100;

	// Convert the megahertz and kilohertz portions into hertz
	frequency = (mhz MHz) + (khz KHz);

	return true;
}

//---------------------------------------------------------------------------
// channeladd::on_backspace (private)
//
// Handles input of a backspace character
//
// Arguments:
//
//	NONE

void channeladd::on_backspace(void)
{
	if(m_input.empty()) return;
	else m_input.erase(m_input.length() - 1);
}

//---------------------------------------------------------------------------
// channeladd::on_digit (private)
//
// Handles input of a digit character
//
// Arguments:
//
//	number		- The number that has been input

void channeladd::on_digit(int digit)
{
	std::string		input(m_input);				// Copy the existing value
	
	// Convert the digit into a character
	char chdigit = static_cast<char>('0' + digit);

	// Only allow input that is valid based on the existing input, the period
	// will be done automatically when the label is formatted
	//
	// Ranges: 87.5 -> 108.0 (FM), 87.9 -> 107.9 (HD Radio)

	// Position 0
	//
	if(input.length() == 0) {

		// [1, 8, 9]
		if((digit == 1) || (digit == 8) || (digit == 9)) input += chdigit;
	}

	// Position 1
	//
	else if(input.length() == 1) {

		// 8x -> [7-9]
		if((input[0] == '8') && (digit >= 7) && (digit <= 9)) input += chdigit;

		// 9x -> [0-9]
		else if(input[0] == '9') input += chdigit;

		// 1x -> [0]
		else if((input[0] == '1') && (digit == 0)) input += chdigit;
	}

	// Position 2
	//
	else if(input.length() == 2) {

		if(m_modulation == modulation::hd) {

			// 87.x -> [9]
			if((input[0] == '8') && (input[1] == '7') && (digit == 9)) input += chdigit;

			// 88.x / 89.x -> [1, 3, 5, 7, 9]
			else if((input[0] == '8') && (input[1] > '7') && ((digit % 2) == 1)) input += chdigit;

			// 9x.x -> [1, 3, 5, 7, 9]
			else if((input[0] == '9') && ((digit % 2) == 1)) input += chdigit;

			// 10x -> [0-7]
			else if((input[0] == '1') && (input[1] == '0') && (digit <= 7)) input += chdigit;
		}

		else {


			// 87.x -> [5, 6, 7, 8, 9]
			if((input[0] == '8') && (input[1] == '7') && (digit >= 5)) input += chdigit;

			// 8x.x -> [0-9]
			else if((input[0] == '8') && (input[1] > '7')) input += chdigit;

			// 9x.x -> [0-9]
			else if(input[0] == '9') input += chdigit;

			// 10x -> [0-8]
			else if((input[0] == '1') && (input[1] == '0') && (digit <= 8)) input += chdigit;
		}
	}

	// Position 3
	//
	else if(input.length() == 3) {

		if(m_modulation == modulation::hd) {

			// 10x.x -> [1, 3, 5, 7, 9]
			if((input[0] == '1') && (input[1] == '0') && ((digit % 2) == 1)) input += chdigit;
		}

		else {

			// 108.x -> [0]
			if((input[0] == '1') && (input[1] == '0') && (input[2] == '8') && (digit == 0)) input += chdigit;

			// 10x.x -> [0-9]
			else if((input[0] == '1') && (input[1] == '0') && (input[2] < '8')) input += chdigit;
		}
	}

	m_input.swap(input);
}

//---------------------------------------------------------------------------
// CWINDOW IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// channeladd::OnAction (CWindow)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	actionId	- The action id to perform

bool channeladd::OnAction(ADDON_ACTION actionId)
{
	bool		handled = false;			// Flag if handled by the addon

	// ADDON_ACTION_REMOTE_0 - ADDON_ACTION_REMOTE_9 --> Digit keypress
	//
	if((actionId >= ADDON_ACTION_REMOTE_0) && (actionId <= ADDON_ACTION_REMOTE_9)) {

		on_digit(actionId - ADDON_ACTION_REMOTE_0);
		handled = true;
	}

	// ADDON_ACTION_NAV_BACK --> Backspace
	//
	else if(actionId == ADDON_ACTION_NAV_BACK) {

		on_backspace();
		handled = true;
	}

	// ADDON_ACTION_SELECT_ITEM --> Add
	//
	else if(actionId == ADDON_ACTION_SELECT_ITEM) {

		// If the user has entered a complete frequency and pressed ENTER
		// (or the equivalent), close the dialog as if the Add button was pressed
		if(get_frequency(m_label_input->GetLabel(), m_channelprops.frequency)) {

			m_result = handled = true;
			Close();
		}
	}

	// Skip actions that we didn't specifically handle (like mouse events)
	if(handled) {

		// Reformat and update the input control text
		std::string formatted(format_input(m_input));
		m_label_input->SetLabel(formatted);

		// Extract the frequency, and if valid, enable the Add button
		bool const hasfrequency = get_frequency(formatted, m_channelprops.frequency);
		m_button_add->SetEnabled(hasfrequency);
	}

	return (handled) ? true : kodi::gui::CWindow::OnAction(actionId);
}

//---------------------------------------------------------------------------
// channeladd::OnClick (CWindow)
//
// Receives click event notifications for a control
//
//	controlId	- GUI control identifier

bool channeladd::OnClick(int controlId)
{
	bool		handled = false;			// Flag if handled by the addon

	switch(controlId) {

		case CONTROL_BUTTON_0:
		case CONTROL_BUTTON_1:
		case CONTROL_BUTTON_2:
		case CONTROL_BUTTON_3:
		case CONTROL_BUTTON_4:
		case CONTROL_BUTTON_5:
		case CONTROL_BUTTON_6:
		case CONTROL_BUTTON_7:
		case CONTROL_BUTTON_8:
		case CONTROL_BUTTON_9:
			on_digit(controlId - CONTROL_BUTTON_0);
			handled = true;
			break;

		case CONTROL_BUTTON_BACKSPACE:
			on_backspace();
			handled = true;
			break;

		case CONTROL_BUTTON_ADD:
			m_result = true;
			Close();
			return true;

		case CONTROL_BUTTON_CLOSE:
			Close();
			return true;
	}
	
	// Skip click events that we didn't actually handle
	if(handled) {

		// Reformat and update the input control text
		std::string formatted(format_input(m_input));
		m_label_input->SetLabel(formatted);

		// Extract the frequency, and if valid, enable the Add button
		bool const hasfrequency = get_frequency(formatted, m_channelprops.frequency);
		m_button_add->SetEnabled(hasfrequency);
	}

	return (handled) ? true : kodi::gui::CWindow::OnClick(controlId);
}

//---------------------------------------------------------------------------
// channeladd::OnInit (CWindow)
//
// Called to initialize the window object
//
// Arguments:
//
//	NONE

bool channeladd::OnInit(void)
{
	try {

		// Get references to all of the manipulable dialog controls
		m_label_input = std::unique_ptr<CLabel>(new CLabel(this, CONTROL_LABEL_INPUT));
		m_button_add = std::unique_ptr<CButton>(new CButton(this, CONTROL_BUTTON_ADD));

		// Set the window title based on if this is a single channel or a multiplex/ensemble
		std::unique_ptr<CLabel> headerlabel(new CLabel(this, CONTROL_LABEL_HEADERLABEL));
		headerlabel->SetLabel(kodi::addon::GetLocalizedString(30300));

		// The add button is disabled by default
		m_button_add->SetEnabled(false);

		// Initialize the non-default channelprops structure members
		m_channelprops.modulation = m_modulation;
		m_channelprops.name = kodi::addon::GetLocalizedString(19204, "New channel");
		m_channelprops.autogain = false;
	}

	catch(...) { return false; }

	return kodi::gui::CWindow::OnInit();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
