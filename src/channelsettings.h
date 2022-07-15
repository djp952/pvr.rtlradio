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

#ifndef __CHANNELSETTINGS_H_
#define __CHANNELSETTINGS_H_
#pragma once

#include <atomic>
#include <glm/glm.hpp>
#include <kodi/gui/controls/Button.h>
#include <kodi/gui/controls/Edit.h>
#include <kodi/gui/controls/Image.h>
#include <kodi/gui/controls/RadioButton.h>
#include <kodi/gui/controls/SettingsSlider.h>
#include <kodi/gui/gl/GL.h>
#include <kodi/gui/gl/Shader.h>
#include <kodi/gui/Window.h>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <utility>

#include "muxscanner.h"
#include "props.h"
#include "renderingcontrol.h"
#include "rtldevice.h"
#include "scalar_condition.h"
#include "signalmeter.h"

#pragma warning(push, 4)

using namespace kodi::gui::controls;

//---------------------------------------------------------------------------
// Class channelsettings
//
// Implements the "Channel Settings" dialog

class ATTR_DLL_LOCAL channelsettings : public kodi::gui::CWindow
{
public:

	// Destructor
	//
	~channelsettings() override;
	
	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new channelsettings instance
	static std::unique_ptr<channelsettings> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops);
	static std::unique_ptr<channelsettings> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops,
		struct channelprops const& channelprops, bool isnew);

	// get_channel_properties
	//
	// Gets the updated channel properties from the dialog box
	void get_channel_properties(struct channelprops& channelprops) const;

	// get_dialog_result
	//
	// Gets the result code from the dialog box
	bool get_dialog_result(void) const;

	// get_subchannel_properties
	//
	// Gets the updated subchannel properties from the dialog box
	void get_subchannel_properties(std::vector<struct subchannelprops>& subchannelprops) const;

