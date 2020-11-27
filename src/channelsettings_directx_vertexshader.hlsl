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

// visualization.waveform (Main_dx.cpp)
// https://github.com/xbmc/visualization.waveform
// Copyright (C) 2008-2020 Team Kodi
// GPLv2

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

cbuffer cbViewPort : register(b0)
{
	float g_viewPortWidth;
	float g_viewPortHeigh;
	float align1;
	float align2;
};

VS_OUT main(float4 pos : POSITION, float4 col : COLOR)
{
	VS_OUT r = (VS_OUT)0;
	r.pos.x = (pos.x / (g_viewPortWidth / 2.0)) - 1;
	r.pos.y = -(pos.y / (g_viewPortHeigh / 2.0)) + 1;
	r.pos.z = pos.z;
	r.pos.w = 1.0;
	r.col = col;
	return r;
}