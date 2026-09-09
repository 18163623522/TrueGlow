using UnrealBuildTool;

public class TrueGlowEditor : ModuleRules
{
	public TrueGlowEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TrueGlow",
			"TrueGlowShaders",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
			"PropertyEditor",
		});
	}
}
