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

#include "MocopiLiveLink.h"
#include "MocopiLog.h"
#include "MocopiMotionFormat.h"

#include "Interfaces/IPluginManager.h"

#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogMocopiLiveLink);

void FMocopiLiveLinkModule::StartupModule()
{

  // Set Localization based on current editor language
  FCultureRef culture = FInternationalization::Get().GetCurrentLanguage();
  FString ISOLanguageName = culture.Get().GetTwoLetterISOLanguageName();
  SetPluginLocalization(ISOLanguageName);

  // Adding a callback to be notified of any culture changes, so we can switch our language
  mCultureChangedHandle = FInternationalization::Get().OnCultureChanged().AddRaw(this, &FMocopiLiveLinkModule::OnCultureChanged);

  // Link to DLL Upon Plugin Load
  const FString pluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("MocopiLiveLink"))->GetBaseDir();
  MocopiMotionFormat::Initialize(std::string(TCHAR_TO_UTF8(*pluginBaseDir)));

}

void FMocopiLiveLinkModule::ShutdownModule()
{
  // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
  // we call this function before unloading the module.

  FInternationalization::Get().OnCultureChanged().Remove(mCultureChangedHandle);

  MocopiMotionFormat::Shutdown();
}

void FMocopiLiveLinkModule::OnCultureChanged()
{
  FCultureRef lang = FInternationalization::Get().GetCurrentLanguage();
  FString ISOLanguageName = lang.Get().GetTwoLetterISOLanguageName();
  SetPluginLocalization(ISOLanguageName);
}

void FMocopiLiveLinkModule::SetPluginLocalization(FString ISOLanguageCode)
{
  // Get the localization path for the plugin
  const FString pluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("MocopiLiveLink"))->GetBaseDir();
  FString localizationPath = FPaths::Combine(pluginBaseDir, "Resources", "Localization");

  FString locResFilePath = FPaths::Combine(localizationPath, ISOLanguageCode, TEXT("MocopiLiveLink.locres"));
  if (FPaths::FileExists(locResFilePath))
  {
    // Set plugin localization based on passed in ISO code
    FTextLocalizationManager::Get().UpdateFromLocalizationResource(locResFilePath);
    return;
  }

  // Default to English 
  locResFilePath = FPaths::Combine(localizationPath, "en", TEXT("MocopiLiveLink.locres"));
  if (FPaths::FileExists(locResFilePath))
  {
    FTextLocalizationManager::Get().UpdateFromLocalizationResource(locResFilePath);
    return;
  }

}


IMPLEMENT_MODULE(FMocopiLiveLinkModule, MocopiLiveLink)
