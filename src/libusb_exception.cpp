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
#include "libusb_exception.h"

#pragma warning(disable:4200)
#include <libusb.h>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// libusb_exception Constructor
//
// Arguments:
//
//	code		- SQLite error code
//	message		- Message to associate with the exception

libusb_exception::libusb_exception(int code)
{
	char	what[512] = { '\0' };			// Formatted exception string

	snprintf(what, std::extent<decltype(what)>::value, "%s (%d) : %s", 
		libusb_error_name(code), code, libusb_strerror(static_cast<libusb_error>(code)));

	m_what.assign(what);
}

//-----------------------------------------------------------------------------
// libusb_exception Copy Constructor

libusb_exception::libusb_exception(libusb_exception const& rhs) : m_what(rhs.m_what)
{
}

//-----------------------------------------------------------------------------
// libusb_exception Move Constructor

libusb_exception::libusb_exception(libusb_exception&& rhs) : m_what(std::move(rhs.m_what))
{
}

//-----------------------------------------------------------------------------
// libusb_exception char const* conversion operator

libusb_exception::operator char const*() const
{
	return m_what.c_str();
}

//-----------------------------------------------------------------------------
// libusb_exception::what
//
// Gets a pointer to the exception message text
//
// Arguments:
//
//	NONE

char const* libusb_exception::what(void) const noexcept
{
	return m_what.c_str();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
