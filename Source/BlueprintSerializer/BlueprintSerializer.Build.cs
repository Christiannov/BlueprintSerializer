using UnrealBuildTool;

public class BlueprintSerializer : ModuleRules
{
	public BlueprintSerializer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AssetRegistry",
				"BlueprintGraph",
				"DeveloperSettings"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AnimGraph",
				"AnimGraphRuntime",
				"EditorStyle",
				"EditorWidgets", 
				"ToolMenus",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"KismetCompiler",
				"PropertyEditor",
				"WorkspaceMenuStructure",
				"ContentBrowser",
				"AssetTools",
				"GraphEditor",
				"Kismet",
				"Projects",
				"Settings",
				"EditorSubsystem",
				"DesktopPlatform",
				"ApplicationCore",
				"ControlRig",
				"RigVM",
				"RigVMDeveloper"
			}
		);
		
		// Add runtime dependencies for content
		if (Target.bBuildEditor)
		{
			RuntimeDependencies.Add("$(PluginDir)/Content/...", StagedFileType.UFS);
		}
	}
}
