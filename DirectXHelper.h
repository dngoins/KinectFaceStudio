//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
//
//*********************************************************

#pragma once

#include <ppltasks.h>
#include <robuffer.h>

using namespace Windows::Storage::Streams;

// Helper utilities for DirectX apps.
namespace DX
{

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}

	auto AsInspectable(Object^ const object) -> Microsoft::WRL::ComPtr<IInspectable>
	{
		return reinterpret_cast<IInspectable*>(object);
	}
	
	
	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline Windows::Foundation::Point ConvertToScaledPoint(Windows::Foundation::Point point, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return Windows::Foundation::Point(point.X * dpi / dipsPerInch, point.Y * dpi / dipsPerInch);
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	inline byte* GetPointerToPixelData(IBuffer^ pixelBuffer)
	{
		// Retrieve the buffer data.
		byte* pixels = nullptr;

		// Query the IBufferByteAccess interface.
		Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
		if (SUCCEEDED(reinterpret_cast<IInspectable*>(pixelBuffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess))))
		{
			bufferByteAccess->Buffer(&pixels);
		}

#pragma warning(suppress: 6102)
		return pixels;
#pragma warning(suppress: 6102)
	}

	
}
