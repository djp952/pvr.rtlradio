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

#ifndef __SOCKET_EXCEPTION_H_
#define __SOCKET_EXCEPTION_H_
#pragma once

#include <exception>
#include <sstream>
#include <string>

#ifdef _WINDOWS
#include "win32_exception.h"
#endif

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// Class socket_exception
//
// std::exception used to describe a socket exception

class socket_exception : public std::exception
{
public:

	// Instance Constructor
	//
	template<typename... _args>
	socket_exception(_args&&... args)
	{
		std::ostringstream stream;
		format_message(stream, std::forward<_args>(args)...);

	#ifdef _WINDOWS	
		stream << ": " << win32_exception(static_cast<DWORD>(WSAGetLastError()));
	#elif __clang__
		int code = errno;
		char buf[512] = {};
		if(strerror_r(code, buf, std::extent<decltype(buf)>::value) == 0) stream << ": " << buf;
		else stream << ": socket_exception code " << code;
	#else
		int code = errno;
		char buf[512] = {};
		stream << ": " << strerror_r(code, buf, std::extent<decltype(buf)>::value);
	#endif

		m_what = stream.str();
	}

	// Copy Constructor
	//
	socket_exception(socket_exception const& rhs) : m_what(rhs.m_what) {}

	// Move Constructor
	//
	socket_exception(socket_exception&& rhs) : m_what(std::move(rhs.m_what)) {}

	// char const* conversion operator
	//
	operator char const*() const
	{
		return m_what.c_str();
	}

	//-------------------------------------------------------------------------
	// Member Functions

	// what (std::exception)
	//
	// Gets a pointer to the exception message text
	virtual char const* what(void) const noexcept override
	{
		return m_what.c_str();
	}

private:

	//-------------------------------------------------------------------------
	// Private Member Functions

	// format_message
	//
	// Variadic string generator used by the constructor
	template<typename... _args>
	static void format_message(std::ostringstream& stream, _args&&... args)
	{
		int unpack[] = { 0, (static_cast<void>(stream << args), 0) ... };
		(void)unpack;
	}

	//-------------------------------------------------------------------------
	// Member Variables

	std::string					m_what;			// Exception message
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SOCKET_EXCEPTION_H_
