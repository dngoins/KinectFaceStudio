//------------------------------------------------------------------------------
// <copyright file="HDFaceD3DPanel.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

//	This control is based on the following article: http://msdn.microsoft.com/en-us/library/windows/apps/hh825871.aspx
//	And the code here: http://code.msdn.microsoft.com/windowsapps/XAML-SwapChainPanel-00cb688b

#pragma once
#include "pch.h"

#pragma warning( disable:28204 )

#include "HDFaceD3DPanel.h"

using namespace Microsoft::WRL;
using namespace Windows::System::Threading;
using namespace DirectX;
using namespace HDFaceVisualizer;
using namespace DX;
using namespace WindowsPreview::Kinect;
using namespace Platform::Runtime::InteropServices;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Foundation::Diagnostics;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;
using namespace Concurrency;


void HDFaceD3DPanel::CalculateNormals(DX::VertexPositionColorNormal* vertices, unsigned int verticesCount, unsigned int* indices, unsigned int indicesCount)
{
	unsigned int requiredTempDataLength = verticesCount * 2;

	if (faceNormalsCalculationDataLength != requiredTempDataLength)
	{
		if (faceNormalsCalculationData != nullptr)
		{
			_aligned_free(faceNormalsCalculationData);
		}

		// We need two temporary XMVECTORs for each vertex. These will be packed as position0, normal0, position1, normal1, ...
		size_t countRequired = requiredTempDataLength *sizeof(XMVECTOR);
		faceNormalsCalculationData = (XMVECTOR*)_aligned_malloc(countRequired, __alignof(XMVECTOR));

		if (nullptr == faceNormalsCalculationData)
		{
			return;
		}

		ZeroMemory(faceNormalsCalculationData, countRequired);

		faceNormalsCalculationDataLength = requiredTempDataLength;
	}

	// Load face tracking vertex locations
	for (unsigned int i = 0; i < verticesCount; i++)
	{
		XMFLOAT4 currectVector(vertices[i].pos);
		faceNormalsCalculationData[2 * i] = XMLoadFloat4(&currectVector);
	}

	// Compute sum of area vectors adjacent to each vertex. This results in an area-weighted average for the face normals.
	for (unsigned int i = 0; i < indicesCount; i+=3)
	{
		uint32 x = indices[i];
		uint32 y = indices[i+1];
		uint32 z = indices[i+2];

		const XMUINT3 &tri = XMUINT3(x, y, z);

		const XMVECTOR &xi = faceNormalsCalculationData[2 * tri.x];
		const XMVECTOR &xj = faceNormalsCalculationData[2 * tri.y];
		const XMVECTOR &xk = faceNormalsCalculationData[2 * tri.z];

		XMVECTOR faceNormal = XMVector3Cross(XMVectorSubtract(xi, xj), XMVectorSubtract(xi, xk));

		faceNormalsCalculationData[2 * tri.x + 1] = XMVectorAdd(faceNormalsCalculationData[2 * tri.x + 1], faceNormal);
		faceNormalsCalculationData[2 * tri.y + 1] = XMVectorAdd(faceNormalsCalculationData[2 * tri.y + 1], faceNormal);
		faceNormalsCalculationData[2 * tri.z + 1] = XMVectorAdd(faceNormalsCalculationData[2 * tri.z + 1], faceNormal);
	}

	FXMVECTOR vInitialColor = { 0.5f, 0.5f, 0.0f, 1.0f };
	// Normalize normals and store into the XMFLOAT3 data in the vertex buffer
	for (unsigned int i = 0; i < verticesCount; i++)
	{
		XMStoreFloat4(&vertices[i].pos, faceNormalsCalculationData[2 * i]);
		XMStoreFloat3(&vertices[i].normal, XMVector3Normalize(faceNormalsCalculationData[2 * i + 1]));
		XMStoreFloat4(&vertices[i].color, vInitialColor);
	}
}

void HDFaceD3DPanel::UpdateMesh(Windows::Foundation::Collections::IVectorView<WindowsPreview::Kinect::CameraSpacePoint>^ newVertices, Windows::Foundation::Collections::IVectorView<UINT>^ newIndices)
//, Windows::Foundation::Collections::IVectorView<ColorSpacePoint>^ rgbFacePoints)
{
	this->UpdateMesh(newVertices, newIndices, nullptr, RectI());
}

