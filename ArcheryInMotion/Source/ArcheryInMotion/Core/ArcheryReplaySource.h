#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "MocopiRawData.h"

class ILiveLinkClient;

/**
 * A Live Link source that plays back recorded mocopi frames.
 *
 * It sends the same data the mocopi plugin sends live (same bone names, parents, and
 * transforms), but under its own subject name (e.g. "MocopiReplay"). A second mannequin
 * whose Live Link Pose node uses that subject name then shows the replay, exactly like
 * live mode, and without the sensors.
 */
class FArcheryReplaySource : public ILiveLinkSource
{
public:
	// ILiveLinkSource
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
	virtual bool IsSourceStillValid() const override;
	virtual bool RequestSourceShutdown() override;
	virtual FText GetSourceType() const override;
	virtual FText GetSourceMachineName() const override;
	virtual FText GetSourceStatus() const override;

	/** Defines (or redefines) the subject's skeleton. Call when a session is opened. */
	void PushSkeleton(FName SubjectName, const FMocopiSkeleton& Skeleton);

	/** Shows one recorded frame. Call every tick, also while paused, so the subject stays valid. */
	void PushPose(FName SubjectName, const FMocopiSkeleton& Skeleton, const FMocopiRawFrame& Frame);

private:
	ILiveLinkClient* Client{ nullptr };
	FGuid SourceGuid;
};
