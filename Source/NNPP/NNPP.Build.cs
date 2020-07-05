// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class NNPP : ModuleRules
{
	public NNPP(ReadOnlyTargetRules Target) : base(Target)
	{
		string ModuleRootDirectory = Path.Combine(ModuleDirectory, "../..");
		string ThirdPartyInclude = "ThirdParty/Include";

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;
		OptimizeCode = CodeOptimization.Never;

		// Json.hpp requires RTTI since it uses dynamic_cast
		bUseRTTI = true;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleRootDirectory, ThirdPartyInclude),
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
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
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
				"UnrealEd",
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
	}
}
