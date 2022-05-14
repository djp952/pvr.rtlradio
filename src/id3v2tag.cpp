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
#include "id3v2tag.h"

#include <assert.h>
#include <stdexcept>
#include <string.h>

#include "string_exception.h"

#ifdef _WINDOWS
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif	// _WINDOWS

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// id3v2tag Constructor (private)
//
// Arguments:
//
//	NONE

id3v2tag::id3v2tag()
{
}

//---------------------------------------------------------------------------
// id3v2tag Constructor (private)
//
// Arguments:
//
//	data		- Pointer to existing ID3v2 tag data
//	length		- Length of existing ID3v2 tag data

id3v2tag::id3v2tag(uint8_t const* data, size_t length)
{
	if(data == nullptr) throw std::invalid_argument("data");
	if(length < sizeof(id3v2_header_t)) throw std::invalid_argument("length");

	// Cast a pointer to the ID3v2 tag header and check some invariants
	id3v2_header_t const* header = reinterpret_cast<id3v2_header_t const*>(data);

	if(memcmp(&header->id, "ID3", 3) != 0) throw string_exception("invalid ID3v2 header");
	if(header->version > 4) throw string_exception("invalid ID3v2 version");
	if(length < (sizeof(id3v2_header_t) + decode_synchsafe(header->size))) throw string_exception("truncated ID3v2 tag");

	// Skip over the header and extended header (if present)
	size_t offset = sizeof(id3v2_header_t) + ((header->extendedheader) ?
		decode_synchsafe(reinterpret_cast<id3v2_extended_header_t const*>(&data[sizeof(id3v2_header_t)])->size) : 0);

	// Disregard the footer (if present), its length is the same as the header (10 bytes)
	if(header->footer) length -= sizeof(id3v2_header_t);

	// Generate a vector<> of the existing ID3 frame tag data
	while(offset < length) {

		id3v2_frame_header_t const* frame = reinterpret_cast<id3v2_frame_header_t const*>(&data[offset]);
		uint32_t framesize = decode_synchsafe(frame->size);

		frame_t newframe = {};
		memcpy(&newframe.id, &frame->id, sizeof(id3v2_frameid_t));
		newframe.flags[0] = frame->flags[0];
		newframe.flags[1] = frame->flags[1];
		newframe.size = framesize;
		newframe.data = std::unique_ptr<uint8_t[]>(new uint8_t[newframe.size]);
		memcpy(newframe.data.get(), &data[offset + sizeof(id3v2_frame_header_t)], newframe.size);
		m_frames.emplace_back(std::move(newframe));

		offset += sizeof(id3v2_frame_header_t) + framesize;
	}

	// All of the existing ID3v2 tag information should have been ingested successfully ...
	assert(offset == length);
	if(offset != length) throw string_exception("truncated ID3v2 tag");
}

//---------------------------------------------------------------------------
// id3v2tag Destructor

id3v2tag::~id3v2tag()
{
}

//---------------------------------------------------------------------------
// id3v2tag::add_text_frame (private)
//
// Adds an ID3v2 text frame
//
// Arguments:
//
//	frameid		- Frame identifier
//	text		- ISO-8859-1 text string
//	append		- Flag to append a new tag rather than replace existing tag

void id3v2tag::add_text_frame(id3v2_frameid_t frameid, char const* text, bool append)
{
	assert(frameid[0] == 'T');

	if(!append) remove_frames(frameid);

	if(text == nullptr) return;
	size_t textlength = strlen(text);

	// encoding | text | terminator
	frame_t frame = {};
	memcpy(frame.id, frameid, sizeof(id3v2_frameid_t));
	frame.size = 1 + textlength + 1;
	frame.data = std::unique_ptr<uint8_t[]>(new uint8_t[frame.size]);

	frame.data[0] = 0x00;											// ISO-8859-1
	if(textlength > 0) memcpy(&frame.data[1], text, textlength);	// Text
	frame.data[textlength + 1] = 0x00;								// NULL terminator

	m_frames.emplace_back(std::move(frame));
}

//---------------------------------------------------------------------------
// id3v2tag::album
//
// Sets the album (TALB) frame
//
// Arguments:
//
//	album		- ISO-8859-1 text string to set for the frame

void id3v2tag::album(char const* album)
{
	id3v2_frameid_t frameid{ 'T', 'A', 'L', 'B' };
	add_text_frame(frameid, album, false);
}

//---------------------------------------------------------------------------
// id3v2tag::artist
//
// Sets or appends an artist (TPE1) frame
//
// Arguments:
//
//	artist		- ISO-8859-1 text string to set for the frame
//	append		- Flag to append a new tag rather than replace existing

void id3v2tag::artist(char const* artist, bool append)
{
	id3v2_frameid_t frameid{ 'T', 'P', 'E', '1' };
	add_text_frame(frameid, artist, append);
}

