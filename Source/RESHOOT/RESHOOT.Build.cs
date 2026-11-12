// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RESHOOT : ModuleRules
{
	public RESHOOT(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"AIModule", "NavigationSystem", "UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Permet les includes "Core/HealthComponent.h", "Director/...", "Medieval/..." depuis tous les dossiers.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
