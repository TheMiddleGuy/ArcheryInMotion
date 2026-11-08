#include "MocopiCaptureSubsystem.h"

#include "HAL/PlatformTime.h"
#include "Misc/ScopeLock.h"

void UMocopiCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Same as "+ Add Source" in the Live Link window, but automatic. Reuses an existing source.
	const FGuid SourceGuid{ FMocopiRawData::FindOrCreateLiveLinkSource(12351, FName(TEXT("MocopiSkeleton"))) };
	if (!SourceGuid.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcheryInMotion] Could not create the mocopi Live Link source (Live Link not available?)"));
	}

	FMocopiRawData::AddListener(this);
}

void UMocopiCaptureSubsystem::Deinitialize()
{
	// After this returns the network thread will not call any more.
	FMocopiRawData::RemoveListener(this);
	Super::Deinitialize();
}

void UMocopiCaptureSubsystem::OnMocopiRawFrame(const FMocopiRawFrame& Frame)
{
	FScopeLock ScopeLock(&Lock);

	// Stream statistics
	++FramesReceived;
	if (bHasLastFrameId)
	{
		const int64 Delta{ static_cast<int64>(Frame.FrameId) - static_cast<int64>(LastFrameId) };
		if (Delta > 1 && Delta < 10000)
		{
			FramesMissing += static_cast<uint64>(Delta - 1);
		}
	}
	LastFrameId = Frame.FrameId;
	bHasLastFrameId = true;
	LastReceiveTime = Frame.ReceiveTime;

	// Rolling history. Trimmed in chunks not to shift the array on every frame.
	RecentFrames.Add(Frame);
	if (Frame.ReceiveTime - RecentFrames[0].ReceiveTime > HistorySeconds * 1.5)
	{
		const double Cutoff{ Frame.ReceiveTime - HistorySeconds };
		int32 FirstToKeep{ 0 };
		while (FirstToKeep < RecentFrames.Num() && RecentFrames[FirstToKeep].ReceiveTime < Cutoff)
		{
			++FirstToKeep;
		}
		RecentFrames.RemoveAt(0, FirstToKeep, EAllowShrinking::No);
	}

	if (bTakeRunning) TakeFrames.Add(Frame);
}

void UMocopiCaptureSubsystem::StartTake(double PreRollSeconds)
{
	const double Cutoff{ FPlatformTime::Seconds() - FMath::Max(0.0, PreRollSeconds) };

	FScopeLock ScopeLock(&Lock);
	TakeFrames.Reset();
	for (const FMocopiRawFrame& Frame : RecentFrames)
	{
		if (Frame.ReceiveTime >= Cutoff)
		{
			TakeFrames.Add(Frame);
		}
	}
	bTakeRunning = true;
}

TArray<FMocopiRawFrame> UMocopiCaptureSubsystem::StopTake()
{
	FScopeLock ScopeLock(&Lock);
	bTakeRunning = false;
	return MoveTemp(TakeFrames);
}

bool UMocopiCaptureSubsystem::IsTakeRunning() const
{
	FScopeLock ScopeLock(&Lock);
	return bTakeRunning;
}

int32 UMocopiCaptureSubsystem::GetTakeFrameCount() const
{
	FScopeLock ScopeLock(&Lock);
	return TakeFrames.Num();
}

FMocopiStreamStats UMocopiCaptureSubsystem::GetStreamStats() const
{
	const double Now{ FPlatformTime::Seconds() };

	FScopeLock ScopeLock(&Lock);
	FMocopiStreamStats Stats;
	Stats.FramesReceived = FramesReceived;
	Stats.FramesMissing = FramesMissing;
	Stats.SecondsSinceLastFrame = LastReceiveTime < 0.0 ? -1.0 : Now - LastReceiveTime;

	int32 FramesInLastSecond{ 0 };
	for (int32 Index{ RecentFrames.Num() - 1 }; Index >= 0 && RecentFrames[Index].ReceiveTime >= Now - 1.0; --Index)
	{
		++FramesInLastSecond;
	}
	Stats.FramesPerSecond = FramesInLastSecond;
	return Stats;
}