void HDFaceD3DPanel::UpdateMesh(Windows::Foundation::Collections::IVectorView<WindowsPreview::Kinect::CameraSpacePoint>^ newVertices, Windows::Foundation::Collections::IVectorView<UINT>^ newIndices, const Platform::Array<ColorSpacePoint>^ rgbFacePoints, Microsoft::Kinect::Face::RectI faceRegion)
{
	bool dxBufferToChange = false;
	XMFLOAT3 currentNormal(0.0f, 0.0f, 0.0f);

	UINT newVerticesCount = newVertices->Size;

	if (newVerticesCount != this->modelVerticesCount)
	{
		dxBufferToChange = true;
		delete[] modelVertices;

		this->modelVerticesCount = newVerticesCount;
		this->modelVertices = new DX::VertexPositionColorNormal[this->modelVerticesCount];
	}
	
	if (nullptr != rgbFacePoints)
	{
		//convert to color point
		//int index = modelVerticesCount / 2;
		//auto facePoint = rgbFacePoints->get(index);
		//auto vert = newVertices->GetAt(index);
		m_facePoints = rgbFacePoints;
		m_faceRegion = faceRegion;
	//	m_xTranslation = vert.X - facePoint.X;
	//	m_yTranslation = vert.Y - facePoint.Y;
	
	}

	for (UINT i = 0; i < this->modelVerticesCount; i++)
	{
		auto currentVertex = newVertices->GetAt(i);
		
		//if (nullptr != rgbFacePoints)
		//{
		//	//convert to color point
		//	auto facePoint = rgbFacePoints->get(i);

		//	//now check for infinity
		//	if (facePoint.X == -(INFINITY) || facePoint.Y == -(INFINITY))
		//	{
		//		this->modelVertices[i] = { XMFLOAT4(currentVertex.X, currentVertex.Y, currentVertex.Z, 0.0f), currentNormal };

		//	}
		//	else
		//	this->modelVertices[i] = { XMFLOAT4(facePoint.X/cColorWidth, facePoint.Y/cColorHeight, currentVertex.Z, 0.0f), currentNormal };
		//}
		//else
		{
			this->modelVertices[i] = { XMFLOAT4(currentVertex.X, currentVertex.Y, currentVertex.Z, 0.0f), currentNormal };

		}
	}

	UINT newIndicesCount = newIndices->Size;

	if (newIndicesCount != this->modelIndicesCount)
	{
		dxBufferToChange = true;
		delete[] modelIndices;

		this->modelIndicesCount = newIndicesCount;
		this->modelIndices = new unsigned int[this->modelIndicesCount];

		for (UINT i = 0; i < this->modelIndicesCount; i++)
		{
			this->modelIndices[i] = newIndices->GetAt(i);
		}
	}

	CalculateNormals(this->modelVertices, this->modelVerticesCount, this->modelIndices, this->modelIndicesCount);

	if (dxBufferToChange)
	{
		this->CreateDXBuffers();
	}
	else
	{
		UpdateDXBuffers();
	}
}

HDFaceD3DPanel::HDFaceD3DPanel() :
    m_degreesPerSecond(0),
	modelVerticesCount(0),
	modelVertices(nullptr),
	modelIndicesCount(0),
	modelIndices(nullptr),	
	faceNormalsCalculationData(nullptr),
	faceNormalsCalculationDataLength(0),
	faceNormalsCalculationDataCalculated(false),
	m_initialColorProcessed(false),
	m_pColorRGBX(nullptr),
	m_sourceStride(0),
	m_xTranslation(-0.176),
	m_yTranslation(0.117),
	m_xScale(0.48),
	m_yScale(0.53),
	m_showMesh(false),
	m_removeBackground(false)
{
    critical_section::scoped_lock lock(m_criticalSection);
    CreateDeviceIndependentResources();
    CreateDeviceResources();
    CreateSizeDependentResources();

	m_pColorRGBX = std::make_unique<RGBQUAD>();
	
	m_sourceStride = cColorWidth * sizeof(RGBQUAD);
	this->m_DepthPoints = ref new Array<DepthSpacePoint>(cColorWidth * cColorHeight);
}

