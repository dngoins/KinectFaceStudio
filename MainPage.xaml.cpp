//------------------------------------------------------------------------------
// <copyright file="MainPage.xaml.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


#include "pch.h"
#include "MainPage.xaml.h"
#include <collection.h>
#include <limits>
#include "ColorName.h"
#include "DirectXHelper.h"
#include <robuffer.h>
#include <ppltasks.h>

using namespace DwightGoins::Utilities::Kinect::KinectFaceStudio;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage() :
	sensor(nullptr),
	bodySource(nullptr),
	bodyReader(nullptr),
	highDefinitionFaceFrameSource(nullptr),
	highDefinitionFaceFrameReader(nullptr),
	m_faceFrameSource(nullptr),
	m_faceFrameReader(nullptr),
	currentFaceAlignment(nullptr),
	currentFaceModel(nullptr),
	cachedFaceIndices(nullptr),
	hdFaceBuilder(nullptr),
	bodies(nullptr),
	currentTrackedBody(nullptr),
	currentTrackingId(0),
	currentModelCollectionOperation(nullptr),
	currentCollectionStatusString(L""),
	currentCaptureStatusString(L"Ready To Start Capture"),
	statusText(L""),
	m_okToGetDeforms(false),
	m_isFrowning(false),
	m_wroteHeader(false),
	faceData(nullptr),
	pHDFaceData(nullptr),
	m_colorWidth(0),
	m_colorHeight(0)
{	
	InitializeComponent();

	this->sensor = KinectSensor::GetDefault();
	this->coordinateMapper = this->sensor->CoordinateMapper;
	UINT colorWidth, colorHeight;
	auto frameDesc = this->sensor->ColorFrameSource->CreateFrameDescription(ColorImageFormat::Rgba);
	
	auto depthDesc = this->sensor->DepthFrameSource->FrameDescription;
	m_cameraHeight = depthDesc->Height;
	m_cameraWidth = depthDesc->Width;

	faceData = ref new Platform::Collections::Vector<String^>();
	colorWidth = frameDesc->Width;
	colorHeight = frameDesc->Height;
	m_colorWidth = colorWidth;
	m_colorHeight = colorHeight;

	this->colorMappedToDepthPoints = ref new Array<DepthSpacePoint>(colorWidth * colorHeight);


	this->bitmap = ref new Windows::UI::Xaml::Media::Imaging::WriteableBitmap(colorWidth, colorHeight);

	//this->bitmapNoBkg = ref new Windows::UI::Xaml::Media::Imaging::WriteableBitmap(colorWidth, colorHeight);

	
	this->DataContext = this;
	this->StatusText = MakeStatusText();
	
	//this->imgKinectView->Source = this->bitmap;

	//get access to a text file for writing out the data
	Windows::Globalization::Calendar^ cal = ref new Windows::Globalization::Calendar();
	Windows::Foundation::DateTime dateToFormat = cal->GetDateTime();

	auto myFile = "HDFaceData_" + dateToFormat.UniversalTime.ToString() + ".csv";
	InitializeCriticalSectionEx(&cs, 0, 0);

	auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;

	auto fullFile = localFolder->Path + "\\" + myFile;

	auto errorNo = _wfopen_s(&pHDFaceData, fullFile->Data(), L"wt+");

	/*auto fileOption = Windows::Storage::CreationCollisionOption::ReplaceExisting;
	task<Windows::Storage::StorageFile^>(localFolder->CreateFileAsync(myFile, fileOption)).then([this](Windows::Storage::StorageFile^ file)
	{
		m_HDFaceData = file;
	});
*/

}

String^ MainPage::StatusText::get()
{
	return this->statusText; 
}

void MainPage::StatusText::set(String^ value) 
{
	if (this->statusText != value) 
	{
		this->statusText = value;

		PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("StatusText");
		this->PropertyChanged(this, args);
	}
}

ImageSource^ MainPage::KinectImageSource::get()
{
	//return this->bitmapNoBkg;
	return this->bitmap;
}


WriteableBitmap^ MainPage::KinectCameraSource::get()
{
	
		return this->bitmap;
	
}
void MainPage::KinectCameraSource::set(WriteableBitmap^ value)
{
	if (this->bitmap != value)
	{
		this->bitmap = value;

		PropertyChangedEventArgs^ args = ref new PropertyChangedEventArgs("KinectCameraSource");
		this->PropertyChanged(this, args);
	}
}


/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
void MainPage::OnNavigatedTo(NavigationEventArgs^ /* e */)
{
	this->HDFaceRenderingPanel->StartRenderLoop();
}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::Page_Loaded(Platform::Object^ /* sender */, Windows::UI::Xaml::RoutedEventArgs^ /* e */)
{
	this->InitializeFace();
	this->InitializeHDFace();
	this->InitializeKinectColor();
	this->sensor->Open();

}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::InitializeKinectColor()
{
	 msFrameReader = this->sensor->OpenMultiSourceFrameReader(FrameSourceTypes::BodyIndex | FrameSourceTypes::Color | FrameSourceTypes::Depth );

	 msFrameReader->MultiSourceFrameArrived += ref new Windows::Foundation::TypedEventHandler<WindowsPreview::Kinect::MultiSourceFrameReader ^, WindowsPreview::Kinect::MultiSourceFrameArrivedEventArgs ^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::OnMultiSourceFrameArrived);

	 

	// Calculate the WriteableBitmap back buffer size
	//this->bitmapBackBufferSize = (UINT)((this->bitmap->BackBufferStride * (this->bitmap->PixelHeight - 1)) + (this->bitmap->PixelWidth * this->bytesPerPixel));

}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::InitializeFace()
{
	this->m_faceFrameSource = ref new FaceFrameSource(this->sensor, 0, FaceFrameFeatures::PointsInColorSpace | FaceFrameFeatures::BoundingBoxInColorSpace | FaceFrameFeatures::BoundingBoxInInfraredSpace | FaceFrameFeatures::PointsInInfraredSpace);

	this->m_faceFrameReader = this->m_faceFrameSource->OpenReader();
	this->m_faceFrameReader->FrameArrived += ref new Windows::Foundation::TypedEventHandler<Microsoft::Kinect::Face::FaceFrameReader ^, Microsoft::Kinect::Face::FaceFrameArrivedEventArgs ^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::OnFrameArrived);

}

