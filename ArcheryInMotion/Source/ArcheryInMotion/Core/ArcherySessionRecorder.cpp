#include "ArcherySessionRecorder.h"

#include "MocopiCaptureSubsystem.h"
#include "MocopiRawData.h"
#include "ShotFiles.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString EnumName(EArcheryBowType Value)
	{
		return StaticEnum<EArcheryBowType>()->GetNameStringByValue(static_cast<int64>(Value));
	}

	FString EnumName(EArcheryDrawHand Value)
	{
		return StaticEnum<EArcheryDrawHand>()->GetNameStringByValue(static_cast<int64>(Value));
	}

	/** Keys for the on-screen lines */
	constexpr int32 StatusKeyBase{ 0x4A1700 };
}

AArcherySessionRecorder::AArcherySessionRecorder()
{
	PrimaryActorTick.bCanEverTick = true;
}

#pragma region Lifetime

void AArcherySessionRecorder::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance{ GetGameInstance() })
	{
		Capture = GameInstance->GetSubsystem<UMocopiCaptureSubsystem>();
	}

	if (bLockCameraMovement)
	{
		if (APlayerController* PlayerController{ GetWorld()->GetFirstPlayerController() })
		{
			PlayerController->SetIgnoreMoveInput(true);
		}
	}

	ChooseSessionFolder();
	ShowMessage(FString::Printf(TEXT("Recordings will be saved to %s"), *SessionDir), FColor::Cyan, 8.0f);
}

void AArcherySessionRecorder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Capture && (State == EShotRecorderState::Recording || State == EShotRecorderState::PostRoll))
	{
		if (State == EShotRecorderState::Recording)
		{
			StopPressTime = FPlatformTime::Seconds();
		}
		const TArray<FMocopiRawFrame> Frames{ Capture->StopTake() };
		FSavedAttempt Attempt;
		if (SaveTake(Frames, TEXT("_interrupted"), Attempt))
		{
			WriteAttemptRow(Attempt, TEXT("interrupted"), 0);
		}
	}
	else if (State == EShotRecorderState::AwaitRating)
	{
		WriteAttemptRow(PendingShot, TEXT("shot"), 0);
	}

	State = EShotRecorderState::Idle;
	Super::EndPlay(EndPlayReason);
}

void AArcherySessionRecorder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PollKeys();

	if (State == EShotRecorderState::PostRoll && FPlatformTime::Seconds() - StopPressTime >= PostRollSeconds)
	{
		FinishShot();
	}

	DrawStatus();
}

#pragma endregion

#pragma region Keys

bool AArcherySessionRecorder::WasPressed(const FKey& Key) const
{
	UWorld* World{ GetWorld() };
	APlayerController* PlayerController{ World ? World->GetFirstPlayerController() : nullptr };
	return PlayerController != nullptr && PlayerController->WasInputKeyJustPressed(Key);
}

