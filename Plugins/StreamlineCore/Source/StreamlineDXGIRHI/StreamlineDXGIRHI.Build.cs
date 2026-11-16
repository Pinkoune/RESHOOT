/*
* Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
* NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
* property and proprietary rights in and to this material, related
* documentation and any modifications thereto. Any use, reproduction,
* disclosure or distribution of this material and related documentation
* without an express license agreement from NVIDIA CORPORATION or
* its affiliates is strictly prohibited.
*/
using UnrealBuildTool;
using System.IO;

public class StreamlineDXGIRHI : ModuleRules
{
	public StreamlineDXGIRHI(ReadOnlyTargetRules Target) : base(Target)
	{
		bValidateFormatStrings = false; // RESHOOT : plugin écrit pour UE 5.8, contrôles de format plus stricts en 5.9
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RHI",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RenderCore",
				"Streamline",
				"StreamlineRHI",
			}
		);
	}
}
