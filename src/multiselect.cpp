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
#include "multiselect.h"

#include <assert.h>
#include <kodi/General.h>

#pragma warning(push, 4)

// Control Identifiers
//
//static const int CONTROL_LABEL_HEADERLABEL	= 1;
static const int CONTROL_BUTTON_OK			= 100;
static const int CONTROL_BUTTON_CANCEL		= 101;
//static const int CONTROL_BUTTON_0			= 200;
//static const int CONTROL_BUTTON_1			= 201;
//static const int CONTROL_BUTTON_2			= 202;
//static const int CONTROL_BUTTON_3			= 203;
//static const int CONTROL_BUTTON_4			= 204;
//static const int CONTROL_BUTTON_5			= 205;
//static const int CONTROL_BUTTON_6			= 206;
//static const int CONTROL_BUTTON_7			= 207;
//static const int CONTROL_BUTTON_8			= 208;
//static const int CONTROL_BUTTON_9			= 209;
//static const int CONTROL_BUTTON_BACKSPACE	= 210;
//static const int CONTROL_LABEL_INPUT		= 300;

//---------------------------------------------------------------------------
// multiselect Constructor (private)
//
// Arguments:
//
//	entries		- vector<> of entries to display in the list

multiselect::multiselect(std::vector<struct entry> const& entries) : kodi::gui::CWindow("multiselect.xml", "skin.estuary", true),
	m_entries(entries)
{
}

//---------------------------------------------------------------------------
// multiselect::create (static)
//
// Factory method, creates a new multiselect instance
//
// Arguments:
//
//	entries		- vector<> of entries to display in the list

std::unique_ptr<multiselect> multiselect::create(std::vector<struct entry> const& entries)
{
	return std::unique_ptr<multiselect>(new multiselect(entries));
}

//---------------------------------------------------------------------------
// multiselect::get_dialog_result
//
// Gets the result code from the dialog box
//
// Arguments:
//
//	NONE

bool multiselect::get_dialog_result(void) const
{
	return m_result;
}

//---------------------------------------------------------------------------
// CWINDOW IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// multiselect::OnAction (CWindow)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	actionId	- The action id to perform

bool multiselect::OnAction(ADDON_ACTION actionId)
{
	bool		handled = false;			// Flag if handled by the addon

	//// ADDON_ACTION_REMOTE_0 - ADDON_ACTION_REMOTE_9 --> Digit keypress
	////
	//if((actionId >= ADDON_ACTION_REMOTE_0) && (actionId <= ADDON_ACTION_REMOTE_9)) {

	//	on_digit(actionId - ADDON_ACTION_REMOTE_0);
	//	handled = true;
	//}

	//// ADDON_ACTION_NAV_BACK --> Backspace
	////
	//else if(actionId == ADDON_ACTION_NAV_BACK) {

	//	on_backspace();
	//	handled = true;
	//}

	//// ADDON_ACTION_SELECT_ITEM --> Add
	////
	//else if(actionId == ADDON_ACTION_SELECT_ITEM) {

	//	// If the user has entered a complete frequency and pressed ENTER
	//	// (or the equivalent), close the dialog as if the Add button was pressed
	//	if(get_frequency(m_label_input->GetLabel(), m_channelprops.frequency)) {

	//		m_result = handled = true;
	//		Close();
	//	}
	//}

	//// Skip actions that we didn't specifically handle (like mouse events)
	//if(handled) {

	//	// Reformat and update the input control text
	//	std::string formatted(format_input(m_input));
	//	m_label_input->SetLabel(formatted);

	//	// Extract the frequency, and if valid, enable the Add button
	//	bool const hasfrequency = get_frequency(formatted, m_channelprops.frequency);
	//	m_button_add->SetEnabled(hasfrequency);
	//}

	return (handled) ? true : kodi::gui::CWindow::OnAction(actionId);
}

//---------------------------------------------------------------------------
// multiselect::OnClick (CWindow)
//
// Receives click event notifications for a control
//
//	controlId	- GUI control identifier

bool multiselect::OnClick(int controlId)
{
	bool		handled = false;			// Flag if handled by the addon

	switch(controlId) {

		case CONTROL_BUTTON_OK:
			m_result = true;
			Close();
			handled = true;
			break;

		case CONTROL_BUTTON_CANCEL:
			Close();
			handled = true;
			break;
	}

	return (handled) ? true : kodi::gui::CWindow::OnClick(controlId);
}

//---------------------------------------------------------------------------
// multiselect::OnInit (CWindow)
//
// Called to initialize the window object
//
// Arguments:
//
//	NONE

bool multiselect::OnInit(void)
{
	try {

		//// Get references to all of the manipulable dialog controls
		//m_label_input = std::unique_ptr<CLabel>(new CLabel(this, CONTROL_LABEL_INPUT));
		//m_button_add = std::unique_ptr<CButton>(new CButton(this, CONTROL_BUTTON_ADD));

		//// Set the window title based on if this is a single channel or a multiplex/ensemble
		//std::unique_ptr<CLabel> headerlabel(new CLabel(this, CONTROL_LABEL_HEADERLABEL));
		//headerlabel->SetLabel(kodi::addon::GetLocalizedString(30300));

		//// The add button is disabled by default
		//m_button_add->SetEnabled(false);
	}

	catch(...) { return false; }

	return kodi::gui::CWindow::OnInit();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
