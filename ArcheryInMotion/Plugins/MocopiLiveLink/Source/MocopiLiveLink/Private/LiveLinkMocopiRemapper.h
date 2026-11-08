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

#include "Remapper/LiveLinkSkeletonRemapper.h"

#include "Features/IModularFeatures.h"
#include "LiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include "LiveLinkMocopiRemapper.generated.h"

/* This Remapper class is to be used when using this plugin inside of UEFN.  (Not needed when used inside UE Editor) */

class USkeletalMesh;

class FLiveLinkMocopiRemapperWorker : public FLiveLinkSkeletonRemapperWorker
{
public:
	virtual void RemapSkeletonStaticData(FLiveLinkSkeletonStaticData& InOutSkeletonData) override
	{
		if (!ReferenceSkeleton)
		{
			return;
		}

		const FReferenceSkeleton& RefSkeleton = ReferenceSkeleton->GetRefSkeleton();

		const TArray<FMeshBoneInfo>& meshBoneInfo = RefSkeleton.GetRefBoneInfo();
		const TArray<FTransform>& meshBoneRefPose = RefSkeleton.GetRefBonePose();

		TArray<FName> sourceBoneNames = InOutSkeletonData.GetBoneNames();

		TArray<FName> targetBoneNames;
		targetBoneNames.Reserve(sourceBoneNames.Num());
		for (FName srcBoneName : RefSkeleton.GetRawRefBoneNames())
		{
			targetBoneNames.Add(GetRemappedBoneName(srcBoneName));
		}

		// Remapped bone is for the outgoing, so it needs to be in the final static data.
		TArray<int32> BoneParents;
		TMap<FName, int32> NameToIndexInRefSkeleton;
		for (int32 Index = 0; Index < meshBoneInfo.Num(); Index++)
		{
			BoneParents.Add(meshBoneInfo[Index].ParentIndex);
		}

		FrameIndexToSkeletonIndex.Reset();
		for (int32 Index = 0; Index < InOutSkeletonData.GetBoneNames().Num(); Index++)
		{
			FName TargetBoneName = GetRemappedBoneName(InOutSkeletonData.GetBoneNames()[Index]);
			int32 BoneIdx = targetBoneNames.IndexOfByKey(TargetBoneName);

			FrameIndexToSkeletonIndex.Add(Index, BoneIdx);
		}


		// Override the final bone parents and bone nanmes
		InOutSkeletonData.SetBoneNames(targetBoneNames);
		InOutSkeletonData.SetBoneParents(BoneParents);
	}

