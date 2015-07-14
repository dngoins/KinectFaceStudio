//------------------------------------------------------------------------------
// <copyright file="pch.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


#pragma once

#include <collection.h>
#include "App.xaml.h"

#pragma once
#include <wrl.h>
#include <algorithm>
#include <wincodec.h>


#pragma warning(disable: 4706)	
#pragma warning(disable: 4127)

namespace wrl = Microsoft::WRL;

using namespace std;
using namespace wrl;



#define ASSERT(expression) _ASSERTE(expression)

#ifdef _DEBUG
#define VERIFY(expression)	ASSERT(expression)
#define HR(expression)	ASSERT(S_OK == (expression	))



inline void TRACE(WCHAR const * const format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR output[2048];
	vswprintf_s(output, format, args);

	OutputDebugString(output);

	va_end(args);
}

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(ComPtr<Interface> pInterfaceToRelease)
{
	if (pInterfaceToRelease)
	{

		pInterfaceToRelease.Reset();
		pInterfaceToRelease = nullptr;
	}
}


// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != nullptr)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = nullptr;
	}
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






