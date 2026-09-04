// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/NPCs/MythicNPCCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Player/MythicCharacter_Player.h"
#include "Settings/MythicAgentDetailSettings.h"
#include "UObject/UnrealType.h"
#include "World/Survival/MythicSurvivalComponent.h"

namespace MythicAgentTickTestsLocal {
    const UCharacterMovementComponent *MovementDefaultsOf(const ACharacter *CharacterCDO) {
        return CharacterCDO ? CharacterCDO->GetCharacterMovement() : nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAgentRotationRateTest,
                                 "Mythic.AI.AgentTick.RotationRate",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAgentRotationRateTest::RunTest(const FString &Parameters) {
    FMythicAgentRotationConfig Config;

    Config.YawRateDegreesPerSecond = 640.0f;
    const FRotator Normal = UMythicAgentDetailSettings::MakeRotationRate(Config);
    TestEqual(TEXT("Authored yaw reaches the movement component"), static_cast<float>(Normal.Yaw), 640.0f);
    TestEqual(TEXT("Pitch stays zero"), static_cast<float>(Normal.Pitch), 0.0f);
    TestEqual(TEXT("Roll stays zero"), static_cast<float>(Normal.Roll), 0.0f);

    Config.YawRateDegreesPerSecond = -50.0f;
    TestEqual(TEXT("A negative authored rate clamps to zero rather than spinning backwards"),
              static_cast<float>(UMythicAgentDetailSettings::MakeRotationRate(Config).Yaw), 0.0f);

    Config.YawRateDegreesPerSecond = 100000.0f;
    TestEqual(TEXT("An absurd authored rate clamps to the ceiling"),
              static_cast<float>(UMythicAgentDetailSettings::MakeRotationRate(Config).Yaw), 3600.0f);

    const UMythicAgentDetailSettings *Settings = GetDefault<UMythicAgentDetailSettings>();
    if (TestNotNull(TEXT("Agent detail settings resolve"), Settings)) {
        TestTrue(TEXT("The shipped default turn rate is usable, not zero"),
                 Settings->NPCRotation.YawRateDegreesPerSecond > 0.0f);
        TestTrue(TEXT("The shipped default keeps rotation inside PerformMovement; the controller-write path is rollback only"),
                 Settings->NPCRotation.bRotateInMovementComponent);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAgentRotationSourceTest,
                                 "Mythic.AI.AgentTick.RotationSource",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAgentRotationSourceTest::RunTest(const FString &Parameters) {
    // An AI pawn must never take its facing from a controller transform write: APawn::FaceRotation runs its own
    // UpdateOverlaps outside any scoped movement update, and does not replay during re-simulation.
    const AMythicNPCCharacter *NPC = GetDefault<AMythicNPCCharacter>();
    if (!TestNotNull(TEXT("NPC character CDO resolves"), NPC)) {
        return false;
    }

    TestFalse(TEXT("NPC yaw does not come from the controller transform write"), NPC->bUseControllerRotationYaw);
    TestFalse(TEXT("NPC pitch does not come from the controller transform write"), NPC->bUseControllerRotationPitch);
    TestFalse(TEXT("NPC roll does not come from the controller transform write"), NPC->bUseControllerRotationRoll);

    const UCharacterMovementComponent *NPCMovement = MythicAgentTickTestsLocal::MovementDefaultsOf(NPC);
    if (TestNotNull(TEXT("NPC movement component defaults resolve"), NPCMovement)) {
        TestTrue(TEXT("Rotation is produced inside PerformMovement instead"),
                 NPCMovement->bUseControllerDesiredRotation || NPCMovement->bOrientRotationToMovement);
    }

    // The player is driven by its own input, never by this band, and must keep the settings it already had.
    const AMythicCharacter_Player *Player = GetDefault<AMythicCharacter_Player>();
    if (TestNotNull(TEXT("Player character CDO resolves"), Player)) {
        TestFalse(TEXT("Player yaw is still not a controller transform write"), Player->bUseControllerRotationYaw);
        const UCharacterMovementComponent *PlayerMovement = MythicAgentTickTestsLocal::MovementDefaultsOf(Player);
        if (TestNotNull(TEXT("Player movement component defaults resolve"), PlayerMovement)) {
            TestTrue(TEXT("Player still orients to its own movement"), PlayerMovement->bOrientRotationToMovement);
            TestFalse(TEXT("Player does not take desired rotation from an AI controller"),
                      PlayerMovement->bUseControllerDesiredRotation);
        }
    }

    TestFalse(TEXT("The player does not derive from the AI character, so no agent detail row can reach it"),
              AMythicCharacter_Player::StaticClass()->IsChildOf(AMythicNPCCharacter::StaticClass()));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAgentFloorBandTest,
                                 "Mythic.AI.AgentTick.FloorBand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAgentFloorBandTest::RunTest(const FString &Parameters) {
    const UMythicAgentDetailSettings *Settings = GetDefault<UMythicAgentDetailSettings>();
    if (!TestNotNull(TEXT("Agent detail settings resolve"), Settings)) {
        return false;
    }

    // Letting an idle AI reuse its cached floor is the shipped default; the engine still forces a sweep after a real
    // move, a teleport, a dynamic base or an invalidated cache.
    TestFalse(TEXT("AI characters reuse the cached floor by default"), Settings->NPCFloor.bAlwaysCheckFloor);

    // The player must keep the engine's per-frame floor sweep: only AMythicNPCCharacter reads the row.
    const AMythicCharacter_Player *Player = GetDefault<AMythicCharacter_Player>();
    if (TestNotNull(TEXT("Player character CDO resolves"), Player)) {
        const UCharacterMovementComponent *PlayerMovement = MythicAgentTickTestsLocal::MovementDefaultsOf(Player);
        if (TestNotNull(TEXT("Player movement component defaults resolve"), PlayerMovement)) {
            TestTrue(TEXT("Player still checks its floor every frame"), PlayerMovement->bAlwaysCheckFloor);
            TestFalse(TEXT("Player is never put into nav walking"),
                      PlayerMovement->DefaultLandMovementMode == MOVE_NavWalking);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSurvivalStatusRootingTest,
                                 "Mythic.World.Survival.StatusEffectsAreRooted",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicSurvivalStatusRootingTest::RunTest(const FString &Parameters) {
    // The mapped status list holds loaded UClass pointers. Unreflected, they survive in the editor only because the
    // asset registry happens to hold the same classes, and dangle in -game after the first collection.
    const FProperty *Property = UMythicSurvivalComponent::StaticClass()->FindPropertyByName(TEXT("MappedStatuses"));
    if (!TestNotNull(TEXT("MappedStatuses is visible to reflection, so the GC can trace it"), Property)) {
        return false;
    }

    const FArrayProperty *ArrayProperty = CastField<FArrayProperty>(Property);
    if (!TestNotNull(TEXT("MappedStatuses is a reflected array"), ArrayProperty)) {
        return false;
    }

    const FStructProperty *Inner = CastField<FStructProperty>(ArrayProperty->Inner);
    if (!TestNotNull(TEXT("MappedStatuses holds a reflected struct"), Inner)) {
        return false;
    }

    const FProperty *EffectProperty = Inner->Struct->FindPropertyByName(TEXT("Effect"));
    if (!TestNotNull(TEXT("The mapped status exposes its effect class to the GC"), EffectProperty)) {
        return false;
    }
    TestNotNull(TEXT("The effect class is a reflected class reference"), CastField<FClassProperty>(EffectProperty));

    return true;
}

#endif
