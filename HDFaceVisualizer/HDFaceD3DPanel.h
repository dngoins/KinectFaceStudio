//------------------------------------------------------------------------------
// <copyright file="HDFaceD3DPanel.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

//	This control is based on the following article: http://msdn.microsoft.com/en-us/library/windows/apps/hh825871.aspx
//	And the code here: http://code.msdn.microsoft.com/windowsapps/XAML-SwapChainPanel-00cb688b

#pragma once
#include "pch.h"
#include "DirectXPanelBase.h"
#include "StepTimer.h"
#include "ShaderStructures.h"

#include "DirectXHelper.h"
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <math.h>
#include <ppltasks.h>
#include <windows.ui.xaml.media.dxinterop.h>

using namespace DirectX;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace DX;
using namespace Microsoft::Kinect::Face;
using namespace Windows::Graphics::Display;

namespace HDFaceVisualizer
{
	public value struct Vector3
	{
	public:
		float X;
		float Y;
		float Z;
	};

	public value struct DrawingVertex
	{
	public:
		Vector3 Location;
	};

    // Hosts a DirectX rendering surface that draws a spinning 3D cube using Direct3D.
    [Windows::Foundation::Metadata::WebHostHidden]
	public ref class HDFaceD3DPanel sealed : public HDFaceVisualizer::DirectXPanelBase
	{
	public:
		HDFaceD3DPanel();
		//~HDFaceD3DPanel();

		void StartRenderLoop();
		void StopRenderLoop();

		void UpdateMesh(Windows::Foundation::Collections::IVectorView<WindowsPreview::Kinect::CameraSpacePoint>^ newVertices, Windows::Foundation::Collections::IVectorView<UINT>^ newIndices, const Platform::Array<WindowsPreview::Kinect::ColorSpacePoint>^ newFacePoints, Microsoft::Kinect::Face::RectI faceRegion);

		void UpdateMesh(Windows::Foundation::Collections::IVectorView<WindowsPreview::Kinect::CameraSpacePoint>^ newVertices, Windows::Foundation::Collections::IVectorView<UINT>^ newIndices);

		void SetFaceColor(Windows::UI::Color color);
		void SetColorFrameReference(WindowsPreview::Kinect::ColorFrameReference ^ colorFrameRef);
		void SetDepthFrameReference(WindowsPreview::Kinect::DepthFrameReference ^ depthFrameRef);
		void SetBodyIndexFrameReference(WindowsPreview::Kinect::BodyIndexFrameReference ^ bodyIndexFrameRef);
		void SetCoordinateMapper(WindowsPreview::Kinect::CoordinateMapper ^ coordinateMapper);

		void SetScale(float xScale, float yScale);
		void SetTranslation(float x, float y);
		property Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ KinectCameraSource
		{
			Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ get();
			void set(Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ value);
		}

		property bool ShowMesh
		{
			bool get();
			void set(bool value);
		}

		property bool RemoveBackground
		{
			bool get();
			void set(bool value);
		}

		property bool ShowVideo
		{
			bool get();
			void set(bool value);
		}

	private protected:

		virtual void Render() override;
		virtual void CreateDeviceResources() override;
		virtual void CreateSizeDependentResources() override;

		Microsoft::WRL::ComPtr<IDXGIOutput>                 m_dxgiOutput;

		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_renderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      m_depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>          m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>           m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_inputLayout;

		Microsoft::WRL::ComPtr<ID3D11Buffer>                m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>                m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>                m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_pixelShaderBuffer;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_WireFrameRS;
		DX::ModelViewProjectionConstantBuffer               m_constantBufferData;

		float	                                            m_degreesPerSecond;

		Windows::Foundation::IAsyncAction^					m_renderLoopWorker;
		// Rendering loop timer.
		DX::StepTimer                                       m_timer;
		D2D1_COLOR_F										m_faceColor;
		bool												m_colorIsSet;
		std::unique_ptr<RGBQUAD>							m_pColorRGBX;
		bool												m_initialColorProcessed;
		bool												m_showMesh;
		Platform::Array<byte>^								m_colorArray;
		static const int									cDepthWidth = 512;
		WindowsPreview::Kinect::ColorFrameReference^		m_colorFrameRef;
		WindowsPreview::Kinect::DepthFrameReference^		m_depthFrameRef;
		WindowsPreview::Kinect::BodyIndexFrameReference^		m_bodyIndexFrameRef;
		UINT												m_sourceStride;
		float												m_xTranslation;
		float												m_yTranslation;
		float												m_xScale;
		float												m_yScale;
		Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ bitmap;
		bool												m_removeBackground;
		bool												m_showVideo;
		Platform::Array<WindowsPreview::Kinect::DepthSpacePoint>^  				   m_DepthPoints;
		WindowsPreview::Kinect::CoordinateMapper^			m_coordMapper;
		const Platform::Array<WindowsPreview::Kinect::ColorSpacePoint>^ m_facePoints;
		RectI												m_faceRegion;


	private:


		~HDFaceD3DPanel();

		void CreateDXBuffers();
		void UpdateDXBuffers();
		void InitRenderState();

		UINT modelVerticesCount;
		DX::VertexPositionColorNormal* modelVertices;

		UINT modelIndicesCount;
		unsigned int *modelIndices;


		critical_section buffersLock;

		XMVECTOR* faceNormalsCalculationData = nullptr;
		int faceNormalsCalculationDataLength = 0;
		bool faceNormalsCalculationDataCalculated = false;
		void CalculateNormals(DX::VertexPositionColorNormal* vertices, unsigned int verticesCount, unsigned int* indices, unsigned int indicesCount);

		void populateColorArrayWithWhiteValues(Platform::Array<byte>^ colorArray);
		byte * RemoveBackgroundPixels(byte * pixels);
		//void DrawFacePoints(Platform::Array<ColorSpacePoint>^ facePoints);
		void HDFaceD3DPanel::DrawFaceRectangle();

				

		inline float ScaleX(float x)
		{				
			auto logicalDpi = DisplayProperties::LogicalDpi;
			auto resScale = DisplayProperties::ResolutionScale;
			return (x*m_dpiX) / logicalDpi;
		}

		inline float ScaleY(float y)
		{
			auto logicalDpi = DisplayProperties::LogicalDpi;

			return ( y * m_dpiY)/logicalDpi;
		}

		inline Microsoft::WRL::ComPtr<ID2D1DeviceContext> CreateRenderTarget(Microsoft::WRL::ComPtr<ID2D1Factory1> const & factory, Microsoft::WRL::ComPtr<ID3D11Device> const & device)
		{
			ASSERT(factory);
			ASSERT(device);

			Microsoft::WRL::ComPtr<IDXGIDevice> dxdevice;
			HR(device.As(&dxdevice));

			Microsoft::WRL::ComPtr<ID2D1Device> d2device;
			HR(factory->CreateDevice(dxdevice.Get(), d2device.GetAddressOf()));

			Microsoft::WRL::ComPtr<ID2D1DeviceContext> target;

			HR(d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, target.GetAddressOf()));

			return target;

		}


