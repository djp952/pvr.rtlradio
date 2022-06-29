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

#ifndef __WIN32_EXCEPTION_H_
#define __WIN32_EXCEPTION_H_
#pragma once

#include <exception>
#include <string>

#pragma warning(push, 4)	

//-----------------------------------------------------------------------------
// Class win32_exception
//
// Exception-based class used to wrap Windows system error codes

class win32_exception : public std::exception
{
public:

	// Instance Constructors
	//
	win32_exception(DWORD code);
	win32_exception(HRESULT code);

	// Copy Constructor
	//
	win32_exception(win32_exception const& rhs);

	// Move Constructor
	//
	win32_exception(win32_exception&& rhs);

	// char const* conversion operator
	//
	operator char const*() const;

	//-------------------------------------------------------------------------
	// Member Functions

	// what (std::exception)
	//
	// Gets a pointer to the exception message text
	virtual char const* what(void) const noexcept override;

private:

	//-------------------------------------------------------------------------
	// Private Member Functions

	// format_message
	//
	// Generates the formatted message string from the project resources
	static std::string format_message(DWORD result);

	//-------------------------------------------------------------------------
	// Member Variables

	std::string					m_what;			// Win32 error message
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __WIN32_EXCEPTION_H_
