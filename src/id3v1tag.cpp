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
#include "id3v1tag.h"

#include <algorithm>
#include <memory.h>
#include <stdexcept>

#include "string_exception.h"

#pragma warning(push, 4)

// g_genres
//
// List of ID3v1 genre strings. Note that this list is not exhaustive and
// only implements the original 79 genres specified by ID3v1:
//
// https://www.oreilly.com/library/view/mp3-the-definitive/1565926617/apa.html
static char const* const g_genres[] = {

	"Blues",
	"Classic Rock",
	"Country",
	"Dance",
	"Disco",
	"Funk",
	"Grunge",
	"Hip-Hop",
	"Jazz",
	"Metal",
	"New Age",
	"Oldies",
	"Other",
	"Pop",
	"R&B",
	"Rap",
	"Reggae",
	"Rock",
	"Techno",
	"Industrial",
	"Alternative",
	"Ska",
	"Death Metal",
	"Pranks",
	"Soundtrack",
	"Euro-Techno",
	"Ambient",
	"Trip-Hop",
	"Vocal",
	"Jazz+Funk",
	"Fusion",
	"Trance",
	"Classical",
	"Instrumental",
	"Acid",
	"House",
	"Game",
	"Sound Clip",
	"Gospel",
	"Noise",
	"AlternRock",
	"Bass",
	"Soul",
	"Punk",
	"Space",
	"Meditative",
	"Instrumental Pop",
	"Instrumental Rock",
	"Ethnic",
	"Gothic",
	"Darkwave",
	"Techno-Industrial",
	"Electronic",
	"Pop-Folk",
	"Eurodance",
	"Dream",
	"Southern Rock",
	"Comedy",
	"Cult",
	"Gangsta",
	"Top 40",
	"Christian Rap",
	"Pop/Funk",
	"Jungle",
	"Native American",
	"Cabaret",
	"New Wave",
	"Psychedelic",
	"Rave",
	"Showtunes",
	"Trailer",
	"Lo-Fi",
	"Tribal",
	"Acid Punk",
	"Acid Jazz",
	"Polka",
	"Retro",
	"Musical",
	"Rock & Roll",
	"Hard Rock",
};

// g_numgenres
//
// Indicates the number of genres in g_genres
static int const g_numgenres = sizeof(g_genres) / sizeof(g_genres[0]);

// id3v1tag::UNSPECIFIED_GENRE
//
// Flag indicating that a genre is unspecified
uint8_t const id3v1tag::UNSPECIFIED_GENRE = 255;

//---------------------------------------------------------------------------
// id3v1tag Constructor (private)
//
// Arguments:
//
//	NONE

id3v1tag::id3v1tag()
{
	memcpy(m_tag.id, "TAG", 3);
	m_tag.genre = UNSPECIFIED_GENRE;
}

//---------------------------------------------------------------------------
// id3v1tag Constructor (private)
//
// Arguments:
//
//	data		- Pointer to existing ID3v1 tag data
//	length		- Length of existing ID3v1 tag data

id3v1tag::id3v1tag(uint8_t const* data, size_t length)
{
	if(data == nullptr) throw std::invalid_argument("data");
	if(length != sizeof(id3v1_tag_t)) throw std::invalid_argument("length");

	if(memcmp(data, "TAG", 3) != 0) throw string_exception("invalid ID3v1 header");

	memcpy(&m_tag, data, length);			// Just overwrite the data
}

//---------------------------------------------------------------------------
// id3v1tag Destructor

id3v1tag::~id3v1tag()
{
}

//---------------------------------------------------------------------------
// id3v1tag::album
//
// Sets the ALBUM tag element
//
// Arguments:
//
//	album		- Value to set the ALBUM tag element to

void id3v1tag::album(char const* album)
{
	if(album == nullptr) album = "";

	memset(&m_tag.album, 0, std::extent<decltype(id3v1_tag_t::album)>::value);
	size_t len = std::min(strlen(album), std::extent<decltype(id3v1_tag_t::album)>::value);
	memcpy(&m_tag.album, album, len);
}

//---------------------------------------------------------------------------
// id3v1tag::artist
//
// Sets the ARTIST tag element
//
// Arguments:
//
//	artist		- Value to set the ARTIST tag element to

void id3v1tag::artist(char const* artist)
{
	if(artist == nullptr) artist = "";

	memset(&m_tag.artist, 0, std::extent<decltype(id3v1_tag_t::artist)>::value);
	size_t len = std::min(strlen(artist), std::extent<decltype(id3v1_tag_t::artist)>::value);
	memcpy(&m_tag.artist, artist, len);
}

//---------------------------------------------------------------------------
// id3v1tag::comment
//
// Sets the COMMENT tag element
//
// Arguments:
//
//	comment		- Value to set the COMMENT tag element to