//---------------------------------------------------------------------------
// id3v2tag::comment
//
// Sets the comment (COMM; no content descriptor) frame
//
// Arguments:
//
//	comment			- ISO-8859-1 text string to set for the frame

void id3v2tag::comment(char const* comment)
{
	id3v2_frameid_t frameid{ 'C', 'O', 'M', 'M' };

	// Remove any existing COMM frames with no description from the tag
	frame_vector_t::iterator it = m_frames.begin();
	while(it != m_frames.end()) {

		if(strncasecmp(it->id, frameid, sizeof(id3v2_frameid_t)) == 0) {

			// If the fourth byte of the COMM frame is zero ([0] = encoding, [1-3] = language),
			// this is a main/no description comment, remove it
			if((it->size >= 4) && (it->data[4] == 0x00)) it = m_frames.erase(it);
		}
		else it++;
	}

	if(comment == nullptr) return;
	size_t commentlength = strlen(comment);

	// encoding | language | content descriptor | text | terminator
	frame_t frame = {};
	memcpy(frame.id, frameid, sizeof(id3v2_frameid_t));
	frame.size = 1 + 3 + 1 + commentlength + 1;
	frame.data = std::unique_ptr<uint8_t[]>(new uint8_t[frame.size]);

	frame.data[0] = 0x00;									// ISO-8859-1
	memcpy(&frame.data[1], "und", 3);						// ISO-639-2
	frame.data[4] = 0x00;									// NULL descriptor
	memcpy(&frame.data[5], comment, commentlength);			// Text
	frame.data[5 + commentlength] = 0x00;					// NULL terminator

	m_frames.emplace_back(std::move(frame));
}

//---------------------------------------------------------------------------
// id3v2tag::coverart
//
// Sets the cover art (APIC; type 0x03) frame
//
// Arguments:
//
//	mimetype		- MIME type of the cover art image
//	data			- Pointer to the cover art image data
//	length			- Length of the cover art image data

void id3v2tag::coverart(char const* mimetype, uint8_t const* data, size_t length)
{
	id3v2_frameid_t frameid{ 'A', 'P', 'I', 'C' };

	// Remove any existing APIC cover art frames (type 0x03) from the tag
	frame_vector_t::iterator it = m_frames.begin();
	while(it != m_frames.end()) {

		if((strncasecmp(it->id, frameid, sizeof(id3v2_frameid_t)) == 0) &&
			(it->data[strlen(reinterpret_cast<char const*>(&it->data[1])) + 1] == 0x03))
			it = m_frames.erase(it);
		else it++;
	}

	if((data == nullptr) || (length == 0)) return;

	if(mimetype == nullptr) mimetype = "image/";
	size_t mimetypelen = strlen(mimetype);

	// encoding | mimetype | picturetype | description | data
	frame_t frame = {};
	memcpy(frame.id, frameid, sizeof(id3v2_frameid_t));
	frame.size = 1 + mimetypelen + 1 + 1 + 1 + length;
	frame.data = std::unique_ptr<uint8_t[]>(new uint8_t[frame.size]);

	frame.data[0] = 0x00;									// ISO-8859-1
	memcpy(&frame.data[1], mimetype, mimetypelen);			// MIME type
	frame.data[1 + mimetypelen] = 0x00;						// NULL terminator
	frame.data[2 + mimetypelen] = 0x03;						// Picture type
	frame.data[3 + mimetypelen] = 0x00;						// Description (NULL)
	memcpy(&frame.data[4 + mimetypelen], data, length);		// Image data

	m_frames.emplace_back(std::move(frame));
}

//---------------------------------------------------------------------------
// id3v2tag::create (static)
//
// Factory method, creates a new id3v2tag instance
//
// Arguments:
//
//	NONE

std::unique_ptr<id3v2tag> id3v2tag::create(void)
{
	return std::unique_ptr<id3v2tag>(new id3v2tag());
}

//---------------------------------------------------------------------------
// id3v2tag::create (static)
//
// Factory method, creates a new id3v2tag instance
//
// Arguments:
//
//	data		- Pointer to existing ID3v2 tag data
//	length		- Length of the xisting ID3v2 tag data

std::unique_ptr<id3v2tag> id3v2tag::create(uint8_t const* data, size_t length)
{
	return std::unique_ptr<id3v2tag>(new id3v2tag(data, length));
}

//---------------------------------------------------------------------------
// id3v2tag::decode_synchsafe (static, private)
//
// Decodes a 32-bit synchsafe integer
//
// Arguments:
//
//	synchsafe	- Reference to a 32-bit synchsafe integer

uint32_t id3v2tag::decode_synchsafe(synchsafe32_t const& synchsafe)
{
	uint8_t const* bytes = reinterpret_cast<uint8_t const*>(&synchsafe);

	return ((bytes[0] & 0x7f) << 21) | ((bytes[1] & 0x7f) << 14) | ((bytes[2] & 0x7f) << 7) | (bytes[3] & 0x7f);
}

