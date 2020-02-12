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
			"Core",
			"CoreUObject",
			"Engine",
            "StateMachineEx",
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
				"BlueprintGraph",
                "UnrealEd",
				"KismetCompiler",
            });
        }
    }
}