/// <summary>
/// Initialize Kinect object
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::InitializeHDFace()
{	

	this->bodySource = this->sensor->BodyFrameSource;
	
    this->bodies = ref new Platform::Collections::Vector<Body^>(this->bodySource->BodyCount);

	this->bodyReader = this->bodySource->OpenReader();
	this->bodyReader->FrameArrived += ref new TypedEventHandler<WindowsPreview::Kinect::BodyFrameReader^, WindowsPreview::Kinect::BodyFrameArrivedEventArgs^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::BodyReader_FrameArrived);

	this->highDefinitionFaceFrameSource = ref new HighDefinitionFaceFrameSource(this->sensor);
	this->highDefinitionFaceFrameReader = this->highDefinitionFaceFrameSource->OpenReader();
	this->highDefinitionFaceFrameReader->FrameArrived += ref new TypedEventHandler<HighDefinitionFaceFrameReader^, HighDefinitionFaceFrameArrivedEventArgs^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::HDFaceReader_FrameArrived);

	this->currentFaceModel = ref new FaceModel();
	this->currentFaceAlignment = ref new FaceAlignment();
	this->cachedFaceIndices = FaceModel::TriangleIndices;

	this->UpdateFaceMesh();


}

/// <summary>
/// Sends the new deformed mesh to be drawn
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::UpdateFaceMesh()
{	

	auto faceVertices = this->currentFaceModel->CalculateVerticesForAlignment(this->currentFaceAlignment);
	Array<CameraSpacePoint>^ faceVerts = ref new Array<CameraSpacePoint> (faceVertices->Size);

	
	//coordinateMapper->MapDepthPointsToCameraSpace(this->colorMappedToDepthPoints, )
	//convert to facePoints
	auto facePoints = ref new Array< ColorSpacePoint> (faceVertices->Size );
	
	for (int i = 0; i<faceVertices->Size; i++)
	{
		faceVerts->set(i, faceVertices->GetAt(i));
	}

	coordinateMapper->MapCameraPointsToColorSpace(faceVerts, facePoints);

	//get the face region in 2-d space
	auto faceRegion = this->m_faceRegion; //this->currentFaceAlignment->FaceBoundingBox;
	auto rendererSize = this->rendererParent->RenderSize;	
	auto windowSize = this->RenderSize;

	this->HDFaceRenderingPanel->UpdateMesh(faceVertices, this->cachedFaceIndices, facePoints, faceRegion);

	this->GetDeforms();
}

/// <summary>
/// Creates the status text message
/// </summary>
String^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::MakeStatusText()
{
    auto text = L"Capture: " + this->currentCaptureStatusString + " Collection: " + this->currentCollectionStatusString + L", Tracking ID = " + this->currentTrackingId.ToString();
	return text;
}

/// <summary>
/// This event fires when a BodyFrame is ready for consumption
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::BodyReader_FrameArrived(BodyFrameReader^ sender, BodyFrameArrivedEventArgs^ e)
{
    auto frameReference = e->FrameReference;
    {
        auto frame = frameReference->AcquireFrame();

        if (frame == nullptr)
        {
			m_isFrowning = false;
            return;
        }

        // Get the latest bodies
        frame->GetAndRefreshBodyData(this->bodies);
    }

	// Do we still see the person we're tracking?
	if (this->currentTrackedBody != nullptr)
	{
		this->currentTrackedBody = this->FindBodyWithTrackingId(this->currentTrackingId);

		if (this->currentTrackedBody != nullptr)
		{
			// We still see the person we're tracking, make no change.
			m_isFrowning = (currentTrackedBody->HandRightState == HandState::Open || currentTrackedBody->HandLeftState == HandState::Open);

			return;
		}
	}

	Body^ selectedBody = this->FindClosestBody();

	if (selectedBody == nullptr)
	{
		this->currentTrackedBody = nullptr;
		this->currentTrackingId = 0;
		m_isFrowning = false;
		return;
	}

	m_isFrowning = (selectedBody->HandRightState == HandState::Open || selectedBody->HandLeftState == HandState::Open);

	this->currentTrackedBody = selectedBody;
	auto trackingID = selectedBody->TrackingId;;
	this->currentTrackingId = trackingID;

	this->highDefinitionFaceFrameSource->TrackingId = this->currentTrackingId;
	this->m_faceFrameSource->TrackingId = this->currentTrackingId;

}

/// <summary>
/// Returns the length of a vector from origin
/// </summary>
double DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::VectorLength(CameraSpacePoint point)
{
	auto result = pow(point.X, 2) + pow(point.Y, 2) + pow(point.Z, 2);

	result = sqrt(result);

	return result;
}

/// <summary>
/// Finds the closest body from the sensor if any
/// </summary>
Body^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FindClosestBody()
{
	Body^ result = nullptr;

	double closestBodyDistance = 10000000.0;

	for (auto body : this->bodies)
	{
		if (body->IsTracked)
		{
			auto joints = body->Joints;

			auto currentLocation = joints->Lookup(JointType::SpineBase).Position;

			auto currentDistance = this->VectorLength(currentLocation);

			if (result == nullptr || currentDistance < closestBodyDistance)
			{
				result = body;
				closestBodyDistance = currentDistance;
			}
		}
	}

	return result;
}

/// <summary>
/// Find if there is a body tracked with the given trackingId
/// </summary>
Body^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FindBodyWithTrackingId(UINT64 trackingId)
{
	Body^ result = nullptr;

	for (auto body : this->bodies)
	{
		if (body->IsTracked)
		{
			if (body->TrackingId == trackingId)
			{
				result = body;
				break;
			}
		}
	}

	return result;
}

