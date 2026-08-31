// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/NPCDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/GameInstance.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicRandomSpawnDefinitionIndexTest,
                                 "Mythic.AI.RandomSpawn.DefinitionIndex",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicRandomSpawnDefinitionIndexTest::RunTest(const FString &Parameters) {
    // The emergent spawners (bounties, raids, ambushes, avengers, apex hunts) all roll through the
    // definition index; an empty index means every one of them silently spawns nothing. Assert against the
    // registry directly first so the test states its denominator.
    const FAssetRegistryModule &Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    Registry.Get().GetAssetsByClass(UNPCDefinition::StaticClass()->GetClassPathName(), Assets, true);
    AddInfo(FString::Printf(TEXT("%d NPCDefinition assets in the registry"), Assets.Num()));
    if (!TestTrue(TEXT("At least one NPC definition is authored"), Assets.Num() > 0)) {
        return false;
    }

    UGameInstance *GI = NewObject<UGameInstance>(GetTransientPackage());
    UMythicNPCManager *Manager = NewObject<UMythicNPCManager>(GI);

    // Every typed definition must be reachable through the index under its own exact tag.
    int32 Typed = 0;
    int32 Reachable = 0;
    for (const FAssetData &Asset : Assets) {
        const UNPCDefinition *Def = Cast<UNPCDefinition>(Asset.GetAsset());
        if (!Def || !Def->NPCType.IsValid()) {
            continue;
        }
        ++Typed;
        if (Manager->CountDefinitionsForType(Def->NPCType) > 0) {
            ++Reachable;
        }
    }
    AddInfo(FString::Printf(TEXT("%d typed definitions, %d reachable through the index"), Typed, Reachable));
    TestEqual(TEXT("Every typed definition is reachable by its type"), Reachable, Typed);

    // The LivingWorld integration contract: a definition the spawners rely on carries its social data into
    // the spawn - the dummy is the authored probe for this.
    if (const UNPCDefinition *Dummy = LoadObject<UNPCDefinition>(nullptr, TEXT("/Game/Mythic/AI/NPC_Defs/NPCD_Dummy.NPCD_Dummy"))) {
        TestTrue(TEXT("The dummy definition is typed"), Dummy->NPCType.IsValid());
        const FMythicNPCData Data(const_cast<UNPCDefinition *>(Dummy));
        TestEqual(TEXT("Spawn data carries the definition's type"), Data.NPCType, Dummy->NPCType);
        TestEqual(TEXT("Spawn data carries the affiliation overrides"),
                  Data.AffiliationOverrides.Num(), Dummy->AffiliationOverrides.Num());
        TestEqual(TEXT("Spawn data carries the fight-or-flight overrides"),
                  Data.FlightOrFightOverrides.Num(), Dummy->FlightOrFightOverrides.Num());
        TestEqual(TEXT("Spawn data carries the actor class"), Data.NPCClass, Dummy->NPCClass);
        TestEqual(TEXT("Spawn data carries the faction"), Data.Faction, Dummy->Faction);
    }

    // Half-authored definitions are the invisible-pawn bug: a typed definition with no actor class spawns
    // the raw C++ base (no mesh, no attack, no stats), and relation maps keyed outside Faction.* key
    // against nothing.
    const FGameplayTag FactionRoot = FGameplayTag::RequestGameplayTag(TEXT("Faction"), false);
    for (const FAssetData &Asset : Assets) {
        const UNPCDefinition *Def = Cast<UNPCDefinition>(Asset.GetAsset());
        if (!Def || !Def->NPCType.IsValid()) {
            continue;
        }
        TestTrue(FString::Printf(TEXT("%s: typed definition has an actor class"), *Def->GetName()),
                 Def->NPCClass != nullptr);
        for (const TPair<FGameplayTag, float> &Pair : Def->AffiliationOverrides) {
            TestTrue(FString::Printf(TEXT("%s: affiliation key %s is a Faction tag"), *Def->GetName(), *Pair.Key.ToString()),
                     FactionRoot.IsValid() && Pair.Key.MatchesTag(FactionRoot));
        }
        for (const TPair<FGameplayTag, float> &Pair : Def->FlightOrFightOverrides) {
            TestTrue(FString::Printf(TEXT("%s: fight-or-flight key %s is a Faction tag"), *Def->GetName(), *Pair.Key.ToString()),
                     FactionRoot.IsValid() && Pair.Key.MatchesTag(FactionRoot));
            TestTrue(FString::Printf(TEXT("%s: fight-or-flight value for %s is a probability"), *Def->GetName(), *Pair.Key.ToString()),
                     Pair.Value >= 0.0f && Pair.Value <= 1.0f);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSpawnerConfigChainTest,
                                 "Mythic.AI.RandomSpawn.SpawnerConfigChain",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnerConfigChainTest::RunTest(const FString &Parameters) {
    // Every emergent spawner is two-key: a master flag and a content tag. A flag that is ON with an unset
    // tag, or a tag naming a type nobody authored, is a system that looks alive and silently spawns
    // nothing. The chain is asserted here so drift between config and content fails a build, not a
    // playtest.
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    if (!TestNotNull(TEXT("Developer settings resolve"), Dev)) {
        return false;
    }

    UGameInstance *GI = NewObject<UGameInstance>(GetTransientPackage());
    UMythicNPCManager *Manager = NewObject<UMythicNPCManager>(GI);

    struct FSlot {
        const TCHAR *Name;
        bool bEnabled;
        FGameplayTag Tag;
    };
    const TArray<FSlot> Slots = {
        {TEXT("BountyHunters.HunterNPCType"), Dev->bEnableBountyHunters, Dev->BountyHunters.HunterNPCType},
        {TEXT("Avengers.AvengerNPCType"), Dev->bEnableAvengers, Dev->Avengers.AvengerNPCType},
        {TEXT("Camping.Events.AmbushNPCType"), Dev->bEnableCampEvents, Dev->Camping.Events.AmbushNPCType},
        {TEXT("Camping.Events.MerchantNPCType"), Dev->bEnableCampEvents, Dev->Camping.Events.MerchantNPCType},
        {TEXT("RegionalPressure.RaidNPCType"), Dev->bEnableFarmRaids, Dev->RegionalPressure.RaidNPCType},
    };

    int32 Live = 0;
    for (const FSlot &Slot : Slots) {
        if (Slot.bEnabled) {
            TestTrue(FString::Printf(TEXT("%s: enabled spawner has a content tag"), Slot.Name), Slot.Tag.IsValid());
        }
        if (Slot.Tag.IsValid()) {
            const int32 Count = Manager->CountDefinitionsForType(Slot.Tag);
            TestTrue(FString::Printf(TEXT("%s: tag %s reaches at least one authored definition"), Slot.Name, *Slot.Tag.ToString()),
                     Count > 0);
            ++Live;
        }
    }
    AddInfo(FString::Printf(TEXT("%d of %d spawner slots carry a content tag"), Live, Slots.Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAttitudeCompositionTest,
                                 "Mythic.AI.Factions.AttitudeComposition",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAttitudeCompositionTest::RunTest(const FString &Parameters) {
    // The banding contract behind GetTeamAttitudeTowards: relation baseline plus per-NPC delta, banded at
    // the shared +-50 thresholds. Baselines with no delta must reproduce the old relation switch exactly.
    constexpr float Hostile = -50.0f;
    constexpr float Friendly = 50.0f;

    TestEqual(TEXT("Allied baseline bands Friendly"), UMythicFactionDatabase::BandStanding(75.0f, Hostile, Friendly), ETeamAttitude::Friendly);
    TestEqual(TEXT("Friendly baseline bands Friendly"), UMythicFactionDatabase::BandStanding(60.0f, Hostile, Friendly), ETeamAttitude::Friendly);
    TestEqual(TEXT("Neutral baseline bands Neutral"), UMythicFactionDatabase::BandStanding(0.0f, Hostile, Friendly), ETeamAttitude::Neutral);
    TestEqual(TEXT("Unfriendly baseline bands Hostile"), UMythicFactionDatabase::BandStanding(-60.0f, Hostile, Friendly), ETeamAttitude::Hostile);
    TestEqual(TEXT("Hostile baseline bands Hostile"), UMythicFactionDatabase::BandStanding(-75.0f, Hostile, Friendly), ETeamAttitude::Hostile);

    // The delta bends a neutral relation into a stance without flipping bands it should not reach.
    TestEqual(TEXT("Neutral relation + raider's -80 delta bands Hostile"),
              UMythicFactionDatabase::BandStanding(FMath::Clamp(0.0f + -80.0f, -100.0f, 100.0f), Hostile, Friendly), ETeamAttitude::Hostile);
    TestEqual(TEXT("Neutral relation + hunter's +40 delta stays Neutral"),
              UMythicFactionDatabase::BandStanding(FMath::Clamp(0.0f + 40.0f, -100.0f, 100.0f), Hostile, Friendly), ETeamAttitude::Neutral);
    TestEqual(TEXT("Friendly relation + -20 delta drops into Neutral"),
              UMythicFactionDatabase::BandStanding(FMath::Clamp(60.0f + -20.0f, -100.0f, 100.0f), Hostile, Friendly), ETeamAttitude::Neutral);
    TestEqual(TEXT("Hostile relation + -80 delta clamps inside the scale"),
              UMythicFactionDatabase::BandStanding(FMath::Clamp(-75.0f + -80.0f, -100.0f, 100.0f), Hostile, Friendly), ETeamAttitude::Hostile);

    return true;
}

#endif
