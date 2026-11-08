#include "ArcheryReplaySource.h"

#include "HAL/PlatformTime.h"
#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

#pragma region ILiveLinkSource

void FArcheryReplaySource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	Client = InClient;
	SourceGuid = InSourceGuid;
}

bool FArcheryReplaySource::IsSourceStillValid() const
{
	return Client != nullptr;
}

bool FArcheryReplaySource::RequestSourceShutdown()
{
	Client = nullptr;
	return true;
}

FText FArcheryReplaySource::GetSourceType() const
{
	return INVTEXT("Archery Replay");
}

FText FArcheryReplaySource::GetSourceMachineName() const
{
	return INVTEXT("Recorded shots");
}

FText FArcheryReplaySource::GetSourceStatus() const
{
	return INVTEXT("Replaying");
}

#pragma endregion

#pragma region Pushing data

void FArcheryReplaySource::PushSkeleton(FName SubjectName, const FMocopiSkeleton& Skeleton)
{
	if (!Client) return;

	FLiveLinkStaticDataStruct StaticDataStruct{ FLiveLinkSkeletonStaticData::StaticStruct() };
	FLiveLinkSkeletonStaticData& StaticData{ *StaticDataStruct.Cast<FLiveLinkSkeletonStaticData>() };
	StaticData.BoneNames = Skeleton.BoneNames;
	StaticData.BoneParents = Skeleton.ParentIndices;

	Client->PushSubjectStaticData_AnyThread(FLiveLinkSubjectKey(SourceGuid, SubjectName),
	                                        ULiveLinkAnimationRole::StaticClass(), MoveTemp(StaticDataStruct));
}

void FArcheryReplaySource::PushPose(FName SubjectName, const FMocopiSkeleton& Skeleton, const FMocopiRawFrame& Frame)
{
	if (!Client) return;

	const int32 NumBones{ Skeleton.BoneNames.Num() };

	FLiveLinkFrameDataStruct FrameDataStruct{ FLiveLinkAnimationFrameData::StaticStruct() };
	FLiveLinkAnimationFrameData& FrameData{ *FrameDataStruct.Cast<FLiveLinkAnimationFrameData>() };
	FrameData.Transforms.SetNum(NumBones);
	
	for (int32 Bone{ 0 }; Bone < NumBones; ++Bone)
	{
		FQuat Rotation{ Frame.LocalRotations.IsValidIndex(Bone) ? FQuat(Frame.LocalRotations[Bone]) : FQuat::Identity };
		Rotation.Normalize();

		const FVector Translation{
			Bone == 0 ? FVector(Frame.RootPosition)
			          : Skeleton.DefaultTranslations.IsValidIndex(Bone) ? FVector(Skeleton.DefaultTranslations[Bone])
			                                                             : FVector::ZeroVector
		};

		FrameData.Transforms[Bone] = FTransform(Rotation, Translation);
	}

	FrameData.WorldTime = FLiveLinkWorldTime(FPlatformTime::Seconds());

	Client->PushSubjectFrameData_AnyThread(FLiveLinkSubjectKey(SourceGuid, SubjectName), MoveTemp(FrameDataStruct));
}

#pragma endregion
