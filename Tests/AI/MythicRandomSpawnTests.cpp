// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/NPCDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/GameInstance.h"

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
    }

    return true;
}

#endif