HDFaceD3DPanel::~HDFaceD3DPanel()
{
	if (m_pColorRGBX)
	{
		//m_pColorRGBX.reset();
		m_pColorRGBX = nullptr;
		//delete[] m_pColorRGBX;

	}
    m_renderLoopWorker->Cancel();

	if (nullptr != modelVertices)
		delete[] modelVertices;

	if (nullptr != modelIndices)
		delete[] modelIndices;

	

	if (faceNormalsCalculationData != nullptr)
		_aligned_free(faceNormalsCalculationData);


}

void HDFaceD3DPanel::StartRenderLoop()
{
    // If the animation render loop is already running then do not start another thread.
    if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
    {
        return;
    }

    // Create a task that will be run on a background thread.
    auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
    {
        // Calculate the updated frame and render once per vertical blanking interval.
        while (action->Status == AsyncStatus::Started)
        {
            m_timer.Tick([&]()
            {
                critical_section::scoped_lock lock(m_criticalSection);
                Render();
            });

            // Halt the thread until the next vblank is reached.
            // This ensures the app isn't updating and rendering faster than the display can refresh,
            // which would unnecessarily spend extra CPU and GPU resources.  This helps improve battery life.
            m_dxgiOutput->WaitForVBlank();
        }
    });
    
    // Run task on a dedicated high priority background thread.
    m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void HDFaceD3DPanel::StopRenderLoop()
{
    // Cancel the asynchronous task and let the render thread exit.
    m_renderLoopWorker->Cancel();
}

