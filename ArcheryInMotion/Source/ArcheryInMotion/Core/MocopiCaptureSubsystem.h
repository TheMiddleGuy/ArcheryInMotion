#pragma once

#include "CoreMinimal.h"
#include "MocopiRawData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MocopiCaptureSubsystem.generated.h"

/** Health of the incoming mocopi stream, for the on-screen status. */
struct FMocopiStreamStats
{
	uint64 FramesReceived = 0;
	uint64 FramesMissing = 0;          // gaps in the mocopi frame counter
	double SecondsSinceLastFrame = -1; // -1 = nothing received yet
	double FramesPerSecond = 0;        // measured over the last second
};

/**
 * Lives as long as the game runs (one per Play session).
 *  - Makes sure the mocopi Live Link source exists
 *  - Receives every raw mocopi frame on the network thread.
 *  - Keeps the last few seconds in memory, so a take can start "in the past" (pre-roll).
 *  - Collects frames into a take between StartTake and StopTake.
 */
UCLASS()
class ARCHERYINMOTION_API UMocopiCaptureSubsystem : public UGameInstanceSubsystem, public IMocopiRawFrameListener
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** IMocopiRawFrameListener. Runs on the plugin's network thread. */
	virtual void OnMocopiRawFrame(const FMocopiRawFrame& Frame) override;

	/** Starts collecting a take. It begins with the frames from the last PreRollSeconds. */
	void StartTake(double PreRollSeconds);

	/** Stops the take and returns its frames, oldest first. */
	TArray<FMocopiRawFrame> StopTake();

	bool IsTakeRunning() const;
	int32 GetTakeFrameCount() const;
	FMocopiStreamStats GetStreamStats() const;

private:
	/** How much history is kept for pre-roll. */
	static constexpr double HistorySeconds{ 10.0 };

	mutable FCriticalSection Lock;

	TArray<FMocopiRawFrame> RecentFrames; // rolling history for pre-roll
	TArray<FMocopiRawFrame> TakeFrames;
	bool bTakeRunning{ false };

	uint64 FramesReceived{ 0 };
	uint64 FramesMissing{ 0 };
	int32 LastFrameId{ 0 };
	bool bHasLastFrameId{ false };
	double LastReceiveTime{ -1.0 };
};
