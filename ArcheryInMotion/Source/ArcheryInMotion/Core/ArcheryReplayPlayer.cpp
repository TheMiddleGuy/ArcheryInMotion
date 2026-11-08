#include "ArcheryReplayPlayer.h"

#include "ArcheryReplaySource.h"
#include "ShotFiles.h"

#include "Algo/BinarySearch.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ILiveLinkClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/** Keys for the on-screen lines (different from the recorder's, so both can be in the level). */
	constexpr int32 ReplayStatusKeyBase{ 0x4A1800 };

	constexpr float SpeedSteps[]{ 0.1f, 0.25f, 0.5f, 1.0f, 2.0f };

	ILiveLinkClient* GetLiveLinkClient()
	{
		IModularFeatures& Features{ IModularFeatures::Get() };
		if (!Features.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName)) return nullptr;
		return &Features.GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}
}

AArcheryReplayPlayer::AArcheryReplayPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
}

#pragma region Lifetime

void AArcheryReplayPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (bLockCameraMovement)
	{
		if (APlayerController* PlayerController{ GetWorld()->GetFirstPlayerController() })
		{
			PlayerController->SetIgnoreMoveInput(true);
		}
	}

	if (ILiveLinkClient* Client{ GetLiveLinkClient() })
	{
		Source = MakeShared<FArcheryReplaySource>();
		SourceGuid = Client->AddSource(Source);
	}
	else
	{
		ShowMessage(TEXT("Live Link is not available - cannot replay."), FColor::Red, 10.0f);
	}

	RefreshSessions();
	if (SessionDirs.Num() == 0)
	{
		ShowMessage(FString::Printf(TEXT("No recorded sessions found in %s"),
		                            *ShotFiles::ResolveRecordingsRoot(RecordingsFolder)), FColor::Orange, 10.0f);
		return;
	}

	int32 StartIndex{ SessionDirs.Num() - 1 }; // newest
	if (!StartSession.IsEmpty())
	{
		for (int32 Index{ 0 }; Index < SessionDirs.Num(); ++Index)
		{
			if (FPaths::GetCleanFilename(SessionDirs[Index]) == StartSession) StartIndex = Index;
		}
	}
	OpenSession(StartIndex);
}

void AArcheryReplayPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Source.IsValid())
	{
		if (ILiveLinkClient* Client{ GetLiveLinkClient() })
		{
			Client->RemoveSource(SourceGuid);
		}
		Source.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AArcheryReplayPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PollKeys();

	if (bPlaying && FrameTimes.Num() > 1)
	{
		PlayTime += DeltaSeconds * PlaybackSpeed;
		if (PlayTime > FrameTimes.Last()) PlayTime = 0.0; // loop

		FrameIndex = FMath::Max(0, Algo::UpperBound(FrameTimes, PlayTime) - 1);
	}

	// Push every tick, also while paused: Live Link drops a subject that stops receiving frames.
	PushCurrentFrame();
	DrawStatus();
}

#pragma endregion

#pragma region Keys

bool AArcheryReplayPlayer::WasPressed(const FKey& Key) const
{
	UWorld* World{ GetWorld() };
	APlayerController* PlayerController{ World ? World->GetFirstPlayerController() : nullptr };
	return PlayerController != nullptr && PlayerController->WasInputKeyJustPressed(Key);
}

void AArcheryReplayPlayer::PollKeys()
{
	if (WasPressed(EKeys::Right))
	{
		RefreshAttempts();
		if (AttemptIndex + 1 < Attempts.Num()) OpenAttempt(AttemptIndex + 1);
		else ShowMessage(TEXT("This is the last shot of the session."), FColor::Orange, 2.0f);
	}
	else if (WasPressed(EKeys::Left))
	{
		RefreshAttempts();
		if (AttemptIndex > 0) OpenAttempt(AttemptIndex - 1);
		else ShowMessage(TEXT("This is the first shot of the session."), FColor::Orange, 2.0f);
	}
	else if (WasPressed(EKeys::Up) || WasPressed(EKeys::Down))
	{
		const int32 Direction{ WasPressed(EKeys::Down) ? 1 : -1 };
		const FString Current{ SessionDirs.IsValidIndex(SessionIndex) ? SessionDirs[SessionIndex] : FString() };

		RefreshSessions(); // pick up sessions recorded since Play started
		const int32 CurrentIndex{ SessionDirs.IndexOfByKey(Current) };
		const int32 NewIndex{ CurrentIndex == INDEX_NONE ? SessionDirs.Num() - 1 : CurrentIndex + Direction };

		if (SessionDirs.IsValidIndex(NewIndex)) OpenSession(NewIndex);
		else ShowMessage(TEXT("No more sessions in that direction."), FColor::Orange, 2.0f);
	}
	else if (WasPressed(EKeys::P))
	{
		bPlaying = !bPlaying;
	}
	else if (WasPressed(EKeys::Comma))
	{
		bPlaying = false;
		SetFrame(FrameIndex - 1);
	}
	else if (WasPressed(EKeys::Period))
	{
		bPlaying = false;
		SetFrame(FrameIndex + 1);
	}
	else if (WasPressed(EKeys::Hyphen) || WasPressed(EKeys::Subtract))
	{
		ChangeSpeed(-1);
	}
	else if (WasPressed(EKeys::Equals) || WasPressed(EKeys::Add))
	{
		ChangeSpeed(+1);
	}
	else if (WasPressed(EKeys::R))
	{
		SetFrame(0);
	}
	else if (WasPressed(EKeys::T))
	{
		// First frame at or after the Space press (t = 0). ReceiveTime holds the "t" column.
		for (int32 Index{ 0 }; Index < Frames.Num(); ++Index)
		{
			if (Frames[Index].ReceiveTime >= 0.0)
			{
				SetFrame(Index);
				break;
			}
		}
	}
}

