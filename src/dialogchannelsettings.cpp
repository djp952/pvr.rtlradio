//---------------------------------------------------------------------------
// Copyright (c) 2020 Michael G. Brehm
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
#include "dialogchannelsettings.h"

#include <assert.h>

#include "string_exception.h"

#pragma warning(push, 4)

// channelsettings.xml
//
#define CONTROL_BUTTON_OK		1

//---------------------------------------------------------------------------
// dialogchannelsettings Constructor (private)
//
// Arguments:
//
//	gui			- Pointer to the Kodi GUI library instance
//	window		- Pointer to the CAddonGUIWindow instance
//	scanner		- scanner object instance

dialogchannelsettings::dialogchannelsettings(CHelper_libKODI_guilib* gui, CAddonGUIWindow* window, std::unique_ptr<scanner> scanner) : 
	m_gui(gui), m_window(window), m_scanner(std::move(scanner))
{
	assert(gui != nullptr);
	assert(window != nullptr);

	m_window->m_cbhdl = this;
	m_window->CBOnAction = onaction;
	m_window->CBOnClick = onclick;
	m_window->CBOnFocus = onfocus;
	m_window->CBOnInit = oninit;
}

//---------------------------------------------------------------------------
// dialogchannelsettings Destructor

dialogchannelsettings::~dialogchannelsettings()
{
}

//---------------------------------------------------------------------------
// dialogchannelsettings::show (static)
//
// Executes the window as a modal dialog box
//
// Arguments:
//
//	gui				- Pointer to the CHelper_libKODI_guilib interface
//	scanner			- scanner object instance
//	channelprops	- Properties of the channel to be manipulated

bool dialogchannelsettings::show(std::unique_ptr<CHelper_libKODI_guilib> const& gui, std::unique_ptr<scanner> scanner,
	struct channelprops &channelprops)
{
	CAddonGUIWindow*			window = nullptr;		// Kodi window instance
	bool						result = false;			// Result from dialog operation

	assert(gui);
	assert(scanner);

	// Create the Kodi window instance
	window = gui->Window_create("channelsettings.xml", "skin.estuary", false, true);
	if(window == nullptr) throw string_exception(__func__, ": failed to create Kodi Window object instance");

	try {

		// Create the dialog class instance
		std::unique_ptr<dialogchannelsettings> dialog(new dialogchannelsettings(gui.get(), window, std::move(scanner)));

		// Copy the channel properties into the dialog class instance
		dialog->m_channelprops = channelprops;

		// Run the dialog as a modal dialog
		window->DoModal();

		// Check if the user changed the settings and copy them back to the caller
		result = dialog->m_changed;
		if(dialog->m_changed) channelprops = dialog->m_channelprops;

		// Destroy the dialog class instance
		dialog.reset(nullptr);

		// Destroy the Kodi window instance
		gui->Window_destroy(window);
	}

	// Destroy the window instance on any thrown exception
	catch(...) { gui->Window_destroy(window); throw; }

	return result;
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onaction (static, private)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	handle		- Window instance handle specified during construction
//	actionid	- Action code to be handled by the window

bool dialogchannelsettings::onaction(GUIHANDLE handle, int actionid)
{
	assert(handle != nullptr);
	return reinterpret_cast<dialogchannelsettings*>(handle)->onaction(actionid);
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onaction (private)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	actionid	- Action code to be handled by the window

bool dialogchannelsettings::onaction(int actionid)
{
	switch(actionid) {

		case ADDON_ACTION_CLOSE_DIALOG:
		case ADDON_ACTION_PREVIOUS_MENU:
		case ADDON_ACTION_NAV_BACK:
			return onclick(CONTROL_BUTTON_OK);
	}

	return false;
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onclick (static, private)
//
// Receives click events that are sent to this window
//
// Arguments:
//
//	handle		- Window instance handle specified during construction
//	controlid	- Control identifier receiving the click event

bool dialogchannelsettings::onclick(GUIHANDLE handle, int controlid)
{
	assert(handle != nullptr);
	return reinterpret_cast<dialogchannelsettings*>(handle)->onclick(controlid);
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onclick (private)
//
// Receives click events that are sent to this window
//
// Arguments:
//
//	controlid	- Control identifier receiving the click event

bool dialogchannelsettings::onclick(int controlid)
{
	switch(controlid) {

		case CONTROL_BUTTON_OK:
			m_window->Close();
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onfocus (static, private)
//
// Receives focus events that are sent to this window
//
// Arguments:
//
//	handle		- Window instance handle specified during construction
//	controlid	- Control identifier receiving the focus event

bool dialogchannelsettings::onfocus(GUIHANDLE handle, int controlid)
{
	assert(handle != nullptr);
	return reinterpret_cast<dialogchannelsettings*>(handle)->onfocus(controlid);
}

//---------------------------------------------------------------------------
// dialogchannelsettings::onfocus (private)
//
// Receives focus events that are sent to this window
//
// Arguments:
//
//	controlid	- Control identifier receiving the focus event

bool dialogchannelsettings::onfocus(int /*controlid*/)
{
	return true;
}

//---------------------------------------------------------------------------
// dialogchannelsettings::oninit (static, private)
//
// Called to initialize the window object
//
// Arguments:
//
//	handle		- Window instance handle specified during construction

bool dialogchannelsettings::oninit(GUIHANDLE handle)
{
	assert(handle != nullptr);
	return reinterpret_cast<dialogchannelsettings*>(handle)->oninit();
}

//---------------------------------------------------------------------------
// dialogchannelsettings::oninit (private)
//
// Called to initialize the window object
//
// Arguments:
//
//	NONE

bool dialogchannelsettings::oninit(void)
{
	// warning cleanup
	if(m_gui == nullptr) return false;

	return true;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
