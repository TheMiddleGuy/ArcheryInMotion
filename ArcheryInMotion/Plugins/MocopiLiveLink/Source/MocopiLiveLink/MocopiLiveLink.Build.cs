////
// Copyright (C) 2026 Sony Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////

using UnrealBuildTool;
using System.IO;

public class MocopiLiveLink : ModuleRules
{
	public MocopiLiveLink(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		// Enable for debugging
		// OptimizeCode = CodeOptimization.Never;


		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"LiveLinkInterface",
				"LiveLink",
				"LiveLinkAnimationCore",

				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Sockets",
				"Networking",
				"Projects",
				
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// Adding MMF library to project packaging
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibraryPath = Path.Combine(ModuleDirectory, "../mocopiMotionSerializer/windows/x64/mocopi_motion_serializer.dll");
			RuntimeDependencies.Add(LibraryPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string BundlePath = Path.Combine(ModuleDirectory, "../mocopiMotionSerializer/mac/mocopi_motion_serializer.bundle");

			string LibraryPath = Path.Combine(BundlePath, "Contents/MacOS/mocopi_motion_serializer");
			RuntimeDependencies.Add(LibraryPath);

			string CodeSignaturePath = Path.Combine(BundlePath, "Contents/_CodeSignature/CodeResources");
			RuntimeDependencies.Add(CodeSignaturePath);

			string InfoPlistPath = Path.Combine(BundlePath, "Contents/Info.plist");
			RuntimeDependencies.Add(InfoPlistPath);
		}

	}
}