/// <summary>
/// This event is fired when a new HDFace frame is ready for consumption
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::HDFaceReader_FrameArrived(HighDefinitionFaceFrameReader^ sender, HighDefinitionFaceFrameArrivedEventArgs^ e)
{
    auto frameReference = e->FrameReference;

    {
        auto frame = frameReference->AcquireFrame();

        // We might miss the chance to acquire the frame; it will be null if it's missed.
        // Also ignore this frame if face tracking failed.
        if (frame == nullptr || !frame->IsFaceTracked)
        {
            return;
        }

        frame->GetAndRefreshFaceAlignmentResult(this->currentFaceAlignment);
    }

    this->UpdateFaceMesh();
    this->StatusText = this->MakeStatusText();
}

/// <summary>
/// This event is fired when the FaceModelBuilder collection status has changed
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FaceModelBuilder_CollectionStatusChanged( FaceModelBuilder^ sender, CollectionStatusChangedEventArgs^ e)
{
    FaceModelBuilderCollectionStatus newStatus = sender->CollectionStatus;      //Query the latest status

    this->currentCollectionStatusString = BuildCollectionStatusText(newStatus);
}

/// <summary>
/// This event is fired when the FaceModelBuilder capture status has changed
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FaceModelBuilder_CaptureStatusChanged( FaceModelBuilder^ sender, CaptureStatusChangedEventArgs^ e)
{
    FaceModelBuilderCaptureStatus newStatus = sender->CaptureStatus;

    this->currentCaptureStatusString = GetCaptureStatusText(newStatus);
}

/// <summary>
/// Start a face capture on clicking the button
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::StartCaptureButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->StartCapture();
}


/// <summary>
/// Start recording the data from the HDFace Frames
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::StartKStudioRecording(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
//	this->StartCapture();
}

/// <summary>
/// Cancel the current face capture operation
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::StopFaceCapture()
{
	if (this->currentModelCollectionOperation != nullptr)
	{
		this->currentModelCollectionOperation->Cancel();
		this->currentModelCollectionOperation = nullptr;
	}

	if (this->hdFaceBuilder != nullptr)
	{
        this->hdFaceBuilder->CollectionStatusChanged -= this->tokenCollectionStatusChanged;
        this->hdFaceBuilder->CaptureStatusChanged -= this->tokenCaptureStatusChanged;
		this->hdFaceBuilder = nullptr;
	}
}

/// <summary>
/// Start a face capture operation
/// </summary>
void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::StartCapture()
{
	if (this->currentModelCollectionOperation != nullptr)
	{
		this->StopFaceCapture();
	}

	this->hdFaceBuilder = this->highDefinitionFaceFrameSource->OpenModelBuilder(FaceModelBuilderAttributes::HairColor | FaceModelBuilderAttributes::SkinColor | FaceModelBuilderAttributes::None);
    this->tokenCollectionStatusChanged = this->hdFaceBuilder->CollectionStatusChanged += ref new TypedEventHandler<FaceModelBuilder^, CollectionStatusChangedEventArgs^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FaceModelBuilder_CollectionStatusChanged);
    this->tokenCaptureStatusChanged = this->hdFaceBuilder->CaptureStatusChanged += ref new TypedEventHandler<FaceModelBuilder^, CaptureStatusChangedEventArgs^>(this, &DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::FaceModelBuilder_CaptureStatusChanged);

	this->currentModelCollectionOperation = this->hdFaceBuilder->CollectFaceDataAsync();
    this->currentModelCollectionOperation->Completed = ref new AsyncOperationCompletedHandler<FaceModelData^>(
		[this](IAsyncOperation<FaceModelData^>^ asyncOp, AsyncStatus status)
	{
		if (status == AsyncStatus::Completed)
		{
			auto modelData = asyncOp->GetResults();
			
			currentFaceModel = modelData->ProduceFaceModel();
			UpdateFaceMesh();
			StopFaceCapture();
			currentCaptureStatusString = "Capture Complete";
			currentCollectionStatusString = "";
			skinColor->Background = ref new SolidColorBrush(currentFaceModel->SkinColor);
			hairColor->Background = ref new SolidColorBrush(currentFaceModel->HairColor);
			
			this->HDFaceRenderingPanel->SetFaceColor(currentFaceModel->SkinColor);

			auto skinColorName = getColorNameFromRgb(currentFaceModel->SkinColor.R, currentFaceModel->SkinColor.G, currentFaceModel->SkinColor.B);

			auto hairColorName = getColorNameFromRgb(currentFaceModel->HairColor.R, currentFaceModel->HairColor.G, currentFaceModel->HairColor.B);

			lblSkinColor->Text = "Skin Color: " + skinColorName;
			lblHairColor->Text = "Hair Color: " + hairColorName;

			//float* pAnimationUnits = new float[this->FaceShapeAnimations_Count];

			
			m_okToGetDeforms = true;

			
		}
		else if (status == AsyncStatus::Error)
		{
			m_okToGetDeforms = false;
			StopFaceCapture();
            currentCaptureStatusString = "Error collecting face data!";
            currentCollectionStatusString = "";
        }
		else
		{
			m_okToGetDeforms = false;
			StopFaceCapture();
            currentCaptureStatusString = "Collecting face data incomplete!";
            currentCollectionStatusString = "";
        }
	});
}

String^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::BuildCollectionStatusText( FaceModelBuilderCollectionStatus status )
{
    String^ res = L"";
	
    if (FaceModelBuilderCollectionStatus::Complete == status)
    {
        return L" Done ";
    }

    if ((int)(status & FaceModelBuilderCollectionStatus::MoreFramesNeeded) != 0)
    {
        res += L" More ";
    }

    if ((int)(status & FaceModelBuilderCollectionStatus::FrontViewFramesNeeded) != 0)
    {
        res += L" Front";
    }

    if ((int)(int)(status & FaceModelBuilderCollectionStatus::LeftViewsNeeded) != 0)
    {
        res += L" Left ";
    }

    if ((int)(status & FaceModelBuilderCollectionStatus::RightViewsNeeded) != 0)
    {
        res += L" Right";
    }

    if ((int)(status & FaceModelBuilderCollectionStatus::TiltedUpViewsNeeded) != 0)
    {
        res += L"  Up  ";
    }

    return res;
}

