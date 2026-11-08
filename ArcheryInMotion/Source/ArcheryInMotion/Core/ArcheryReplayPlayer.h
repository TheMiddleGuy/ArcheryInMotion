// Plays back recorded shots on a mannequin. Place one in the level and press Play.
//
// Keys (click into the game viewport first):
//   Left / Right   previous / next shot (re-reads shots.csv, so new shots show up)
//   Up / Down      previous / next session
//   P              play / pause
//   , / .          one frame back / forward (pauses)
//   - / =          slower / faster
//   R              back to the start of the shot
//   T              jump to the Space press (t = 0)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "MocopiRawData.h"
#include "ArcheryReplayPlayer.generated.h"

class FArcheryReplaySource;

/** One row of shots.csv. */
struct FReplayAttempt
{
	FString File; // relative to the session folder
	int32 End{ 0 };
	int32 Arrow{ 0 };
	int32 Attempt{ 0 };
	FString Type;   // shot / letdown / interrupted
	FString Rating; // empty = not rated
	double StopPressT{ 0.0 };
};

UCLASS()
class ARCHERYINMOTION_API AArcheryReplayPlayer : public AActor
{
	GENERATED_BODY()

public:
	AArcheryReplayPlayer();

	virtual void Tick(float DeltaSeconds) override;

	/** Live Link subject the replay mannequin listens to (its Live Link Pose node must use this name). */
	UPROPERTY(EditAnywhere, Category = "Replay")
	FName SubjectName{ TEXT("MocopiReplay") };

	/** Leave empty to use the "Recordings" folder next to the Unreal project folder (same as the recorder). */
	UPROPERTY(EditAnywhere, Category = "Replay")
	FString RecordingsFolder;

	/** Session folder name to open first, e.g. "2026-11-08_21-57-23_Archer_Compound". Empty = newest. */
	UPROPERTY(EditAnywhere, Category = "Replay")
	FString StartSession;

	UPROPERTY(EditAnywhere, Category = "Replay", meta = (ClampMin = "0.05", ClampMax = "4"))
	float PlaybackSpeed{ 1.0f };

	/** Stops arrow keys from moving the camera. */
	UPROPERTY(EditAnywhere, Category = "Replay")
	bool bLockCameraMovement{ true };

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Keys
	void PollKeys();
	bool WasPressed(const FKey& Key) const;

	// Browsing
	void RefreshSessions();
	void OpenSession(int32 Index);
	void RefreshAttempts();
	void OpenAttempt(int32 Index);

	// Playback
	void SetFrame(int32 Index);
	void ChangeSpeed(int32 Direction);
	void PushCurrentFrame();

	// On-screen status
	void DrawStatus() const;
	void ShowMessage(const FString& Text, const FColor& Color, float Seconds = 4.0f);

	TSharedPtr<FArcheryReplaySource> Source;
	FGuid SourceGuid;

	TArray<FString> SessionDirs; // full paths, oldest first
	int32 SessionIndex{ INDEX_NONE };

	TArray<FReplayAttempt> Attempts;
	int32 AttemptIndex{ INDEX_NONE };

	FMocopiSkeleton Skeleton;
	bool bHasSkeleton{ false };

	TArray<FMocopiRawFrame> Frames;
	TArray<double> FrameTimes; // seconds since the first frame (mocopi clock)
	int32 FrameIndex{ 0 };
	double PlayTime{ 0.0 };
	bool bPlaying{ true };

	FString Message;
	FColor MessageColor{ FColor::White };
	double MessageUntil{ 0.0 };
};
