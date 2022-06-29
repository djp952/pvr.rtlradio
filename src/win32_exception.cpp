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
#include "win32_exception.h"

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// win32_exception Constructor
//
// Arguments:
//
//	code		- Win32 error code

win32_exception::win32_exception(DWORD code)
{
	char	what[512] = { '\0' };			// Formatted exception string

	// For a normal DWORD error use decimal formatting
	snprintf(what, std::extent<decltype(what)>::value, "%s (%u)", format_message(code).c_str(), code);
	m_what.assign(what);
}

//-----------------------------------------------------------------------------
// win32_exception Constructor
//
// Arguments:
//
//	code		- Win32 error code

win32_exception::win32_exception(HRESULT code)
{
	char	what[512] = { '\0' };			// Formatted exception string

	// For an HRESULT error use hexadecimal formatting
	snprintf(what, std::extent<decltype(what)>::value, "%s (0x%08X)", format_message(code).c_str(), code);
	m_what.assign(what);
}

//-----------------------------------------------------------------------------
// win32_exception Copy Constructor

win32_exception::win32_exception(win32_exception const& rhs) : m_what(rhs.m_what)
{
}

//-----------------------------------------------------------------------------
// win32_exception Move Constructor

win32_exception::win32_exception(win32_exception&& rhs) : m_what(std::move(rhs.m_what))
{
}

//-----------------------------------------------------------------------------
// win32_exception char const* conversion operator

win32_exception::operator char const*() const
{
	return m_what.c_str();
}

//-----------------------------------------------------------------------------
// win32_exception::what
//
// Gets a pointer to the exception message text
//
// Arguments:
//
//	NONE

char const* win32_exception::what(void) const noexcept
{
	return m_what.c_str();
}

//-----------------------------------------------------------------------------
// win32_exception::format_message (private, static)
//
// Generates the formatted message string
//
// Arguments:
//
//	result		- Windows system error code for which to generate the message

std::string win32_exception::format_message(DWORD result)
{
	LPSTR					formatted = nullptr;		// Allocated string from ::FormatMessageA
	std::string				message;					// std::string version of the formatted message

	// Attempt to format the message from the current module resources
	DWORD cchformatted = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		result, GetThreadUILanguage(), reinterpret_cast<LPSTR>(&formatted), 0, nullptr);
	if(cchformatted == 0) {

		// The message could not be looked up in the specified module; generate the default message instead
		if(formatted) { LocalFree(formatted); formatted = nullptr; }
		cchformatted = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			"win32_exception code %1!lu!", 0, 0, reinterpret_cast<LPSTR>(&formatted), 0, reinterpret_cast<va_list*>(&result));
	}

	// If a formatted message was generated, assign it to the std::string and release it
	if(cchformatted > 0) message.assign(formatted, cchformatted);
	if(formatted) LocalFree(formatted);

	// Return the formatted message to the caller
	return message;
}

//-----------------------------------------------------------------------------

#pragma warning(pop)