#pragma endregion

#pragma region Browsing

void AArcheryReplayPlayer::RefreshSessions()
{
	const FString Root{ ShotFiles::ResolveRecordingsRoot(RecordingsFolder) };

	TArray<FString> Names;
	IFileManager::Get().FindFiles(Names, *FPaths::Combine(Root, TEXT("*")), /*Files*/ false, /*Directories*/ true);
	Names.Sort(); // folder names start with the date, so this is oldest first

	SessionDirs.Reset();
	for (const FString& Name : Names)
	{
		const FString Dir{ FPaths::Combine(Root, Name) };
		if (FPaths::FileExists(FPaths::Combine(Dir, TEXT("shots.csv")))) SessionDirs.Add(Dir);
	}
}

void AArcheryReplayPlayer::OpenSession(int32 Index)
{
	if (!SessionDirs.IsValidIndex(Index)) return;
	SessionIndex = Index;

	// The skeleton holds this archer's bone lengths; fall back to the live one if the file is missing.
	bHasSkeleton = ShotFiles::LoadSkeletonCsv(FPaths::Combine(SessionDirs[Index], TEXT("skeleton.csv")), Skeleton)
	               || FMocopiRawData::GetSkeleton(Skeleton);

	if (bHasSkeleton && Source.IsValid())
	{
		Source->PushSkeleton(SubjectName, Skeleton);
	}
	else if (!bHasSkeleton)
	{
		ShowMessage(TEXT("This session has no skeleton.csv - cannot replay it."), FColor::Red, 6.0f);
	}

	AttemptIndex = INDEX_NONE;
	RefreshAttempts();
	OpenAttempt(Attempts.Num() - 1); // newest shot first
}

void AArcheryReplayPlayer::RefreshAttempts()
{
	Attempts.Reset();
	if (!SessionDirs.IsValidIndex(SessionIndex)) return;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *FPaths::Combine(SessionDirs[SessionIndex], TEXT("shots.csv")));

	// Columns: shot,end,arrow,attempt,type,file,frames,duration_s,stop_press_t,missing_frames,self_rating,...
	TArray<FString> Cells;
	for (int32 LineIndex{ 1 }; LineIndex < Lines.Num(); ++LineIndex)
	{
		Lines[LineIndex].ParseIntoArray(Cells, TEXT(","), /*CullEmpty*/ false);
		if (Cells.Num() < 11) continue;

		FReplayAttempt& Row{ Attempts.AddDefaulted_GetRef() };
		Row.End = FCString::Atoi(*Cells[1]);
		Row.Arrow = FCString::Atoi(*Cells[2]);
		Row.Attempt = FCString::Atoi(*Cells[3]);
		Row.Type = Cells[4];
		Row.File = Cells[5];
		Row.StopPressT = FCString::Atod(*Cells[8]);
		Row.Rating = Cells[10];
	}
}

void AArcheryReplayPlayer::OpenAttempt(int32 Index)
{
	Frames.Reset();
	FrameTimes.Reset();
	FrameIndex = 0;
	PlayTime = 0.0;

	if (Attempts.Num() == 0)
	{
		AttemptIndex = INDEX_NONE;
		return;
	}

	AttemptIndex = FMath::Clamp(Index, 0, Attempts.Num() - 1);
	const FString Path{ FPaths::Combine(SessionDirs[SessionIndex], Attempts[AttemptIndex].File) };
	if (!ShotFiles::LoadShotCsv(Path, Frames))
	{
		ShowMessage(FString::Printf(TEXT("Could not read %s"), *Path), FColor::Red, 6.0f);
		return;
	}

	// Timing from mocopi's own clock (exactly 60 fps). If it jumps, fall back to 1/60 s steps.
	FrameTimes.SetNum(Frames.Num());
	FrameTimes[0] = 0.0;
	for (int32 FrameNumber{ 1 }; FrameNumber < Frames.Num(); ++FrameNumber)
	{
		const double Step{ static_cast<double>(Frames[FrameNumber].MocopiTimestamp - Frames[FrameNumber - 1].MocopiTimestamp) };
		FrameTimes[FrameNumber] = FrameTimes[FrameNumber - 1] + ((Step > 0.0 && Step < 0.5) ? Step : 1.0 / 60.0);
	}

	bPlaying = true;
}

#pragma endregion

#pragma region Playback