void AArcherySessionRecorder::PollKeys()
{
	if (WasPressed(EKeys::SpaceBar))
	{
		OnSpace();
		return;
	}
	if (WasPressed(EKeys::L))
	{
		OnLetDown();
		return;
	}
	if (WasPressed(EKeys::BackSpace))
	{
		OnDiscard();
		return;
	}

	const FKey TopRow[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	const FKey NumPad[] = { EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive };
	for (int32 Rating = 0; Rating <= 5; ++Rating)
	{
		if (WasPressed(TopRow[Rating]) || WasPressed(NumPad[Rating]))
		{
			OnRating(Rating);
			return;
		}
	}
}

void AArcherySessionRecorder::OnSpace()
{
	switch (State)
	{
	case EShotRecorderState::Idle:
		StartShot();
		break;

	case EShotRecorderState::Recording:
		// Keep recording the follow-through for PostRollSeconds, then save (see Tick).
		StopPressTime = FPlatformTime::Seconds();
		State = EShotRecorderState::PostRoll;
		break;

	case EShotRecorderState::PostRoll:
		break; // already stopping

	case EShotRecorderState::AwaitRating:
		// Space before rating = no rating, and straight on to the next shot.
		OnRating(0);
		StartShot();
		break;
	}
}

void AArcherySessionRecorder::OnLetDown()
{
	if (State == EShotRecorderState::Recording || State == EShotRecorderState::PostRoll)
	{
		if (State == EShotRecorderState::Recording)
		{
			StopPressTime = FPlatformTime::Seconds();
		}
		const TArray<FMocopiRawFrame> Frames{ Capture->StopTake() };
		FSavedAttempt Attempt;
		if (SaveTake(Frames, TEXT("_letdown"), Attempt))
		{
			WriteAttemptRow(Attempt, TEXT("letdown"), 0);
			++AttemptNumber;
			ShowMessage(TEXT("Let-down saved. Same arrow again: press Space when ready."), FColor::Orange);
		}
		State = EShotRecorderState::Idle;
	}
	else if (State == EShotRecorderState::AwaitRating)
	{
		// Stopped with Space, but it was actually a let-down.
		WriteAttemptRow(PendingShot, TEXT("letdown"), 0);
		++AttemptNumber;
		ShowMessage(TEXT("Marked as let-down. Same arrow again: press Space when ready."), FColor::Orange);
		State = EShotRecorderState::Idle;
	}
}

void AArcherySessionRecorder::OnDiscard()
{
	if (State == EShotRecorderState::Recording || State == EShotRecorderState::PostRoll)
	{
		Capture->StopTake();
		ShowMessage(TEXT("Take discarded."), FColor::Orange);
		State = EShotRecorderState::Idle;
	}
	else if (State == EShotRecorderState::AwaitRating)
	{
		IFileManager::Get().Delete(*FPaths::Combine(SessionDir, PendingShot.RelativeFile));
		ShowMessage(TEXT("Shot thrown away. Same arrow again: press Space when ready."), FColor::Orange);
		State = EShotRecorderState::Idle;
	}
}

void AArcherySessionRecorder::OnRating(int32 Rating)
{
	if (State != EShotRecorderState::AwaitRating) return;

	WriteAttemptRow(PendingShot, TEXT("shot"), Rating);
	AdvanceArrow();
	State = EShotRecorderState::Idle;
}

#pragma endregion

#pragma region ShotFlow

void AArcherySessionRecorder::StartShot()
{
	if (!Capture)
	{
		ShowMessage(TEXT("Mocopi capture is not running - cannot record."), FColor::Red);
		return;
	}

	StartPressTime = FPlatformTime::Seconds();
	Capture->StartTake(PreRollSeconds);
	State = EShotRecorderState::Recording;
}

void AArcherySessionRecorder::FinishShot()
{
	const TArray<FMocopiRawFrame> Frames{ Capture->StopTake() };
	State = SaveTake(Frames, TEXT(""), PendingShot) ? EShotRecorderState::AwaitRating : EShotRecorderState::Idle;
}

bool AArcherySessionRecorder::SaveTake(const TArray<FMocopiRawFrame>& Frames, const TCHAR* FileSuffix, FSavedAttempt& OutAttempt)
{
	if (Frames.Num() == 0)
	{
		ShowMessage(TEXT("Nothing saved: no mocopi frames arrived. Is the phone app streaming to this PC?"), FColor::Red, 8.0f);
		return false;
	}

	EnsureSessionStarted();
	EnsureSkeletonSaved();

	OutAttempt = FSavedAttempt();
	OutAttempt.RelativeFile = FString::Printf(TEXT("shots/E%02d_A%d_T%d%s.csv"), EndNumber, ArrowNumber, AttemptNumber, FileSuffix);

	FMocopiSkeleton Skeleton;
	FMocopiRawData::GetSkeleton(Skeleton); // only used for column names; files are still fine without it

	const FString FullPath{ FPaths::Combine(SessionDir, OutAttempt.RelativeFile) };
	if (!ShotFiles::SaveShotCsv(FullPath, Frames, Skeleton.BoneNames, StartPressTime))
	{
		ShowMessage(FString::Printf(TEXT("Could not write %s"), *FullPath), FColor::Red, 8.0f);
		return false;
	}

	OutAttempt.Frames = Frames.Num();
	OutAttempt.DurationSeconds = Frames.Last().ReceiveTime - Frames[0].ReceiveTime;
	OutAttempt.StopPressT = StopPressTime - StartPressTime;
	OutAttempt.MissingFrames = ShotFiles::CountMissingFrames(Frames);
	return true;
}

void AArcherySessionRecorder::WriteAttemptRow(const FSavedAttempt& Attempt, const TCHAR* Type, int32 Rating)
{
	++RowNumber;
	const FString RatingText{ Rating > 0 ? FString::FromInt(Rating) : FString() };

	// The last six columns (spot, x_mm, y_mm, score, is_x, notes) are filled in later.
	const FString Row{
		FString::Printf(TEXT("%d,%d,%d,%d,%s,%s,%d,%.3f,%.3f,%d,%s,,,,,,"),
						 RowNumber, EndNumber, ArrowNumber, AttemptNumber, Type,
						 *Attempt.RelativeFile, Attempt.Frames, Attempt.DurationSeconds,
						 Attempt.StopPressT, Attempt.MissingFrames, *RatingText)
	};

	ShotFiles::AppendCsvLine(FPaths::Combine(SessionDir, TEXT("shots.csv")), ShotFiles::ShotsCsvHeader, Row);

	LastSummary = FString::Printf(TEXT("Last: End %d Arrow %d (attempt %d) %s - %d frames, %.1f s, %d missing%s"),
	                              EndNumber, ArrowNumber, AttemptNumber, Type, Attempt.Frames, Attempt.DurationSeconds,
	                              Attempt.MissingFrames,
	                              Rating > 0 ? *FString::Printf(TEXT(", rated %d"), Rating) : TEXT(""));
}

void AArcherySessionRecorder::AdvanceArrow()
{
	AttemptNumber = 1;
	if (++ArrowNumber > ArrowsPerEnd)
	{
		ArrowNumber = 1;
		++EndNumber;
		ShowMessage(FString::Printf(TEXT("End %d complete - go and pull your arrows."), EndNumber - 1), FColor::Green, 10.0f);
	}
}

#pragma endregion

#pragma region Session files

void AArcherySessionRecorder::ChooseSessionFolder()
{
	const FString Root{ ShotFiles::ResolveRecordingsRoot(RecordingsFolder) };

	const FString FolderName{
		FPaths::MakeValidFileName(FString::Printf(TEXT("%s_%s_%s"),
			*FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S")), *ArcherName, *EnumName(BowType)))
	};

	SessionDir = FPaths::Combine(Root, FolderName);
}

void AArcherySessionRecorder::EnsureSessionStarted()
{
	if (bSessionStarted) return;
	
	IFileManager::Get().MakeDirectory(*FPaths::Combine(SessionDir, TEXT("shots")), /*Tree*/ true);
	WriteSessionJson();
	bSessionStarted = true;
}

void AArcherySessionRecorder::WriteSessionJson() const
{
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("archer"), ArcherName);
	Json->SetStringField(TEXT("bow_type"), EnumName(BowType));
	Json->SetStringField(TEXT("draw_hand"), EnumName(DrawHand));
	Json->SetStringField(TEXT("target_face"), TargetFace);
	Json->SetNumberField(TEXT("distance_m"), DistanceMeters);
	Json->SetNumberField(TEXT("arrow_diameter_mm"), ArrowDiameterMm);
	Json->SetNumberField(TEXT("arrows_per_end"), ArrowsPerEnd);
	Json->SetStringField(TEXT("tracking_mode"), TrackingMode);
	Json->SetStringField(TEXT("recorded_data"), TEXT("raw mocopi frames, captured before the Live Link plugin's pose smoothing"));
	Json->SetNumberField(TEXT("pre_roll_s"), PreRollSeconds);
	Json->SetNumberField(TEXT("post_roll_s"), PostRollSeconds);
	Json->SetStringField(TEXT("started_at"), FDateTime::Now().ToIso8601());
	Json->SetStringField(TEXT("notes"), Notes);

	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Json, Writer);

	FFileHelper::SaveStringToFile(Text, *FPaths::Combine(SessionDir, TEXT("session.json")),
	                              FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void AArcherySessionRecorder::EnsureSkeletonSaved()
{
	if (bSkeletonSaved || !bSessionStarted) return;
	

	FMocopiSkeleton Skeleton;
	if (FMocopiRawData::GetSkeleton(Skeleton))
	{
		bSkeletonSaved = ShotFiles::SaveSkeletonCsv(FPaths::Combine(SessionDir, TEXT("skeleton.csv")), Skeleton);
	}
}

#pragma endregion

#pragma region OnScreen

void AArcherySessionRecorder::ShowMessage(const FString& Text, const FColor& Color, float Seconds)
{
	Message = Text;
	MessageColor = Color;
	MessageUntil = FPlatformTime::Seconds() + Seconds;
	UE_LOG(LogTemp, Display, TEXT("[ArcheryInMotion] %s"), *Text);
}

void AArcherySessionRecorder::DrawStatus() const
{
	if (GEngine == nullptr) return;
	
	const double Now{ FPlatformTime::Seconds() };
	int32 Key{ StatusKeyBase };
	const FVector2D Scale(1.4, 1.4);
	
	auto Line = [&Key, &Scale](const FString& Text, const FColor& Color)
	{
		GEngine->AddOnScreenDebugMessage(Key++, 0.5f, Color, Text.IsEmpty() ? FString(TEXT(" ")) : Text, /*bNewerOnTop*/ false, Scale);
	};

	Line(FString::Printf(TEXT("ARCHERY IN MOTION - %s, %s"), *ArcherName, *EnumName(BowType)), FColor::Cyan);

	// Stream health
	if (!Capture)
	{
		Line(TEXT("mocopi: capture not running"), FColor::Red);
	}
	else
	{
		const FMocopiStreamStats Stats{ Capture->GetStreamStats() };
		if (Stats.SecondsSinceLastFrame < 0.0)
		{
			Line(TEXT("mocopi: NO DATA yet - start sending from the mocopi app (port 12351)"), FColor::Red);
		}
		else if (Stats.SecondsSinceLastFrame > 1.0)
		{
			Line(FString::Printf(TEXT("mocopi: NO DATA for %.0f s - check phone app / Wi-Fi"), Stats.SecondsSinceLastFrame), FColor::Red);
		}
		else
		{
			Line(FString::Printf(TEXT("mocopi: receiving %.0f fps | %llu frames missing so far"),
			                     Stats.FramesPerSecond, static_cast<unsigned long long>(Stats.FramesMissing)), FColor::Green);
		}
	}

	Line(FString::Printf(TEXT("End %d    Arrow %d / %d    Attempt %d"),
			EndNumber, ArrowNumber, ArrowsPerEnd, AttemptNumber), FColor::White);

	switch (State)
	{
	case EShotRecorderState::Idle:
		Line(TEXT("READY - not recording"), FColor::Yellow);
		Line(TEXT("SPACE = start recording the shot"), FColor::Silver);
		break;
	case EShotRecorderState::Recording:
		Line(FString::Printf(TEXT("[REC] RECORDING  %.1f s  (%d frames)"), Now - StartPressTime,
		                     Capture ? Capture->GetTakeFrameCount() : 0), FColor::Red);
		Line(TEXT("SPACE = stop after the arrow lands    L = let-down    BACKSPACE = discard"), FColor::Silver);
		break;
	case EShotRecorderState::PostRoll:
		Line(TEXT("[REC] recording follow-through..."), FColor::Orange);
		Line(TEXT("L = let-down    BACKSPACE = discard"), FColor::Silver);
		break;
	case EShotRecorderState::AwaitRating:
		Line(TEXT("Shot saved - how did it feel? Press 1-5"), FColor::Yellow);
		Line(TEXT("0 = no rating    L = it was a let-down    BACKSPACE = throw away    SPACE = skip rating + next shot"), FColor::Silver);
		break;
	}

	Line(LastSummary, FColor(180, 180, 180));
	Line(Now < MessageUntil ? Message : FString(), MessageColor);
}

#pragma endregion