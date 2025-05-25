// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MythicEditorTarget : TargetRules
{
	public MythicEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Mythic");
		//  Comment this LyraGame out for Packaging
		ExtraModuleNames.Add("LyraGame");
	}
}
