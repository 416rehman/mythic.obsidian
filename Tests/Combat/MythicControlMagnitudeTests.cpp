#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"

namespace {
// A minimal infinite effect that grants a state tag and optionally carries a rolled control magnitude, standing in
// for a real GE_Status_* so the helper can be exercised without the whole buildup pipeline.
void ApplyControlLike(UAbilitySystemComponent *ASC, const FGameplayTag &StateTag, bool bSetMagnitude, float Magnitude) {
    UGameplayEffect *GE = NewObject<UGameplayEffect>(GetTransientPackage());
    GE->DurationPolicy = EGameplayEffectDurationType::Infinite;

    UTargetTagsGameplayEffectComponent &TagComp = GE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer Container;
    Container.AddTag(StateTag);
    TagComp.SetAndApplyTargetTagChanges(Container);

    FGameplayEffectSpec Spec(GE, ASC->MakeEffectContext(), 1.0f);
    if (bSetMagnitude) {
        Spec.SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE, Magnitude);
    }
    ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicControlMagnitudeTest,
    "Mythic.Combat.ControlMagnitude",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicControlMagnitudeTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };

    using Reg = UMythicStatusRegistry;

    // A target with no ability system, and no control status, is untouched.
    TestEqual(TEXT("no ability system is unscaled"), Reg::GetControlReductionMultiplier(nullptr, GAS_DEBUFF_SLOWED, 0.5f), 1.0f);

    AActor *Target = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Target);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Target, Target);

    TestEqual(TEXT("no active slow leaves movement unscaled"),
              Reg::GetControlReductionMultiplier(ASC, GAS_DEBUFF_SLOWED, 0.5f), 1.0f);

    // One authored slow of 0.5 leaves half speed.
    ApplyControlLike(ASC, GAS_DEBUFF_SLOWED, true, 0.5f);
    TestTrue(TEXT("a single 50% slow leaves half speed"),
             FMath::IsNearlyEqual(Reg::GetControlReductionMultiplier(ASC, GAS_DEBUFF_SLOWED, 0.0f), 0.5f, 0.01f));

    // A second stacks multiplicatively rather than summing to a full stop: 0.5 * 0.5 = 0.25.
    ApplyControlLike(ASC, GAS_DEBUFF_SLOWED, true, 0.5f);
    const float Stacked = Reg::GetControlReductionMultiplier(ASC, GAS_DEBUFF_SLOWED, 0.0f);
    TestTrue(*FString::Printf(TEXT("two 50%% slows stack multiplicatively to ~0.25, got %.3f"), Stacked),
             FMath::IsNearlyEqual(Stacked, 0.25f, 0.01f));
    TestTrue(TEXT("a stacked slow never reaches a full stop"), Stacked >= 0.05f);

    // Even an extreme stack floors above zero.
    for (int32 i = 0; i < 6; ++i) {
        ApplyControlLike(ASC, GAS_DEBUFF_SLOWED, true, 0.9f);
    }
    TestTrue(TEXT("a deep slow stack still leaves a sliver of movement"),
             Reg::GetControlReductionMultiplier(ASC, GAS_DEBUFF_SLOWED, 0.0f) >= 0.05f);

    // A terrify bonus stacks upward.
    AActor *Victim = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *VictimASC = NewObject<UMythicAbilitySystemComponent>(Victim);
    VictimASC->RegisterComponent();
    VictimASC->InitAbilityActorInfo(Victim, Victim);
    ApplyControlLike(VictimASC, GAS_DEBUFF_TERRIFIED, true, 0.3f);
    TestTrue(TEXT("a 30% terrify raises incoming damage to 1.3x"),
             FMath::IsNearlyEqual(Reg::GetControlBonusMultiplier(VictimASC, GAS_DEBUFF_TERRIFIED, 0.0f), 1.3f, 0.01f));

    // A control status with no authored band falls back to the pre-band constant, applied once.
    AActor *Weakened = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *WeakASC = NewObject<UMythicAbilitySystemComponent>(Weakened);
    WeakASC->RegisterComponent();
    WeakASC->InitAbilityActorInfo(Weakened, Weakened);
    ApplyControlLike(WeakASC, GAS_DEBUFF_WEAKENED, false, 0.0f);
    TestTrue(TEXT("an unbanded weaken uses the fallback constant"),
             FMath::IsNearlyEqual(Reg::GetControlReductionMultiplier(WeakASC, GAS_DEBUFF_WEAKENED, 0.25f), 0.75f, 0.01f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicControlPotencyTest,
    "Mythic.Combat.ControlPotency",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicControlPotencyTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };

    AActor *Applier = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Applier);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Applier, Applier);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Applier));

    const FGameplayAttribute Potency = UMythicAttributeSet_Offense::GetControlPotencyAttribute();

    TestEqual(TEXT("no potency leaves the roll unscaled"),
              UMythicStatusRegistry::ResolveApplierBonus(Applier, Potency), 1.0f);

    ASC->SetNumericAttributeBase(Potency, 0.3f);
    const float Low = UMythicStatusRegistry::ResolveApplierBonus(Applier, Potency);
    TestTrue(TEXT("gear-granted potency strengthens the roll"), Low > 1.0f);

    ASC->SetNumericAttributeBase(Potency, 0.9f);
    const float High = UMythicStatusRegistry::ResolveApplierBonus(Applier, Potency);
    TestTrue(TEXT("more potency is more"), High > Low);

    // The authored curve bends it: the second tranche of potency is worth less than the first.
    TestTrue(TEXT("potency diminishes as it stacks"), (High - Low) < (Low - 1.0f) * 2.01f);

    return true;
}
