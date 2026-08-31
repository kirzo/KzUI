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
				"KzUI"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealEd",
				"BlueprintGraph",
				"KismetCompiler"
			}
			);
	}
}