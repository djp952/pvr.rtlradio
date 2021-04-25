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
#include "channeladd.h"

#pragma warning(push, 4)

// Control Identifiers
//
static const int CONTROL_BUTTON_OK = 100;
static const int CONTROL_BUTTON_CANCEL = 101;

//---------------------------------------------------------------------------
// channeladd Constructor (private)
//
// Arguments:
//
//	NONE

channeladd::channeladd() : kodi::gui::CWindow("channeladd.xml", "skin.estuary", true)
{
}

//---------------------------------------------------------------------------
// channeladd::create (static)
//
// Factory method, creates a new channeladd instance
//
// Arguments:
//
//	NONE

std::unique_ptr<channeladd> channeladd::create(void)
{
	return std::unique_ptr<channeladd>(new channeladd());
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
	return kodi::gui::CWindow::OnAction(actionId);
}

//---------------------------------------------------------------------------
// channeladd::OnClick (CWindow)
//
// Receives click event notifications for a control
//
//	controlId	- GUI control identifier

bool channeladd::OnClick(int controlId)
{
	switch(controlId) {

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

	}

	catch(...) { return false; }

	return kodi::gui::CWindow::OnInit();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