	virtual void RemapSkeletonFrameData(const FLiveLinkSkeletonStaticData& InOutSkeletonData, FLiveLinkAnimationFrameData& InOutFrameData) override
	{
		if (!ReferenceSkeleton)
		{
			return;
		}

		// At this point, source bone names must match the reference skeleton bone names? 
		TArray<FName> sourceBoneNames = InOutSkeletonData.GetBoneNames();

		const FReferenceSkeleton& RefSkeleton = ReferenceSkeleton->GetRefSkeleton();

		const TArray<FMeshBoneInfo>& meshBoneInfo = RefSkeleton.GetRefBoneInfo();
		const TArray<FTransform>& meshBoneRefPose = RefSkeleton.GetRefBonePose();

		TMap<FName, int32> NameToIndexInRefSkeleton;
		for (int32 Index = 0; Index < meshBoneInfo.Num(); Index++)
		{
			NameToIndexInRefSkeleton.FindOrAdd(meshBoneInfo[Index].Name) = Index;
		}

		// Initialize World Space rotations
		if (!bHasInitialized)
		{
			// Initializing rotations from Mesh bones. (Max number of bones)
			int32 meshBoneCount = meshBoneRefPose.Num();
			WorldSpaceRotations.Init(FQuat::Identity, meshBoneCount);

			for (int32 i = 0; i < meshBoneCount; i++)
			{
				FQuat rotation = meshBoneRefPose[i].GetRotation();

				int32 ParentIndex = RefSkeleton.GetRefBoneInfo()[i].ParentIndex;
				if ((ParentIndex != INDEX_NONE) && (ParentIndex < meshBoneCount))
				{
					rotation = WorldSpaceRotations[ParentIndex] * rotation;
				}

				WorldSpaceRotations[i] = rotation;
			}

			bHasInitialized = true;
		}


		TArray<FTransform> BoneCopies;

		BoneCopies.Reserve(InOutFrameData.Transforms.Num());
		BoneCopies = InOutFrameData.Transforms;

		InOutFrameData.Transforms.Reset(meshBoneInfo.Num());
		InOutFrameData.Transforms.AddDefaulted(meshBoneInfo.Num());

		// Process incoming data
		for (int32 i = 0; i < FrameIndexToSkeletonIndex.Num(); ++i)
		{
			FName boneName = sourceBoneNames[i];

			int32 BoneIdx = FrameIndexToSkeletonIndex[i];
			if (BoneIdx == INDEX_NONE)
			{
				FString StringToPutBreakpoint;
			}

			int32 refBoneIndex = BoneIdx;

			// Here we would fetch the correct bone name, but we should probably just override the bone names in the static data.
			const FTransform& boneTransform = BoneCopies[i];
			{
				if (boneName == TargetSkeletonHipBoneName)
				{
					FQuat rotation = ConvertBoneRotation(boneTransform.GetRotation()) * WorldSpaceRotations[refBoneIndex];
					FVector position = ConvertRootPosition(boneTransform.GetLocation());
					FTransform hipsTransform(rotation, position);

					InOutFrameData.Transforms[refBoneIndex] = hipsTransform;
					continue;
				}

				int32 parentMeshIndex = RefSkeleton.GetRefBoneInfo()[refBoneIndex].ParentIndex;

				if (parentMeshIndex != INDEX_NONE)
				{
					FTransform TransformCopy = meshBoneRefPose[refBoneIndex];
					FQuat rotationInComponentSpace = ConvertBoneRotation(boneTransform.GetRotation()) * WorldSpaceRotations[refBoneIndex];
					FQuat rotationInParentBoneSpace = WorldSpaceRotations[parentMeshIndex].Inverse() * rotationInComponentSpace;
					TransformCopy.SetRotation(rotationInParentBoneSpace);
					InOutFrameData.Transforms[refBoneIndex] = TransformCopy;
				}
			}
		}
	}

	FVector ConvertRootPosition(FVector position) const
	{
		return FVector(position.X, position.Z, position.Y);
	}

	FQuat ConvertBoneRotation(FQuat rotation) const
	{
		return FQuat(rotation.X, rotation.Z, rotation.Y, -rotation.W);
	}

	TStrongObjectPtr<USkeletalMesh> ReferenceSkeleton;

	/**
	 * Map of FrameData bone index to skeleton frame index.
	 * Populated by the RemapStaticData method.
	 */
	TMap<int32, int32> FrameIndexToSkeletonIndex; 

	FName TargetSkeletonHipBoneName = "root";

	bool bHasInitialized = false;

	/** World Space rotations for all bones */
	TArray<FQuat> WorldSpaceRotations;

};

UCLASS(EditInlineNew)
class ULiveLinkMocopiRemapper : public ULiveLinkSkeletonRemapper
{
public:
	GENERATED_BODY()

	ULiveLinkMocopiRemapper()
	{
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			ReferenceSkeleton = FSoftObjectPath(TEXT("/MocopiLiveLink/Mocopi/MocopiMannequin.MocopiMannequin"));
			ReferenceSkeleton.LoadSynchronous();
		}
	}

	virtual FWorkerSharedPtr GetWorker() const override
	{
		return Instance;
	}

	virtual FWorkerSharedPtr CreateWorker() override
	{
		Instance = MakeShared<FLiveLinkMocopiRemapperWorker>();
		Instance->BoneNameMap = BoneNameMap;
		Instance->ReferenceSkeleton = TStrongObjectPtr<USkeletalMesh>(ReferenceSkeleton.LoadSynchronous());
		Instance->TargetSkeletonHipBoneName = TargetSkeletonHipBoneName;
		return Instance;
	}

public:
	TSharedPtr<FLiveLinkMocopiRemapperWorker> Instance;

	FName TargetSkeletonHipBoneName = "root";
};
