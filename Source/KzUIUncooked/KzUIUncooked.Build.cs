// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzUIUncooked : ModuleRules
{
	public KzUIUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"BlueprintGraph"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"SlateCore",
				"UMG",
				"UnrealEd",
				"KismetCompiler",
				"KzUI"
			}
			);
	}
}