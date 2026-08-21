// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Mythic : ModuleRules
{
	public Mythic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Will use the Mythic folder as the root for includes
		PublicIncludePaths.AddRange(
			new string[]
			{
				"Mythic",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreOnline",
			"CoreUObject",
			"ApplicationCore",
			"Engine",
			"PhysicsCore",
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",
			"AIModule",
			"ModularGameplay",
			"ModularGameplayActors",
			"DataRegistry",
			"ReplicationGraph",
			"GameFeatures",
			"SignificanceManager",
			"Hotfix",
			"Niagara",
			"AsyncMixin",
			"ControlFlows",
			"PropertyPath",
			// EOS
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemEOS",
			"NavigationSystem",
			"DataConfigCore",
			"Json",
			"JsonUtilities",
			// MASS ECS — Living World population layer
			"MassCore",   // UE 5.8: FMassElement/FMassFragment/FMassTag base reflection types split into MassCore
			"MassEntity",
			"MassSpawner",
			"MassSimulation",
			"MassSignals",
			"MassCommon",
			"StructUtils"
		});

		// NVIDIA DLSS is an optional, separately-licensed plugin. Depend on it only when it is actually
		// present, so the game still builds for anyone who has not installed it, and gate the code on the
		// define rather than assuming the headers exist.
		bool bHasDLSS = System.IO.Directory.Exists(
			System.IO.Path.Combine(ModuleDirectory, "..", "..", "Plugins", "DLSS"));
		PublicDefinitions.Add("MYTHIC_WITH_DLSS=" + (bHasDLSS ? "1" : "0"));
		if (bHasDLSS)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "DLSSBlueprint", "StreamlineDLSSGBlueprint" });
		}

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"Slate",
			"SlateCore",
			"RenderCore",
			"DeveloperSettings",
			"EnhancedInput",
			"NetCore",
			"RHI",
			"Projects",
			"Gauntlet",
			"UMG",
			"CommonUI",
			"CommonInput",
			"GameSettings",
			"CommonGame",
			"CommonUser",
			"GameSubtitles",
			"GameplayMessageRuntime",
			"AudioMixer",
			"NetworkReplayStreaming",
			"UIExtension",
			"ClientPilot",
			"AudioModulation",
			"EngineSettings",
			"DTLSHandlerComponent",
			"CommonLoadingScreen",
			"ModelViewViewModel",
			// INotifyFieldValueChanged, which UMVVMView::SetViewModelByClass takes. ModelViewViewModel alone links
			// the view but not the interface's UClass.
			"FieldNotification",
		});

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);

		// Generate compile errors if using DrawDebug functions in test/shipping builds.
		PublicDefinitions.Add("SHIPPING_DRAW_DEBUG_ERROR=1");

		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);
	}
}