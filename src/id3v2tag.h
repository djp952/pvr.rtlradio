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

#ifndef __ID3V2TAG_H_
#define __ID3V2TAG_H_
#pragma once

#include <memory>
#include <vector>

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class id3v2tag
//
// Implements a simple ID3v2 tag generator

class id3v2tag
{
public:

	// Destructor
	//
	~id3v2tag();

	//-----------------------------------------------------------------------
	// Member Functions

	// album
	//
	// Sets the album (TALB) frame
	void album(char const* album);

	// artist
	//
	// Sets or append an artist (TPE1) frame
	void artist(char const* artist, bool append = false);

	// comment
	//
	// Sets the comment (COMM; no content descriptor) frame
	void comment(char const* comment);

	// coverart
	//
	// Sets the cover art (APIC; type 0x03) frame
	void coverart(char const* mimetype, uint8_t const* data, size_t length);

	// create (static)
	//
	// Factory method, creates a new id3v2tag instance
	static std::unique_ptr<id3v2tag> create(void);
	static std::unique_ptr<id3v2tag> create(uint8_t const* data, size_t length);

	// genre
	//
	// Sets or append an genre (TCON) frame
	void genre(char const* artist, bool append = false);

	// size
	//
	// Gets the number of bytes required to persist the tag to storage
	size_t size(void) const;

	// title
	//
	// Sets the title (TIT2) frame
	void title(char const* title);

	// track
	//
	// Sets the track (TRCK) frame
	void track(char const* track);

	// write
	//
	// Writes the tag into a memory buffer
	bool write(uint8_t* buffer, size_t length) const;

	// year
	//
	// Sets the year (TYER) frame
	void year(char const* year);

private:

	id3v2tag(id3v2tag const&) = delete;
	id3v2tag& operator=(id3v2tag const&) = delete;

	// synchsafe32_t
	//
	// A 32-bit synchsafe integer
	using synchsafe32_t = uint8_t[4];

	// id3v2_header_t
	//
	// Defines the ID3v2 header structure
#ifdef _WINDOWS
#pragma pack(push, 1)
	struct id3v2_header_t {

		uint8_t			id[3];
		uint8_t			version;
		uint8_t			revision;
		uint8_t			footer : 1;
		uint8_t			experimental : 1;
		uint8_t			extendedheader : 1;
		uint8_t			unsynchronization : 1;
		uint8_t			reserved : 4;
		synchsafe32_t	size;
	};

	struct id3v2_extended_header_t {

		synchsafe32_t	size;
		uint8_t			bytes;
		uint8_t			flags;
	};

	struct id3v2_frame_header_t {

		uint8_t			id[4];
		synchsafe32_t	size;
		uint8_t			flags[2];
	};
#pragma pack(pop)
#else
	struct id3v2_header_t {

		uint8_t			id[3];
		uint8_t			version;
		uint8_t			revision;
		uint8_t			footer : 1;
		uint8_t			experimental : 1;
		uint8_t			extendedheader : 1;
		uint8_t			unsynchronization : 1;
		uint8_t			reserved : 4;
		synchsafe32_t	size;
	} __attribute__((packed));

	struct id3v2_extended_header_t {

		synchsafe32_t	size;
		uint8_t			bytes;
		uint8_t			flags;
	} __attribute__((packed));

	struct id3v2_frame_header_t {

		uint8_t			id[4];
		synchsafe32_t	size;
		uint8_t			flags[2];
	} __attribute__((packed));
#endif

	static_assert(sizeof(id3v2_header_t) == 10, "id3v2_header_t must be 10 bytes in length");
	static_assert(sizeof(id3v2_extended_header_t) == 6, "id3v2_extended_header_t must be 6 bytes in length");
	static_assert(sizeof(id3v2_frame_header_t) == 10, "id3v2_frame_header_t must be 10 bytes in length");

	// id3v2_frameid_t
	//
	// An ID3v2 frame identifier
	using id3v2_frameid_t = char[4];

	// frame_t
	//
	// Internal representation of an ID3v2 frame object
	struct frame_t {

		id3v2_frameid_t				id;
		uint8_t						flags[2];
		size_t						size;
		std::unique_ptr<uint8_t[]>	data;
	};

	// frame_vector_t
	//
	// Internal representation of the collection of ID3v2 frame objects
	using frame_vector_t = std::vector<frame_t>;

	// Instance Constructors
	//
	id3v2tag();
	id3v2tag(uint8_t const* data, size_t length);

	//-----------------------------------------------------------------------
	// Private Member Functions

	// add_text_frame
	//
	// Adds a text frame
	void add_text_frame(id3v2_frameid_t frameid, char const* text, bool append = false);

	// decode_synchsafe
	//
	// Decodes a 32-bit synchsafe integer
	static uint32_t decode_synchsafe(synchsafe32_t const& synchsafe);

	// encode_synchsafe
	//
	// Encodes a 32-bit synchsafe integer
	static void encode_synchsafe(uint32_t val, synchsafe32_t& synchsafe);

	// remove_frames
	//
	// Removes all instances of a specific frame from the stored state
	void remove_frames(id3v2_frameid_t frameid);

	//-----------------------------------------------------------------------
	// Member Variables

	frame_vector_t				m_frames;			// vector<> of tag frames
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __ID3V2TAG_H_
