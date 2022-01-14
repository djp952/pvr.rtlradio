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
#include "channelsettings.h"

#include <assert.h>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <kodi/General.h>
#include <kodi/gui/dialogs/FileBrowser.h>

#ifdef HAS_GLES
#include <EGL/egl.h>
#endif

#pragma warning(push, 4)

// Control Identifiers
//
static const int CONTROL_BUTTON_OK				= 100;
static const int CONTROL_BUTTON_CANCEL			= 101;
static const int CONTROL_EDIT_FREQUENCY			= 200;
static const int CONTROL_EDIT_CHANNELNAME		= 201;
static const int CONTROL_BUTTON_CHANNELICON		= 202;
static const int CONTROL_IMAGE_CHANNELICON		= 203;
static const int CONTROL_RADIO_AUTOMATICGAIN	= 204;
static const int CONTROL_SLIDER_MANUALGAIN		= 205;
static const int CONTROL_RENDER_SIGNALMETER		= 206;
static const int CONTROL_EDIT_METERGAIN			= 207;
static const int CONTROL_EDIT_METERPOWER		= 208;
static const int CONTROL_EDIT_METERSNR			= 209;

// channelsettings::FFT_BANDWIDTH
//
// Bandwidth of the FFT display
uint32_t const channelsettings::FFT_BANDWIDTH = (400 KHz);

// channelsettings::FFT_MINDB
//
// Maximum decibel level supported by the FFT
float const channelsettings::FFT_MAXDB = 4.0f;

// channelsettings::FFT_MINDB
//
// Minimum decibel level supported by the FFT
float const channelsettings::FFT_MINDB = -72.0f;

// channelsettings::FMRADIO_BANDWIDTH
//
// Bandwidth of an analog FM radio channel
uint32_t const channelsettings::FMRADIO_BANDWIDTH = (200 KHz);

// channelsettings::HDRADIO_BANDWIDTH
//
// Bandwidth of a Hybrid Digital (HD) FM radio channel
uint32_t const channelsettings::HDRADIO_BANDWIDTH = (400 KHz);

// channelsettings::WXRADIO_BANDWIDTH
//
// Bandwidth of a VHF weather radio channel
uint32_t const channelsettings::WXRADIO_BANDWIDTH = (10 KHz);

