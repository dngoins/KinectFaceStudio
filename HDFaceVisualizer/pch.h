//------------------------------------------------------------------------------
// <copyright file="pch.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include <wrl/client.h>
#include <d3d11_2.h>
#include <d2d1_2.h>

#include <DirectXMath.h>

#include <wrl.h>
#include <algorithm>
#include <wincodec.h>
#include <UIAnimation.h>


#include <Windows.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "Gdi32.lib")

#pragma comment (lib, "d2d1")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "dxgi")

#pragma warning(disable: 4706)	
#pragma warning(disable: 4127)

namespace wrl = Microsoft::WRL;
namespace d2d = D2D1;

using namespace std;
using namespace d2d;
using namespace wrl;



#define ASSERT(expression) _ASSERTE(expression)
D2D1_COLOR_F	const COLOR_BLUE = { 0.26f, 0.56f, 0.87f, 1.0f };
D2D1_COLOR_F	const COLOR_DARKBLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
D2D1_COLOR_F	const COLOR_YELLOW = { 0.99f, 0.85f, 0.0f, 1.0f };
D2D1_COLOR_F	const COLOR_BLACK = { 0.0f, 0.0f, 0.07f, 1.0f };
D2D1_COLOR_F	const COLOR_WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
D2D1_COLOR_F	const COLOR_GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
D2D1_COLOR_F	const COLOR_RED = { 1.0f, 0.0f, 0.0f, 1.0f };

#ifdef _DEBUG
#define VERIFY(expression)	ASSERT(expression)
#define HR(expression)	ASSERT(S_OK == (expression	))


inline void TRACE(WCHAR const * const format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR output[512];
	vswprintf_s(output, format, args);

	OutputDebugString(output);

	va_end(args);
}

#else
#define VERIFY(expression) (expression)

struct ComException
{
	HRESULT const hr;
	ComException(HRESULT const value) :hr(value) {}

};

inline void HR(HRESULT const hr)
{
	if (S_OK != hr) throw ComException(hr);

}

#define TRACE __noop
#endif

