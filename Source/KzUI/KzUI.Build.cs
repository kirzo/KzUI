// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzUI : ModuleRules
{
	public KzUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"InputCore",
				"UMG",
				"DeveloperSettings"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"ApplicationCore",
				"Slate",
				"SlateCore"
			}
			);
	}
}