void HDFaceD3DPanel::Render()
{
	
	auto colorProcessed = false;
	RGBQUAD *pColorBuffer = nullptr;

	if (nullptr == modelVertices || nullptr == modelIndices || nullptr == m_facePoints )
	{
		return;
	}

	if (!m_loadingComplete || m_timer.GetFrameCount() == 0)
	{
		return;
	}

	buffersLock.lock();

	static const XMVECTORF32 eye = { 0.0f, 0.0f, -0.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, 0.0f, 1.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	// Convert degrees to radians, then convert seconds to rotation angle
	float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
	double totalRotation = m_timer.GetTotalSeconds() * radiansPerSecond;
	float animRadians = (float)fmod(totalRotation, XM_2PI);

	// Prepare to pass the view matrix, and updated model matrix, to the shader
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(animRadians)));

	float xScale = m_xScale;// 512.0f / 1920;
	float yScale = m_yScale;// 424.0f / 1080;

	FXMVECTOR scaling = { xScale, yScale, 0.05f, 1.0f };
	static const FXMVECTOR rotationOrgin = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const FXMVECTOR rotationQuat = { 0.0f, 0.0f, 0.0f, 0.0f };
	XMVECTORF32 translation = { m_xTranslation, m_yTranslation, 0.0f, 0.0f };
	FXMVECTOR vInitColor = { 0.2f, 0.7f, 0.2f, 1.0f };

	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixAffineTransformation(scaling, rotationOrgin, rotationQuat, translation)));

	//set the color to color the face
	if (m_colorIsSet)
	{
		FXMVECTOR vFaceColor = { m_faceColor.r, m_faceColor.g, m_faceColor.b, m_faceColor.a };
		XMStoreFloat4(&m_constantBufferData.vertexcolor, vFaceColor);
	}
	else
	{
		XMStoreFloat4(&m_constantBufferData.vertexcolor, vInitColor);
	}

	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_renderTargetView.Get() };

	m_d3dContext->OMSetRenderTargets(1, targets, m_depthStencilView.Get());
	auto device = CreateDevice();
	auto m_target = CreateRenderTarget(m_d2dFactory, device);
	//m_swapChain = CreateSwapChainForHwnd(device, m_hWnd);

	//CreateDeviceSwapChainBitmap(m_swapChain, m_target);

	//set DPI make sure project is DPI aware see bottom
	m_target->SetDpi(m_dpiX, m_dpiY);
	auto size = m_target->GetSize();

	// Clear the back buffer and depth stencil view.
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), DirectX::Colors::White);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	if (m_showVideo)
	{
		if (nullptr != this->m_colorFrameRef)
		{
			auto colorFrame = m_colorFrameRef->AcquireFrame();

			Windows::Storage::Streams::IBuffer^ colorBuffer = nullptr;
			if (nullptr != colorFrame)
			{
				auto frameWidth = colorFrame->FrameDescription->Width;
				auto frameHeight = colorFrame->FrameDescription->Height;
				m_sourceStride = cColorWidth * sizeof(RGBQUAD);

				auto imageFormat = colorFrame->RawColorImageFormat;
				if (imageFormat == ColorImageFormat::Bgra)
				{
					colorBuffer = colorFrame->LockRawImageBuffer();
					colorProcessed = true;
				}
				else
				{
					m_colorArray = ref new Platform::Array<byte>(frameWidth * frameHeight * sizeof(RGBQUAD));
					colorFrame->CopyConvertedFrameDataToArray(m_colorArray, ColorImageFormat::Bgra);

					DataWriter^ dw = ref new DataWriter();
					dw->WriteBytes(m_colorArray);
					colorBuffer = dw->DetachBuffer();
					colorProcessed = true;

				}


				// Convert from C++/CX to the ABI IInspectable*:
				byte* pixels(nullptr);
				byte* removedBackground(nullptr);

				try
				{
					// Get the IBufferByteAccess interface:
					pixels = GetPointerToPixelData(colorBuffer);
					if (m_removeBackground)
					{
						removedBackground = RemoveBackgroundPixels(pixels);
						pixels = removedBackground;
					}

					pColorBuffer = reinterpret_cast<RGBQUAD*>(pixels);
					m_pColorRGBX.release();
					m_pColorRGBX.reset(pColorBuffer);
					colorProcessed = true;
					m_initialColorProcessed = true;
					pColorBuffer = nullptr;

				}
				catch (const std::exception&)
				{
					colorProcessed = false;
					delete colorFrame;

				}

				if (nullptr != m_d2dTargetBitmap && colorProcessed)
				{
					D2D1_RECT_U destinationSize = {};
					destinationSize.top = 0;
					destinationSize.bottom = 1080;
					destinationSize.left = 0;
					destinationSize.right = 1920;

					//auto hr = m_d2dTargetBitmap->CopyFromMemory(NULL, m_pColorRGBX.get(), m_sourceStride);

					auto hr = m_d2dTargetBitmap->CopyFromMemory(&destinationSize, m_pColorRGBX.get(), m_sourceStride);

				}


			}
			else 	if (m_initialColorProcessed)
			{

				//we have a previous color array
				DataWriter^ dw = ref new DataWriter();
				dw->WriteBytes(m_colorArray);
				auto colorBuffer = dw->DetachBuffer();

				// Convert from C++/CX to the ABI IInspectable*:
				byte* pixels(nullptr);

				// Get the IBufferByteAccess interface:
				pixels = GetPointerToPixelData(colorBuffer);
				pColorBuffer = reinterpret_cast<RGBQUAD*>(pixels);

				if (nullptr != m_d2dTargetBitmap)
				{
					D2D1_RECT_U destinationSize = {};
					destinationSize.top = 0;
					destinationSize.bottom = 1080;
					destinationSize.left = 0;
					destinationSize.right = 1920;

					auto hr = m_d2dTargetBitmap->CopyFromMemory(&destinationSize, pColorBuffer, m_sourceStride);

				}
				pColorBuffer = nullptr;
			}
		}
		else if (m_initialColorProcessed)
		{
			//we have a previous color array
			DataWriter^ dw = ref new DataWriter();
			dw->WriteBytes(m_colorArray);
			auto colorBuffer = dw->DetachBuffer();

			// Convert from C++/CX to the ABI IInspectable*:
			byte* pixels(nullptr);

			// Get the IBufferByteAccess interface:
			pixels = GetPointerToPixelData(colorBuffer);
			pColorBuffer = reinterpret_cast<RGBQUAD*>(pixels);

			if (nullptr != m_d2dTargetBitmap)
			{
				D2D1_RECT_U destinationSize = {};
				destinationSize.top = 0;
				destinationSize.bottom = 1080;
				destinationSize.left = 0;
				destinationSize.right = 1920;

				auto hr = m_d2dTargetBitmap->CopyFromMemory(&destinationSize, pColorBuffer, m_sourceStride);
			}
			pColorBuffer = nullptr;

		}
	}

	//auto facePoints = m_facePoints->Data;
	//auto faceSize = m_facePoints->Length;
	//DrawFacePoints(m_facePoints);
	DrawFaceRectangle();
	
    // Prepare the constant buffer to send it to the Graphics device.
    m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//PixelShaderBufferType* dataPtr;
	unsigned int bufferNumber;

	//// Lock the constant buffer so it can be written to.
	//HRESULT result = m_d3dContext->Map(m_pixelShaderBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	//

	//// Get a pointer to the data in the constant buffer.
	//dataPtr = (PixelShaderBufferType*)mappedResource.pData;

	//// Copy the matrices into the constant buffer.
	//dataPtr->color.x = 0.5f;
	//dataPtr->color.y = 0.5f;
	//dataPtr->color.z = 0.0f;
	//dataPtr->color.w = 1.0f;
	//
	//// Unlock the constant buffer.
	//m_d3dContext->Unmap(m_pixelShaderBuffer.Get(), 0);

	if (m_showMesh)
	{
		//// Set the position of the constant buffer in the vertex shader.
		bufferNumber = 0;

		// Finanly set the constant buffer in the vertex shader with the updated values.
		m_d3dContext->VSSetConstantBuffers(bufferNumber, 1, m_pixelShaderBuffer.GetAddressOf());

		// Each vertex is one instance of the VertexPositionColor struct.
		UINT stride = sizeof(VertexPositionColorNormal);
		UINT offset = 0;
		m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

		m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//TO Use a wire frame just un-comment this line
		// Attach the WireFrame Rasterizer
		m_d3dContext->RSSetState(m_WireFrameRS.Get());

		m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_d3dContext->IASetInputLayout(m_inputLayout.Get());

		// Attach our vertex shader.
		m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);

		// Send the constant buffer to the Graphics device.
		m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

		// Attach our pixel shader.
		m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);


		// Draw the objects.
		m_d3dContext->DrawIndexed(this->modelIndicesCount, 0, 0);
	}

    Present();

	buffersLock.unlock();
}

