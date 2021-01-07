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

#ifndef __DBTYPES_H_
#define __DBTYPES_H_
#pragma once

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------------------

// DATABASE_CONNECTIONPOOL_SIZE
//
// Specifies the default size of the database connection pool
static size_t const DATABASE_CONNECTIONPOOL_SIZE = 3;

//---------------------------------------------------------------------------
// DATA TYPES
//---------------------------------------------------------------------------

// channel
//
// Information about a single channel enumerated from the database
struct channel {
	
	unsigned int		id;
	unsigned int		channel;
	unsigned int		subchannel;
	char const*			name;
	bool				hidden;
	char const*			logourl;
};

//---------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __DBTYPES_H_
