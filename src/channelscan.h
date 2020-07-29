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

#ifndef __CHANNELSCAN_H_
#define __CHANNELSCAN_H_
#pragma once

#include <kodi/gui/Window.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class channelscan
//
// Implements the "Channel Settings" dialog

class ATTRIBUTE_HIDDEN channelscan : public kodi::gui::CWindow
{
public:

	// Instance Constructor
	//
	channelscan();

	// Destructor
	//
	virtual ~channelscan();

	//-----------------------------------------------------------------------
	// Member Functions

private:

	channelscan(channelscan const&) = delete;
	channelscan& operator=(channelscan const&) = delete;

	//-------------------------------------------------------------------------
	// Private Member Functions

	// GetContextButtons
	//
	// Get context menu buttons for list entry
	void GetContextButtons(int itemNumber, std::vector<std::pair<unsigned int, std::string>>& buttons) override;

	// OnAction
	//
	// Receives action codes that are sent to this window
	bool OnAction(int actionId, uint32_t buttoncode, wchar_t unicode) override;

	// OnClick
	//
	// Receives click event notifications for a control
	bool OnClick(int controlId) override;

	// OnContextButton
	//
	// Called after selection in context menu
	bool OnContextButton(int itemNumber, unsigned int button) override;

	// OnFocus
	//
	// Receives focus event notifications for a control
	bool OnFocus(int controlId) override;

	// OnInit
	//
	// Called to initialize the window object
	bool OnInit(void) override;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __CHANNELSCAN_H_
