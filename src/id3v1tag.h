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

#ifndef __ID3V1TAG_H_
#define __ID3V1TAG_H_
#pragma once

#include <memory>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class id3v1tag
//
// Implements a simple ID3v1 tag generator

class id3v1tag
{
public:

	// Destructor
	//
	~id3v1tag();

	//-----------------------------------------------------------------------
	// Member Functions

	// album
	//
	// Sets the ALBUM tag element
	void album(char const* album);

	// artist
	//
	// Sets the ARTIST tag element
	void artist(char const* artist);

	// comment
	//
	// Sets the COMMENT tag element
	void comment(char const* comment);

	// create (static)
	//
	// Factory method, creates a new id3v1tag instance
	static std::unique_ptr<id3v1tag> create(void);
	static std::unique_ptr<id3v1tag> create(uint8_t const* data, size_t length);

	// genre
	//
	// Sets the GENRE tag element
	void genre(uint8_t genre);
	void genre(char const* genre);

	// size
	//
	// Gets the number of bytes required to persist the tag to storage
	size_t size(void) const;

	// song
	//
	// Sets the SONG tag element
	void song(char const* song);

	// track
	//
	// Sets the TRACK tag element
	void track(uint8_t track);

	// write
	//
	// Writes the tag into a memory buffer
	bool write(uint8_t* buffer, size_t length) const;

	// year
	//
	// Sets the YEAR tag element
	void year(char const* year);

private:

	id3v1tag(id3v1tag const&) = delete;
	id3v1tag& operator=(id3v1tag const&) = delete;

	// id3v1_tag_t
	//
	// Defines the ID3v1 tag structure
	struct id3v1_tag_t {

		uint8_t			id[3];
		uint8_t			song[30];
		uint8_t			artist[30];
		uint8_t			album[30];
		uint8_t			year[4];
		uint8_t			comment[30];
		uint8_t			genre;
	};

	static_assert(sizeof(id3v1_tag_t) == 128, "id3v1_tag_t must be 128 bytes in length");

	// Instance Constructors
	//
	id3v1tag();
	id3v1tag(uint8_t const* data, size_t length);

	// UNSPECIFIED_GENRE
	//
	// Flag indicating that a genre is unspecified
	static uint8_t const UNSPECIFIED_GENRE;

	//-----------------------------------------------------------------------
	// Member Variables

	id3v1_tag_t					m_tag = {};			// ID3v1 tag information
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __ID3V1TAG_H_
