using UnrealBuildTool;

public class BlacklineEditorTarget : TargetRules
{
	public BlacklineEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Blackline");
	}
}