void HDFaceD3DPanel::UpdateDXBuffers()
{
	if (!m_loadingComplete || m_timer.GetFrameCount() == 0)
	{
		return;
	}

	buffersLock.lock();

	D3D11_MAPPED_SUBRESOURCE vertexBufferMappedResource;
	ZeroMemory(&vertexBufferMappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	ThrowIfFailed(
		m_d3dContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexBufferMappedResource)
		);
	//	Update the vertex buffer here.
	auto verteciesSize = sizeof(DX::VertexPositionColorNormal) * this->modelVerticesCount;
	memcpy(vertexBufferMappedResource.pData, modelVertices, verteciesSize);
	//	Reenable GPU access to the vertex buffer data.
	m_d3dContext->Unmap(m_vertexBuffer.Get(), 0);

	//	Disable GPU access to the index buffer data.
	D3D11_MAPPED_SUBRESOURCE indexBufferMappedResource;
	ZeroMemory(&indexBufferMappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	ThrowIfFailed(
		m_d3dContext->Map(m_indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &indexBufferMappedResource)
		);
	//	Update the index buffer here.
	auto indicesSize = this->modelIndicesCount * sizeof(unsigned int);
	memcpy(indexBufferMappedResource.pData, modelIndices, indicesSize);
	//	Reenable GPU access to the index buffer data.
	m_d3dContext->Unmap(m_indexBuffer.Get(), 0);

	////update the pixel shader buffer
	//D3D11_MAPPED_SUBRESOURCE pixelShaderBufferMappedResource;
	//ZeroMemory(&pixelShaderBufferMappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	//ThrowIfFailed(
	//	m_d3dContext->Map(m_pixelShaderBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &pixelShaderBufferMappedResource)
	//	);

	buffersLock.unlock();
}

void HDFaceD3DPanel::SetColorFrameReference(WindowsPreview::Kinect::ColorFrameReference ^ colorFrameRef)
{
	m_colorFrameRef = colorFrameRef;
}
void HDFaceD3DPanel::SetDepthFrameReference(WindowsPreview::Kinect::DepthFrameReference ^ depthFrameRef)
{
	m_depthFrameRef = depthFrameRef;
}

void HDFaceD3DPanel::SetCoordinateMapper(WindowsPreview::Kinect::CoordinateMapper ^ coordinateMapper)
{
	m_coordMapper = coordinateMapper;
}

void HDFaceD3DPanel::SetBodyIndexFrameReference(WindowsPreview::Kinect::BodyIndexFrameReference ^ bodyIndexFrameRef)
{
	m_bodyIndexFrameRef = bodyIndexFrameRef;
}

void HDFaceD3DPanel::CreateDXBuffers()
{
	if (nullptr == modelVertices || nullptr == modelIndices)
	{
		return;
	}

	// Load mesh vertices. Each vertex has a position and a color.

	m_vertexBuffer = nullptr;
	
	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = modelVertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	UINT modelVerticesSize = this->modelVerticesCount * sizeof(VertexPositionColorNormal);
	CD3D11_BUFFER_DESC vertexBufferDesc(modelVerticesSize, D3D11_BIND_VERTEX_BUFFER);
	vertexBufferDesc.ByteWidth = modelVerticesSize;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	ThrowIfFailed(
		m_d3dDevice->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&m_vertexBuffer
		)
		);

	m_indexBuffer = nullptr;

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = modelIndices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	UINT modelIndicesSize = this->modelIndicesCount * sizeof(unsigned int);

	CD3D11_BUFFER_DESC indexBufferDesc(modelIndicesSize, D3D11_BIND_INDEX_BUFFER);
	indexBufferDesc.ByteWidth = modelIndicesSize;
	indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	ThrowIfFailed(
		m_d3dDevice->CreateBuffer(
		&indexBufferDesc,
		&indexBufferData,
		&m_indexBuffer
		)
		);

	

}

