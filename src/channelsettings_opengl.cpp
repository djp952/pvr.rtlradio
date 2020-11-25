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
#include "channelsettings.h"

#include <kodi/gui/gl/GL.h>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// channelsettings::signal_meter_oncreate (private)
//
// Creates the rendering control for Kodi
//
// Arguments:
//
//	x		- Horizontal position
//	y		- Vertical position
//	w		- Width of control
//	h		- Height of control
//	device	- The device to use

bool channelsettings::signal_meter_oncreate(int /*x*/, int /*y*/, int /*w*/, int /*h*/, kodi::HardwareContext /*device*/)
{
	return true;
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_ondirty (private)
//
// Determines if a render region is dirty
//
// Arguments:
//
//	NONE

bool channelsettings::signal_meter_ondirty(void)
{
	return false;
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_onrender (private)
//
// Invoked to render the control
//
// Arguments:
//
//	NONE

void channelsettings::signal_meter_onrender(void)
{
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_onstop (private)
//
// Invoked to stop the rendering process
//
// Arguments:
//
//	NONE

void channelsettings::signal_meter_onstop(void)
{
}

//---------------------------------------------------------------------------

#pragma warning(pop)
