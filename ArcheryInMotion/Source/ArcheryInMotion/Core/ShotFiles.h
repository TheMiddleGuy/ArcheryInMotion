#pragma once

#include "CoreMinimal.h"
#include "MocopiRawData.h"

namespace ShotFiles
{
	// Reading and writing recorded shots as CSV files.
	//
	// Session folder layout:
	//   Recordings/<date>_<archer>_<bow>/
	//     session.json      who/what/where, written when the session starts
	//     skeleton.csv      bone names, parents and bone offsets (body size) for this session
	//     shots.csv         one row per attempt (shot / let-down / interrupted)
	//     shots/E01_A1_T1.csv  raw motion of one attempt, one row per mocopi frame

	
	/** Header of shots.csv. Arrow position columns stay empty until target marking exists. */
	extern const TCHAR* ShotsCsvHeader;

	/**
	 * Writes one attempt. Column "t" is seconds relative to TimeZero (the Space press),
	 * so pre-roll frames have negative t.
	 */
	bool SaveShotCsv(const FString& Path, const TArray<FMocopiRawFrame>& Frames,
	                 const TArray<FName>& BoneNames, double TimeZero);

	/** Reads a file written by SaveShotCsv. ReceiveTime is filled with the "t" column. */
	bool LoadShotCsv(const FString& Path, TArray<FMocopiRawFrame>& OutFrames);

	bool SaveSkeletonCsv(const FString& Path, const FMocopiSkeleton& Skeleton);
	bool LoadSkeletonCsv(const FString& Path, FMocopiSkeleton& OutSkeleton);

	/** Appends a line to a CSV file, writing Header first if the file doesn't exist yet. */
	bool AppendCsvLine(const FString& Path, const TCHAR* Header, const FString& Line);

	/** Number of frames lost inside the take (gaps in the mocopi frame counter). */
	int32 CountMissingFrames(const TArray<FMocopiRawFrame>& Frames);

	/** Makes free text safe for one CSV cell. */
	FString CsvSafe(const FString& Text);

	/** Absolute path of the Recordings folder. Empty Override = "Recordings" next to the Unreal project folder. */
	FString ResolveRecordingsRoot(const FString& Override);
}