void HDFaceD3DPanel::CreateDeviceResources()
{
    DirectXPanelBase::CreateDeviceResources();

    // Retrieve DXGIOutput representing the main adapter output.
    ComPtr<IDXGIFactory1> dxgiFactory;
    ThrowIfFailed(
        CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
        );

    ComPtr<IDXGIAdapter> dxgiAdapter;
    ThrowIfFailed(
        dxgiFactory->EnumAdapters(0, &dxgiAdapter)
        );

    ThrowIfFailed(
        dxgiAdapter->EnumOutputs(0, &m_dxgiOutput)
        );

	
    // Asynchronously load vertex shader and create input layout.
    auto loadVSTask = DX::ReadDataAsync(L"HDFaceVisualizer\\HDFaceVertexShader.cso");
    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
        ThrowIfFailed(
            m_d3dDevice->CreateVertexShader(
            &fileData[0],
            fileData.size(),
            nullptr,
            &m_vertexShader
            )
            );

        static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        ThrowIfFailed(
            m_d3dDevice->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            &fileData[0],
            fileData.size(),
            &m_inputLayout
            )
            );
    });

	
    // Asynchronously load vertex shader and create constant buffer.
    auto loadPSTask = DX::ReadDataAsync(L"HDFaceVisualizer\\HDFacePixelShader.cso");
    auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
        ThrowIfFailed(
            m_d3dDevice->CreatePixelShader(
            &fileData[0],
            fileData.size(),
            nullptr,
            &m_pixelShader
            )
            );

		/*CD3D11_BUFFER_DESC pixelBufferDesc(sizeof(PixelShaderBuffer), D3D11_BIND_CONSTANT_BUFFER);
		ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
			&pixelBufferDesc,
			nullptr,
			&m_pixelShaderBuffer
			)
			);*/
		
        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
        ThrowIfFailed(
            m_d3dDevice->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
            )
            );
    });

    // Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this]() {
		CreateDXBuffers();
	});

    // Once the cube is loaded, the object is ready to be rendered.
    createCubeTask.then([this]() {
        m_loadingComplete = true;
    });

	//create brushes

}

void HDFaceD3DPanel::CreateSizeDependentResources()
{
    m_renderTargetView = nullptr;
    m_depthStencilView = nullptr;

    DirectXPanelBase::CreateSizeDependentResources();

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        );

    // Create render target view.
    ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &m_renderTargetView)
        );
	
	
    // Create and set viewport.
    D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        m_renderTargetWidth,
        m_renderTargetHeight
        );

    m_d3dContext->RSSetViewports(1, &viewport);

    // Create depth/stencil buffer descriptor.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        static_cast<UINT>(m_renderTargetWidth),
        static_cast<UINT>(m_renderTargetHeight),
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL
        );

    // Allocate a 2-D surface as the depth/stencil buffer.
    ComPtr<ID3D11Texture2D> depthStencil;
    ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil)
        );

    // Create depth/stencil view based on depth/stencil buffer.
    const CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
	
    ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
        depthStencil.Get(),
        &depthStencilViewDesc,
        &m_depthStencilView
        )
        );

	
    float aspectRatio = m_renderTargetWidth / m_renderTargetHeight;

	float cameraFovAngleY = XMConvertToRadians(70.0f);
    if (aspectRatio < 1.0f)
    {
		cameraFovAngleY /= aspectRatio;
    }

    XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		cameraFovAngleY,
        aspectRatio,
        0.01f,
        100.0f
        );

	XMMATRIX orientationMatrix = XMMatrixIdentity();

    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
        );

	//m_renderTargetView->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);

	InitRenderState();
}

