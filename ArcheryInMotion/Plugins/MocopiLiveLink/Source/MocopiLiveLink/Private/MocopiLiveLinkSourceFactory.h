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

#include "CoreMinimal.h"
#include "LiveLinkSourceFactory.h"
#include "SMocopiLiveLinkSourceFactory.h"
#include "MocopiLiveLinkSourceFactory.generated.h"


UCLASS()
class UMocopiLiveLinkSourceFactory : public ULiveLinkSourceFactory
{
public:
  GENERATED_BODY()

  virtual FText GetSourceDisplayName() const;
  virtual FText GetSourceTooltip() const;

  virtual EMenuType GetMenuType() const override { return EMenuType::SubPanel; }
  virtual TSharedPtr<SWidget> BuildCreationPanel(FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const override;

  virtual TSharedPtr<ILiveLinkSource> CreateSource(const FString& ConnectionString) const override;

private:

  // Creates the Mocopi LiveLinkSource
  void OnCreateClicked(uint16 inputPort, FName subjectName, FOnLiveLinkSourceCreated onLiveLinkSourceCreated) const;
};
