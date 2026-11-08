#include "ShotFiles.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ShotFiles
{
	const TCHAR* ShotsCsvHeader{
		TEXT("shot,end,arrow,attempt,type,file,frames,duration_s,stop_press_t,missing_frames,self_rating,")
		TEXT("spot,x_mm,y_mm,score,is_x,notes")
	};

	namespace
	{
		constexpr int32 FixedShotColumns{ 7 }; // frame_id,t,mocopi_time,utc_time,root_x,root_y,root_z

		bool WriteText(const FString& Path, const FString& Text)
		{
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), /*Tree*/ true);
			return FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		}

		double Cell(const TArray<FString>& Cells, int32 Index)
		{
			return Cells.IsValidIndex(Index) ? FCString::Atod(*Cells[Index]) : 0.0;
		}

		float CellF(const TArray<FString>& Cells, int32 Index)
		{
			return static_cast<float>(Cell(Cells, Index));
		}
	}

	bool SaveShotCsv(const FString& Path, const TArray<FMocopiRawFrame>& Frames,
	                 const TArray<FName>& BoneNames, double TimeZero)
	{
		const int32 NumBones{ Frames.Num() > 0 ? Frames[0].LocalRotations.Num() : BoneNames.Num() };

		FString Csv;
		Csv.Reserve((Frames.Num() + 1) * (100 + NumBones * 50));

		Csv += TEXT("frame_id,t,mocopi_time,utc_time,root_x,root_y,root_z");
		for (int32 Bone{ 0 }; Bone < NumBones; ++Bone)
		{
			const FString Name = BoneNames.IsValidIndex(Bone) ? BoneNames[Bone].ToString() : FString::Printf(TEXT("bone%d"), Bone);
			Csv.Appendf(TEXT(",%s_qx,%s_qy,%s_qz,%s_qw"), *Name, *Name, *Name, *Name);
		}
		Csv += TEXT("\n");

		for (const FMocopiRawFrame& Frame : Frames)
		{
			Csv.Appendf(TEXT("%d,%.4f,%.6f,%.6f,%.4f,%.4f,%.4f"),
			            Frame.FrameId,
			            Frame.ReceiveTime - TimeZero,
			            static_cast<double>(Frame.MocopiTimestamp),
			            Frame.UtcTime,
			            static_cast<double>(Frame.RootPosition.X),
			            static_cast<double>(Frame.RootPosition.Y),
			            static_cast<double>(Frame.RootPosition.Z));

			for (const FQuat4f& Rotation : Frame.LocalRotations)
			{
				Csv.Appendf(TEXT(",%.7g,%.7g,%.7g,%.7g"),
				            static_cast<double>(Rotation.X), static_cast<double>(Rotation.Y),
				            static_cast<double>(Rotation.Z), static_cast<double>(Rotation.W));
			}
			Csv += TEXT("\n");
		}

		return WriteText(Path, Csv);
	}

	bool LoadShotCsv(const FString& Path, TArray<FMocopiRawFrame>& OutFrames)
	{
		OutFrames.Reset();

		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path) || Lines.Num() < 2)
		{
			return false;
		}

		TArray<FString> Cells;
		for (int32 LineIndex{ 1 }; LineIndex < Lines.Num(); ++LineIndex) // line 0 is the header
		{
			Lines[LineIndex].ParseIntoArray(Cells, TEXT(","), /*CullEmpty*/ false);
			if (Cells.Num() < FixedShotColumns) continue;
			

			FMocopiRawFrame& Frame{ OutFrames.AddDefaulted_GetRef() };
			Frame.FrameId = FCString::Atoi(*Cells[0]);
			Frame.ReceiveTime = Cell(Cells, 1);
			Frame.MocopiTimestamp = CellF(Cells, 2);
			Frame.UtcTime = Cell(Cells, 3);
			Frame.RootPosition = FVector3f(CellF(Cells, 4), CellF(Cells, 5), CellF(Cells, 6));

			const int32 NumBones{ (Cells.Num() - FixedShotColumns) / 4 };
			Frame.LocalRotations.SetNum(NumBones);
			for (int32 Bone{ 0 }; Bone < NumBones; ++Bone)
			{
				const int32 First{ FixedShotColumns + Bone * 4 };
				Frame.LocalRotations[Bone] = FQuat4f(CellF(Cells, First), CellF(Cells, First + 1),
				                                     CellF(Cells, First + 2), CellF(Cells, First + 3));
			}
		}
		return OutFrames.Num() > 0;
	}

	bool SaveSkeletonCsv(const FString& Path, const FMocopiSkeleton& Skeleton)
	{
		FString Csv{ TEXT("index,name,parent,tx,ty,tz,qx,qy,qz,qw\n") };
		for (int32 Bone{ 0 }; Bone < Skeleton.BoneNames.Num(); ++Bone)
		{
			const FVector3f T{
				Skeleton.DefaultTranslations.IsValidIndex(Bone) ? Skeleton.DefaultTranslations[Bone] : FVector3f::ZeroVector
			};
			const FQuat4f Q{
				Skeleton.DefaultRotations.IsValidIndex(Bone) ? Skeleton.DefaultRotations[Bone] : FQuat4f::Identity
			};
			const int32 Parent{
				Skeleton.ParentIndices.IsValidIndex(Bone) ? Skeleton.ParentIndices[Bone] : -1
			};
			Csv.Appendf(TEXT("%d,%s,%d,%.5f,%.5f,%.5f,%.7g,%.7g,%.7g,%.7g\n"),
			            Bone, *Skeleton.BoneNames[Bone].ToString(), Parent,
			            static_cast<double>(T.X), static_cast<double>(T.Y), static_cast<double>(T.Z),
			            static_cast<double>(Q.X), static_cast<double>(Q.Y), static_cast<double>(Q.Z), static_cast<double>(Q.W));
		}
		return WriteText(Path, Csv);
	}

	bool LoadSkeletonCsv(const FString& Path, FMocopiSkeleton& OutSkeleton)
	{
		OutSkeleton = FMocopiSkeleton();

		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path) || Lines.Num() < 2) return false;

		TArray<FString> Cells;
		for (int32 LineIndex { 1 }; LineIndex < Lines.Num(); ++LineIndex)
		{
			Lines[LineIndex].ParseIntoArray(Cells, TEXT(","), /*CullEmpty*/ false);
			if (Cells.Num() < 10) continue;
			
			OutSkeleton.BoneNames.Add(FName(*Cells[1]));
			OutSkeleton.ParentIndices.Add(FCString::Atoi(*Cells[2]));
			OutSkeleton.DefaultTranslations.Add(FVector3f(CellF(Cells, 3), CellF(Cells, 4), CellF(Cells, 5)));
			OutSkeleton.DefaultRotations.Add(FQuat4f(CellF(Cells, 6), CellF(Cells, 7), CellF(Cells, 8), CellF(Cells, 9)));
		}
		return OutSkeleton.BoneNames.Num() > 0;
	}

	bool AppendCsvLine(const FString& Path, const TCHAR* Header, const FString& Line)
	{
		if (!FPaths::FileExists(Path))
		{
			return WriteText(Path, FString(Header) + TEXT("\n") + Line + TEXT("\n"));
		}
		return FFileHelper::SaveStringToFile(Line + TEXT("\n"), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		                                     &IFileManager::Get(), FILEWRITE_Append);
	}

	int32 CountMissingFrames(const TArray<FMocopiRawFrame>& Frames)
	{
		int32 Missing{ 0 };
		for (int32 Index{ 1 }; Index < Frames.Num(); ++Index)
		{
			const int64 Delta{ 
				static_cast<int64>(Frames[Index].FrameId) - static_cast<int64>(Frames[Index - 1].FrameId) 
			};
			if (Delta > 1 && Delta < 10000)
			{
				Missing += static_cast<int32>(Delta - 1);
			}
		}
		return Missing;
	}

	FString CsvSafe(const FString& Text)
	{
		FString Safe{ Text.Replace(TEXT(","), TEXT(";")).Replace(TEXT("\""), TEXT("'")) };
		Safe.ReplaceInline(TEXT("\r"), TEXT(" "));
		Safe.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Safe;
	}

	FString ResolveRecordingsRoot(const FString& Override)
	{
		return FPaths::ConvertRelativePathToFull(
			Override.IsEmpty() ? FPaths::Combine(FPaths::ProjectDir(), TEXT("../Recordings")) : Override);
	}
}
