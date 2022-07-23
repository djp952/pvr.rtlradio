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

#ifndef __MULTISELECT_H_
#define __MULTISELECT_H_
#pragma once

#include <kodi/gui/Window.h>
#include <vector>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class multiselect
//
// Implements the "Add Channel" dialog

class ATTR_DLL_LOCAL multiselect : public kodi::gui::CWindow
{
public:

	//-----------------------------------------------------------------------
	// Type Declarations

	// entry
	//
	// Structure used to define a selection entry
	struct entry {

		int				id;					// Numeric identifier
		std::string		label;				// Label
		bool			selected;			// Selected flag
	};

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new channelsettings instance
	static std::unique_ptr<multiselect> create(std::vector<struct entry> const& entries);

	// get_dialog_result
	//
	// Gets the result code from the dialog box
	bool get_dialog_result(void) const;

private:

	multiselect(multiselect const&) = delete;
	multiselect& operator=(multiselect const&) = delete;

	// Instance Constructor
	//
	multiselect(std::vector<struct entry> const& entries);

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

	//-------------------------------------------------------------------------
	// Member Variables

	std::vector<struct entry>	m_entries;				// List entries
	bool						m_result = false;		// Dialog result
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __MULTISELECT_H_