		inline HRESULT CreateDevice(D3D_DRIVER_TYPE const type, wrl::ComPtr<ID3D11Device> & device)
		{
			ASSERT(!device);

			UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if _DEBUG
			flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, nullptr);
		}

		inline wrl::ComPtr<ID3D11Device> CreateDevice()
		{
			wrl::ComPtr<ID3D11Device> device;
			auto hr = CreateDevice(D3D_DRIVER_TYPE_HARDWARE, device);

			if (DXGI_ERROR_UNSUPPORTED == hr)
			{
				hr = CreateDevice(D3D_DRIVER_TYPE_WARP, device);
			}

			HR(hr);
			return device;

		}


		inline wrl::ComPtr<IDXGISwapChain1> CreateSwapChainForHwnd(wrl::ComPtr<ID3D11Device> const & device, HWND window)
		{
			ASSERT(device);
			ASSERT(window);

			auto const factory = GetDxgiFactory(device);

			DXGI_SWAP_CHAIN_DESC1 props = {};
			props.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			props.SampleDesc.Count = 1;
			props.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			props.BufferCount = 2;

			wrl::ComPtr<IDXGISwapChain1> swapChain;

			HR(factory->CreateSwapChainForHwnd(device.Get(), window, &props, nullptr, nullptr, swapChain.GetAddressOf()));

			return swapChain;

		}


		inline wrl::ComPtr<IDXGIFactory2> GetDxgiFactory(wrl::ComPtr<ID3D11Device> const & device)
		{
			ASSERT(device);
			wrl::ComPtr<IDXGIDevice> dxdevice;
			HR(device.As(&dxdevice));

			wrl::ComPtr<IDXGIAdapter> adapter;
			HR(dxdevice->GetAdapter(adapter.GetAddressOf()));

			wrl::ComPtr<IDXGIFactory2> factory;
			HR(adapter->GetParent(__uuidof(factory), reinterpret_cast<void **>(factory.GetAddressOf())));

			return factory;
		}

		inline void CreateDeviceSwapChainBitmap(wrl::ComPtr<IDXGISwapChain1> const & swapchain,
			wrl::ComPtr<ID2D1DeviceContext> const & target)
		{

			ASSERT(swapchain);
			ASSERT(target);

			wrl::ComPtr<IDXGISurface> surface;

			HR(swapchain->GetBuffer(0, __uuidof(surface),
				reinterpret_cast<void **>(surface.GetAddressOf())));

			auto const props = d2d::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				d2d::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

			wrl::ComPtr<ID2D1Bitmap1> bitmap;

			HR(target->CreateBitmapFromDxgiSurface(surface.Get(), props, bitmap.GetAddressOf()));

			target->SetTarget(bitmap.Get());
			

		}

		

    };
}
