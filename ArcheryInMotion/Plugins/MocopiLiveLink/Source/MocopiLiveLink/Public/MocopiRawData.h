// [ArcheryInMotion] Addition to Sony's mocopi plugin (Apache 2.0).
//
// Gives other modules access to mocopi frames exactly as they arrive from the network,
// BEFORE the plugin's pose smoothing and before Live Link interpolates them. This is the
// data the archery recorder saves.

#pragma once

#include "CoreMinimal.h"

/** One mocopi frame as decoded from the network, before any smoothing. */
struct FMocopiRawFrame
{
	/** Frame counter from the mocopi app. Gaps mean frames were lost on the way. */
	int32 FrameId = 0;

	/** Timestamp from the mocopi app (seconds, sender clock). */
	float MocopiTimestamp = 0.0f;

	/** UTC time from the mocopi app (seconds since 1970). */
	double UtcTime = 0.0;

	/** FPlatformTime::Seconds() on this PC when the frame was decoded. */
	double ReceiveTime = 0.0;

	/** Position of the root (hips) bone in centimetres, as the plugin passes it to Live Link. */
	FVector3f RootPosition = FVector3f::ZeroVector;

	/** Local rotation of every bone relative to its parent, in the plugin's bone order. */
	TArray<FQuat4f> LocalRotations;
};

/** Bone layout of the mocopi skeleton. Sent by the app once when streaming starts. */
struct FMocopiSkeleton
{
	TArray<FName> BoneNames;
	TArray<int32> ParentIndices;           // -1 for the root
	TArray<FVector3f> DefaultTranslations; // bone offsets in cm (depend on the archer's body size)
	TArray<FQuat4f> DefaultRotations;
};

/** Implement this to receive raw frames. */
class IMocopiRawFrameListener
{
public:
	virtual ~IMocopiRawFrameListener() = default;

	/** Called on the plugin's network thread for every accepted frame. Must be quick and thread-safe. */
	virtual void OnMocopiRawFrame(const FMocopiRawFrame& Frame) = 0;
};

class MOCOPILIVELINK_API FMocopiRawData
{
public:
	/** Start/stop receiving raw frames. After RemoveListener returns, no more calls arrive. */
	static void AddListener(IMocopiRawFrameListener* Listener);
	static void RemoveListener(IMocopiRawFrameListener* Listener);
	static bool HasListeners();

	/** Latest skeleton received from the app. Returns false if none has arrived yet. */
	static bool GetSkeleton(FMocopiSkeleton& OutSkeleton);

	/**
	 * Returns the existing mocopi Live Link source, or creates one listening on Port.
	 * This is the code version of clicking "+ Add Source" in the Live Link window.
	 * Must be called on the game thread. Returns an invalid FGuid if Live Link is unavailable.
	 */
	static FGuid FindOrCreateLiveLinkSource(uint16 Port = 12351, FName SubjectName = FName(TEXT("MocopiSkeleton")));

	// Called by the plugin itself.
	static void PublishFrame(const FMocopiRawFrame& Frame);
	static void PublishSkeleton(const FMocopiSkeleton& Skeleton);
};