void HDFaceD3DPanel::SetFaceColor(Windows::UI::Color color)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_colorIsSet = true;
	m_faceColor = DX::ConvertToColorF(color);
}

void HDFaceD3DPanel::InitRenderState()
{
	D3D11_RASTERIZER_DESC wfd;
	ZeroMemory(&wfd, sizeof(D3D11_RASTERIZER_DESC));
	wfd.FillMode = D3D11_FILL_WIREFRAME;
	wfd.CullMode = D3D11_CULL_NONE;
	
	//to enable clipping
	wfd.DepthClipEnable = true;

	m_d3dDevice->CreateRasterizerState(&wfd, m_WireFrameRS.GetAddressOf());

}

void HDFaceD3DPanel::SetScale(float xScale, float yScale)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_xScale = xScale;
	m_yScale = yScale;
}

void HDFaceD3DPanel::SetTranslation(float x, float y)
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_xTranslation = x;
	m_yTranslation = y;
}

void HDFaceD3DPanel::populateColorArrayWithWhiteValues(Platform::Array<byte>^ colorArray) {

	auto size = colorArray->Length;
	for (UINT i = 0; i < size; i++)
	{
		critical_section::scoped_lock lock(m_criticalSection);
		colorArray->set(i, 255);
	}

}


WriteableBitmap^ HDFaceD3DPanel::KinectCameraSource::get()
{

	return this->bitmap;

}
void HDFaceD3DPanel::KinectCameraSource::set(WriteableBitmap^ value)
{
	if (this->bitmap != value)
	{
		critical_section::scoped_lock lock(m_criticalSection);
		this->bitmap = value;

	//	PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("KinectCameraSource");
	
	}
}


bool HDFaceD3DPanel::ShowMesh::get()
{
	return this->m_showMesh;
}

void HDFaceD3DPanel::ShowMesh::set(bool value)
{
	if (this->m_showMesh != value)
	{
		critical_section::scoped_lock lock(m_criticalSection);
		this->m_showMesh = value;

		//	PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("KinectCameraSource");

	}
}


bool HDFaceD3DPanel::ShowVideo::get()
{
	return this->m_showVideo;
}

void HDFaceD3DPanel::ShowVideo::set(bool value)
{
	if (this->m_showVideo != value)
	{
		critical_section::scoped_lock lock(m_criticalSection);
		this->m_showVideo = value;

		//	PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("KinectCameraSource");

	}
}

bool HDFaceD3DPanel::RemoveBackground::get()
{
	return this->m_removeBackground;
}

void HDFaceD3DPanel::RemoveBackground::set(bool value)
{
	if (this->m_removeBackground != value)
	{
		critical_section::scoped_lock lock(m_criticalSection);
		this->m_removeBackground = value;

		//	PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("KinectCameraSource");

	}
}

