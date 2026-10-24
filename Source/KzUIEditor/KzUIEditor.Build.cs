// Copyright 2026 kirzo

using UnrealBuildTool;

public class KzUIEditor : ModuleRules
{
	public KzUIEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"KzUIUncooked",
				"BlueprintGraph",
				"UnrealEd",
				"AssetDefinition",
				"PropertyEditor",
				"Projects"
			}
			);
	}
}