void id3v1tag::comment(char const* comment)
{
	if(comment == nullptr) comment = "";

	// Track number shares the final two bytes of the comment field. If a track 
	// number has already been set, the comment field becomes 2 bytes shorter
	size_t fieldlength = std::extent<decltype(id3v1_tag_t::comment)>::value;
	if((m_tag.comment[28] == 0x00) && (m_tag.comment[29] != 0x00)) fieldlength -= 2;

	memset(&m_tag.comment, 0, fieldlength);
	size_t len = std::min(strlen(comment), fieldlength);
	memcpy(&m_tag.comment, comment, len);
}

//---------------------------------------------------------------------------
// id3v1tag::create (static)
//
// Factory method, creates a new id3v1tag instance
//
// Arguments:
//
//	NONE

std::unique_ptr<id3v1tag> id3v1tag::create(void)
{
	return std::unique_ptr<id3v1tag>(new id3v1tag());
}

//---------------------------------------------------------------------------
// id3v1tag::create (static)
//
// Factory method, creates a new id3v1tag instance
//
// Arguments:
//
//	data		- Pointer to existing ID3v2 tag data
//	length		- Length of the xisting ID3v2 tag data

std::unique_ptr<id3v1tag> id3v1tag::create(uint8_t const* data, size_t length)
{
	return std::unique_ptr<id3v1tag>(new id3v1tag(data, length));
}

//---------------------------------------------------------------------------
// id3v1tag::genre
//
// Sets the GENRE tag element
//
// Arguments:
//
//	genre		- Value to set the GENRE tag element to

void id3v1tag::genre(uint8_t genre)
{
	m_tag.genre = genre;
}

//---------------------------------------------------------------------------
// id3v1tag::genre
//
// Sets the GENRE tag element
//
// Arguments:
//
//	genre		- Value to set the GENRE tag element to

void id3v1tag::genre(char const* genre)
{
	if(genre == nullptr) genre = "";

	// Assume there will not be a matching genre string in the table
	uint8_t mapped = UNSPECIFIED_GENRE;

	// Try to match the string-based genre with one of the known values in the table
	for(int index = 0; index < g_numgenres; index++) {

#ifdef _WINDOWS
		if(_stricmp(genre, g_genres[index]) == 0) mapped = static_cast<uint8_t>(index);
#else
		if(strcasecmp(genre, g_genres[index]) == 0) mapped = static_cast<uint8_t>(index);
#endif
		break;
	}

	m_tag.genre = mapped;
}

//---------------------------------------------------------------------------
// id3v1tag::size
//
// Gets the number of bytes required to persist the tag to storage
//
// Arguments:
//
//	NONE

size_t id3v1tag::size(void) const
{
	return sizeof(id3v1_tag_t);			// Always 128 bytes
}

//---------------------------------------------------------------------------
// id3v1tag::song
//
// Sets the SONG tag element
//
// Arguments:
//
//	song		- Value to set the SONG tag element to

void id3v1tag::song(char const* song)
{
	if(song == nullptr) song = "";

	memset(&m_tag.song, 0, std::extent<decltype(id3v1_tag_t::song)>::value);
	size_t len = std::min(strlen(song), std::extent<decltype(id3v1_tag_t::song)>::value);
	memcpy(&m_tag.song, song, len);
}

//---------------------------------------------------------------------------
// id3v1tag::track
//
// Sets the TRACK tag element
//
// Arguments:
//
//	track		- Value to set the TRACK tag element to

void id3v1tag::track(uint8_t track)
{
	// Track number shares the final two bytes of the comment field.  Clear an
	// existing track number only if comment[28] is already zero
	if((track == 0) && (m_tag.comment[28] == 0x00)) m_tag.comment[29] = 0x00;

	// Otherwise, take over the final two bytes of the comment field
	else {

		m_tag.comment[28] = 0x00;
		m_tag.comment[29] = track;
	}
}

//---------------------------------------------------------------------------
// id3v1tag::write
//
// Writes the tag into a memory buffer
//
// Arguments:
//
//	buffer	- Address of the buffer to write the tag data into
//	length	- Length of the destination buffer in uint8_t units

bool id3v1tag::write(uint8_t* buffer, size_t length) const
{
	if(buffer == nullptr) throw std::invalid_argument("buffer");
	if(length < sizeof(id3v1_tag_t)) throw std::invalid_argument("length");

	memset(buffer, 0, length);
	memcpy(buffer, &m_tag, sizeof(id3v1_tag_t));
	return true;
}

//---------------------------------------------------------------------------
// id3v1tag::year
//
// Sets the YEAR tag element
//
// Arguments:
//
//	year		- Value to set the YEAR tag element to

void id3v1tag::year(char const* year)
{
	if(year == nullptr) year = "";

	memset(&m_tag.year, 0, std::extent<decltype(id3v1_tag_t::year)>::value);
	size_t len = std::min(strlen(year), std::extent<decltype(id3v1_tag_t::year)>::value);
	memcpy(&m_tag.year, year, len);
}

//---------------------------------------------------------------------------

#pragma warning(pop)
