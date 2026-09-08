using UnrealBuildTool;

public class KuroGlow : ModuleRules
{
	public KuroGlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"KuroGlowShaders",
		});
	}
}