//---------------------------------------------------------------------------
// id3v2tag::encode_synchsafe (static, private)
//
// Encodes a 32-bit synchsafe integer
//
// Arguments:
//
//	val			- 32-bit value to be encoded
//	synchsafe	- Reference to the encoded synchsafe32_t variable

void id3v2tag::encode_synchsafe(uint32_t val, synchsafe32_t& synchsafe)
{
	uint8_t* bytes = reinterpret_cast<uint8_t*>(&synchsafe);

	bytes[0] = (val >> 21) & 0x7F;
	bytes[1] = (val >> 14) & 0x7F;
	bytes[2] = (val >> 7) & 0x7F;
	bytes[3] = val & 0x7F;
}

//---------------------------------------------------------------------------
// id3v2tag::genre
//
// Sets or appends a genre (TCON) frame
//
// Arguments:
//
//	genre		- ISO-8859-1 text string to set for the frame
//	append		- Flag to append a new tag rather than replace existing

void id3v2tag::genre(char const* artist, bool append)
{
	id3v2_frameid_t frameid{ 'T', 'C', 'O', 'N' };
	add_text_frame(frameid, artist, append);
}

//---------------------------------------------------------------------------
// id3v2tag::remove_frames (private)
//
// Removes all existing frames with the specified identifier
//
// Arguments:
//
//	frameid		- Frame identifier to be removed from the tag

void id3v2tag::remove_frames(id3v2_frameid_t frameid)
{
	frame_vector_t::iterator it = m_frames.begin();
	while(it != m_frames.end()) {

		if(strncasecmp(it->id, frameid, sizeof(id3v2_frameid_t)) == 0)
			it = m_frames.erase(it);
		else it++;
	}
}

//---------------------------------------------------------------------------
// id3v2tag::size
//
// Gets the number of bytes required to persist the tag to storage
//
// Arguments:
//
//	NONE

size_t id3v2tag::size(void) const
{
	size_t size = sizeof(id3v2_header_t);
	for(auto const& frame : m_frames) size += sizeof(id3v2_frame_header_t) + frame.size;

	return size;
}

//---------------------------------------------------------------------------
// id3v2tag::title
//
// Sets the title (TIT2) frame
//
// Arguments:
//
//	title		- ISO-8859-1 text string to set for the frame

void id3v2tag::title(char const* title)
{
	id3v2_frameid_t frameid{ 'T', 'I', 'T', '2' };
	add_text_frame(frameid, title, false);
}

//---------------------------------------------------------------------------
// id3v2tag::track
//
// Sets the track (TRCK) frame
//
// Arguments:
//
//	track		- ISO-8859-1 text string to set for the frame

void id3v2tag::track(char const* track)
{
	id3v2_frameid_t frameid{ 'T', 'R', 'C', 'K' };
	add_text_frame(frameid, track, false);
}

//---------------------------------------------------------------------------
// id3v2tag::write
//
// Writes the tag into a memory buffer
//
// Arguments:
//
//	buffer	- Address of the buffer to write the tag data into
//	length	- Length of the destination buffer in uint8_t units

bool id3v2tag::write(uint8_t* buffer, size_t length) const
{
	if(buffer == nullptr) throw std::invalid_argument("buffer");
	if(length < sizeof(id3v2_header_t)) throw std::invalid_argument("length");

	uint32_t tagsize = static_cast<uint32_t>(size());
	if(length < tagsize) return false;

	memset(buffer, 0, length);

	// Header
	id3v2_header_t* header = reinterpret_cast<id3v2_header_t*>(buffer);
	memcpy(header->id, "ID3", 3);
	header->version = 0x04;
	encode_synchsafe(tagsize, header->size);
	buffer += sizeof(id3v2_header_t);

	// Frames
	for(auto const& frame : m_frames) {

		id3v2_frame_header_t* frameheader = reinterpret_cast<id3v2_frame_header_t*>(buffer);
		memcpy(frameheader->id, frame.id, 4);
		encode_synchsafe(static_cast<uint32_t>(frame.size), frameheader->size);
		memcpy(frameheader->flags, frame.flags, 2);
		buffer += sizeof(id3v2_frame_header_t);

		memcpy(buffer, &frame.data[0], frame.size);
		buffer += frame.size;
	}

	return true;
}

//---------------------------------------------------------------------------
// id3v2tag::year
//
// Sets the year (TYER) frame
//
// Arguments:
//
//	year		- ISO-8859-1 text string to set for the frame

void id3v2tag::year(char const* year)
{
	id3v2_frameid_t frameid{ 'T', 'Y', 'E', 'R' };
	add_text_frame(frameid, year, false);
}

//---------------------------------------------------------------------------

#pragma warning(pop)
