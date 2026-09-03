
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicCombatSettings.h"

namespace {

struct FMythicHitFixture {
    UGameInstance *GameInstance = nullptr;
    UMythicAbilitySystemComponent *SourceASC = nullptr;
    UMythicAbilitySystemComponent *TargetASC = nullptr;
    UGameplayEffect *Damage = nullptr;
};

// Two bare actors and the real damage execution, so nothing sits between the context and the number the target
// reports. Dodge and shield are pinned to zero: either would swallow a hit and make the ratio meaningless.
bool BuildHitFixture(FAutomationTestBase &Test, FMythicHitFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AActor *Source = World->SpawnActor<AActor>();
    Out.SourceASC = NewObject<UMythicAbilitySystemComponent>(Source);
    Out.SourceASC->RegisterComponent();
    Out.SourceASC->InitAbilityActorInfo(Source, Source);
    Out.SourceASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Source));
    Out.SourceASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetPowerAttribute(), 1.0f);
    Out.SourceASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), 10.0f);

    AActor *Target = World->SpawnActor<AActor>();
    Out.TargetASC = NewObject<UMythicAbilitySystemComponent>(Target);
    Out.TargetASC->RegisterComponent();
    Out.TargetASC->InitAbilityActorInfo(Target, Target);
    Out.TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Target));
    Out.TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Target));
    Out.TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 1000.0f);
    Out.TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 1000.0f);
    Out.TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetMaxShieldAttribute(), 0.0f);
    Out.TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), 0.0f);
    Out.TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetDodgeChanceAttribute(), 0.0f);

    Out.Damage = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_RuneHitMultiplier")));
    Out.Damage->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayEffectExecutionDefinition ExecDef;
    ExecDef.CalculationClass = UMythicDamageApplication::StaticClass();
    Out.Damage->Executions.Add(ExecDef);
    return true;
}

// Lands one hit through the execution and returns the damage the target reported on Dmg.Received.
float DealHit(FMythicHitFixture &Fixture, const FGameplayEffectContextHandle &Context) {
    float Received = 0.0f;
    const FDelegateHandle Handle =
        Fixture.TargetASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddLambda(
            [&Received](const FGameplayEventData *Payload) {
                Received = Payload ? Payload->EventMagnitude : 0.0f;
            });
    Fixture.SourceASC->ApplyGameplayEffectToTarget(Fixture.Damage, Fixture.TargetASC, 1.0f, Context);
    Fixture.TargetASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).Remove(Handle);
    return Received;
}

