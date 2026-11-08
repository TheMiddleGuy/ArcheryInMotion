#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ArcherySessionRecorder.generated.h"

class UMocopiCaptureSubsystem;
struct FMocopiRawFrame;

UENUM(BlueprintType)
enum class EArcheryBowType : uint8
{
	Compound,
	Recurve,
	Barebow,
	Longbow,
	Other
};

UENUM(BlueprintType)
enum class EArcheryDrawHand : uint8
{
	Right,
	Left
};

/** Where the recorder is in the shot flow. */
enum class EShotRecorderState : uint8
{
	Idle,
	Recording,
	PostRoll,
	AwaitRating
};

/** A saved attempt whose shots.csv row hasn't been written yet. */
struct FSavedAttempt
{
	FString RelativeFile;
	int32 Frames = 0;
	double DurationSeconds = 0.0;
	double StopPressT = 0.0;
	int32 MissingFrames = 0;
};

// Keys:
//   Space      start recording a shot / stop it after the arrow has landed
//   L          let-down: saves the attempt flagged as let-down, same arrow is recorded again
//   Backspace  discard: throws away the current take (or the shot that's waiting for a rating)
//   1-5        rate the shot you just shot (0 = no rating). Space also skips the rating.

UCLASS()
class ARCHERYINMOTION_API AArcherySessionRecorder : public AActor
{
	GENERATED_BODY()

public:
	AArcherySessionRecorder();

	virtual void Tick(float DeltaSeconds) override;

	// --- Session info, written to session.json ---

	UPROPERTY(EditAnywhere, Category = "Session")
	FString ArcherName{ TEXT("Archer") };

	UPROPERTY(EditAnywhere, Category = "Session")
	EArcheryBowType BowType{ EArcheryBowType::Compound };

	UPROPERTY(EditAnywhere, Category = "Session")
	EArcheryDrawHand DrawHand{ EArcheryDrawHand::Right };

	UPROPERTY(EditAnywhere, Category = "Session")
	FString TargetFace{ TEXT("WA 40cm triple spot") };

	UPROPERTY(EditAnywhere, Category = "Session", meta = (ClampMin = "1", ForceUnits = "m"))
	float DistanceMeters{ 18.0f };

	UPROPERTY(EditAnywhere, Category = "Session", meta = (ClampMin = "0", ForceUnits = "mm"))
	float ArrowDiameterMm{ 9.0f };

	UPROPERTY(EditAnywhere, Category = "Session", meta = (ClampMin = "1", ClampMax = "12"))
	int32 ArrowsPerEnd{ 3 };

	/** The tracking mode selected in the mocopi phone app (the app doesn't send it). */
	UPROPERTY(EditAnywhere, Category = "Session")
	FString TrackingMode{ TEXT("Upper-body focus") };

	/** Sensor placement, how you feel, conditions... */
	UPROPERTY(EditAnywhere, Category = "Session", meta = (MultiLine = true))
	FString Notes;

	// --- Recording ---

	/** Seconds of motion before the Space press that are included in each take. */
	UPROPERTY(EditAnywhere, Category = "Recording", meta = (ClampMin = "0", ClampMax = "10", ForceUnits = "s"))
	float PreRollSeconds{ 5.0f };

	/** Seconds that keep recording after the stop press, so the follow-through is never cut off. */
	UPROPERTY(EditAnywhere, Category = "Recording", meta = (ClampMin = "0", ClampMax = "5", ForceUnits = "s"))
	float PostRollSeconds{ 1.5f };

	/** Leave empty to use the "Recordings" folder next to the Unreal project folder. */
	UPROPERTY(EditAnywhere, Category = "Recording")
	FString RecordingsFolder;

	/** Stops Space/WASD from moving the camera while recording. */
	UPROPERTY(EditAnywhere, Category = "Recording")
	bool bLockCameraMovement{ true };

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- Key handling ---
	void PollKeys();
	bool WasPressed(const FKey& Key) const;
	void OnSpace();
	void OnLetDown();
	void OnDiscard();
	void OnRating(int32 Rating);

	// --- Shot flow ---
	void StartShot();
	void FinishShot();
	bool SaveTake(const TArray<FMocopiRawFrame>& Frames, const TCHAR* FileSuffix, FSavedAttempt& OutAttempt);
	void WriteAttemptRow(const FSavedAttempt& Attempt, const TCHAR* Type, int32 Rating);
	void AdvanceArrow();

	// --- Session files ---
	void ChooseSessionFolder();
	void EnsureSessionStarted();
	void WriteSessionJson() const;
	void EnsureSkeletonSaved();

	// --- On-screen status ---
	void DrawStatus() const;
	void ShowMessage(const FString& Text, const FColor& Color, float Seconds = 4.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMocopiCaptureSubsystem> Capture;

	EShotRecorderState State{ EShotRecorderState::Idle };

	FString SessionDir;
	bool bSessionStarted{ false };
	bool bSkeletonSaved{ false };

	int32 EndNumber{ 1 };
	int32 ArrowNumber{ 1 };
	int32 AttemptNumber{ 1 };
	int32 RowNumber{ 0 };

	double StartPressTime{ 0.0 };
	double StopPressTime{ 0.0 };

	FSavedAttempt PendingShot;
	FString LastSummary;

	FString Message;
	FColor MessageColor{ FColor::White };
	double MessageUntil{ 0.0 };
};
