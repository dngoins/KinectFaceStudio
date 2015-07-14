//------------------------------------------------------------------------------
// <copyright file="MainPage.xaml.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


#pragma once

#include "MainPage.g.h"

using namespace WindowsPreview::Kinect;
using namespace Microsoft::Kinect::Face;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;



namespace DwightGoins
{
	namespace Utilities
	{
		namespace Kinect
		{
			namespace KinectFaceStudio
			{
				/// <summary>
				/// An empty page that can be used on its own or navigated to within a Frame.
				/// </summary>
				[Windows::UI::Xaml::Data::Bindable]
				public ref class MainPage sealed : Windows::UI::Xaml::Data::INotifyPropertyChanged
				{
				public:

					/// <summary>
					/// Initializes a new instance of the MainPage class.
					/// </summary>
					MainPage();

					/// <summary>
					/// Property changed event
					/// </summary>
					virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

					/// <summary>
					/// Gets or sets the current status text to display
					/// </summary>
					property Platform::String^ StatusText
					{
						Platform::String^ get();
						void set(Platform::String^ value);
					}

					property Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ KinectCameraSource
					{
						Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ get();
						void set(Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ value);
					}

					property Windows::UI::Xaml::Media::ImageSource^ KinectImageSource
					{
						Windows::UI::Xaml::Media::ImageSource^ get();						
					}


				protected:

					/// <summary>
					/// When the page is navigated to event handler
					/// </summary>
					/// <param name="e">event arguments</param>
					virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
				private:

					//DepthSpacePoint[] colorMappedToDepthPoints = null;
					CRITICAL_SECTION cs;

					Windows::Storage::StorageFile ^ m_HDFaceData;

					Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ bitmap;
					Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ bitmapNoBkg;
					/// <summary>
					/// The current status text to display
					/// </summary>
					String^ statusText;

					UINT bitmapBackBufferSize;
					UINT bytesPerPixel;
					Platform::Collections::Vector<String^>^ faceData;

					static const int        cColorWidth = 1920;
					static const int        cColorHeight = 1080;

					/// <summary>
					/// Helper function to format a status message
					/// </summary>
					/// <returns>Status text</returns>
					String^ MakeStatusText();

					/// <summary>
					/// Start a face capture on clicking the button
					/// </summary>
					/// <param name="sender">object sending the event</param>
					/// <param name="e">event arguments</param>
					void StartCaptureButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

					void StartKStudioRecording(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

					void RecordData(Platform::Collections::Vector<String^> ^ data);

					FILE * pHDFaceData;
					void RecordData(String^ data);
					/// <summary>
					/// Fires when Window is Loaded
					/// </summary>
					/// <param name="sender">object sending the event</param>
					/// <param name="e">event arguments</param>
					void Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);


				  Windows::Foundation::Collections::PropertySet^ initColorList();

				  String^ getColorNameFromRgb(int r, int g, int b);

					/// <summary>
					/// Currently used KinectSensor
					/// </summary>
					KinectSensor^ sensor;

					/// <summary>
					/// Body frame source to get a BodyFrameReader
					/// </summary>
					BodyFrameSource^ bodySource;

					/// <summary>
					/// Body frame reader to get body frames
					/// </summary>
					BodyFrameReader^ bodyReader;
					 
					MultiSourceFrameReader^ msFrameReader;
					/// <summary>
					/// HighDefinitionFaceFrameSource to get a reader and a builder from.
					/// Also to set the currently tracked user id to get High Definition Face Frames of
					/// </summary>
					HighDefinitionFaceFrameSource^ highDefinitionFaceFrameSource;

					/// <summary>
					/// HighDefinitionFaceFrameReader to read HighDefinitionFaceFrame to get FaceAlignment
					/// </summary>
					HighDefinitionFaceFrameReader^ highDefinitionFaceFrameReader;

					/// <summary>
					/// FaceAlignment is the result of tracking a face, it has face animations location and orientation
					/// </summary>
					FaceAlignment^ currentFaceAlignment;

					/// <summary>
					/// FaceModel is a result of capturing a face
					/// </summary>
					FaceModel^ currentFaceModel;

					/// <summary>
					/// Indices don't change, save them one time is enough
					/// </summary>
					IVectorView<UINT>^ cachedFaceIndices;

					/// <summary>
					/// FaceModelBuilder is used to produce a FaceModel
					/// </summary>
					FaceModelBuilder^ hdFaceBuilder;

					/// <summary>
					/// Current face model builder collection status string
					/// </summary>
                    String^ currentCollectionStatusString;

                    /// <summary>
                    /// Current face model builder capture status string
                    /// </summary>
                    String^ currentCaptureStatusString;

                    /// <summary>
                    /// Vector of the bodies acquired from a BodyFrame
                    /// </summary>
                    IVector<Body^>^ bodies;

					/// <summary>
					/// The currently tracked body
					/// </summary>
					Body^ currentTrackedBody;

					CoordinateMapper^ coordinateMapper;

					/// <summary>
					/// The currently tracked body Id
					/// </summary>
					UINT64 currentTrackingId;

					/// <summary>
					/// Face capture operation
					/// </summary>
					IAsyncOperation<FaceModelData^>^ currentModelCollectionOperation;

					/// <summary>
					/// Initialize Kinect object
					/// </summary>
					void InitializeHDFace();

					void InitializeKinectColor();
					/// <summary>
					/// This event is fired when a new HDFace frame is ready for consumption
					/// </summary>
					/// <param name="sender">object sending the event</param>
					/// <param name="e">event arguments</param>
					void HDFaceReader_FrameArrived(HighDefinitionFaceFrameReader^ sender, HighDefinitionFaceFrameArrivedEventArgs^ e);

					
                    /// <summary>
                    /// This event is fired when the FaceModelBuilder collection status has changed
                    /// </summary>
                    /// <param name="sender">object sending the event</param>
                    /// <param name="e">event arguments</param>
                    void FaceModelBuilder_CollectionStatusChanged(FaceModelBuilder^ sender, CollectionStatusChangedEventArgs^ e);

                    /// <summary>
                    /// This event is fired when the FaceModelBuilder capture status has changed
                    /// </summary>
                    /// <param name="sender">object sending the event</param>
                    /// <param name="e">event arguments</param>
                    void FaceModelBuilder_CaptureStatusChanged( FaceModelBuilder^ sender, CaptureStatusChangedEventArgs^ e);

					/// <summary>
					/// This event fires when a BodyFrame is ready for consumption
					/// </summary>
					/// <param name="sender">object sending the event</param>
					/// <param name="e">event arguments</param>
					void BodyReader_FrameArrived(BodyFrameReader^ sender, BodyFrameArrivedEventArgs^ e);

					/// <summary>
					/// Sends the new deformed mesh to be drawn
					/// </summary>
					void UpdateFaceMesh();

					/// <summary>
					/// Start a face capture operation
					/// </summary>
					void StartCapture();

					/// <summary>
					/// Cancel the current face capture operation
					/// </summary>
					void StopFaceCapture();

					/// <summary>
					/// Build a string from the collection status bit flag combinations (simplified)
					/// </summary>
					/// <param name="status">Status value</param>
					/// <returns>Status value as text</returns>
                    String^ BuildCollectionStatusText(FaceModelBuilderCollectionStatus status);

                    /// <summary>
                    /// Gets the current capture status
                    /// </summary>
                    /// <param name="status">Status value</param>
                    /// <returns>Status value as text</returns>
                    String^ GetCaptureStatusText(FaceModelBuilderCaptureStatus status);

					/// <summary>
					/// Finds the closest body from the sensor if any
					/// </summary>
					/// <returns>Closest body, null of none</returns>
					Body^ FindClosestBody();

					/// <summary>
					/// Find if there is a body tracked with the given trackingId
					/// </summary>
					/// <param name="trackingId">The tracking Id</param>
					/// <returns>The body object, null of none</returns>
					Body^ FindBodyWithTrackingId(UINT64 trackingId);

					Array<DepthSpacePoint>^ colorMappedToDepthPoints;
					/// <summary>
					/// Returns the length of a vector from origin
					/// </summary>
					/// <param name="point">Point in space to find it's distance from origin</param>
					/// <returns>Distance from origin</returns>
					double VectorLength(CameraSpacePoint point);

                    /// <summary>
                    /// CollectionStatus eventing handle
                    /// </summary>
                    Windows::Foundation::EventRegistrationToken tokenCollectionStatusChanged;

                    /// <summary>
                    /// CaptureStatus eventing handle
                    /// </summary>
                    Windows::Foundation::EventRegistrationToken tokenCaptureStatusChanged;

					bool m_okToGetDeforms;
					void GetDeforms();
					bool m_isFrowning;
					bool m_wroteHeader;
					UINT faceDataCount = 0;
					UINT m_colorWidth = 0;
					UINT m_colorHeight = 0;
					UINT m_cameraWidth = 0;
					UINT m_cameraHeight = 0;


					int FaceShapeAnimations_Count = 17;
					void Page_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
					void OnMultiSourceFrameArrived(WindowsPreview::Kinect::MultiSourceFrameReader ^sender, WindowsPreview::Kinect::MultiSourceFrameArrivedEventArgs ^args);
					void xScale_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
					void xTrans_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
					void showMesh_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
					void showVideo_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
					void showVideo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
					void showMesh_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
};
	
					/*auto AsInspectable(Object^ const object) -> Microsoft::WRL::ComPtr<IInspectable>
					{
						return reinterpret_cast<IInspectable*>(object);
					}

					auto ThrowIfFailed(HRESULT const hr) -> void
					{
						if (FAILED(hr))
							throw Platform::Exception::CreateException(hr);
					}*/

			}


		}
	}

}