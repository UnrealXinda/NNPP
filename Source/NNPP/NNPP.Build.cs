// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NNPP : ModuleRules
{
	public NNPP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		PublicIncludePaths.AddRange(
			new string[] {
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"),
				System.IO.Path.Combine(GetModuleDirectory("RenderCore"), "Private"),
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"NNECore",
				"NNPPShaders",
				"Slate",
				"SlateCore",
				"RenderCore",
				"Renderer",
				"RHI",
				"Projects",
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}