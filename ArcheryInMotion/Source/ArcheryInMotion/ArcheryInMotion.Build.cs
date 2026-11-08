// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ArcheryInMotion : ModuleRules
{
	public ArcheryInMotion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"MocopiLiveLink",    // raw mocopi frames + creating the Live Link source (plugin in Plugins/)
			"LiveLinkInterface", // replay: our own Live Link source that plays recorded shots
			"Json",              // session.json
		});
	}
}
