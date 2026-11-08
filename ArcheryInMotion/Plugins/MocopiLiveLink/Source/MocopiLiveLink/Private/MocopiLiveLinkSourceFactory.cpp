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

#include "MocopiLiveLinkSourceFactory.h"

#include "MocopiLiveLinkSource.h"

#define LOCTEXT_NAMESPACE "MocopiLiveLinkModule"

FText UMocopiLiveLinkSourceFactory::GetSourceDisplayName() const
{
  return LOCTEXT("SourceDisplayName", "Mocopi LiveLink");
}

FText UMocopiLiveLinkSourceFactory::GetSourceTooltip() const
{
  return LOCTEXT("SourceTooltip", "Creates a connection to Mocopi Sensors");
}

TSharedPtr<SWidget> UMocopiLiveLinkSourceFactory::BuildCreationPanel(FOnLiveLinkSourceCreated InOnLiveLinkSourceCreated) const
{
  return SNew(SMocopiLiveLinkSourceFactory)
    .OnCreateClicked(SMocopiLiveLinkSourceFactory::FOnCreateClicked::CreateUObject(this, &UMocopiLiveLinkSourceFactory::OnCreateClicked, InOnLiveLinkSourceCreated));
}

TSharedPtr<ILiveLinkSource> UMocopiLiveLinkSourceFactory::CreateSource(const FString& ConnectionString) const
{
  uint64 port = DEFAULT_MOCOPI_PORT;
  FName subjectName = FName(*DEFAULT_SKELETON_NAME);

  TArray<FString, FDefaultAllocator> Parts;
  int result = ConnectionString.ParseIntoArrayLines(Parts, true);
  if (Parts.Num() >= 2)
  {
    // No error checking for valid data on the connection string.
    // That should be done prior to calling this function.
    port = FCString::Atoi(*Parts[0]);
    subjectName = FName(*Parts[1]);
  }

  return MakeShared<FMocopiLiveLinkSource>(port, subjectName);
}

void UMocopiLiveLinkSourceFactory::OnCreateClicked(uint16 inputPort, FName subjectName, FOnLiveLinkSourceCreated onLiveLinkSourceCreated) const
{

  FString connectionString = FString::Format(TEXT("{0}\n{1}"), { inputPort, subjectName.ToString() });

  onLiveLinkSourceCreated.ExecuteIfBound(MakeShared<FMocopiLiveLinkSource>(inputPort, subjectName), connectionString);
}


#undef LOCTEXT_NAMESPACE
