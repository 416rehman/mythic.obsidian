#include "Misc/AutomationTest.h"

#include <limits>

#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Tests/Combat/MythicStatusDamagePresentationTestTypes.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicDeveloperSettings.h"
#include "UI/MythicDamageNumberSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageHealthResolutionPolicyTest,
    "Mythic.Combat.StatusDamagePresentation.HealthResolutionPolicy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageHealthResolutionPolicyTest::RunTest(const FString &Parameters) {
    using Life = UMythicAttributeSet_Life;

    TestEqual(TEXT("ordinary damage reports the health actually removed"),
              Life::ResolveAppliedHealthDamage(100.0f, 75.0f), 25.0f);
    TestEqual(TEXT("lethal overkill reports remaining health rather than the authored hit"),
              Life::ResolveAppliedHealthDamage(3.25f, 0.0f), 3.25f);
    TestEqual(TEXT("unchanged health resolves no damage"),
              Life::ResolveAppliedHealthDamage(50.0f, 50.0f), 0.0f);
    TestEqual(TEXT("a new rolled status stack adds to the snapshotted aggregate"),
              UMythicStatusRegistry::ResolveStackedDamageMagnitude(12.0f, 7.5f, 2, 3), 19.5f);
    TestEqual(TEXT("a capped status reapplication cannot rewrite aggregate tick damage"),
              UMythicStatusRegistry::ResolveStackedDamageMagnitude(19.5f, 999.0f, 3, 3), 19.5f);
    TestEqual(TEXT("rolled Slow stacks snapshot their exact multiplicative reduction"),
              UMythicStatusRegistry::ResolveStackedControlMagnitude(
                  0.5f, 0.5f, 1, 3, EMythicStatusControlOperation::Reduction),
              0.75f);
    TestEqual(TEXT("rolled Terrify stacks snapshot their exact multiplicative bonus"),
              UMythicStatusRegistry::ResolveStackedControlMagnitude(
                  0.5f, 0.5f, 1, 3, EMythicStatusControlOperation::Bonus),
              1.25f);
    TestEqual(TEXT("a capped control reapplication cannot rewrite its aggregate"),
              UMythicStatusRegistry::ResolveStackedControlMagnitude(
                  0.875f, 0.9f, 3, 3, EMythicStatusControlOperation::Reduction),
              0.875f);
    TestEqual(TEXT("a health increase never becomes negative damage"),
              Life::ResolveAppliedHealthDamage(25.0f, 40.0f), 0.0f);
    TestEqual(TEXT("fractional shield absorption is preserved exactly for transport"),
              Life::ResolveAppliedShieldDamage(3.25f), 3.25f);
    TestEqual(TEXT("negative shield absorption fails closed"),
              Life::ResolveAppliedShieldDamage(-4.0f), 0.0f);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    TestEqual(TEXT("a non-finite old health fails closed"),
              Life::ResolveAppliedHealthDamage(NaN, 0.0f), 0.0f);
    TestEqual(TEXT("a non-finite new health fails closed"),
              Life::ResolveAppliedHealthDamage(10.0f, NaN), 0.0f);
    TestEqual(TEXT("infinite health input fails closed"),
              Life::ResolveAppliedHealthDamage(Infinity, 0.0f), 0.0f);
    TestEqual(TEXT("non-finite shield absorption fails closed"),
              Life::ResolveAppliedShieldDamage(NaN), 0.0f);
    TestEqual(TEXT("infinite shield absorption fails closed"),
              Life::ResolveAppliedShieldDamage(Infinity), 0.0f);

    TestTrue(TEXT("positive authoritative damage emits combat text"),
             Life::ShouldEmitResolvedCombatText(0.01f, true));
    TestFalse(TEXT("a client-side resolution cannot emit a duplicate"),
              Life::ShouldEmitResolvedCombatText(10.0f, false));
    TestFalse(TEXT("zero damage emits no number"),
              Life::ShouldEmitResolvedCombatText(0.0f, true));
    TestFalse(TEXT("negative damage emits no number"),
              Life::ShouldEmitResolvedCombatText(-10.0f, true));
    TestFalse(TEXT("NaN damage emits no number"),
              Life::ShouldEmitResolvedCombatText(NaN, true));
    TestFalse(TEXT("infinite damage emits no number"),
              Life::ShouldEmitResolvedCombatText(Infinity, true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageRoutingPolicyTest,
    "Mythic.Combat.StatusDamagePresentation.RoutingPolicy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageRoutingPolicyTest::RunTest(const FString &Parameters) {
    using Life = UMythicAttributeSet_Life;

    TestTrue(TEXT("a distinct source viewer receives an outgoing copy"),
             Life::ShouldRouteResolvedCombatTextToSource(true, false));
    TestTrue(TEXT("a target viewer receives an incoming copy"),
             Life::ShouldRouteResolvedCombatTextToTarget(true));

    TestFalse(TEXT("an actor-less world source has no outgoing viewer"),
              Life::ShouldRouteResolvedCombatTextToSource(false, false));
    TestTrue(TEXT("actor-less world damage still reaches its target as incoming"),
             Life::ShouldRouteResolvedCombatTextToTarget(true));

    TestFalse(TEXT("self-authored damage does not masquerade as outgoing damage dealt"),
              Life::ShouldRouteResolvedCombatTextToSource(true, true));
    TestTrue(TEXT("self-authored damage is retained as one incoming copy"),
             Life::ShouldRouteResolvedCombatTextToTarget(true));

    TestFalse(TEXT("a missing target viewer receives no incoming copy"),
              Life::ShouldRouteResolvedCombatTextToTarget(false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageVisibilityPolicyTest,
    "Mythic.Combat.StatusDamagePresentation.VisibilityPolicy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageVisibilityPolicyTest::RunTest(const FString &Parameters) {
    using DamageNumbers = UMythicDamageNumberSubsystem;

    TestFalse(TEXT("mode 0 hides outgoing damage numbers"),
              DamageNumbers::ShouldPresentResolvedEvent(0, true));
    TestFalse(TEXT("mode 0 hides incoming damage numbers"),
              DamageNumbers::ShouldPresentResolvedEvent(0, false));

    TestTrue(TEXT("mode 1 shows outgoing damage numbers"),
             DamageNumbers::ShouldPresentResolvedEvent(1, true));
    TestFalse(TEXT("mode 1 hides incoming damage numbers"),
              DamageNumbers::ShouldPresentResolvedEvent(1, false));

    TestTrue(TEXT("mode 2 shows outgoing damage numbers"),
             DamageNumbers::ShouldPresentResolvedEvent(2, true));
    TestTrue(TEXT("mode 2 shows incoming damage numbers"),
             DamageNumbers::ShouldPresentResolvedEvent(2, false));

    TestFalse(TEXT("an unknown mode fails closed for outgoing damage numbers"),
              DamageNumbers::ShouldPresentResolvedEvent(3, true));
    TestFalse(TEXT("the maximum byte mode fails closed for incoming damage numbers"),
              DamageNumbers::ShouldPresentResolvedEvent(MAX_uint8, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageContextClassificationTest,
    "Mythic.Combat.StatusDamagePresentation.ContextClassification",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageContextClassificationTest::RunTest(const FString &Parameters) {
    UMythicStatusEffectDefinition *Definition = NewObject<UMythicStatusEffectDefinition>();
    TestNotNull(TEXT("a canonical status definition can be constructed"), Definition);

    FMythicGameplayEffectContext *StatusContext = new FMythicGameplayEffectContext();
    StatusContext->SetCriticalHit(true);
    FGameplayEffectContextHandle StatusHandle(StatusContext);
    StatusHandle.AddSourceObject(Definition);

    TestTrue(TEXT("the context carries the exact status Data Asset as SourceObject"),
             StatusHandle.GetSourceObject() == Definition);
    const UMythicStatusEffectDefinition *ResolvedStatus =
        UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(1.0f, StatusHandle);
    TestTrue(TEXT("a periodic typed context resolves the exact canonical status definition"),
             ResolvedStatus == Definition);

    TestNull(TEXT("the same typed context remains direct when its effect is not periodic"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(0.0f, StatusHandle));
    TestNull(TEXT("negative periods cannot classify as status ticks"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(-1.0f, StatusHandle));
    TestNull(TEXT("non-finite periods cannot classify as status ticks"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(
                 std::numeric_limits<float>::quiet_NaN(), StatusHandle));
    TestNull(TEXT("an empty context cannot classify as a status tick"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(
                 1.0f, FGameplayEffectContextHandle()));

    UGameplayEffect *UntypedSource = NewObject<UGameplayEffect>();
    FGameplayEffectContextHandle UntypedHandle(new FGameplayEffectContext());
    UntypedHandle.AddSourceObject(UntypedSource);
    TestNull(TEXT("an arbitrary periodic SourceObject is not mistaken for a status definition"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(1.0f, UntypedHandle));

    FMythicResolvedCombatTextEvent StatusEvent;
    StatusEvent.StatusDefinition = Definition;
    StatusEvent.Origin = ResolvedStatus
                             ? EMythicCombatTextOrigin::StatusTick
                             : EMythicCombatTextOrigin::DirectDamage;
    StatusEvent.bCritical = !ResolvedStatus && StatusContext->IsCriticalHit();
    TestTrue(TEXT("the resolved definition makes the event buildable as a status tick"),
             StatusEvent.Origin == EMythicCombatTextOrigin::StatusTick);
    TestFalse(TEXT("a status tick cannot inherit an application-hit critical flag"),
              StatusEvent.bCritical);

    const UMythicStatusEffectDefinition *DirectStatus =
        UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(0.0f, StatusHandle);
    FMythicResolvedCombatTextEvent DirectEvent;
    DirectEvent.Origin = DirectStatus
                             ? EMythicCombatTextOrigin::StatusTick
                             : EMythicCombatTextOrigin::DirectDamage;
    DirectEvent.bCritical = !DirectStatus && StatusContext->IsCriticalHit();
    TestTrue(TEXT("the direct path remains independently eligible for critical presentation"),
             DirectEvent.Origin == EMythicCombatTextOrigin::DirectDamage && DirectEvent.bCritical);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageSourceASCTest,
    "Mythic.Combat.StatusDamagePresentation.SourceASC",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageSourceASCTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AMythicPlayerState *Instigator = World->SpawnActor<AMythicPlayerState>();
    UAbilitySystemComponent *InstigatorASC = Instigator ? Instigator->GetAbilitySystemComponent() : nullptr;
    AActor *Target = World->SpawnActor<AActor>();
    UAbilitySystemComponent *TargetASC = Target ? NewObject<UAbilitySystemComponent>(Target) : nullptr;
    AActor *HazardA = World->SpawnActor<AActor>();
    AActor *HazardB = World->SpawnActor<AActor>();
    AMythicStatusDamageTestGameState *EnvironmentSource = World->SpawnActor<AMythicStatusDamageTestGameState>();
    if (EnvironmentSource) {
        World->SetGameState(EnvironmentSource);
    }
    UAbilitySystemComponent *EnvironmentASC = EnvironmentSource
                                                   ? EnvironmentSource->GetAbilitySystemComponent()
                                                   : nullptr;

    if (!TestNotNull(TEXT("the typed instigator spawns"), Instigator)
        || !TestNotNull(TEXT("the typed instigator owns an ASC"), InstigatorASC)
        || !TestNotNull(TEXT("the target ASC exists"), TargetASC)
        || !TestNotNull(TEXT("the first ASC-less hazard exists"), HazardA)
        || !TestNotNull(TEXT("the second ASC-less hazard exists"), HazardB)
        || !TestNotNull(TEXT("the authoritative environmental source exists"), EnvironmentSource)
        || !TestNotNull(TEXT("the environmental source owns an ASC"), EnvironmentASC)) {
        return false;
    }

    TargetASC->RegisterComponent();
    TargetASC->InitAbilityActorInfo(Target, Target);

    TestTrue(TEXT("a status spec uses the instigator ASC when one exists"),
             UMythicStatusRegistry::ResolveStatusEffectSourceASC(Instigator, TargetASC) == InstigatorASC);
    UAbilitySystemComponent *HazardAASC = UMythicStatusRegistry::ResolveStatusEffectSourceASC(HazardA, TargetASC);
    UAbilitySystemComponent *HazardBASC = UMythicStatusRegistry::ResolveStatusEffectSourceASC(HazardB, TargetASC);
    TestNotNull(TEXT("an ASC-less hazard receives a transient stacking identity"), HazardAASC);
    TestTrue(TEXT("the same hazard deterministically reuses its stacking identity"),
             UMythicStatusRegistry::ResolveStatusEffectSourceASC(HazardA, TargetASC) == HazardAASC);
    TestTrue(TEXT("distinct ASC-less hazards never share rolls or a stack cap"),
             HazardAASC && HazardBASC && HazardAASC != HazardBASC);
    TestTrue(TEXT("a hazard's stacking identity is owned by that exact actor"),
             HazardAASC && HazardAASC->GetOwnerActor() == HazardA);
    TestTrue(TEXT("a truly actor-less world status uses the GameState ASC rather than defender captures"),
             UMythicStatusRegistry::ResolveStatusEffectSourceASC(nullptr, TargetASC) == EnvironmentASC);
    TestTrue(TEXT("the environmental stacking bucket is never the target ASC"),
             EnvironmentASC != TargetASC);
    TestNull(TEXT("no instigator ASC and no target ASC resolves null"),
             UMythicStatusRegistry::ResolveStatusEffectSourceASC(nullptr, nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageApplicationProvenanceTest,
    "Mythic.Combat.StatusDamagePresentation.ApplicationProvenance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageApplicationProvenanceTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    UWorld *World = GameInstance->GetWorld();
    UMythicStatusRegistry *Registry = GameInstance->GetSubsystem<UMythicStatusRegistry>();
    if (!TestNotNull(TEXT("standalone world exists"), World)
        || !TestNotNull(TEXT("status registry exists"), Registry)) {
        return false;
    }

    UMythicStatusEffectDefinition *Burn = Registry->FindStatus(
        FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Burn")), false));
    if (!TestNotNull(TEXT("Burn resolves from the canonical status registry"), Burn)) {
        return false;
    }

    AMythicPlayerState *Source = World->SpawnActor<AMythicPlayerState>();
    AActor *Target = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *SourceASC = Source ? Source->GetMythicAbilitySystemComponent() : nullptr;
    UMythicAbilitySystemComponent *TargetASC = Target ? NewObject<UMythicAbilitySystemComponent>(Target) : nullptr;
    if (!TestNotNull(TEXT("source actor spawns"), Source)
        || !TestNotNull(TEXT("source ASC exists"), SourceASC)
        || !TestNotNull(TEXT("target actor spawns"), Target)
        || !TestNotNull(TEXT("target ASC exists"), TargetASC)) {
        return false;
    }

    SourceASC->InitAbilityActorInfo(Source, Source);
    TargetASC->RegisterComponent();
    TargetASC->InitAbilityActorInfo(Target, Target);
    TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Target));

    if (!TestTrue(TEXT("Burn applies through the source-owned outgoing spec"),
                  UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, Source, Source))) {
        return false;
    }

    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
        FGameplayTagContainer(Burn->GrantedStateTag));
    const TArray<FActiveGameplayEffectHandle> ActiveHandles = TargetASC->GetActiveEffects(Query);
    if (!TestEqual(TEXT("the target owns exactly one active Burn effect"), ActiveHandles.Num(), 1)) {
        return false;
    }

    const FActiveGameplayEffect *ActiveBurn = TargetASC->GetActiveGameplayEffect(ActiveHandles[0]);
    if (!TestNotNull(TEXT("the active Burn effect resolves"), ActiveBurn)) {
        return false;
    }

    const FGameplayEffectContextHandle &Context = ActiveBurn->Spec.GetEffectContext();
    TestTrue(TEXT("the applied spec retains the exact Burn definition as SourceObject"),
             Context.GetSourceObject() == Burn);
    TestTrue(TEXT("the applied spec retains the real source actor"),
             Context.GetOriginalInstigator() == Source);
    TestTrue(TEXT("the applied spec retains the source actor's ASC"),
             Context.GetOriginalInstigatorAbilitySystemComponent() == SourceASC);
    TestTrue(TEXT("the active periodic spec resolves back to the exact Burn definition"),
             UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(
                 ActiveBurn->Spec.GetPeriod(), Context) == Burn);

    const FMythicGameplayEffectContext *MythicContext =
        FMythicGameplayEffectContext::ExtractEffectContext(Context);
    if (TestNotNull(TEXT("the status spec uses a Mythic gameplay-effect context"), MythicContext)) {
        TestFalse(TEXT("a fresh status context does not inherit critical-hit state"),
                  MythicContext->IsCriticalHit());
        TestFalse(TEXT("a fresh status context does not reuse Burn proc-intent as damage identity"),
                  MythicContext->IsBurn());
        TestFalse(TEXT("a fresh status context does not reuse Bleed proc-intent"),
                  MythicContext->IsBleed());
        TestFalse(TEXT("a fresh status context does not reuse Poison proc-intent"),
                  MythicContext->IsPoison());
    }

    const float FirstAggregate = ActiveBurn->Spec.GetSetByCallerMagnitude(
        GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f);
    TestTrue(TEXT("the first Burn stack snapshots a positive aggregate tick"), FirstAggregate > 0.0f);

    TestTrue(TEXT("a second Burn stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, Source, Source));
    const FActiveGameplayEffect *SecondBurn = TargetASC->GetActiveGameplayEffect(ActiveHandles[0]);
    if (!TestNotNull(TEXT("the second Burn stack resolves"), SecondBurn)) {
        return false;
    }
    const float SecondAggregate = SecondBurn->Spec.GetSetByCallerMagnitude(
        GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f);
    TestEqual(TEXT("the second application increments one source-owned stack"), SecondBurn->Spec.GetStackCount(), 2);
    TestTrue(TEXT("the second roll adds to rather than rewrites aggregate tick damage"),
             SecondAggregate > FirstAggregate);

    TestTrue(TEXT("a third Burn stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, Source, Source));
    const FActiveGameplayEffect *ThirdBurn = TargetASC->GetActiveGameplayEffect(ActiveHandles[0]);
    if (!TestNotNull(TEXT("the third Burn stack resolves"), ThirdBurn)) {
        return false;
    }
    const float CappedAggregate = ThirdBurn->Spec.GetSetByCallerMagnitude(
        GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f);
    TestEqual(TEXT("the third application reaches the authored cap"), ThirdBurn->Spec.GetStackCount(), 3);
    TestTrue(TEXT("the third roll remains represented in aggregate tick damage"),
             CappedAggregate > SecondAggregate);

    TestTrue(TEXT("a capped Burn reapplication can refresh its bundle"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, Source, Source));
    const FActiveGameplayEffect *RefreshedBurn = TargetASC->GetActiveGameplayEffect(ActiveHandles[0]);
    if (!TestNotNull(TEXT("the refreshed capped Burn resolves"), RefreshedBurn)) {
        return false;
    }
    TestEqual(TEXT("a capped reapplication cannot exceed the stack cap"), RefreshedBurn->Spec.GetStackCount(), 3);
    TestEqual(TEXT("a capped reapplication preserves every prior stack roll"),
              RefreshedBurn->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f),
              CappedAggregate);

    UMythicStatusEffectDefinition *Slow = Registry->FindStatus(
        FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Slow")), false));
    UMythicStatusEffectDefinition *Terrify = Registry->FindStatus(
        FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Terrify")), false));
    if (!TestNotNull(TEXT("Slow resolves for live control-stack verification"), Slow)
        || !TestNotNull(TEXT("Terrify resolves for live control-stack verification"), Terrify)) {
        return false;
    }
    TestTrue(TEXT("Slow is authored as a multiplicative reduction"),
             Slow->ControlOperation == EMythicStatusControlOperation::Reduction);
    TestTrue(TEXT("Terrify is authored as a multiplicative bonus"),
             Terrify->ControlOperation == EMythicStatusControlOperation::Bonus);

    TestTrue(TEXT("the first Slow stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Slow, Source, Source));
    const float FirstSlowFactor = UMythicStatusRegistry::GetControlReductionMultiplier(
        TargetASC, Slow->GrantedStateTag, 0.0f);
    TestTrue(TEXT("the second Slow stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Slow, Source, Source));
    const float SecondSlowFactor = UMythicStatusRegistry::GetControlReductionMultiplier(
        TargetASC, Slow->GrantedStateTag, 0.0f);
    TestTrue(TEXT("two independently rolled Slow stacks are stronger than one"),
             SecondSlowFactor < FirstSlowFactor);

    TestTrue(TEXT("the first Terrify stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Terrify, Source, Source));
    const float FirstTerrifyFactor = UMythicStatusRegistry::GetControlBonusMultiplier(
        TargetASC, Terrify->GrantedStateTag, 0.0f);
    TestTrue(TEXT("the second Terrify stack applies"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Terrify, Source, Source));
    const float SecondTerrifyFactor = UMythicStatusRegistry::GetControlBonusMultiplier(
        TargetASC, Terrify->GrantedStateTag, 0.0f);
    TestTrue(TEXT("two independently rolled Terrify stacks are stronger than one"),
             SecondTerrifyFactor > FirstTerrifyFactor);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageEnvironmentalProvenanceTest,
    "Mythic.Combat.StatusDamagePresentation.EnvironmentalProvenance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageEnvironmentalProvenanceTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    UWorld *World = GameInstance->GetWorld();
    UMythicStatusRegistry *Registry = GameInstance->GetSubsystem<UMythicStatusRegistry>();
    AMythicStatusDamageTestGameState *EnvironmentSource =
        World ? World->SpawnActor<AMythicStatusDamageTestGameState>() : nullptr;
    if (World && EnvironmentSource) {
        World->SetGameState(EnvironmentSource);
    }
    UAbilitySystemComponent *EnvironmentASC = EnvironmentSource
                                                   ? EnvironmentSource->GetAbilitySystemComponent()
                                                   : nullptr;
    UMythicStatusEffectDefinition *Burn = Registry
                                              ? Registry->FindStatus(FGameplayTag::RequestGameplayTag(
                                                    FName(TEXT("Status.Type.Burn")), false))
                                              : nullptr;
    AActor *Hazard = World ? World->SpawnActor<AActor>() : nullptr;
    AActor *SecondHazard = World ? World->SpawnActor<AActor>() : nullptr;
    AActor *Target = World ? World->SpawnActor<AActor>() : nullptr;
    UMythicAbilitySystemComponent *TargetASC = Target ? NewObject<UMythicAbilitySystemComponent>(Target) : nullptr;
    if (!TestNotNull(TEXT("the standalone world exists"), World)
        || !TestNotNull(TEXT("the status registry exists"), Registry)
        || !TestNotNull(TEXT("the environmental source exists"), EnvironmentSource)
        || !TestNotNull(TEXT("the environmental source ASC exists"), EnvironmentASC)
        || !TestNotNull(TEXT("Burn resolves"), Burn)
        || !TestNotNull(TEXT("the ASC-less hazard exists"), Hazard)
        || !TestNotNull(TEXT("the second ASC-less hazard exists"), SecondHazard)
        || !TestNotNull(TEXT("the target exists"), Target)
        || !TestNotNull(TEXT("the target ASC exists"), TargetASC)) {
        return false;
    }

    TargetASC->RegisterComponent();
    TargetASC->InitAbilityActorInfo(Target, Target);
    TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Target));

    for (int32 ApplicationIndex = 0; ApplicationIndex < 4; ++ApplicationIndex) {
        TestTrue(*FString::Printf(TEXT("environmental Burn application %d succeeds"), ApplicationIndex + 1),
                 UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, Hazard, Hazard));
    }

    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
        FGameplayTagContainer(Burn->GrantedStateTag));
    const TArray<FActiveGameplayEffectHandle> Handles = TargetASC->GetActiveEffects(Query);
    if (!TestEqual(TEXT("applications from one hazard share one bounded source stack"), Handles.Num(), 1)) {
        return false;
    }
    const FActiveGameplayEffect *ActiveBurn = TargetASC->GetActiveGameplayEffect(Handles[0]);
    if (!TestNotNull(TEXT("the environmental Burn resolves"), ActiveBurn)) {
        return false;
    }

    const FGameplayEffectContextHandle &Context = ActiveBurn->Spec.GetEffectContext();
    UAbilitySystemComponent *HazardASC = Context.GetOriginalInstigatorAbilitySystemComponent();
    const float FirstHazardAggregate = ActiveBurn->Spec.GetSetByCallerMagnitude(
        GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f);
    TestEqual(TEXT("environmental Burn respects the authored cap"), ActiveBurn->Spec.GetStackCount(), 3);
    TestTrue(TEXT("the physical hazard remains the gameplay instigator for exact attribution"),
             Context.GetOriginalInstigator() == Hazard);
    TestTrue(TEXT("environmental stacking is owned by the hazard's transient source ASC"),
             HazardASC && HazardASC->GetOwnerActor() == Hazard && HazardASC != EnvironmentASC);
    TestTrue(TEXT("the physical hazard remains exact typed attribution"), Context.GetEffectCauser() == Hazard);
    TestTrue(TEXT("the canonical Burn definition remains the SourceObject"), Context.GetSourceObject() == Burn);

    TestTrue(TEXT("a second hazard can apply an independent Burn"),
             UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Burn, SecondHazard, SecondHazard));
    const TArray<FActiveGameplayEffectHandle> IndependentHandles = TargetASC->GetActiveEffects(Query);
    if (!TestEqual(TEXT("distinct hazards own distinct Burn handles"), IndependentHandles.Num(), 2)) {
        return false;
    }

    const FActiveGameplayEffect *FirstHazardBurn = nullptr;
    const FActiveGameplayEffect *SecondHazardBurn = nullptr;
    for (const FActiveGameplayEffectHandle &Handle : IndependentHandles) {
        const FActiveGameplayEffect *Candidate = TargetASC->GetActiveGameplayEffect(Handle);
        if (!Candidate) {
            continue;
        }
        const AActor *CandidateInstigator = Candidate->Spec.GetEffectContext().GetOriginalInstigator();
        if (CandidateInstigator == Hazard) {
            FirstHazardBurn = Candidate;
        }
        else if (CandidateInstigator == SecondHazard) {
            SecondHazardBurn = Candidate;
        }
    }
    TestNotNull(TEXT("the original hazard's capped Burn remains present"), FirstHazardBurn);
    TestNotNull(TEXT("the second hazard's independent Burn resolves"), SecondHazardBurn);
    if (FirstHazardBurn && SecondHazardBurn) {
        TestEqual(TEXT("a second hazard cannot rewrite the first hazard's stack count"),
                  FirstHazardBurn->Spec.GetStackCount(), 3);
        TestEqual(TEXT("a second hazard starts at one independent stack"),
                  SecondHazardBurn->Spec.GetStackCount(), 1);
        TestEqual(TEXT("a second hazard cannot rewrite the first hazard's aggregate tick"),
                  FirstHazardBurn->Spec.GetSetByCallerMagnitude(
                      GAS_SETBYCALLER_STATUS_DAMAGE, false, 0.0f),
                  FirstHazardAggregate);
        TestTrue(TEXT("the second hazard retains its own source ASC"),
                 SecondHazardBurn->Spec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent() != HazardASC);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusDamageContentContractTest,
    "Mythic.Combat.StatusDamagePresentation.ContentContract",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusDamageContentContractTest::RunTest(const FString &Parameters) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    UMythicStatusEffectLibrary *Library = Settings ? Settings->StatusEffectLibrary.LoadSynchronous() : nullptr;
    if (!TestNotNull(TEXT("status library loads"), Library)) {
        return false;
    }

    const FGameplayTag StaleHealingCue = UGameplayTagsManager::Get().RequestGameplayTag(
        FName(TEXT("GameplayCue.Ability.Heal.Received")), false);
    if (!TestTrue(TEXT("the stale healing cue tag is registered, so the contract can detect it"),
                  StaleHealingCue.IsValid())) {
        return false;
    }

    const TCHAR *DamagingStatusNames[] = {
        TEXT("Status.Type.Burn"),
        TEXT("Status.Type.Bleed"),
        TEXT("Status.Type.Poison"),
    };

    for (const TCHAR *StatusName : DamagingStatusNames) {
        const FGameplayTag StatusTag = FGameplayTag::RequestGameplayTag(FName(StatusName), false);
        const UMythicStatusEffectDefinition *Definition = nullptr;
        for (const UMythicStatusEffectDefinition *Candidate : Library->Statuses) {
            if (Candidate && Candidate->StatusType == StatusTag) {
                Definition = Candidate;
                break;
            }
        }

        if (!TestNotNull(*FString::Printf(TEXT("%s resolves from the status library"), StatusName), Definition)) {
            continue;
        }
        const UGameplayEffect *Effect = Definition->EffectToApply
                                            ? GetDefault<UGameplayEffect>(Definition->EffectToApply)
                                            : nullptr;
        if (!TestNotNull(*FString::Printf(TEXT("%s resolves its GameplayEffect"), StatusName), Effect)) {
            continue;
        }

        const float Period = Effect->Period.GetValueAtLevel(1.0f);
        TestTrue(*FString::Printf(TEXT("%s has a finite positive tick period"), StatusName),
                 FMath::IsFinite(Period) && Period > 0.0f);
        TestFalse(*FString::Printf(TEXT("%s does not tick immediately on application"), StatusName),
                  Effect->bExecutePeriodicEffectOnApplication);
        TestTrue(*FString::Printf(TEXT("%s stacks independently for each source"), StatusName),
                 Effect->GetStackingType() == EGameplayEffectStackingType::AggregateBySource);
        TestTrue(*FString::Printf(TEXT("%s reapplication cannot reset and starve the tick cadence"), StatusName),
                 Effect->StackPeriodResetPolicy == EGameplayEffectStackingPeriodPolicy::NeverReset);
        TestEqual(*FString::Printf(TEXT("%s has the authored three-stack cap"), StatusName),
                  Effect->StackLimitCount, 3);
        TestFalse(*FString::Printf(TEXT("%s stores aggregate rolled damage without multiplying stack count twice"), StatusName),
                  Effect->bFactorInStackCount);
        TestTrue(*FString::Printf(TEXT("%s owns no stale or presentation-coupled GameplayCues"), StatusName),
                 Effect->GameplayCues.IsEmpty());

        const FLinearColor &DisplayColor = Definition->DisplayColor;
        TestTrue(*FString::Printf(TEXT("%s has a finite canonical presentation color"), StatusName),
                 FMath::IsFinite(DisplayColor.R)
                     && FMath::IsFinite(DisplayColor.G)
                     && FMath::IsFinite(DisplayColor.B)
                     && FMath::IsFinite(DisplayColor.A));

        bool bHasStaleHealingCue = false;
        for (const FGameplayEffectCue &Cue : Effect->GameplayCues) {
            bHasStaleHealingCue |= Cue.GameplayCueTags.HasTagExact(StaleHealingCue);
        }
        TestFalse(*FString::Printf(TEXT("%s has no stale healing GameplayCue"), StatusName),
                  bHasStaleHealingCue);
    }
    return true;
}