// The same bit archives replication uses, with no package map: the context under test carries no actors.
bool RoundTripContext(const FMythicGameplayEffectContext &Source, FMythicGameplayEffectContext &OutLoaded, int64 &OutBits) {
    FBitWriter Writer(4096, true);
    FMythicGameplayEffectContext Saved = Source;
    bool bSaved = false;
    Saved.NetSerialize(Writer, nullptr, bSaved);
    if (!bSaved || Writer.IsError()) {
        return false;
    }
    OutBits = Writer.GetNumBits();

    FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
    bool bLoaded = false;
    OutLoaded.NetSerialize(Reader, nullptr, bLoaded);
    return bLoaded && !Reader.IsError();
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHitMultiplierExecutionTest,
    "Mythic.GAS.RuneHit.MultiplierScalesExecution",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHitMultiplierExecutionTest::RunTest(const FString &Parameters) {
    FMythicHitFixture Fixture;
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!BuildHitFixture(*this, Fixture)) {
        return false;
    }

    // The weapon roll is uniform over [DamagePerHit, DamagePerHit x Max]. Pinning Max to 1 makes both hits roll
    // the same number, so the ratio between them is the multiplier and nothing else.
    TGuardValue<float> FlatRoll(GetMutableDefault<UMythicCombatSettings>()->WeaponDamageMaximumMultiplier, 1.0f);

    FGameplayEffectContextHandle Plain = Fixture.SourceASC->MakeEffectContext();
    FGameplayEffectContextHandle Empowered = Fixture.SourceASC->MakeEffectContext();
    if (!TestNotNull(TEXT("the effect context is a Mythic context - the execution aborts on anything else"),
                     FMythicGameplayEffectContext::ExtractEffectContext(Plain))) {
        return false;
    }
    TestEqual(TEXT("a fresh context is neutral"),
              UMythicGameplayEffectContextLibrary::GetBonusDamageMultiplier(Plain), 1.0f);

    // Through the Blueprint wrappers, handle by value: it shares the context, so the write has to land on the
    // object the execution reads.
    UMythicGameplayEffectContextLibrary::SetBonusDamageMultiplier(Empowered, 6.0f);
    UMythicGameplayEffectContextLibrary::AddHitTag(Empowered, GAS_HIT_RUNE_FIRSTSTRIKE);
    TestEqual(TEXT("the by-value handle wrote through to the shared context"),
              UMythicGameplayEffectContextLibrary::GetBonusDamageMultiplier(Empowered), 6.0f);
    TestTrue(TEXT("the hit tag rides the same context"),
             FMythicGameplayEffectContext::ExtractEffectContext(Empowered)->GetHitTags().HasTagExact(GAS_HIT_RUNE_FIRSTSTRIKE));

    const float HealthBefore = Fixture.TargetASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
    const float Baseline = DealHit(Fixture, Plain);
    const float Boosted = DealHit(Fixture, Empowered);
    const float HealthAfter = Fixture.TargetASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());

    if (!TestTrue(TEXT("the baseline hit landed"), Baseline > 0.0f)) {
        return false;
    }
    TestEqual(TEXT("a x6 context deals six times the identical hit"), Boosted, Baseline * 6.0f, Baseline * 0.001f);
    TestEqual(TEXT("both hits reached health, not only the event"), HealthBefore - HealthAfter, Baseline + Boosted, 0.01f);

    AddInfo(FString::Printf(TEXT("baseline %.3f, empowered %.3f, ratio %.4f, health %.1f -> %.1f"),
                            Baseline, Boosted, Baseline > 0.0f ? Boosted / Baseline : 0.0f, HealthBefore, HealthAfter));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHitContextNetSerializeTest,
    "Mythic.GAS.RuneHit.ContextNetSerializeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHitContextNetSerializeTest::RunTest(const FString &Parameters) {
    // Fast tag replication ships a net index, so an unregistered tag would come back as None and the round trip
    // below would pass for the wrong reason. IsValid() alone does not prove registration.
    if (!TestTrue(TEXT("the first-strike hit tag is registered"),
                  UGameplayTagsManager::Get().RequestGameplayTag(GAS_HIT_RUNE_FIRSTSTRIKE.GetTag().GetTagName(), false).IsValid())) {
        return false;
    }

    FMythicGameplayEffectContext Source;
    Source.SetCriticalHit(true);
    Source.SetBonusDamageMultiplier(6.0f);
    Source.AddHitTag(GAS_HIT_RUNE_FIRSTSTRIKE);

    FMythicGameplayEffectContext Loaded;
    int64 EmpoweredBits = 0;
    if (!TestTrue(TEXT("an empowered context round-trips"), RoundTripContext(Source, Loaded, EmpoweredBits))) {
        return false;
    }
    TestEqual(TEXT("the multiplier survives quantisation"), Loaded.GetBonusDamageMultiplier(), 6.0f, 0.001f);
    TestTrue(TEXT("the hit tag survives"), Loaded.GetHitTags().HasTagExact(GAS_HIT_RUNE_FIRSTSTRIKE));
    TestEqual(TEXT("and no tag was invented"), Loaded.GetHitTags().Num(), 1);
    TestTrue(TEXT("the flags ahead of it still read"), Loaded.IsCriticalHit());

    FMythicGameplayEffectContext Fractional;
    Fractional.SetBonusDamageMultiplier(1.15f);
    FMythicGameplayEffectContext FractionalLoaded;
    int64 FractionalBits = 0;
    if (TestTrue(TEXT("a fractional multiplier round-trips"), RoundTripContext(Fractional, FractionalLoaded, FractionalBits))) {
        TestEqual(TEXT("the wire carries more than whole numbers"), FractionalLoaded.GetBonusDamageMultiplier(), 1.15f, 0.001f);
        TestTrue(TEXT("no hit tag appears from nowhere"), FractionalLoaded.GetHitTags().IsEmpty());
    }

    // An ordinary hit must cost what it did before: the flag stays clear and the rune fields spend no bits.
    FMythicGameplayEffectContext Neutral;
    FMythicGameplayEffectContext NeutralLoaded;
    NeutralLoaded.SetBonusDamageMultiplier(3.0f);
    NeutralLoaded.AddHitTag(GAS_HIT_RUNE_FIRSTSTRIKE);
    int64 NeutralBits = 0;
    if (TestTrue(TEXT("a neutral context round-trips"), RoundTripContext(Neutral, NeutralLoaded, NeutralBits))) {
        TestEqual(TEXT("loading a neutral context resets the multiplier"), NeutralLoaded.GetBonusDamageMultiplier(), 1.0f);
        TestTrue(TEXT("and clears stale hit tags"), NeutralLoaded.GetHitTags().IsEmpty());
        TestTrue(TEXT("a neutral context is smaller than an empowered one"), NeutralBits < EmpoweredBits);
    }

    AddInfo(FString::Printf(TEXT("neutral %lld bits, fractional %lld bits, empowered %lld bits"),
                            NeutralBits, FractionalBits, EmpoweredBits));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHitContextDuplicateTest,
    "Mythic.GAS.RuneHit.DuplicatePreservesFields",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHitContextDuplicateTest::RunTest(const FString &Parameters) {
    FMythicGameplayEffectContext *Original = new FMythicGameplayEffectContext();
    Original->SetBonusDamageMultiplier(6.0f);
    Original->AddHitTag(GAS_HIT_RUNE_FIRSTSTRIKE);
    const FGameplayEffectContextHandle Handle(Original);

    // ApplyDamageContainerSpec duplicates the swing's context once per target. A rune that wrote into the shared
    // context on Dmg.Pre has to reach every copy.
    const FGameplayEffectContextHandle CopyHandle = Handle.Duplicate();
    FMythicGameplayEffectContext *Copy = FMythicGameplayEffectContext::ExtractEffectContext(CopyHandle);
    if (!TestNotNull(TEXT("the duplicate is still a Mythic context"), Copy)) {
        return false;
    }
    TestTrue(TEXT("the duplicate is a new object"), Copy != Original);
    TestEqual(TEXT("the multiplier was copied"), Copy->GetBonusDamageMultiplier(), 6.0f);
    TestTrue(TEXT("the hit tag was copied"), Copy->GetHitTags().HasTagExact(GAS_HIT_RUNE_FIRSTSTRIKE));

    // Per-target results are written after the split. They must stay with their copy.
    Copy->SetBonusDamageMultiplier(1.0f);
    Copy->AddHitTag(GAS_HIT_CRITICAL);
    TestEqual(TEXT("writing the copy leaves the original's multiplier alone"), Original->GetBonusDamageMultiplier(), 6.0f);
    TestFalse(TEXT("and its hit tags"), Original->GetHitTags().HasTagExact(GAS_HIT_CRITICAL));

    // A plain GAS context is not a Mythic one; the wrappers must neither crash nor invent a multiplier.
    const FGameplayEffectContextHandle PlainHandle(new FGameplayEffectContext());
    UMythicGameplayEffectContextLibrary::SetBonusDamageMultiplier(PlainHandle, 6.0f);
    UMythicGameplayEffectContextLibrary::AddHitTag(PlainHandle, GAS_HIT_RUNE_FIRSTSTRIKE);
    TestEqual(TEXT("a non-Mythic context reads as neutral"),
              UMythicGameplayEffectContextLibrary::GetBonusDamageMultiplier(PlainHandle), 1.0f);
    TestEqual(TEXT("an empty handle reads as neutral"),
              UMythicGameplayEffectContextLibrary::GetBonusDamageMultiplier(FGameplayEffectContextHandle()), 1.0f);

    return true;
}
