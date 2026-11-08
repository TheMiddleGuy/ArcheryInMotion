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

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Runtime/Launch/Resources/Version.h"

#include <unordered_map>

const int DEFAULT_MOCOPI_PORT = 12351;
const FString DEFAULT_SKELETON_NAME = "MocopiSkeleton";

#if (ENGINE_MAJOR_VERSION >= 5) && (ENGINE_MINOR_VERSION < 3)
#define USE_DEPRECATED_DEBUGF
#endif

class SMocopiLiveLinkSourceFactory : public SCompoundWidget
{
public:
  DECLARE_DELEGATE_TwoParams(FOnCreateClicked, uint16, FName);

  SLATE_BEGIN_ARGS(SMocopiLiveLinkSourceFactory) {}
    SLATE_EVENT(FOnCreateClicked, OnCreateClicked)
  SLATE_END_ARGS()

  ~SMocopiLiveLinkSourceFactory();

  void Construct(const FArguments& Args);

  static void RemoveSubject(uint16 inputPort);

  static void AddSubject(uint16 inputPort, FName subjectName);

private:

  // map of subjects(sources) created
  static std::unordered_map<uint16, FName> mPortToSubjectNameMap;

  TWeakPtr<SEditableTextBox> mInputPortField;
  TWeakPtr<SEditableTextBox> mSubjectNameField;

  FReply OnCreateClicked();

  FOnCreateClicked CreateClicked;

  FString GetSuggestedSkeletonName();
  FString GetSuggestedPort();

};
