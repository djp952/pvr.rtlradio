//---------------------------------------------------------------------------
// Copyright (c) 2020 Michael G. Brehm
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

#include <codecvt>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <kodi/Filesystem.h>
#include <kodi/gui/General.h>
#include <locale>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

#pragma warning(push, 4)

// DirectX rendering implementation based on:
//
// visualization.waveform (Main_dx.cpp)
// https://github.com/xbmc/visualization.waveform
// Copyright (C) 2008-2020 Team Kodi
// GPLv2

// graphics_state
//
// Defines the graphics state for this implementation
struct graphics_state {

	ID3D11Device1*								device = nullptr;
	ID3D11DeviceContext1*						context = nullptr;
	D3D11_VIEWPORT								viewport = {};
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexshader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	inputlayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelshader;
};

//---------------------------------------------------------------------------
// channelsettings::signal_meter_oncreate (private)
//
// Creates the rendering control for Kodi
//
// Arguments:
//
//	x		- Horizontal position
//	y		- Vertical position
//	w		- Width of control
//	h		- Height of control
//	device	- The device to use (ID3DDevice1*)

bool channelsettings::signal_meter_oncreate(int x, int y, int w, int h, kodi::HardwareContext device)
{
	HRESULT						hresult;			// Result from DirectX function call

	assert(device);
	assert(m_gfx == nullptr);

	// If the graphics state has already been initialized, something went wrong
	if(m_gfx != nullptr) return false;

	// Get the paths to the compiled shader files and ensure that they exist
	std::string pixelshaderfilename_a = kodi::GetAddonPath("resources\\shaders\\channelsettings_directx_pixelshader.cso");
	std::string vertexshaderfilename_a = kodi::GetAddonPath("resources\\shaders\\channelsettings_directx_vertexshader.cso");
	if(!kodi::vfs::FileExists(pixelshaderfilename_a) || !kodi::vfs::FileExists(vertexshaderfilename_a)) return false;

	// Convert the paths to UTF-16 for D3DReadFileToBlob
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring pixelshaderfilename_w = converter.from_bytes(pixelshaderfilename_a);
	std::wstring vertexshaderfilename_w = converter.from_bytes(vertexshaderfilename_a);

	// Load the shader files into ID3DBlob instances
	Microsoft::WRL::ComPtr<ID3DBlob> pixelshaderblob;
	Microsoft::WRL::ComPtr<ID3DBlob> vertexshaderblob;
	if(FAILED(D3DReadFileToBlob(pixelshaderfilename_w.c_str(), &pixelshaderblob))) return false;
	if(FAILED(D3DReadFileToBlob(vertexshaderfilename_w.c_str(), &vertexshaderblob))) return false;

	// Allocate and initialize a new graphics_state structure
	graphics_state* gfx = new graphics_state();
	gfx->device = static_cast<ID3D11Device1*>(device);
	gfx->context = static_cast<ID3D11DeviceContext1*>(kodi::gui::GetHWContext());

	// Set the viewport position and depth values
	gfx->viewport.TopLeftX = static_cast<FLOAT>(x);
	gfx->viewport.TopLeftY = static_cast<FLOAT>(y);
	gfx->viewport.Width = static_cast<FLOAT>(w);
	gfx->viewport.Height = static_cast<FLOAT>(h);
	gfx->viewport.MinDepth = static_cast<FLOAT>(0);
	gfx->viewport.MaxDepth = static_cast<FLOAT>(1);

	// Create the vertex shader instance
	hresult = gfx->device->CreateVertexShader(vertexshaderblob->GetBufferPointer(), vertexshaderblob->GetBufferSize(), nullptr, &gfx->vertexshader);
	if(FAILED(hresult)) { delete gfx; return false; }

	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hresult = gfx->device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderblob->GetBufferPointer(), vertexshaderblob->GetBufferSize(), &gfx->inputlayout);
	if(FAILED(hresult)) { delete gfx; return false; }

	// Create the pixel shader instance
	hresult = gfx->device->CreatePixelShader(pixelshaderblob->GetBufferPointer(), pixelshaderblob->GetBufferSize(), nullptr, &gfx->pixelshader);
	if(FAILED(hresult)) { delete gfx; return false; }

	// graphics_state is implementation dependent; store as a void*
	m_gfx = reinterpret_cast<void*>(gfx);

	return true;
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_ondirty (private)
//
// Determines if a render region is dirty
//
// Arguments:
//
//	NONE

bool channelsettings::signal_meter_ondirty(void)
{
	struct graphics_state* gfx = reinterpret_cast<struct graphics_state*>(m_gfx);
	assert(gfx);

	if(gfx == nullptr) return false;

	return false;
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_onrender (private)
//
// Invoked to render the control
//
// Arguments:
//
//	NONE

void channelsettings::signal_meter_onrender(void)
{
	struct graphics_state* gfx = reinterpret_cast<struct graphics_state*>(m_gfx);
	assert(gfx);

	if(gfx == nullptr) return;
}

//---------------------------------------------------------------------------
// channelsettings::signal_meter_onstop (private)
//
// Invoked to stop the rendering process
//
// Arguments:
//
//	NONE

void channelsettings::signal_meter_onstop(void)
{
	struct graphics_state* gfx = reinterpret_cast<struct graphics_state*>(m_gfx);
	assert(gfx);

	if(gfx == nullptr) return;

	delete gfx;						// Release the state structure
	m_gfx = nullptr;				// Reset the member variable
}

//---------------------------------------------------------------------------

#pragma warning(pop)