byte * HDFaceD3DPanel::RemoveBackgroundPixels(byte * pixels)
{
	auto result = pixels;
	if (nullptr == m_bodyIndexFrameRef) return result;

	if (nullptr == m_depthFrameRef) return result;

	if (nullptr == m_coordMapper) return result;

	auto bodyIndexFrame = m_bodyIndexFrameRef->AcquireFrame();
	if (nullptr == bodyIndexFrame) return result;

	auto depthFrame = m_depthFrameRef->AcquireFrame();
	if (nullptr == depthFrame) {
		delete bodyIndexFrame;
		return result;
	}

	Windows::Storage::Streams::IBuffer^ depthFrameData = depthFrame->LockImageBuffer();

	this->m_coordMapper->MapColorFrameToDepthSpaceUsingIBuffer(depthFrameData, this->m_DepthPoints);

	UINT* pDest = reinterpret_cast<UINT*>(pixels);

	FrameDescription^ bodyIndexFrameDescription = bodyIndexFrame->FrameDescription;

	// Access the body index frame data directly via LockImageBuffer to avoid making a copy
	auto bodyIndexFrameData = bodyIndexFrame->LockImageBuffer();

	// Convert from C++/CX to the ABI IInspectable*:
	ComPtr<IInspectable> bodyIndexInspectable(DX::As_Inspectable(bodyIndexFrameData));

	// Get the IBufferByteAccess interface:
	ComPtr<IBufferByteAccess> bodyIndexBufferBytes;
	DX::ThrowIfFailed(bodyIndexInspectable.As(&bodyIndexBufferBytes));

	// Use it:
	byte* bodyIndexBytes(nullptr);
	DX::ThrowIfFailed(bodyIndexBufferBytes->Buffer(&bodyIndexBytes));

	bodyIndexFrame = nullptr;
	delete bodyIndexFrame;


	int colorMappedToDepthPointCount = this->m_DepthPoints->Length;
	// Treat the color data as 4-byte pixels
	//auto bitmapPixelsPointer = (UINT*)pDest;

	//TODO: Fix background removal performance issues
	//The below code causes performance issues
	//will need to do this using another algorithm.
	//maybe starting with a zero Bitmap
	//and then only loopin through a region of the 
	//bitmap based on widest and tallest pixels of body joints


	// Loop over each row and column of the color image
	// Zero out any pixels that don't correspond to a body index
	for (UINT colorIndex = 0; colorIndex < colorMappedToDepthPointCount; ++colorIndex)
	{
		auto depthPts = this->m_DepthPoints;
		auto depthPt = depthPts->get(colorIndex);

		// The sentinel value is -inf, -inf, meaning that no depth pixel corresponds to this color pixel.
		if ((depthPt.X == -std::numeric_limits<float>::infinity()) || (depthPt.Y == -std::numeric_limits<float>::infinity()))
		{
			pDest[colorIndex] = 0;
			continue;
		}

		// Make sure the depth pixel maps  to a valid point in color space
		int depthX = (int)(depthPt.X + 0.5f);
		int depthY = (int)(depthPt.Y + 0.5f);
		int depthWidth = 512;
		int depthHeight = 424;

		// If the point is not valid, there is no body index there.
		if ((depthX >= 0) && (depthX < depthWidth) && (depthY >= 0) && (depthY < depthHeight))
		{
			int depthIndex = (depthY * depthWidth) + depthX;

			// If we are tracking a body for the current pixel, do not zero out the pixel
			BYTE player = bodyIndexBytes[depthIndex];
			if (player != 0xff)
			{
				//really do nothing because pDest already has the pixeels
				//pDest[colorIndex] = bitmapPixelsPointer[colorIndex];
				continue;

			}
		}
		pDest[colorIndex] = 0;

	}
	return (byte*)pDest;

}

void HDFaceD3DPanel::DrawFaceRectangle()
{
	if (m_faceRegion.Right > 0)
	{
		const D2D1_RECT_F faceRegion = { ScaleX( m_faceRegion.Left), ScaleY(m_faceRegion.Top), ScaleX(m_faceRegion.Right), ScaleY(m_faceRegion.Bottom) };		
		m_d2dContext->BeginDraw();
		m_d2dContext->DrawRectangle(&faceRegion, m_colorBrush.Get(), 1.0f, NULL);
		m_d2dContext->EndDraw();
	}
}

//
//void HDFaceD3DPanel::DrawFacePoints(Platform::Array<ColorSpacePoint>^ facePoints)
//{
//	if (nullptr != facePoints)
//	{
//		
//		//critical_section::scoped_lock lock(m_criticalSection);
//		ColorSpacePoint * points = facePoints->Data;
//		auto size = facePoints->Length;
//
//		ColorSpacePoint facePoint = { 0 };
//
//		for (UINT i = 0; i < size; i++)
//		{
//			facePoint = points[i];
//				
//				if ((facePoint.X == -std::numeric_limits<float>::infinity()) || (facePoint.Y == -std::numeric_limits<float>::infinity())) continue;
//
//					if (m_d2dContext)
//					{
//						D2D1_ELLIPSE ellipse = {};
//						ellipse.point = { facePoint.X, facePoint.Y };
//						ellipse.radiusX = 0.2f;
//						ellipse.radiusY = 0.2f;
//
//						m_d2dContext->BeginDraw();
//						m_d2dContext->DrawEllipse(&ellipse, m_colorBrush.Get(), 1.0f, NULL);
//						m_d2dContext->EndDraw();
//					}
//				
//			
//		}
//	}
//}