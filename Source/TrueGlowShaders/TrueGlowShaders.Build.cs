using System;
using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class TrueGlowShaders : ModuleRules
{
	public TrueGlowShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RHI",
			"RenderCore",
			"Projects",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Renderer",
		});

		string RendererPrivatePath = ResolveRendererPrivatePath(Target);
		if (RendererPrivatePath != null)
		{
			PrivateIncludePaths.Add(RendererPrivatePath);
		}
		else
		{
			throw new BuildException(
				"TrueGlowShaders needs Engine/Source/Runtime/Renderer/Private on the include path " +
				"(ScreenPass.h / PostProcess/PostProcessMaterial.h). Could not locate the engine directory. " +
				"This plugin is pinned to a local source-build UE 4.26.2; set UE426_ENGINE_ROOT if auto-detection fails.");
		}
	}

	static string ResolveRendererPrivatePath(ReadOnlyTargetRules Target)
	{
		List<string> Candidates = new List<string>();

		try
		{
			string UbtPath = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
			if (!string.IsNullOrEmpty(UbtPath) && UbtPath.IndexOf("UnrealBuildTool", StringComparison.OrdinalIgnoreCase) >= 0)
			{
				string UbtDir = Path.GetDirectoryName(UbtPath);
				string EngineRoot = Path.GetFullPath(Path.Combine(UbtDir, "..", ".."));
				Candidates.Add(Path.Combine(EngineRoot, "Source", "Runtime", "Renderer", "Private"));
			}
		}
		catch (Exception)
		{
		}

		string EnvRoot = Environment.GetEnvironmentVariable("UE426_ENGINE_ROOT");
		if (!string.IsNullOrEmpty(EnvRoot))
		{
			Candidates.Add(Path.Combine(EnvRoot, "Source", "Runtime", "Renderer", "Private"));
		}

		if (Target.ProjectFile != null)
		{
			string ProjectDir = Path.GetDirectoryName(Target.ProjectFile.FullName);
			Candidates.Add(Path.GetFullPath(Path.Combine(ProjectDir, "Engine", "Source", "Runtime", "Renderer", "Private")));
		}

		foreach (string Candidate in Candidates)
		{
			try
			{
				string Full = Path.GetFullPath(Candidate);
				if (File.Exists(Path.Combine(Full, "PostProcess", "PostProcessMaterial.h")))
				{
					return Full;
				}
			}
			catch (Exception)
			{
			}
		}
		return null;
	}
}