String^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::GetCaptureStatusText(FaceModelBuilderCaptureStatus status)
{
    switch (status)
    {
        case FaceModelBuilderCaptureStatus::FaceTooFar:
            return L"  Too Far ";
        case FaceModelBuilderCaptureStatus::FaceTooNear:
            return L" Too Near ";
        case FaceModelBuilderCaptureStatus::GoodFrameCapture:
            return L"Good Frame";
        case FaceModelBuilderCaptureStatus::LostFaceTrack:
            return L"Lost Track";
        case FaceModelBuilderCaptureStatus::MovingTooFast:
            return L" Too Fast ";
        case FaceModelBuilderCaptureStatus::OtherViewsNeeded:
            return L"More Views";
        case FaceModelBuilderCaptureStatus::SystemError:
            return L"  Error   ";
    }

    return L"";
}

PropertySet^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::initColorList() {
	 PropertySet^ colorList = ref new PropertySet();
	 
	 int key = 0;
	colorList->Insert((key++).ToString(), ref new ColorName("AliceBlue", 0xF0, 0xF8, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("AntiqueWhite", 0xFA, 0xEB, 0xD7));
	colorList->Insert((key++).ToString(), ref new ColorName("Aqua", 0x00, 0xFF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("Aquamarine", 0x7F, 0xFF, 0xD4));
	colorList->Insert((key++).ToString(), ref new ColorName("Azure", 0xF0, 0xFF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("Beige", 0xF5, 0xF5, 0xDC));
	colorList->Insert((key++).ToString(), ref new ColorName("Bisque", 0xFF, 0xE4, 0xC4));
	colorList->Insert((key++).ToString(), ref new ColorName("Black", 0x00, 0x00, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("BlanchedAlmond", 0xFF, 0xEB, 0xCD));
	colorList->Insert((key++).ToString(), ref new ColorName("Blue", 0x00, 0x00, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("BlueViolet", 0x8A, 0x2B, 0xE2));
	colorList->Insert((key++).ToString(), ref new ColorName("Brown", 0xA5, 0x2A, 0x2A));
	colorList->Insert((key++).ToString(), ref new ColorName("BurlyWood", 0xDE, 0xB8, 0x87));
	colorList->Insert((key++).ToString(), ref new ColorName("CadetBlue", 0x5F, 0x9E, 0xA0));
	colorList->Insert((key++).ToString(), ref new ColorName("Chartreuse", 0x7F, 0xFF, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("Chocolate", 0xD2, 0x69, 0x1E));
	colorList->Insert((key++).ToString(), ref new ColorName("Coral", 0xFF, 0x7F, 0x50));
	colorList->Insert((key++).ToString(), ref new ColorName("CornflowerBlue", 0x64, 0x95, 0xED));
	colorList->Insert((key++).ToString(), ref new ColorName("Cornsilk", 0xFF, 0xF8, 0xDC));
	colorList->Insert((key++).ToString(), ref new ColorName("Crimson", 0xDC, 0x14, 0x3C));
	colorList->Insert((key++).ToString(), ref new ColorName("Cyan", 0x00, 0xFF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkBlue", 0x00, 0x00, 0x8B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkCyan", 0x00, 0x8B, 0x8B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkGoldenRod", 0xB8, 0x86, 0x0B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkGray", 0xA9, 0xA9, 0xA9));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkGreen", 0x00, 0x64, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkKhaki", 0xBD, 0xB7, 0x6B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkMagenta", 0x8B, 0x00, 0x8B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkOliveGreen", 0x55, 0x6B, 0x2F));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkOrange", 0xFF, 0x8C, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkOrchid", 0x99, 0x32, 0xCC));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkRed", 0x8B, 0x00, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkSalmon", 0xE9, 0x96, 0x7A));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkSeaGreen", 0x8F, 0xBC, 0x8F));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkSlateBlue", 0x48, 0x3D, 0x8B));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkSlateGray", 0x2F, 0x4F, 0x4F));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkTurquoise", 0x00, 0xCE, 0xD1));
	colorList->Insert((key++).ToString(), ref new ColorName("DarkViolet", 0x94, 0x00, 0xD3));
	colorList->Insert((key++).ToString(), ref new ColorName("DeepPink", 0xFF, 0x14, 0x93));
	colorList->Insert((key++).ToString(), ref new ColorName("DeepSkyBlue", 0x00, 0xBF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("DimGray", 0x69, 0x69, 0x69));
	colorList->Insert((key++).ToString(), ref new ColorName("DodgerBlue", 0x1E, 0x90, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("FireBrick", 0xB2, 0x22, 0x22));
	colorList->Insert((key++).ToString(), ref new ColorName("FloralWhite", 0xFF, 0xFA, 0xF0));
	colorList->Insert((key++).ToString(), ref new ColorName("ForestGreen", 0x22, 0x8B, 0x22));
	colorList->Insert((key++).ToString(), ref new ColorName("Fuchsia", 0xFF, 0x00, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("Gainsboro", 0xDC, 0xDC, 0xDC));
	colorList->Insert((key++).ToString(), ref new ColorName("GhostWhite", 0xF8, 0xF8, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("Gold", 0xFF, 0xD7, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("GoldenRod", 0xDA, 0xA5, 0x20));
	colorList->Insert((key++).ToString(), ref new ColorName("Gray", 0x80, 0x80, 0x80));
	colorList->Insert((key++).ToString(), ref new ColorName("Green", 0x00, 0x80, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("GreenYellow", 0xAD, 0xFF, 0x2F));
	colorList->Insert((key++).ToString(), ref new ColorName("HoneyDew", 0xF0, 0xFF, 0xF0));
	colorList->Insert((key++).ToString(), ref new ColorName("HotPink", 0xFF, 0x69, 0xB4));
	colorList->Insert((key++).ToString(), ref new ColorName("IndianRed", 0xCD, 0x5C, 0x5C));
	colorList->Insert((key++).ToString(), ref new ColorName("Indigo", 0x4B, 0x00, 0x82));
	colorList->Insert((key++).ToString(), ref new ColorName("Ivory", 0xFF, 0xFF, 0xF0));
	colorList->Insert((key++).ToString(), ref new ColorName("Khaki", 0xF0, 0xE6, 0x8C));
	colorList->Insert((key++).ToString(), ref new ColorName("Lavender", 0xE6, 0xE6, 0xFA));
	colorList->Insert((key++).ToString(), ref new ColorName("LavenderBlush", 0xFF, 0xF0, 0xF5));
	colorList->Insert((key++).ToString(), ref new ColorName("LawnGreen", 0x7C, 0xFC, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("LemonChiffon", 0xFF, 0xFA, 0xCD));
	colorList->Insert((key++).ToString(), ref new ColorName("LightBlue", 0xAD, 0xD8, 0xE6));
	colorList->Insert((key++).ToString(), ref new ColorName("LightCoral", 0xF0, 0x80, 0x80));
	colorList->Insert((key++).ToString(), ref new ColorName("LightCyan", 0xE0, 0xFF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("LightGoldenRodYellow", 0xFA, 0xFA, 0xD2));
	colorList->Insert((key++).ToString(), ref new ColorName("LightGray", 0xD3, 0xD3, 0xD3));
	colorList->Insert((key++).ToString(), ref new ColorName("LightGreen", 0x90, 0xEE, 0x90));
	colorList->Insert((key++).ToString(), ref new ColorName("LightPink", 0xFF, 0xB6, 0xC1));
	colorList->Insert((key++).ToString(), ref new ColorName("LightSalmon", 0xFF, 0xA0, 0x7A));
	colorList->Insert((key++).ToString(), ref new ColorName("LightSeaGreen", 0x20, 0xB2, 0xAA));
	colorList->Insert((key++).ToString(), ref new ColorName("LightSkyBlue", 0x87, 0xCE, 0xFA));
	colorList->Insert((key++).ToString(), ref new ColorName("LightSlateGray", 0x77, 0x88, 0x99));
	colorList->Insert((key++).ToString(), ref new ColorName("LightSteelBlue", 0xB0, 0xC4, 0xDE));
	colorList->Insert((key++).ToString(), ref new ColorName("LightYellow", 0xFF, 0xFF, 0xE0));
	colorList->Insert((key++).ToString(), ref new ColorName("Lime", 0x00, 0xFF, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("LimeGreen", 0x32, 0xCD, 0x32));
	colorList->Insert((key++).ToString(), ref new ColorName("Linen", 0xFA, 0xF0, 0xE6));
	colorList->Insert((key++).ToString(), ref new ColorName("Magenta", 0xFF, 0x00, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("Maroon", 0x80, 0x00, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumAquaMarine", 0x66, 0xCD, 0xAA));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumBlue", 0x00, 0x00, 0xCD));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumOrchid", 0xBA, 0x55, 0xD3));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumPurple", 0x93, 0x70, 0xDB));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumSeaGreen", 0x3C, 0xB3, 0x71));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumSlateBlue", 0x7B, 0x68, 0xEE));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumSpringGreen", 0x00, 0xFA, 0x9A));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumTurquoise", 0x48, 0xD1, 0xCC));
	colorList->Insert((key++).ToString(), ref new ColorName("MediumVioletRed", 0xC7, 0x15, 0x85));
	colorList->Insert((key++).ToString(), ref new ColorName("MidnightBlue", 0x19, 0x19, 0x70));
	colorList->Insert((key++).ToString(), ref new ColorName("MintCream", 0xF5, 0xFF, 0xFA));
	colorList->Insert((key++).ToString(), ref new ColorName("MistyRose", 0xFF, 0xE4, 0xE1));
	colorList->Insert((key++).ToString(), ref new ColorName("Moccasin", 0xFF, 0xE4, 0xB5));
	colorList->Insert((key++).ToString(), ref new ColorName("NavajoWhite", 0xFF, 0xDE, 0xAD));
	colorList->Insert((key++).ToString(), ref new ColorName("Navy", 0x00, 0x00, 0x80));
	colorList->Insert((key++).ToString(), ref new ColorName("OldLace", 0xFD, 0xF5, 0xE6));
	colorList->Insert((key++).ToString(), ref new ColorName("Olive", 0x80, 0x80, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("OliveDrab", 0x6B, 0x8E, 0x23));
	colorList->Insert((key++).ToString(), ref new ColorName("Orange", 0xFF, 0xA5, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("OrangeRed", 0xFF, 0x45, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("Orchid", 0xDA, 0x70, 0xD6));
	colorList->Insert((key++).ToString(), ref new ColorName("PaleGoldenRod", 0xEE, 0xE8, 0xAA));
	colorList->Insert((key++).ToString(), ref new ColorName("PaleGreen", 0x98, 0xFB, 0x98));
	colorList->Insert((key++).ToString(), ref new ColorName("PaleTurquoise", 0xAF, 0xEE, 0xEE));
	colorList->Insert((key++).ToString(), ref new ColorName("PaleVioletRed", 0xDB, 0x70, 0x93));
	colorList->Insert((key++).ToString(), ref new ColorName("PapayaWhip", 0xFF, 0xEF, 0xD5));
	colorList->Insert((key++).ToString(), ref new ColorName("PeachPuff", 0xFF, 0xDA, 0xB9));
	colorList->Insert((key++).ToString(), ref new ColorName("Peru", 0xCD, 0x85, 0x3F));
	colorList->Insert((key++).ToString(), ref new ColorName("Pink", 0xFF, 0xC0, 0xCB));
	colorList->Insert((key++).ToString(), ref new ColorName("Plum", 0xDD, 0xA0, 0xDD));
	colorList->Insert((key++).ToString(), ref new ColorName("PowderBlue", 0xB0, 0xE0, 0xE6));
	colorList->Insert((key++).ToString(), ref new ColorName("Purple", 0x80, 0x00, 0x80));
	colorList->Insert((key++).ToString(), ref new ColorName("Red", 0xFF, 0x00, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("RosyBrown", 0xBC, 0x8F, 0x8F));
	colorList->Insert((key++).ToString(), ref new ColorName("RoyalBlue", 0x41, 0x69, 0xE1));
	colorList->Insert((key++).ToString(), ref new ColorName("SaddleBrown", 0x8B, 0x45, 0x13));
	colorList->Insert((key++).ToString(), ref new ColorName("Salmon", 0xFA, 0x80, 0x72));
	colorList->Insert((key++).ToString(), ref new ColorName("SandyBrown", 0xF4, 0xA4, 0x60));
	colorList->Insert((key++).ToString(), ref new ColorName("SeaGreen", 0x2E, 0x8B, 0x57));
	colorList->Insert((key++).ToString(), ref new ColorName("SeaShell", 0xFF, 0xF5, 0xEE));
	colorList->Insert((key++).ToString(), ref new ColorName("Sienna", 0xA0, 0x52, 0x2D));
	colorList->Insert((key++).ToString(), ref new ColorName("Silver", 0xC0, 0xC0, 0xC0));
	colorList->Insert((key++).ToString(), ref new ColorName("SkyBlue", 0x87, 0xCE, 0xEB));
	colorList->Insert((key++).ToString(), ref new ColorName("SlateBlue", 0x6A, 0x5A, 0xCD));
	colorList->Insert((key++).ToString(), ref new ColorName("SlateGray", 0x70, 0x80, 0x90));
	colorList->Insert((key++).ToString(), ref new ColorName("Snow", 0xFF, 0xFA, 0xFA));
	colorList->Insert((key++).ToString(), ref new ColorName("SpringGreen", 0x00, 0xFF, 0x7F));
	colorList->Insert((key++).ToString(), ref new ColorName("SteelBlue", 0x46, 0x82, 0xB4));
	colorList->Insert((key++).ToString(), ref new ColorName("Tan", 0xD2, 0xB4, 0x8C));
	colorList->Insert((key++).ToString(), ref new ColorName("Teal", 0x00, 0x80, 0x80));
	colorList->Insert((key++).ToString(), ref new ColorName("Thistle", 0xD8, 0xBF, 0xD8));
	colorList->Insert((key++).ToString(), ref new ColorName("Tomato", 0xFF, 0x63, 0x47));
	colorList->Insert((key++).ToString(), ref new ColorName("Turquoise", 0x40, 0xE0, 0xD0));
	colorList->Insert((key++).ToString(), ref new ColorName("Violet", 0xEE, 0x82, 0xEE));
	colorList->Insert((key++).ToString(), ref new ColorName("Wheat", 0xF5, 0xDE, 0xB3));
	colorList->Insert((key++).ToString(), ref new ColorName("White", 0xFF, 0xFF, 0xFF));
	colorList->Insert((key++).ToString(), ref new ColorName("WhiteSmoke", 0xF5, 0xF5, 0xF5));
	colorList->Insert((key++).ToString(), ref new ColorName("Yellow", 0xFF, 0xFF, 0x00));
	colorList->Insert((key++).ToString(), ref new ColorName("YellowGreen", 0x9A, 0xCD, 0x32));
	return colorList;
}

String^ DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::getColorNameFromRgb(int r, int g, int b) 
{
	auto colorList = this->initColorList();
	ColorName^ closestMatch;
	int minMSE = MAXINT;
	int mse;
	
	for (UINT i = 0; i < colorList->Size; i++)
	{
		ColorName^ c = (ColorName^)colorList->Lookup(i.ToString());
		mse = c->computeMSE(r, g, b);
		if (mse < minMSE) {
			minMSE = mse;
			closestMatch = c;
		}
	}

	if (closestMatch != nullptr) {
		return closestMatch->getName();
	}
	else {
		return "No matched color name.";
	}
}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::GetDeforms()
{
	if (m_okToGetDeforms)
	{
		auto deformsMap = this->currentFaceModel->FaceShapeDeformations;

		String^ strDeforms = ref new String();
		for (UINT j = 0; j < deformsMap->Size; j++)
		{
			auto deformType = (Microsoft::Kinect::Face::FaceShapeDeformations)j;

			auto deform = deformsMap->Lookup(deformType);
			strDeforms += "FaceDeform: " + deformType.ToString() + " = " + deform.ToString() + " \n";
			//m_traceChannel->LogMessage(L"Face Deform: %d = Value %f\n", j, deform.Value );
//			TRACE(L"Face Deform: %d = Value %f\n", j, deform);
		}

		lblFaceDeforms->Text = strDeforms;

		String^ strAnimUnits = ref new String();
		String^ strAnimUnitsCSV = ref new String();
		String^ strAnimUnitsHdr = ref new String();

		auto animUnitsMap = this->currentFaceAlignment->AnimationUnits;

		for (UINT k = 0; k < animUnitsMap->Size; k++)
		{

			auto animUnitType = (Microsoft::Kinect::Face::FaceShapeAnimations)k;
			auto animUnit = animUnitsMap->Lookup(animUnitType);
			strAnimUnits += "AnimUnit: " + animUnitType.ToString() + " = " + animUnit.ToString() + " \n";
			if (animUnitsMap->Size - 1 == k)
			{
				strAnimUnitsHdr += animUnitType.ToString() + ",isFrowning\n";
				strAnimUnitsCSV += animUnit.ToString() + "," + m_isFrowning.ToString() + "\n";
				//if (!m_wroteHeader) m_wroteHeader = true;
			}
			else
			{
				strAnimUnitsHdr += animUnitType.ToString() + ",";
				strAnimUnitsCSV += animUnit.ToString() + ",";
			}
			//TRACE(L"Anim Unit: %d = Value %f\n", k, animUnit);
		}
		
		
		String^ strTraceAnimUnits = ref new String();
		if (m_wroteHeader)
			strTraceAnimUnits =  strAnimUnitsCSV;
		else{
			strTraceAnimUnits = strAnimUnitsHdr + strAnimUnitsCSV;
			m_wroteHeader = true;
		}
		TRACE(strTraceAnimUnits->Data());
		RecordData(strTraceAnimUnits);
		/*faceData->Append(strTraceAnimUnits);
		if (faceData->Size > 60)
		{
			RecordData(faceData);
			for (UINT i = 1; i < 61; i++)
				faceData->RemoveAt(i);
		}
*/
	
		lblAnimUnits->Text = strAnimUnits;
	}
}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::RecordData(Platform::Collections::Vector<String^> ^ data)
{
	try{

		for_each(begin(data), end(data), ([this](String^ str) {
			
			//EnterCriticalSection(&cs);
			//auto _cs = cs;

			//task<void>(Windows::Storage::FileIO::AppendTextAsync(m_HDFaceData, str)).wait();
		}));
	}
	catch (Exception^)
	{
	//	LeaveCriticalSection(&cs);
	}

	//LeaveCriticalSection(&cs);
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::RecordData(String^ data)
{
	try{
			//task<void>(Windows::Storage::FileIO::AppendTextAsync(m_HDFaceData, data)).wait();
		// Write a string into the file.
		auto str = data->Data();
		//wcscpy_s(str, sizeof(str) / sizeof(wchar_t), L"<xmlTag>\n");
		auto strSize = wcslen(str);
		if (fwrite(str, sizeof(wchar_t), strSize, pHDFaceData) != strSize)
		{
			wprintf(L"fwrite failed!\n");
		}

	}
	catch (Exception^)
	{
		//	LeaveCriticalSection(&cs);
	}

	//LeaveCriticalSection(&cs);
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::Page_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//RecordData(faceData);


	// Close the file.
	if (fclose(pHDFaceData))
	{
		wprintf(L"fclose failed!\n");
	}

	faceData->Clear();
	faceData = nullptr;
	m_HDFaceData = nullptr;
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::OnMultiSourceFrameArrived(WindowsPreview::Kinect::MultiSourceFrameReader ^sender, WindowsPreview::Kinect::MultiSourceFrameArrivedEventArgs ^args)
{
	int depthWidth = 0;
	int depthHeight = 0;

	DepthFrame^ depthFrame = nullptr;
	ColorFrame^ colorFrame = nullptr;
	BodyIndexFrame^ bodyIndexFrame = nullptr;
	bool isBitmapLocked = false;

	MultiSourceFrame^ multiSourceFrame = args->FrameReference->AcquireFrame();

	// If the Frame has expired by the time we process this event, return.
	if (multiSourceFrame == nullptr)
	{
		delete multiSourceFrame;
		delete depthFrame;
		delete colorFrame;
		delete bodyIndexFrame;
		return;
	}

	// We use a try/finally to ensure that we clean up before we exit the function.  
	// This includes calling Dispose on any Frame objects that we may have and unlocking the bitmap back buffer.
	try
	{
		depthFrame = multiSourceFrame->DepthFrameReference->AcquireFrame();
		colorFrame = multiSourceFrame->ColorFrameReference->AcquireFrame();
		bodyIndexFrame = multiSourceFrame->BodyIndexFrameReference->AcquireFrame();

		// If any frame has expired by the time we process this event, return.
		// The "finally" statement will Dispose any that are not null.
		if ((depthFrame == nullptr) || (colorFrame == nullptr) || (bodyIndexFrame == nullptr))
		{
			delete multiSourceFrame;
			delete depthFrame;
			delete colorFrame;
			delete bodyIndexFrame;
			return;
		}	

		HDFaceRenderingPanel->SetColorFrameReference(multiSourceFrame->ColorFrameReference);
		HDFaceRenderingPanel->SetDepthFrameReference(multiSourceFrame->DepthFrameReference);
		HDFaceRenderingPanel->SetBodyIndexFrameReference(multiSourceFrame->BodyIndexFrameReference);
		HDFaceRenderingPanel->SetCoordinateMapper(this->coordinateMapper);
		HDFaceRenderingPanel->RemoveBackground = true;

		// Process Depth
		FrameDescription^ depthFrameDescription = depthFrame->FrameDescription;

		depthWidth = depthFrameDescription->Width;
		depthHeight = depthFrameDescription->Height;

		// Access the depth frame data directly via LockImageBuffer to avoid making a copy
		Windows::Storage::Streams::IBuffer^ depthFrameData = depthFrame->LockImageBuffer();

		this->coordinateMapper->MapColorFrameToDepthSpaceUsingIBuffer(depthFrameData, this->colorMappedToDepthPoints);

		// We're done with the DepthFrame 
		//depthFrame->Dispose();
		depthFrame = nullptr;
		delete depthFrame;

		// We must force a release of the IBuffer in order to ensure that we have dropped all references to it.		
		
		depthFrameData = nullptr;
		delete depthFrameData;
		
		//colorFrame->CopyRawFrameDataToBuffer(this->bitmap->PixelBuffer);
		auto colorFrameDescription = colorFrame->FrameDescription;
		auto colorProcessed = false;
		auto mappedPointsLength = this->colorMappedToDepthPoints->Length;
		UINT clrWidth, clrHeight;
		clrWidth = colorFrameDescription->Width;
		clrHeight = colorFrameDescription->Height;

		UINT length = colorFrame->FrameDescription->LengthInPixels * colorFrame->FrameDescription->BytesPerPixel;
		colorFrame->CopyConvertedFrameDataToBuffer(this->bitmap->PixelBuffer, ColorImageFormat::Bgra);		
		colorFrame = nullptr;
		delete colorFrame;

		/*IBuffer^ buffer = this->bitmap->PixelBuffer;
		UINT16* pSrc = reinterpret_cast<UINT16*>(DX::GetPointerToPixelData(buffer));
		*/// Get the buffer from the WriteableBitmap:
		IBuffer^ pixelBuffer = bitmap->PixelBuffer;

		// Convert from C++/CX to the ABI IInspectable*:
		ComPtr<IInspectable> bufferInspectable(DX::AsInspectable(pixelBuffer));
		byte* pixels(nullptr);

		try
		{
			// Get the IBufferByteAccess interface:
			ComPtr<IBufferByteAccess> pxlbufferBytes;
			DX::ThrowIfFailed(bufferInspectable.As(&pxlbufferBytes));

			// Use it:			
			DX::ThrowIfFailed(pxlbufferBytes->Buffer(&pixels));
			colorProcessed = true;
			pxlbufferBytes = nullptr;
		}
		catch (const std::exception&)
		{
			delete multiSourceFrame;
			delete depthFrame;
			delete colorFrame;
			delete bodyIndexFrame;
			return;
		}

		UINT* pDest = reinterpret_cast<UINT*>(pixels);	
		FrameDescription^ bodyIndexFrameDescription = bodyIndexFrame->FrameDescription;

		// Access the body index frame data directly via LockImageBuffer to avoid making a copy
		auto bodyIndexFrameData = bodyIndexFrame->LockImageBuffer();

		// Convert from C++/CX to the ABI IInspectable*:
		ComPtr<IInspectable> bodyIndexInspectable(DX::AsInspectable(bodyIndexFrameData));

		// Get the IBufferByteAccess interface:
		ComPtr<IBufferByteAccess> bodyIndexBufferBytes;
		DX::ThrowIfFailed(bodyIndexInspectable.As(&bodyIndexBufferBytes));

		// Use it:
		byte* bodyIndexBytes(nullptr);
		DX::ThrowIfFailed(bodyIndexBufferBytes->Buffer(&bodyIndexBytes));
		
		bodyIndexFrame = nullptr;
		delete bodyIndexFrame;
	

		int colorMappedToDepthPointCount = mappedPointsLength;		
		// Treat the color data as 4-byte pixels
		//auto bitmapPixelsPointer = (UINT*)pDest;

		//TODO: Fix background removal performance issues
		//The below code causes performance issues
		//will need to do this using another algorithm.
		//maybe starting with a zero Bitmap
		//and then only loopin through a region of the 
		//bitmap based on widest and tallest pixels of body joints
#if INCLUDE_COLOR

		// Loop over each row and column of the color image
		// Zero out any pixels that don't correspond to a body index
		for (UINT colorIndex = 0; colorIndex < colorMappedToDepthPointCount; ++colorIndex)
		{
			auto depthPts = this->colorMappedToDepthPoints;
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

			// If the point is not valid, there is no body index there.
			if ((depthX >= 0) && (depthX < depthWidth) && (depthY >= 0) && (depthY < depthHeight))
			{
				int depthIndex = (depthY * depthWidth) + depthX;

				// If we are tracking a body for the current pixel, do not zero out the pixel
				BYTE player = bodyIndexBytes[depthIndex];
				if ( player != 0xff)
				{	
					//really do nothing because pDest already has the pixeels
					//pDest[colorIndex] = bitmapPixelsPointer[colorIndex];
						continue;
										
				}
			}
			pDest[colorIndex] = 0;
			
		}


#endif

		if (colorProcessed) {
			this->bitmap->Invalidate();
			//this->HDFaceRenderingPanel->KinectCameraSource = this->bitmap;

		}
		bufferInspectable = nullptr;
		pDest = nullptr;
		bodyIndexFrameData = nullptr;
		bodyIndexBytes = nullptr;
		//bitmapPixelsPointer = nullptr;
		bodyIndexInspectable = nullptr;
		bodyIndexBufferBytes = nullptr;
		bodyIndexBytes = nullptr;
		
		pixels = nullptr;		
		delete pixels;
		//bitmapPixelsPointer = nullptr;
		//delete bitmapPixelsPointer;

		bodyIndexBufferBytes = nullptr;
		bodyIndexBytes = nullptr;
		delete bodyIndexBytes;

		pDest = nullptr;
		delete pDest;


	}
	catch (exception e)
	{
	
		depthFrame = nullptr;
		colorFrame = nullptr;
		bodyIndexFrame = nullptr;
		multiSourceFrame = nullptr;
		delete depthFrame;
		delete colorFrame;
		delete bodyIndexFrame;
		delete multiSourceFrame;

	}

//	((IDisposable^)bodyIndexFrame)->Dispose();
	depthFrame = nullptr;
	colorFrame = nullptr;
	bodyIndexFrame = nullptr;
	multiSourceFrame = nullptr;
	delete depthFrame;
	delete colorFrame;
	delete bodyIndexFrame;
	delete multiSourceFrame;

}

void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::xScale_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	if(nullptr != this->HDFaceRenderingPanel)
	this->HDFaceRenderingPanel->SetScale(this->xScale->Value, this->yScale->Value);

}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::xTrans_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	if (nullptr != this->HDFaceRenderingPanel)
		this->HDFaceRenderingPanel->SetTranslation(this->xTrans->Value, this->yTrans->Value);
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::showMesh_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (nullptr != showMesh)
	{
		auto _show = showMesh->IsChecked;
		this->HDFaceRenderingPanel->ShowMesh = _show->Value;
	}
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::showVideo_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (nullptr != showVideo)
	{
		auto _show = showVideo->IsChecked;
		this->HDFaceRenderingPanel->ShowVideo = _show->Value;
	}
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::showVideo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (nullptr != showVideo)
	{
		auto _show = showVideo->IsChecked;
		this->HDFaceRenderingPanel->ShowVideo = _show->Value;
	}
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::showMesh_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (nullptr != showMesh)
	{
		auto _show = showMesh->IsChecked;
		this->HDFaceRenderingPanel->ShowMesh = _show->Value;
	}
}


void DwightGoins::Utilities::Kinect::KinectFaceStudio::MainPage::OnFrameArrived(Microsoft::Kinect::Face::FaceFrameReader ^sender, Microsoft::Kinect::Face::FaceFrameArrivedEventArgs ^args)
{
	auto faceFrame = this->m_faceFrameReader->AcquireLatestFrame();
	if (nullptr != faceFrame)
	{
		if (nullptr != faceFrame->FaceFrameResult)
		{
			 m_faceRegion = faceFrame->FaceFrameResult->FaceBoundingBoxInColorSpace;
			
		}
	}
}
