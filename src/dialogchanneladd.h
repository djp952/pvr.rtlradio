//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __DIALOGCHANNELADD_H_
#define __DIALOGCHANNELADD_H_
#pragma once

#include <libKODI_guilib.h>
#include <memory>

#include "scanner.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class dialogchanneladd
//
// Implements the "Add Channel" dialog

class dialogchanneladd
{
public:

	// Destructor
	//
	~dialogchanneladd();

	//-----------------------------------------------------------------------
	// Member Functions

	// show (static)
	//
	// Executes the dialog as a modal dialog box
	static void show(std::unique_ptr<CHelper_libKODI_guilib> const& gui, std::unique_ptr<scanner> scanner);

private:

	dialogchanneladd(dialogchanneladd const&) = delete;
	dialogchanneladd& operator=(dialogchanneladd const&) = delete;

	// Instance Constructor
	//
	dialogchanneladd(CHelper_libKODI_guilib* gui, CAddonGUIWindow* window, std::unique_ptr<scanner> scanner);

	//-------------------------------------------------------------------------
	// Private Member Functions

	// onaction
	//
	// Receives action codes that are sent to this window
	static bool onaction(GUIHANDLE handle, int actionid);
	bool onaction(int actionid);

	// onclick
	//
	// Receives click event notifications for a control
	static bool onclick(GUIHANDLE handle, int controlid);
	bool onclick(int controlid);

	// onfocus
	//
	// Receives focus event notifications for a control
	static bool onfocus(GUIHANDLE handle, int controlid);
	bool onfocus(int controlid);

	// oninit
	//
	// Called to initialize the window object
	static bool oninit(GUIHANDLE handle);
	bool oninit(void);

	//-------------------------------------------------------------------------
	// Member Variables

	CHelper_libKODI_guilib* const	m_gui = nullptr;		// Kodi GUI API instance
	CAddonGUIWindow* const			m_window = nullptr;		// Kodi GUI window instance
	std::unique_ptr<scanner>		m_scanner;				// Channel scanner instance
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DIALOGCHANNELADD_H_
