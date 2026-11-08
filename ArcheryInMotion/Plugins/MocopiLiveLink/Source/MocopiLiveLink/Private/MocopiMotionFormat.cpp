////
// Copyright (C) 2026 Sony Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////

#include "MocopiMotionFormat.h"
#include "MocopiLog.h"

#ifdef __APPLE__
#include <dlfcn.h>
#endif

#define LOCTEXT_NAMESPACE "MocopiLiveLinkModule"

#include "HAL/PlatformProcess.h"


#define MMF_IMPORT(Func) \
  Func = (Func##Proc)FPlatformProcess::GetDllExport(mDllHandle, TEXT(#Func)); \
	if (Func == nullptr) \
	{ \
        FText error = LOCTEXT("MMFImportError", "Failed to import MocopiMotionFormat function"); \
		UE_LOG(LogMocopiLiveLink, Error, TEXT("%s %s"), *(error.ToString()), TEXT(#Func)); \
		Shutdown(); \
	}


#define MMF_DEFINE(Func) \
    Func##Proc MocopiMotionFormat::Func = nullptr;


MMF_DEFINE(IsMmfBytes)
MMF_DEFINE(IsSkeletonDefinitionBytes)
MMF_DEFINE(IsFrameDataBytes)
MMF_DEFINE(ConvertBytesToSkeletonDefinition)
MMF_DEFINE(ConvertBytesToFrameData)


LibHandle MocopiMotionFormat::mDllHandle = nullptr;
std::string MocopiMotionFormat::mPluginPath;


void MocopiMotionFormat::Initialize(const std::string& pluginPath)
{
  if (mDllHandle == nullptr)
  {
    mPluginPath = pluginPath;

    if (LoadDll())
    {
      MMF_IMPORT(IsMmfBytes);
      MMF_IMPORT(IsSkeletonDefinitionBytes);
      MMF_IMPORT(IsFrameDataBytes);
      MMF_IMPORT(ConvertBytesToSkeletonDefinition);
      MMF_IMPORT(ConvertBytesToFrameData);
    }
  }
}

void MocopiMotionFormat::Shutdown()
{
  if (mDllHandle != nullptr)
  {
    FPlatformProcess::FreeDllHandle(mDllHandle);
    mDllHandle = nullptr;
  }
}

bool MocopiMotionFormat::IsValid()
{
  return mDllHandle != nullptr;
}

bool MocopiMotionFormat::LoadDll()
{
  const std::string DllPath = GetDLLPath();

  const FString fsDllPath = FString(DllPath.c_str());

  mDllHandle = FPlatformProcess::GetDllHandle(*fsDllPath);

  if (mDllHandle == nullptr)
  {
    FText error = LOCTEXT("MMFLoadError", "Failed to load the required MocopiMotionFormat library. Plugin will not be functional. File Path: ");
    UE_LOG(LogMocopiLiveLink, Error, TEXT("%s %s"), *(error.ToString()), *fsDllPath);
    return false;
  }

  return true;
}

std::string MocopiMotionFormat::GetDLLPath()
{ 
#ifdef _WIN64
  const std::string DllName = "mocopi_motion_serializer.dll";
  const std::string Platform = "windows";
  const std::string Arch = "x64";

#elif __APPLE__
  const std::string DllName = "mocopi_motion_serializer.bundle/Contents/MacOS/mocopi_motion_serializer";
  const std::string Platform = "mac";
  const std::string Arch = "";
#endif

  const std::string MMFFolder = "Source/mocopiMotionSerializer";

  std::string finalPath = mPluginPath + "/" + MMFFolder + "/" + Platform + "/" + Arch + "/" + DllName;

  return finalPath;
}

#undef LOCTEXT_NAMESPACE
