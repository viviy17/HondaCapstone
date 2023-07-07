// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class InworldAIIntegration : ModuleRules
{
    private string ThirdPartyDirectory
    {
        get
        {
            return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));
        }
    }

    private string ThirdPartyLibrariesDirectory
    {
        get
        {
            if(Target.Platform == UnrealTargetPlatform.Win64)
            {
                return Path.Combine(ThirdPartyDirectory, "Libraries/Win64");
            }
            else if(Target.Platform == UnrealTargetPlatform.Mac)
            {
                return Path.Combine(ThirdPartyDirectory, "Libraries/Mac");
            }
            else if(Target.Platform == UnrealTargetPlatform.IOS)
            {
                return Path.Combine(ThirdPartyDirectory, "Libraries/iOS");
            }
            else
            {
                return Path.Combine(ThirdPartyDirectory, "Libraries/Unknown");
            }
        }
    }

    public InworldAIIntegration(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[] 
            { 
                "Core", 
                "CoreUObject", 
                "Engine", 
                "InputCore",
                "InworldAIClient",
                "InworldAIPlatform",
                "InworldAINdk",
                "AudioCaptureCore",
            });


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "ApplicationCore",
                "AudioMixer",
                "Projects",
            }
            );

        PublicIncludePaths.Add(Path.Combine(ThirdPartyDirectory, "Includes"));

        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
    }
}
