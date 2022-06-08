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

#ifndef __CHANNELADD_H_
#define __CHANNELADD_H_
#pragma once

#include <kodi/gui/controls/Button.h>
#include <kodi/gui/controls/Label.h>
#include <kodi/gui/Window.h>

#include "props.h"

#pragma warning(push, 4)

using namespace kodi::gui::controls;

//---------------------------------------------------------------------------
// Class channeladd
//
// Implements the "Add Channel" dialog

class ATTR_DLL_LOCAL channeladd : public kodi::gui::CWindow
{
public:

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new channelsettings instance
	static std::unique_ptr<channeladd> create(enum modulation modulation);

	// get_channel_properties
	//
	// Gets the updated channel properties from the dialog box
	void get_channel_properties(struct channelprops& channelprops) const;

	// get_dialog_result
	//
	// Gets the result code from the dialog box
	bool get_dialog_result(void) const;

private:

	channeladd(channeladd const&) = delete;
	channeladd& operator=(channeladd const&) = delete;

	// Instance Constructor
	//
	channeladd(enum modulation modulation);

	//-------------------------------------------------------------------------
	// CWindow Implementation

	// OnAction
	//
	// Receives action codes that are sent to this window
	bool OnAction(ADDON_ACTION actionId) override;

	// OnClick
	//
	// Receives click event notifications for a control
	bool OnClick(int controlId) override;

	// OnInit
	//
	// Called to initialize the window object
	bool OnInit(void) override;

	//-------------------------------------------------------------------------
	// Private Member Functions

	// format_input
	//
	// Formats the current input string for display
	std::string format_input(std::string const& input) const;

	// get_frequency
	//
	// Attempts to parse a formatted input string to get the frequency
	bool get_frequency(std::string const& input, uint32_t& frequency) const;

	// on_backspace
	//
	// Handles input of a backspace character
	void on_backspace(void);

	// on_digit
	//
	// Handles input of a digit character
	void on_digit(int number);

	//-------------------------------------------------------------------------
	// Member Variables

	enum modulation const		m_modulation;				// Channel modulation
	struct channelprops			m_channelprops = {};		// Channel properties
	bool						m_result = false;			// Dialog result
	std::string					m_input;					// Input string

	// CONTROLS
	//
	std::unique_ptr<CLabel>		m_label_input;				// Input
	std::unique_ptr<CButton>	m_button_add;				// Add button
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __CHANNELADD_H_