void AArcheryReplayPlayer::SetFrame(int32 Index)
{
	if (FrameTimes.Num() == 0) return;

	FrameIndex = FMath::Clamp(Index, 0, FrameTimes.Num() - 1);
	PlayTime = FrameTimes[FrameIndex];
}

void AArcheryReplayPlayer::ChangeSpeed(int32 Direction)
{
	// Find the step closest to the current speed, then move one step.
	constexpr int32 NumSteps{ static_cast<int32>(UE_ARRAY_COUNT(SpeedSteps)) };

	int32 Closest{ 0 };
	for (int32 Index{ 1 }; Index < NumSteps; ++Index)
	{
		if (FMath::Abs(SpeedSteps[Index] - PlaybackSpeed) < FMath::Abs(SpeedSteps[Closest] - PlaybackSpeed)) Closest = Index;
	}
	PlaybackSpeed = SpeedSteps[FMath::Clamp(Closest + Direction, 0, NumSteps - 1)];
}

void AArcheryReplayPlayer::PushCurrentFrame()
{
	if (Source.IsValid() && bHasSkeleton && Frames.IsValidIndex(FrameIndex))
	{
		Source->PushPose(SubjectName, Skeleton, Frames[FrameIndex]);
	}
}

#pragma endregion

#pragma region On-screen status

void AArcheryReplayPlayer::ShowMessage(const FString& Text, const FColor& Color, float Seconds)
{
	Message = Text;
	MessageColor = Color;
	MessageUntil = FPlatformTime::Seconds() + Seconds;
	UE_LOG(LogTemp, Display, TEXT("[ArcheryInMotion Replay] %s"), *Text);
}

void AArcheryReplayPlayer::DrawStatus() const
{
	if (GEngine == nullptr) return;

	int32 Key{ ReplayStatusKeyBase };
	const FVector2D Scale{ 1.4, 1.4 };

	// Always draw the same number of lines, so they keep their order on screen.
	auto Line = [&Key, &Scale](const FString& Text, const FColor& Color)
	{
		GEngine->AddOnScreenDebugMessage(Key++, 0.5f, Color, Text.IsEmpty() ? FString(TEXT(" ")) : Text, /*bNewerOnTop*/ false, Scale);
	};

	Line(TEXT(" "), FColor::White); // gap below the recorder's lines

	const FString SessionName{ SessionDirs.IsValidIndex(SessionIndex) ? FPaths::GetCleanFilename(SessionDirs[SessionIndex]) : FString(TEXT("-")) };
	Line(FString::Printf(TEXT("REPLAY - %s  (session %d / %d)"), *SessionName, SessionIndex + 1, SessionDirs.Num()), FColor::Cyan);

	if (Attempts.IsValidIndex(AttemptIndex))
	{
		const FReplayAttempt& Row{ Attempts[AttemptIndex] };
		Line(FString::Printf(TEXT("Shot %d / %d:  End %d  Arrow %d  Attempt %d  [%s]  rating %s"),
		                     AttemptIndex + 1, Attempts.Num(), Row.End, Row.Arrow, Row.Attempt, *Row.Type,
		                     Row.Rating.IsEmpty() ? TEXT("-") : *Row.Rating), FColor::White);
	}
	else
	{
		Line(TEXT("No shots in this session"), FColor::Orange);
	}

	if (Frames.IsValidIndex(FrameIndex))
	{
		const double StopPressT{ Attempts.IsValidIndex(AttemptIndex) ? Attempts[AttemptIndex].StopPressT : 0.0 };
		Line(FString::Printf(TEXT("%s  frame %d / %d   t = %+.2f s   speed %.2gx"),
		                     bPlaying ? TEXT("[PLAYING]") : TEXT("[PAUSED]"), FrameIndex + 1, Frames.Num(),
		                     Frames[FrameIndex].ReceiveTime, PlaybackSpeed), bPlaying ? FColor::Green : FColor::Yellow);

		// Timeline: | = Space pressed (start), | = Space pressed (stop), # = current frame
		constexpr int32 Width{ 60 };
		const double First{ Frames[0].ReceiveTime };
		const double Length{ FMath::Max(Frames.Last().ReceiveTime - First, 0.001) };
		auto Column = [First, Length](double T) { return FMath::Clamp(FMath::RoundToInt((T - First) / Length * (Width - 1)), 0, Width - 1); };

		FString Bar{ FString::ChrN(Width, TEXT('-')) };
		Bar[Column(0.0)] = TEXT('|');
		Bar[Column(StopPressT)] = TEXT('|');
		Bar[Column(Frames[FrameIndex].ReceiveTime)] = TEXT('#');
		Line(FString::Printf(TEXT("[%s]   (| = Space presses)"), *Bar), FColor::White);
	}
	else
	{
		Line(TEXT(" "), FColor::White);
		Line(TEXT(" "), FColor::White);
	}

	Line(TEXT("LEFT/RIGHT shot   UP/DOWN session   P play/pause   , . frame step   - = speed   R restart   T jump to Space press"),
	     FColor::Silver);

	Line(FPlatformTime::Seconds() < MessageUntil ? Message : FString(), MessageColor);
}

#pragma endregion
