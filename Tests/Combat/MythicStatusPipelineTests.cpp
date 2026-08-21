
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"

namespace {
// Buildup must arrive as a real effect. Writing the attribute directly skips PostGameplayEffectExecute,
// which is where the threshold lives, so a direct write can never trigger a status.
void PushBuildup(UAbilitySystemComponent *ASC, const FGameplayAttribute &Attribute, float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_StatusBuildup")));
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = Attribute;
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusPipelineTest,
    "Mythic.Combat.StatusPipeline",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusPipelineTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    // The buildup handler resolves the registry through the game instance, so a bare world would make every
    // status silently no-op and the test would pass for the wrong reason.
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    UMythicStatusRegistry *Registry = GameInstance->GetSubsystem<UMythicStatusRegistry>();
    if (!TestNotNull(TEXT("status registry subsystem exists"), Registry)) {
        return false;
    }

    UMythicStatusEffectDefinition *Burn = Registry->FindStatus(FGameplayTag::RequestGameplayTag(FName("Status.Type.Burn"), false));
    if (!TestNotNull(TEXT("Burn resolves from the registry — an empty registry must fail, not pass quietly"), Burn)) {
        return false;
    }
    if (!TestTrue(TEXT("Burn has a buildup attribute"), Burn->BuildupAttribute.IsValid())) {
        return false;
    }

    AActor *Target = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("target actor spawns"), Target)) {
        return false;
    }

    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Target);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Target, Target);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Target));
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Target));

    TestFalse(TEXT("target starts clean"), ASC->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));

    // Below the threshold nothing should land, or the bar means nothing.
    PushBuildup(ASC, Burn->BuildupAttribute, UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f) * 0.5f);
    TestFalse(TEXT("half a threshold does not apply the status"), ASC->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));

    // Crossing it must actually apply the effect, which is the whole chain: threshold, reaction check,
    // Event.ApplyStatus, the effect itself and the granted tag.
    PushBuildup(ASC, Burn->BuildupAttribute, UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f));
    TestTrue(TEXT("crossing the threshold applies the status"), ASC->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));

    // Crossing consumes the threshold rather than leaving the meter full, or the status would re-apply forever.
    TestTrue(TEXT("crossing consumes the buildup it spent"),
             ASC->GetNumericAttribute(Burn->BuildupAttribute) < UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f));

    // The rolled magnitudes have to reach the effect. Without this the tag still lands and the status does
    // nothing at all, which every other assertion here would happily pass.
    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(GAS_DEBUFF_BURNING));
    const TArray<FActiveGameplayEffectHandle> Active = ASC->GetActiveEffects(Query);
    if (TestTrue(TEXT("the burn is present as an active effect"), Active.Num() > 0)) {
        const FActiveGameplayEffect *Effect = ASC->GetActiveGameplayEffect(Active[0]);
        if (TestNotNull(TEXT("the active burn resolves"), Effect)) {
            const float Damage = Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE, false, -1.0f);
            const float Duration = Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DURATION, false, -1.0f);

            TestTrue(*FString::Printf(TEXT("burn carries rolled damage inside its authored band, got %.2f"), Damage),
                     Damage >= Burn->DamagePerTick.Min && Damage <= Burn->DamagePerTick.Max);
            TestTrue(*FString::Printf(TEXT("burn carries a rolled duration inside its authored band, got %.2f"), Duration),
                     Duration >= Burn->DurationSeconds.Min && Duration <= Burn->DurationSeconds.Max);
            TestTrue(TEXT("the authored damage band is not degenerate, so two applications can differ"),
                     Burn->DamagePerTick.Max > Burn->DamagePerTick.Min);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusSuppressionTest,
    "Mythic.Combat.StatusSuppression",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusSuppressionTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    UMythicStatusRegistry *Registry = GameInstance->GetSubsystem<UMythicStatusRegistry>();
    if (!TestNotNull(TEXT("status registry subsystem exists"), Registry)) {
        return false;
    }

    UMythicStatusEffectDefinition *Stun = Registry->FindStatus(FGameplayTag::RequestGameplayTag(FName("Status.Type.Stun"), false));
    UMythicStatusEffectDefinition *Burn = Registry->FindStatus(FGameplayTag::RequestGameplayTag(FName("Status.Type.Burn"), false));
    if (!TestNotNull(TEXT("Stun resolves"), Stun) || !TestNotNull(TEXT("Burn resolves"), Burn)) {
        return false;
    }

    auto MakeTarget = [World]() -> UMythicAbilitySystemComponent * {
        AActor *Actor = World->SpawnActor<AActor>();
        UMythicAbilitySystemComponent *NewASC = NewObject<UMythicAbilitySystemComponent>(Actor);
        NewASC->RegisterComponent();
        NewASC->InitAbilityActorInfo(Actor, Actor);
        NewASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Actor));
        NewASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Actor));
        return NewASC;
    };

    // Hard crowd control is eaten by immunity, and the buildup is still spent so an immune target cannot bank it.
    {
        UMythicAbilitySystemComponent *Immune = MakeTarget();
        Immune->AddLooseGameplayTag(GAS_IMMUNE_HARDCC);
        const float Threshold = UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f);
        PushBuildup(Immune, Stun->BuildupAttribute, Threshold * 2.0f);

        TestFalse(TEXT("hard CC immunity blocks the stun"), Immune->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED));
        TestTrue(TEXT("the blocked stun still consumes its buildup"),
                 Immune->GetNumericAttribute(Stun->BuildupAttribute) < Threshold * 2.0f);
    }

    // A reaction that suppresses the status must actually stop it landing: Burn onto a poisoned target detonates
    // instead of igniting. Authored in DA_Status_Burn, so this fails if the reaction is mis-authored.
    {
        UMythicAbilitySystemComponent *Poisoned = MakeTarget();
        Poisoned->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.State.Poisoned"), false));
        PushBuildup(Poisoned, Burn->BuildupAttribute, UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f) * 2.0f);

        TestFalse(TEXT("the poison reaction suppresses the burn instead of applying it"),
                  Poisoned->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));
    }

    // Without the reaction present, the same push must apply normally, or the test above would pass for any reason.
    {
        UMythicAbilitySystemComponent *Plain = MakeTarget();
        PushBuildup(Plain, Burn->BuildupAttribute, UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f) * 2.0f);
        TestTrue(TEXT("without the reaction the burn lands"), Plain->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));
    }

    return true;
}
