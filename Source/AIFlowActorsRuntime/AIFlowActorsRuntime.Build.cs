using UnrealBuildTool;

public class AIFlowActorsRuntime : ModuleRules
{
	public AIFlowActorsRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"AIModule",
			"FlowActorsRuntime",
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});
	}
}
