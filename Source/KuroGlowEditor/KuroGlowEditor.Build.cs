using UnrealBuildTool;

public class KuroGlowEditor : ModuleRules
{
	public KuroGlowEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"KuroGlow",
			"Settings",
		});
	}
}