// is_platform_opengles (local)
//
// Helper function to determine if the platform is full OpenGL or OpenGL ES
static bool is_platform_opengles(void)
{
#if defined(TARGET_WINDOWS)
	return true;
#elif defined(TARGET_ANDROID)
	return true;
#elif defined(TARGET_DARWIN_OSX)
	return false;
#else
	return (eglQueryAPI() == EGL_OPENGL_ES_API);
#endif
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol Constructor
//
// Arguments:
//
//	window		- Parent CWindow instance
//	controlid	- Identifier of the control within the parent CWindow instance

channelsettings::fftcontrol::fftcontrol(kodi::gui::CWindow* window, int controlid) : renderingcontrol(window, controlid)
{
	// Store some handy floating point copies of the width and height for rendering
	m_widthf = static_cast<GLfloat>(m_width);
	m_heightf = static_cast<GLfloat>(m_height);

	// Allocate the buffer to hold the scaled FFT data
	m_fft = std::unique_ptr<glm::vec2[]>(new glm::vec2[m_width]);

	// Create the model/view/projection matrix based on width and height
	m_modelProjMat = glm::ortho(0.0f, m_widthf, m_heightf, 0.0f);

	// Create the vertex buffer object
	glGenBuffers(1, &m_vertexVBO);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol Destructor

channelsettings::fftcontrol::~fftcontrol()
{
	// Delete the vertex buffer object
	glDeleteBuffers(1, &m_vertexVBO);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::db_to_height (private)
//
// Converts a power level specified in decibels to a height for the FFT
//
// Arguments:
//
//	db			- Power level in decibels

inline GLfloat channelsettings::fftcontrol::db_to_height(float db) const
{
	return m_heightf * ((db - channelsettings::FFT_MAXDB) / (channelsettings::FFT_MINDB - channelsettings::FFT_MAXDB));
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::dirty (private)
//
// Indicates if there are dirty regions in the control that need to be rendered
//
// Arguments:
//
//	NONE

bool channelsettings::fftcontrol::dirty(void)
{
	return m_dirty;
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::height
//
// Retrieves the height of the control
//
// Arguments:
//
//	NONE

size_t channelsettings::fftcontrol::height(void) const
{
	return static_cast<size_t>(m_height);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render (private)
//
// Renders all dirty regions within the control
//
// Arguments:
//
//	NONE

void channelsettings::fftcontrol::render(void)
{
	// Ensure that the shader was compiled properly before continuing
	assert(m_shader.ShaderOK());
	if(!m_shader.ShaderOK()) return;

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable the shader program
	m_shader.EnableShader();

	// Set the model/view/projection matrix
	glUniformMatrix4fv(m_shader.uModelProjMatrix(), 1, GL_FALSE, glm::value_ptr(m_modelProjMat));

	// Bind the vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);

	// Enable the vertex array
	glEnableVertexAttribArray(m_shader.aPosition());

	// Set the vertex attribute pointer type
	glVertexAttribPointer(m_shader.aPosition(), 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), BUFFER_OFFSET(offsetof(glm::vec2, x)));

#ifndef HAS_ANGLE
	// Background
	glm::vec2 backgroundrect[4] = { { 0.0f, 0.0f }, { 0.0f, m_heightf }, { m_widthf, 0.0f }, { m_widthf, m_heightf } };
	render_rect(glm::vec3(0.0f, 0.0f, 0.0f), backgroundrect);
#endif

	// 0dB level
	GLfloat zerodb = db_to_height(0.0f);
	glm::vec2 zerodbline[2] = { { 0.0f, zerodb }, { m_widthf, zerodb } };
	render_line(glm::vec4(1.0f, 1.0f, 0.0f, 0.75f), zerodbline);

	// -6dB increment levels
	for(int index = -6; index >= static_cast<int>(channelsettings::FFT_MINDB); index -= 6) {

		GLfloat y = db_to_height(static_cast<float>(index));
		glm::vec2 dbline[2] = { { 0.0f, y }, { m_widthf, y } };
		render_line(glm::vec4(1.0f, 1.0f, 1.0f, 0.25f), dbline);
	}

	// Bandwidth
	glm::vec2 cutrect[4] = { { m_lowcut, 0.0f }, { m_lowcut, m_heightf }, { m_highcut, 0.0f }, { m_highcut, m_heightf } };
	render_rect(glm::vec4(1.0f, 1.0f, 1.0f, 0.1f), cutrect);

	// Power range
	glm::vec2 powerrect[4] = { { 0.0f, m_power }, { m_widthf, m_power }, { 0.0f, m_noise }, { m_widthf, m_noise } };
	render_rect(glm::vec4(0.0f, 1.0f, 0.0f, 0.1f), powerrect);
	glm::vec2 powerline[2] = { { 0.0f, m_power }, { m_widthf, m_power } };
	render_line(glm::vec4(0.0f, 1.0f, 0.0f, 0.75f), powerline);

	// Noise range
	glm::vec2 noiserect[4] = { { 0.0f, m_noise }, { m_widthf, m_noise }, { 0.0f, m_heightf }, { m_widthf, m_heightf } };
	render_rect(glm::vec4(1.0f, 0.0f, 0.0f, 0.1f), noiserect);
	glm::vec2 noiseline[2] = { { 0.0f, m_noise }, { m_width, m_noise } };
	render_line(glm::vec4(1.0f, 0.0f, 0.0f, 0.75f), noiseline);

	// Center frequency
	glm::vec2 centerline[2] = { { m_widthf / 2.0f, 0.0f }, { m_widthf / 2.0f, m_heightf } };
	render_line(glm::vec4(1.0f, 1.0f, 0.0f, 0.75f), centerline);

	// FFT
	if(!m_overload) render_line_strip(glm::vec3(1.0f, 1.0f, 1.0f), m_fft.get(), m_width);
	else render_line_strip(glm::vec3(1.0f, 0.0f, 0.0f), m_fft.get(), m_width);

	glDisableVertexAttribArray(m_shader.aPosition());	// Disable the vertex array
	glBindBuffer(GL_ARRAY_BUFFER, 0);					// Unbind the VBO

	m_shader.DisableShader();							// Disable the shader program
	glDisable(GL_BLEND);								// Disable blending

	// Render state is clean until the next update from the meter instance
	m_dirty = false;
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_line (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Line vertices

void channelsettings::fftcontrol::render_line(glm::vec3 color, glm::vec2 vertices[2]) const
{
	return render_line(glm::vec4(color, 1.0f), vertices);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_line (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Line vertices

void channelsettings::fftcontrol::render_line(glm::vec4 color, glm::vec2 vertices[2]) const
{
	// Set the specific color value before drawing the line
	glUniform4f(m_shader.uColor(), color.r, color.g, color.b, color.a);

	glm::vec2 p(vertices[1].x - vertices[0].x, vertices[1].y - vertices[0].y);
	p = glm::normalize(p);

	// Pre-calculate the required deltas for the line thickness
#if defined(WIN32) && defined(HAS_ANGLE)
	GLfloat const dx = (m_linewidthf);
	GLfloat const dy = (m_lineheightf);
#else
	GLfloat const dx = (m_linewidthf / 2.0f);
	GLfloat const dy = (m_lineheightf / 2.0f);
#endif

	glm::vec2 const p1(-p.y, p.x);
	glm::vec2 const p2(p.y, -p.x);

	glm::vec2 strip[] = {

		{ vertices[0].x + p1.x * dx, vertices[0].y + p1.y * dy },
		{ vertices[0].x + p2.x * dx, vertices[0].y + p2.y * dy },
		{ vertices[1].x + p1.x * dx, vertices[1].y + p1.y * dy },
		{ vertices[1].x + p2.x * dx, vertices[1].y + p2.y * dy }
	};

	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 4), &strip[0], GL_STATIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_line_strip (private)
//
// Renders a line strip primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Line strip vertices
//	numvertices	- Number of line strip vertices

void channelsettings::fftcontrol::render_line_strip(glm::vec3 color, glm::vec2 vertices[], size_t numvertices) const
{
	return render_line_strip(glm::vec4(color, 1.0f), vertices, numvertices);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_line_strip (private)
//
// Renders a line strip primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Line vertices
//	numvertices	- Number of line strip vertices

void channelsettings::fftcontrol::render_line_strip(glm::vec4 color, glm::vec2 vertices[], size_t numvertices) const
{
	// Set the specific color value before drawing the line
	glUniform4f(m_shader.uColor(), color.r, color.g, color.b, color.a);

	// Each point in the line strip will be represented by six vertices in the triangle strip
	std::unique_ptr<glm::vec2[]> strip(new glm::vec2[numvertices * 6]);
	size_t strippos = 0;

	// Pre-calculate the required deltas for the line thickness
#if defined(WIN32) && defined(HAS_ANGLE)
	GLfloat const dx = (m_linewidthf);
	GLfloat const dy = (m_lineheightf);
#else
	GLfloat const dx = (m_linewidthf / 2.0f);
	GLfloat const dy = (m_lineheightf / 2.0f);
#endif

	for(size_t index = 0; index < numvertices - 1; index++)
	{
		glm::vec2 const& a = vertices[index];
		glm::vec2 const& b = vertices[index + 1];

		glm::vec2 p(b.x - a.x, b.y - a.y);
		p = glm::normalize(p);

		glm::vec2 const p1(-p.y, p.x);
		glm::vec2 const p2(p.y, -p.x);

		strip[strippos++] = a;
		strip[strippos++] = b;
		strip[strippos++] = glm::vec2(a.x + p1.x * dx, a.y + p1.y * dy);
		strip[strippos++] = glm::vec2(a.x + p2.x * dx, a.y + p2.y * dy);
		strip[strippos++] = glm::vec2(b.x + p1.x * dx, b.y + p1.y * dy);
		strip[strippos++] = glm::vec2(b.x + p2.x * dx, b.y + p2.y * dy);
	}

	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * strippos), strip.get(), GL_STATIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(strippos));
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_rect (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Rectangle vertices

void channelsettings::fftcontrol::render_rect(glm::vec3 color, glm::vec2 vertices[4]) const
{
	return render_rect(glm::vec4(color, 1.0f), vertices);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_rect (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Rectangle vertices

void channelsettings::fftcontrol::render_rect(glm::vec4 color, glm::vec2 vertices[4]) const
{
	// Set the specific color value before drawing the line
	glUniform4f(m_shader.uColor(), color.r, color.g, color.b, color.a);

	// Render the rectangle as a 4-vertex GL_TRIANGLE_STRIP primitive
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 4), &vertices[0], GL_STATIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_triangle (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Triangle vertices

void channelsettings::fftcontrol::render_triangle(glm::vec3 color, glm::vec2 vertices[3]) const
{
	return render_triangle(glm::vec4(color, 1.0f), vertices);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::render_triangle (private)
//
// Renders a line primitive
//
// Arguments:
//
//	color		- Color value to use when rendering
//	vertices	- Triangle vertices

void channelsettings::fftcontrol::render_triangle(glm::vec4 color, glm::vec2 vertices[3]) const
{
	// Set the specific color value before drawing the line
	glUniform4f(m_shader.uColor(), color.r, color.g, color.b, color.a);

	// Render the triangle as a 3-vertex GL_TRIANGLES primitive
	glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 3), &vertices[0], GL_STATIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::update
//
// Updates the rendering control state and flags it as dirty
//
// Arguments:
//
//	NONE

void channelsettings::fftcontrol::update(struct fmmeter::signal_status const& status)
{
	// Power and noise values are supplied as dB and need to be scaled to the viewport
	m_power = db_to_height(status.power);
	m_noise = db_to_height(status.noise);

	// Calculate the horizontal offsets of the low and high cut points and where to put the power meter
	m_lowcut = (((status.fftbandwidth / 2.0f) + status.fftlowcut) / status.fftbandwidth) * m_widthf;
	m_highcut = (((status.fftbandwidth / 2.0f) + status.ffthighcut) / status.fftbandwidth) * m_widthf;

	// The length of the fft data should match the width of the control, but watch for overrruns
	assert(status.fftsize == m_width);
	size_t length = std::min(status.fftsize, static_cast<size_t>(m_width));

	// The FFT data merely needs to be converted into an X,Y vertex to be used by the renderer
	for(size_t index = 0; index < length; index++) m_fft[index] = glm::vec2(static_cast<float>(index), 
		static_cast<float>(status.fftdata[index]));

	// The FFT line strip will be shown in a different color upon overload
	m_overload = status.overload;

	// In the event of an FFT data underrun, flat-line the remainder of the data points
	if(m_width > status.fftsize) {

		for(size_t index = length; index < m_width; index++) m_fft[index] = glm::vec2(static_cast<float>(index), m_heightf);
	}

	m_dirty = true;					// Scene needs to be re-rendered
}

//---------------------------------------------------------------------------
// channelsettings::fftcontrol::width
//
// Retrieves the width of the control
//
// Arguments:
//
//	NONE

size_t channelsettings::fftcontrol::width(void) const
{
	return static_cast<size_t>(m_width);
}

//---------------------------------------------------------------------------
// channelsettings::fftshader Constructor
//
// Arguments:
//
//	NONE

channelsettings::fftshader::fftshader()
{
	// VERTEX SHADER
	std::string const vertexshader(
		R"(
uniform mat4 u_modelViewProjectionMatrix;

#ifdef GL_ES
attribute vec2 a_position;
#else
in vec2 a_position;
#endif

void main()
{
  gl_Position = u_modelViewProjectionMatrix * vec4(a_position, 0.0, 1.0);
}
		)");

	// FRAGMENT SHADER
	std::string const fragmentshader(
		R"(
#ifdef GL_ES
precision mediump float;
#else
precision highp float;
#endif

uniform vec4 u_color;

#ifndef GL_ES
out vec4 FragColor;
#endif

void main()
{
#ifdef GL_ES
  gl_FragColor = u_color;
#else
  FragColor = u_color;
#endif
}
		)");

	// Compile and link the shader programs during construction
	if(is_platform_opengles()) CompileAndLink("#version 100\n", vertexshader, "#version 100\n", fragmentshader);
	else CompileAndLink("#version 150\n", vertexshader, "#version 150\n", fragmentshader);
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::aPosition (const)
//
// Gets the location of the aPosition shader variable
//
// Arguments:
//
//	NONE

GLint channelsettings::fftshader::aPosition(void) const
{
	assert(m_aPosition != -1);
	return m_aPosition;
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::OnCompiledAndLinked (CShaderProgram)
//
// Invoked when the shader has been compiled and linked
//
// Arguments:
//
//	NONE

void channelsettings::fftshader::OnCompiledAndLinked(void)
{
	m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
	m_uColor = glGetUniformLocation(ProgramHandle(), "u_color");
	m_uModelProjMatrix = glGetUniformLocation(ProgramHandle(), "u_modelViewProjectionMatrix");
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::OnDisabled (CShaderProgram)
//
// Invoked when the shader has been disabled
//
// Arguments:
//
//	NONE

void channelsettings::fftshader::OnDisabled(void)
{
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::OnEnabled (CShaderProgram)
//
// Invoked when the shader has been enabled
//
// Arguments:
//
//	NONE

bool channelsettings::fftshader::OnEnabled(void)
{
	return true;
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::uColor (const)
//
// Gets the location of the uColor shader variable
//
// Arguments:
//
//	NONE

GLint channelsettings::fftshader::uColor(void) const
{
	assert(m_uColor != -1);
	return m_uColor;
}

//---------------------------------------------------------------------------
// channelsettings::fftshader::uModelProjMatrix (const)
//
// Gets the location of the aPosition shader variable
//
// Arguments:
//
//	NONE

GLint channelsettings::fftshader::uModelProjMatrix(void) const
{
	assert(m_uModelProjMatrix != -1);
	return m_uModelProjMatrix;
}

//---------------------------------------------------------------------------
// channelsettings Constructor (private)
//
// Arguments:
//
//	device			- Device instance
//	tunerprops		- Tuner properties
//	channelprops	- Channel properties

channelsettings::channelsettings(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, struct channelprops const& channelprops) 
	: kodi::gui::CWindow("channelsettings.xml", "skin.estuary", true), m_channelprops(channelprops)
{
	uint32_t				bandwidth = FMRADIO_BANDWIDTH;			// Default to wideband FM radio bandwidth

	assert(device);

	// Adjust the signal meter bandwidth based on the underlying channel type
	switch(get_channel_type(channelprops)) {

		case channeltype::fmradio: bandwidth = FMRADIO_BANDWIDTH; break;
		case channeltype::hdradio: bandwidth = HDRADIO_BANDWIDTH; break;
		case channeltype::wxradio: bandwidth = WXRADIO_BANDWIDTH; break;
	}

	// Create the signal meter instance with the specified device and tuner properties, set for a 500ms callback rate
	m_signalmeter = fmmeter::create(std::move(device), tunerprops, channelprops.frequency, bandwidth, FFT_BANDWIDTH,
		std::bind(&channelsettings::fm_meter_status, this, std::placeholders::_1), 500, 
		std::bind(&channelsettings::fm_meter_exception, this, std::placeholders::_1));

	// Get the vector<> of valid manual gain values for the attached device
	m_signalmeter->get_valid_manual_gains(m_manualgains);
}

//---------------------------------------------------------------------------
// channelsettings Destructor

channelsettings::~channelsettings()
{
	// Stop the signal meter
	m_signalmeter->stop();
}

//---------------------------------------------------------------------------
// channelsettings::create (static)
//
// Factory method, creates a new channelsettings instance
//
// Arguments:
//
//	device			- Device instance
//	tunerprops		- Tuner properties
//	channelprops	- Channel properties

std::unique_ptr<channelsettings> channelsettings::create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
	struct channelprops const& channelprops)
{
	return std::unique_ptr<channelsettings>(new channelsettings(std::move(device), tunerprops, channelprops));
}

//---------------------------------------------------------------------------
// channelsettings::fm_meter_exception (private)
//
// Callback to handle an exception raised by the signal meter
//
// Arguments:
//
//	ex		- Exception that was raised by the signal meter

void channelsettings::fm_meter_exception(std::exception const& ex)
{
	kodi::QueueFormattedNotification(QueueMsg::QUEUE_ERROR, "Unable to read from device: %s", ex.what());
}

//---------------------------------------------------------------------------
// channelsettings::fm_meter_status (private)
//
// Updates the state of the signal meter
//
// Arguments:
//
//	status		- Updated signal status from the signal meter

void channelsettings::fm_meter_status(struct fmmeter::signal_status const& status)
{
	char strbuf[64] = {};						// snprintf() text buffer

	// Signal Meter
	//
	m_render_signalmeter->update(status);

	// Signal Strength
	//
	if(!std::isnan(status.power)) {

		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%.1F dB", status.power);
		m_edit_signalpower->SetText(strbuf);
	}
	else m_edit_signalpower->SetText("N/A");

	// Signal-to-noise
	//
	if(!std::isnan(status.snr)) {
		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%d dB", static_cast<int>(status.snr));
		m_edit_signalsnr->SetText(strbuf);
	}
	else m_edit_signalsnr->SetText("N/A");
}

//---------------------------------------------------------------------------
// channelsettings::gain_to_percent (private)
//
// Converts a manual gain value into a percentage
//
// Arguments:
//
//	gain		- Gain to convert

int channelsettings::gain_to_percent(int gain) const
{
	if(m_manualgains.empty()) return 0;

	// Convert the gain into something that's valid for the tuner
	gain = nearest_valid_gain(gain);

	// Use the index within the gain table to generate the percentage
	for(size_t index = 0; index < m_manualgains.size(); index++) {

		if(gain == m_manualgains[index]) return static_cast<int>((index * 100) / (m_manualgains.size() - 1));
	}

	return 0;
}

//---------------------------------------------------------------------------
// channelsettings::get_channel_properties
//
// Gets the updated channel properties from the dialog box
//
// Arguments:
//
//	channelprops	- Structure to receive the updated channel properties

void channelsettings::get_channel_properties(struct channelprops& channelprops) const
{
	channelprops = m_channelprops;
}

//---------------------------------------------------------------------------
// channelsettings::get_dialog_result
//
// Gets the result code from the dialog box
//
// Arguments:
//
//	NONE

bool channelsettings::get_dialog_result(void) const
{
	return m_result;
}

//---------------------------------------------------------------------------
// channelsettings::nearest_valid_gain (private)
//
// Gets the closest valid value for a manual gain setting
//
// Arguments:
//
//	gain		- Gain to adjust

int channelsettings::nearest_valid_gain(int gain) const
{
	if(m_manualgains.empty()) return 0;

	// Select the gain value that's closest to what has been requested
	int nearest = m_manualgains[0];
	for(size_t index = 0; index < m_manualgains.size(); index++) {

		if(std::abs(gain - m_manualgains[index]) < std::abs(gain - nearest)) nearest = m_manualgains[index];
	}

	return nearest;
}

//---------------------------------------------------------------------------
// channelsettings::percent_to_gain (private)
//
// Converts a percentage into a manual gain value
//
// Arguments:
//
//	percent		- Percentage to convert

int channelsettings::percent_to_gain(int percent) const
{
	if(m_manualgains.empty()) return 0;

	if(percent == 0) return m_manualgains.front();
	else if(percent == 100) return m_manualgains.back();

	return m_manualgains[(percent * m_manualgains.size()) / 100];
}

//---------------------------------------------------------------------------
// channelsettings::update_gain (private)
//
// Updates the state of the gain control
//
// Arguments:
//
//	NONE

void channelsettings::update_gain(void)
{
	char strbuf[64] = {};						// snprintf() text buffer

	if(!m_channelprops.autogain) {

		// Convert the gain value from tenths of a decibel into XX.X dB format
		snprintf(strbuf, std::extent<decltype(strbuf)>::value, "%.1f dB", m_channelprops.manualgain / 10.0);
		m_edit_signalgain->SetText(strbuf);
	}

	else m_edit_signalgain->SetText("Auto");
}

//---------------------------------------------------------------------------
// CWINDOW IMPLEMENTATION
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// channelsettings::OnAction (CWindow)
//
// Receives action codes that are sent to this window
//
// Arguments:
//
//	actionId	- The action id to perform

bool channelsettings::OnAction(ADDON_ACTION actionId)
{
	return kodi::gui::CWindow::OnAction(actionId);
}

//---------------------------------------------------------------------------
// channelsettings::OnClick (CWindow)
//
// Receives click event notifications for a control
//
//	controlId	- GUI control identifier

bool channelsettings::OnClick(int controlId)
{
	switch(controlId) {

		case CONTROL_EDIT_CHANNELNAME:
			m_channelprops.name = m_edit_channelname->GetText();
			break;

		case CONTROL_BUTTON_CHANNELICON:
			kodi::gui::dialogs::FileBrowser::ShowAndGetImage("local|network|pictures", kodi::addon::GetLocalizedString(30406), m_channelprops.logourl);
			m_image_channelicon->SetFileName(m_channelprops.logourl, false);
			return true;

		case CONTROL_RADIO_AUTOMATICGAIN:
			m_channelprops.autogain = m_radio_autogain->IsSelected();
			m_signalmeter->set_automatic_gain(m_channelprops.autogain);
			m_slider_manualgain->SetEnabled(!m_channelprops.autogain);
			update_gain();
			return true;

		case CONTROL_SLIDER_MANUALGAIN:
			m_channelprops.manualgain = percent_to_gain(static_cast<int>(m_slider_manualgain->GetPercentage()));
			m_signalmeter->set_manual_gain(m_channelprops.manualgain);
			update_gain();
			return true;

		case CONTROL_BUTTON_OK:
			m_result = true;
			Close();
			return true;

		case CONTROL_BUTTON_CANCEL:
			Close();
			return true;
	}

	return kodi::gui::CWindow::OnClick(controlId);
}

//---------------------------------------------------------------------------
// channelsettings::OnInit (CWindow)
//
// Called to initialize the window object
//
// Arguments:
//
//	NONE

bool channelsettings::OnInit(void)
{
	try {

		// Get references to all of the manipulable dialog controls
		m_edit_frequency = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_FREQUENCY));
		m_edit_channelname = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_CHANNELNAME));
		m_button_channelicon = std::unique_ptr<CButton>(new CButton(this, CONTROL_BUTTON_CHANNELICON));
		m_image_channelicon = std::unique_ptr<CImage>(new CImage(this, CONTROL_IMAGE_CHANNELICON));
		m_radio_autogain = std::unique_ptr<CRadioButton>(new CRadioButton(this, CONTROL_RADIO_AUTOMATICGAIN));
		m_slider_manualgain = std::unique_ptr<CSettingsSlider>(new CSettingsSlider(this, CONTROL_SLIDER_MANUALGAIN));
		m_render_signalmeter = std::unique_ptr<fftcontrol>(new fftcontrol(this, CONTROL_RENDER_SIGNALMETER));
		m_edit_signalgain = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERGAIN));
		m_edit_signalpower = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERPOWER));
		m_edit_signalsnr = std::unique_ptr<CEdit>(new CEdit(this, CONTROL_EDIT_METERSNR));

		// Set the channel frequency in XXX.X MHz or XXX.XXX format
		char freqstr[128];
		double frequency = (m_channelprops.frequency / static_cast<double>(100000)) / 10.0;
		if(get_channel_type(m_channelprops) == channeltype::wxradio) snprintf(freqstr, std::extent<decltype(freqstr)>::value, "%.3f", frequency);
		else snprintf(freqstr, std::extent<decltype(freqstr)>::value, "%.1f", frequency);
		m_edit_frequency->SetText(freqstr);

		// Set the channel name and logo/icon
		m_edit_channelname->SetText(m_channelprops.name);
		m_image_channelicon->SetFileName(m_channelprops.logourl, false);

		// Adjust the manual gain value to match something that the tuner supports
		m_channelprops.manualgain = nearest_valid_gain(m_channelprops.manualgain);

		// Set the tuner gain parameters
		m_radio_autogain->SetSelected(m_channelprops.autogain);
		m_slider_manualgain->SetEnabled(!m_channelprops.autogain);
		m_slider_manualgain->SetPercentage(static_cast<float>(gain_to_percent(m_channelprops.manualgain)));
		update_gain();

		// Set the default text for the signal indicators
		m_edit_signalpower->SetText("N/A");
		m_edit_signalsnr->SetText("N/A");

		// Start the signal meter instance
		m_signalmeter->set_automatic_gain(m_channelprops.autogain);
		m_signalmeter->set_manual_gain(m_channelprops.manualgain);
		m_signalmeter->start(FFT_MAXDB, FFT_MINDB, m_render_signalmeter->height(), m_render_signalmeter->width());
	}

	catch(...) { return false; }

	return kodi::gui::CWindow::OnInit();
}

//---------------------------------------------------------------------------

#pragma warning(pop)
