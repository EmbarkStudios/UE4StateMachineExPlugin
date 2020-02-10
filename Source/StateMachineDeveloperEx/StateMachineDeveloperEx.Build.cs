using System.IO;
using UnrealBuildTool;

public class StateMachineDeveloperEx : ModuleRules
{
	public StateMachineDeveloperEx(ReadOnlyTargetRules Target)
		: base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateIncludePaths.AddRange(new string[] 
		{
			Path.Combine(ModuleDirectory, "Private"),
		});
		
		PublicDependencyModuleNames.AddRange(new string[] 
		{
			"BlueprintGraph",
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
            "KismetCompiler",
            "StateMachineEx",
        });
	}
}
