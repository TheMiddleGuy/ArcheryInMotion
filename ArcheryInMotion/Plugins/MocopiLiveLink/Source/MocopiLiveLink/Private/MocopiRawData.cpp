// [ArcheryInMotion] Addition to Sony's mocopi plugin (Apache 2.0). See MocopiRawData.h.

#include "MocopiRawData.h"
#include "MocopiLiveLinkSource.h"

#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "Misc/ScopeLock.h"

#include <atomic>

#define LOCTEXT_NAMESPACE "MocopiLiveLinkModule"

namespace
{
	FCriticalSection GListenerLock;
	TArray<IMocopiRawFrameListener*> GListeners;
	std::atomic<int32> GListenerCount{0};

	FCriticalSection GSkeletonLock;
	FMocopiSkeleton GSkeleton;
	bool GHasSkeleton = false;
}

void FMocopiRawData::AddListener(IMocopiRawFrameListener* Listener)
{
	if (Listener == nullptr)
	{
		return;
	}

	FScopeLock ScopeLock(&GListenerLock);
	GListeners.AddUnique(Listener);
	GListenerCount.store(GListeners.Num());
}

void FMocopiRawData::RemoveListener(IMocopiRawFrameListener* Listener)
{
	// Taking the lock waits for a PublishFrame call that is still running on the network thread.
	FScopeLock ScopeLock(&GListenerLock);
	GListeners.Remove(Listener);
	GListenerCount.store(GListeners.Num());
}

bool FMocopiRawData::HasListeners()
{
	return GListenerCount.load() > 0;
}

void FMocopiRawData::PublishFrame(const FMocopiRawFrame& Frame)
{
	FScopeLock ScopeLock(&GListenerLock);
	for (IMocopiRawFrameListener* Listener : GListeners)
	{
		Listener->OnMocopiRawFrame(Frame);
	}
}

void FMocopiRawData::PublishSkeleton(const FMocopiSkeleton& Skeleton)
{
	FScopeLock ScopeLock(&GSkeletonLock);
	GSkeleton = Skeleton;
	GHasSkeleton = true;
}

bool FMocopiRawData::GetSkeleton(FMocopiSkeleton& OutSkeleton)
{
	FScopeLock ScopeLock(&GSkeletonLock);
	if (!GHasSkeleton)
	{
		return false;
	}
	OutSkeleton = GSkeleton;
	return true;
}

FGuid FMocopiRawData::FindOrCreateLiveLinkSource(uint16 Port, FName SubjectName)
{
	IModularFeatures& Features = IModularFeatures::Get();
	if (!Features.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		return FGuid();
	}

	ILiveLinkClient& Client = Features.GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);

	// Reuse a mocopi source that already exists (added by hand, by a preset, or by an earlier
	// Play session). A second one on the same port would fail to open its socket.
	const FText MocopiSourceType = LOCTEXT("sourceType", "Mocopi LiveLink");
	for (const FGuid& SourceGuid : Client.GetSources())
	{
		if (Client.GetSourceType(SourceGuid).EqualTo(MocopiSourceType))
		{
			return SourceGuid;
		}
	}

	TSharedPtr<ILiveLinkSource> Source = MakeShared<FMocopiLiveLinkSource>(Port, SubjectName);
	return Client.AddSource(Source);
}

#undef LOCTEXT_NAMESPACE
