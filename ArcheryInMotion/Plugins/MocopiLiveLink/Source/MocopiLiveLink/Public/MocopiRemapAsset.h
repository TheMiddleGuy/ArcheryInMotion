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
#include "LiveLinkRemapAsset.h"
#include "MocopiRemapAsset.generated.h"

/**
 * Retarget class for Mocopi Skeleton.
 */
UCLASS()
class MOCOPILIVELINK_API UMocopiRemapAsset : public ULiveLinkRemapAsset
{
	GENERATED_BODY()

		// ULiveLinkRemapAsset interface
		void BuildPoseFromAnimationData(float DeltaTime,
			const FLiveLinkSkeletonStaticData* InSkeletonData,
			const FLiveLinkAnimationFrameData* InFrameData,
			FCompactPose& OutPose) override;


protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Custom)
		FName TargetSkeletonHipBoneName = "root";

	// cached bone mapping from GetRemappedBoneName
	TMap<FName, FName> mBoneNameMap;

	virtual FVector ConvertRootPosition(FVector position) const;
	virtual FQuat ConvertBoneRotation(FQuat rotation) const;

	bool mHasInitialized = false;

	// World Space rotations for all bones
	TArray<FQuat> mWorldSpaceRotations;

};
