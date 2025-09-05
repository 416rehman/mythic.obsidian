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
		});

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
			"ModelViewViewModel"
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