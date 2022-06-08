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

#ifndef __RENDERINGCONTROL_H_
#define __RENDERINGCONTROL_H_
#pragma once

#include <kodi/c-api/gui/controls/rendering.h>
#include <kodi/gui/Window.h>
#include <kodi/gui/renderHelper.h>

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// Class renderingcontrol
//
// Replacement CRendering control; required since the one provided by Kodi
// cannot be made to work for either of its valid use cases.  If you derive
// from CRendering, it will invoke a virtual member function during construction
// that needs to be overridden (oops).  If you instead opt to use independent
// static callback functions, the required pre and post-rendering code will
// never get invoked, and you can't do that in your callbacks because the
// member variable you need is marked as private to CRendering.
//
// There is also another problem that can occur if the application is being
// shut down while a render control is active; it's calling Stop() after the
// object has been destroyed; just take Stop() out for now, I don't need it

class ATTR_DLL_LOCAL renderingcontrol : public kodi::gui::CAddonGUIControlBase
{
public:

	// Instance Constructor
	//
	renderingcontrol(kodi::gui::CWindow* window, int controlid) : CAddonGUIControlBase(window)
	{
		// Find the control handle
		m_controlHandle = m_interface->kodi_gui->window->get_control_render_addon(m_interface->kodiBase,
			m_Window->GetControlHandle(), controlid);

		// Initialize the control callbacks; be advised this implicitly calls the OnCreate callback
		if(m_controlHandle) m_interface->kodi_gui->control_rendering->set_callbacks(m_interface->kodiBase,
			m_controlHandle, this, OnCreate, OnRender, OnStop, OnDirty);
	}

	// Destructor
	//
	~renderingcontrol() override
	{
		// Destroy the control
		m_interface->kodi_gui->control_rendering->destroy(m_interface->kodiBase, m_controlHandle);
	}

	//----------------------------------------------------------------------------
	// Member Functions
	//----------------------------------------------------------------------------

	// dirty (virtual)
	//
	// Determines if a region is dirty and needs to be rendered
	virtual bool dirty(void) 
	{
		return false;
	}

	// render (virtual)
	//
	// Render all dirty regions
	virtual void render(void)
	{
	}

	// Stop (virtual)
	//
	// Stop the rendering process
	//virtual void Stop(void)
	//{
	//}

protected:

	//-------------------------------------------------------------------------
	// Protected Member Variables
	//-------------------------------------------------------------------------

	size_t					m_left{};		// Horizontal position of the control
	size_t					m_top{};		// Vertical position of the control
	size_t					m_width{};		// Width of the control
	size_t					m_height{};		// Height of the control

	kodi::HardwareContext	m_device{};		// Device to use, only set for DirectX

private:

	// OnCreate
	//
	// Invoked by the underlying rendering control when it's being initialize
	static bool OnCreate(KODI_GUI_CLIENT_HANDLE handle, int x, int y, int w, int h, ADDON_HARDWARE_CONTEXT device)
	{
		assert(handle);
		renderingcontrol* instance = reinterpret_cast<renderingcontrol*>(handle);

		// This is called during object construction, virtual member functions are not available.
		// Store off all of the provided size, position, and device information
		instance->m_left = static_cast<size_t>(x);
		instance->m_top = static_cast<size_t>(y);
		instance->m_width = static_cast<size_t>(w);
		instance->m_height = static_cast<size_t>(h);
		instance->m_device = device;

		// Access the rendering helper instance
		instance->m_renderHelper = kodi::gui::GetRenderHelper();

		return true;
	}

	// OnDirty
	//
	// Invoked by the underlying rendering control to inquire about dirty regions
	static bool OnDirty(KODI_GUI_CLIENT_HANDLE handle)
	{
		assert(handle);
		return reinterpret_cast<renderingcontrol*>(handle)->dirty();
	}

	// OnRender
	//
	// Invoked by the underlying rendering control to render any dirty regions
	static void OnRender(KODI_GUI_CLIENT_HANDLE handle)
	{
		assert(handle);
		renderingcontrol* instance = reinterpret_cast<renderingcontrol*>(handle);

		if(!instance->m_renderHelper) return;

		// Render the control
		instance->m_renderHelper->Begin();
		instance->render();
		instance->m_renderHelper->End();
	}

	// OnStop
	//
	// Invoked by the underlying rendering control to stop the rendering process
	static void OnStop(KODI_GUI_CLIENT_HANDLE /*handle*/)
	{
		//
		// Removed implementation due to this being called after the object pointed
		// to by handle has been destroyed if the application is closing. The render
		// helper instance will be released automatically during destruction
		//

		//assert(handle);
		//renderingcontrol* instance = reinterpret_cast<renderingcontrol*>(handle);

		//// Stop rendering
		//instance->Stop();

		//// Reset the rendering helper instance pointer
		//instance->m_renderHelper = nullptr;
	}

	//-------------------------------------------------------------------------
	// Member Variables
	//-------------------------------------------------------------------------

	std::shared_ptr<kodi::gui::IRenderHelper>	m_renderHelper;		// Helper
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __RENDERINGCONTROL_H_
