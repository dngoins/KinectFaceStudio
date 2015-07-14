//------------------------------------------------------------------------------
// <copyright file="App.xaml.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


#pragma once

#include "App.g.h"

namespace DwightGoins
{
	namespace Utilities
	{
		namespace Kinect
		{
			namespace KinectFaceStudio
			{
				/// <summary>
				/// Provides application-specific behavior to supplement the default Application class.
				/// </summary>
				ref class App sealed
				{
				public:
					App();
					virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;

				private:
					void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
				};
			}
		}
	}
}
