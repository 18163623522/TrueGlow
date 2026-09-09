using UnrealBuildTool;

public class TrueGlow : ModuleRules
{
	public TrueGlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TrueGlowShaders",
		});
	}
}