private:

	channelsettings(channelsettings const&) = delete;
	channelsettings& operator=(channelsettings const&) = delete;

	// FFT_BANDWIDTH
	//
	// Bandwidth of the FFT display
	static uint32_t const FFT_BANDWIDTH;

	// FFT_MAXDB
	//
	// Maximum decibel value supported by the FFT
	static float const FFT_MAXDB;

	// FFT_MINDB
	//
	// Minimum decibel level supported by the FFT
	static float const FFT_MINDB;

	// Instance Constructor
	//
	channelsettings(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, struct channelprops const& channelprops, bool isnew);

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// Class fftshader
	//
	// Implements the shader for the FFT rendering control
	class fftshader : public kodi::gui::gl::CShaderProgram
	{
	public:

		// Constructor
		//
		fftshader();

		// Member Functions
		//
		GLint	aPosition(void) const;
		GLint	uColor() const;
		GLint	uModelProjMatrix(void) const;

	private:

		fftshader(fftshader const&) = delete;
		fftshader& operator=(fftshader const&) = delete;

		// CShaderProgram overrides
		//
		void OnCompiledAndLinked(void) override;
		void OnDisabled(void) override;
		bool OnEnabled(void) override;

		GLint		m_aPosition = -1;			// Attribute location
		GLint		m_uColor = -1;				// Attribute location
		GLint		m_uModelProjMatrix = -1;	// Uniform location
	};

	// Class fftcontrol
	//
	// Implements the FFT rendering control
	class fftcontrol : public renderingcontrol
	{
	public:

		// Constructor / Destructor
		//
		fftcontrol(kodi::gui::CWindow* window, int controlid);
		~fftcontrol() override;

		// Member Functions
		//
		size_t	height(void) const;
		size_t	width(void) const;
		void	update(struct signalmeter::signal_status const& status, bool signallock, bool muxlockupdate);

	private:

		fftcontrol(fftcontrol const&) = delete;
		fftcontrol& operator=(fftcontrol const&) = delete;

		// Private Member Functions
		//
		GLfloat db_to_height(float db) const;
		void render_line(glm::vec3 color, glm::vec2 vertices[2]) const;
		void render_line(glm::vec4 color, glm::vec2 vertices[2]) const;
		void render_line_strip(glm::vec3 color, glm::vec2 vertices[], size_t numvertices) const;
		void render_line_strip(glm::vec4 color, glm::vec2 vertices[], size_t numvertices) const;
		void render_rect(glm::vec3 color, glm::vec2 vertices[4]) const;
		void render_rect(glm::vec4 color, glm::vec2 vertices[4]) const;
		void render_triangle(glm::vec3 color, glm::vec2 vertices[3]) const;
		void render_triangle(glm::vec4 color, glm::vec2 vertices[3]) const;

		// renderingcontrol overrides
		//
		bool dirty(void) override;
		void render(void) override;

		GLfloat					m_widthf;				// Width as GLfloat
		GLfloat					m_heightf;				// Height as GLfloat
		GLfloat					m_linewidthf = 1.25f;	// Line width factor
		GLfloat					m_lineheightf = 1.25f;	// Line height factor

		fftshader				m_shader;				// Shader instance
		GLuint					m_vertexVBO;			// Vertex buffer object
		glm::mat4				m_modelProjMat;			// Model/Projection matrix

		bool					m_dirty = false;		// Dirty flag
		GLfloat					m_power = 0.0f;			// Power level
		GLfloat					m_noise = 0.0f;			// Noise level
		bool					m_overload = false;		// Overload flag
		bool					m_signallock = false;	// Signal lock flag
		bool					m_muxlock = false;		// Multiplex lock flag

		std::unique_ptr<glm::vec2[]>	m_fft;			// FFT vertices
		int								m_fftlowcut;	// FFT low cut index
		int								m_ffthighcut;	// FFT high cut index
	};

	//-------------------------------------------------------------------------
	// CWindow Implementation

	// OnAction
	//
	// Receives action codes that are sent to this window
	bool OnAction(ADDON_ACTION actionId) override;

	// OnClick
	//
	// Receives click event notifications for a control
	bool OnClick(int controlId) override;

	// OnInit
	//
	// Called to initialize the window object
	bool OnInit(void) override;

	//-------------------------------------------------------------------------
	// Private Member Functions

	// close
	//
	// Closes the dialog box
	void close(bool result);

	// gain_to_percent
	//
	// Converts a manual gain value into a percentage
	int gain_to_percent(int gain) const;

	// meter_status
	//
	// Updates the state of the signal meter
	void meter_status(struct signalmeter::signal_status const& status);

	// mux_data
	//
	// Updates the state of the multiplex information
	void mux_data(struct muxscanner::multiplex const& muxdata);

	// nearest_valid_gain
	//
	// Gets the closest valid value for a manual gain setting
	int nearest_valid_gain(int gain) const;

	// percent_to_gain
	//
	// Converts a percentage into a manual gain value
	int percent_to_gain(int percent) const;

	// update_gain
	//
	// Updates the state of the gain control
	void update_gain(void);

	// worker
	//
	// Worker thread procedure used to pump data into the signal meter
	void worker(scalar_condition<bool>& started);
		
	//-------------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>			m_device;				// Device instance
	struct tunerprops					m_tunerprops = {};		// Tuner properties
	struct channelprops					m_channelprops = {};	// Channel properties
	struct signalprops					m_signalprops = {};		// Signal properties
	struct muxscanner::multiplex		m_muxdata = {};			// Multiplex properties
	mutable std::mutex					m_muxdatalock;			// Multiplex properties lock
	bool								m_isnew = false;		// New channel flag
	std::unique_ptr<signalmeter>		m_signalmeter;			// Signal meter instance
	std::unique_ptr<muxscanner>			m_muxscanner;			// Multiplex scanner instance
	std::vector<int>					m_manualgains;			// Manual gain values
	bool								m_result = false;		// Dialog result

	std::thread							m_worker;				// Worker thread
	std::exception_ptr					m_worker_exception;		// Exception on worker thread
	scalar_condition<bool>				m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>					m_stopped{ false };		// Data transfer stopped flag
	
	// CONTROLS
	//
	std::unique_ptr<CButton>			m_button_ok;			// OK button
	std::unique_ptr<CEdit>				m_edit_frequency;		// Frequency
	std::unique_ptr<CEdit>				m_edit_channelname;		// Channel name
	std::unique_ptr<CEdit>				m_edit_modulation;		// Channel modulation
	std::unique_ptr<CButton>			m_button_channelicon;	// Channel icon
	std::unique_ptr<CImage>				m_image_channelicon;	// Channel icon
	std::unique_ptr<CRadioButton>		m_radio_autogain;		// Automatic gain
	std::unique_ptr<CSettingsSlider>	m_slider_manualgain;	// Manual gain
	std::unique_ptr<CSettingsSlider>	m_slider_correction;	// Frequency correction
	std::unique_ptr<fftcontrol>			m_render_signalmeter;	// Signal meter
	std::unique_ptr<CEdit>				m_edit_signalgain;		// Active gain
	std::unique_ptr<CEdit>				m_edit_signalpower;		// Active power
	std::unique_ptr<CEdit>				m_edit_signalsnr;		// Active SNR
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __CHANNELSETTINGS_H_
