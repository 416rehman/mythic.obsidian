#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Itemization/Affixes/MythicPermanentStatLedger.h"
#include "Misc/ScopeExit.h"

namespace {
FMythicPermanentStatContribution MakeContribution(
    const FGuid RollGuid,
    const EGameplayModOp::Type ModifierOp,
    const float Magnitude) {
    FMythicPermanentStatContribution Result;
    Result.SourceGuid = RollGuid;
    Result.ModifierOp = ModifierOp;
    Result.Magnitude = Magnitude;
    return Result;
}

FMythicPermanentStatContribution MakeAttributedContribution(
    const FGuid SourceGuid,
    const FGameplayAttribute &Attribute,
    const EGameplayModOp::Type ModifierOp,
    const float Magnitude,
    const EMythicPermanentStatContributionLayer Layer) {
    FMythicPermanentStatContribution Result = MakeContribution(
        SourceGuid, ModifierOp, Magnitude);
    Result.Attribute = Attribute;
    Result.Layer = Layer;
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPermanentStatLedgerCompositionTest,
    "Mythic.Itemization.Affixes.PermanentStatLedger.GASComposition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPermanentStatLedgerCompositionTest::RunTest(const FString &Parameters) {
    const FGuid A(1, 0, 0, 0);
    const FGuid B(2, 0, 0, 0);
    const FGuid C(3, 0, 0, 0);
    const FGuid D(4, 0, 0, 0);
    const FGuid E(5, 0, 0, 0);
    const FGuid F(6, 0, 0, 0);
    const FGuid G(7, 0, 0, 0);
    const FGuid H(8, 0, 0, 0);
    const FGuid I(9, 0, 0, 0);

    const TArray<FMythicPermanentStatContribution> Contributions{
        MakeContribution(H, EGameplayModOp::AddFinal, 3.0f),
        MakeContribution(D, EGameplayModOp::MultiplyAdditive, 1.2f),
        MakeContribution(A, EGameplayModOp::AddBase, 20.0f),
        MakeContribution(G, EGameplayModOp::MultiplyCompound, 0.8f),
        MakeContribution(E, EGameplayModOp::DivideAdditive, 2.0f),
        MakeContribution(B, EGameplayModOp::AddBase, 5.0f),
        MakeContribution(F, EGameplayModOp::DivideAdditive, 1.5f),
        MakeContribution(C, EGameplayModOp::MultiplyAdditive, 1.5f),
        MakeContribution(I, EGameplayModOp::MultiplyCompound, 1.1f),
    };

    float Composed = 0.0f;
    TestTrue(TEXT("all six non-override GAS channels compose"),
             FMythicPermanentStatLedger::Compose(100.0f, Contributions, Composed));
    TestTrue(TEXT("composition matches ((base+add)*additive/divide*compound)+final"),
             FMath::IsNearlyEqual(Composed, 77.8f, UE_KINDA_SMALL_NUMBER));

    const TArray<FMythicPermanentStatContribution> ZeroDivisor{
        MakeContribution(A, EGameplayModOp::DivideAdditive, 0.5f),
        MakeContribution(B, EGameplayModOp::DivideAdditive, 0.5f),
    };
    TestTrue(TEXT("near-zero additive divisor follows GAS fallback"),
             FMythicPermanentStatLedger::Compose(10.0f, ZeroDivisor, Composed));
    TestEqual(TEXT("GAS substitutes one for a near-zero divisor"), Composed, 10.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPermanentStatLedgerOverrideTest,
    "Mythic.Itemization.Affixes.PermanentStatLedger.DeterministicOverride",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPermanentStatLedgerOverrideTest::RunTest(const FString &Parameters) {
    const TArray<FMythicPermanentStatContribution> Forward{
        MakeContribution(FGuid(2, 0, 0, 0), EGameplayModOp::Override, 222.0f),
        MakeContribution(FGuid(1, 0, 0, 0), EGameplayModOp::Override, 111.0f),
        MakeContribution(FGuid(3, 0, 0, 0), EGameplayModOp::AddFinal, 999.0f),
    };
    TArray<FMythicPermanentStatContribution> Reverse = Forward;
    Algo::Reverse(Reverse);

    float ForwardValue = 0.0f;
    float ReverseValue = 0.0f;
    TestTrue(TEXT("forward override composition succeeds"),
             FMythicPermanentStatLedger::Compose(10.0f, Forward, ForwardValue));
    TestTrue(TEXT("reverse override composition succeeds"),
             FMythicPermanentStatLedger::Compose(10.0f, Reverse, ReverseValue));
    TestEqual(TEXT("lexically first roll deterministically wins Override"), ForwardValue, 111.0f);
    TestEqual(TEXT("input order cannot change Override winner"), ReverseValue, ForwardValue);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPermanentStatLedgerSourceLayerTest,
    "Mythic.Itemization.Affixes.PermanentStatLedger.SourceLayers",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPermanentStatLedgerSourceLayerTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AActor *Owner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("authoritative owner spawned"), Owner)) {
        return false;
    }
    UMythicAbilitySystemComponent *AbilitySystem =
        NewObject<UMythicAbilitySystemComponent>(Owner);
    AbilitySystem->RegisterComponent();
    AbilitySystem->InitAbilityActorInfo(Owner, Owner);
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Defense>(Owner));
    if (!TestTrue(TEXT("ability system answers as authority"),
                  AbilitySystem->IsOwnerActorAuthoritative())) {
        return false;
    }

    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();
    AbilitySystem->SetNumericAttributeBase(Armor, 100.0f);

    const FGuid ProgressionSource(1, 2, 3, 4);
    const FGuid EquipmentSource(5, 6, 7, 8);
    TArray<FMythicPermanentStatContribution> Desired{
        // AddFinal makes the layer boundary observable: progression yields 110, then equipment doubles to 220.
        // A single flattened GAS channel would instead produce 210.
        MakeAttributedContribution(
            ProgressionSource, Armor, EGameplayModOp::AddFinal, 10.0f,
            EMythicPermanentStatContributionLayer::Progression),
        MakeAttributedContribution(
            EquipmentSource, Armor, EGameplayModOp::MultiplyAdditive, 2.0f,
            EMythicPermanentStatContributionLayer::Equipment),
    };

    FMythicPermanentStatLedger Ledger;
    FMythicPermanentStatReconcileResult Result;
    TestTrue(TEXT("typed progression and equipment sources reconcile"),
             Ledger.ReconcileTransactional(*AbilitySystem, Desired, Result));
    TestTrue(TEXT("successful reconcile reports success"), Result.bSucceeded);
    TestTrue(TEXT("progression is composed before equipment"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 220.0f));

    TArray<FMythicPermanentStatLayerSnapshot> Snapshots;
    Ledger.GetLayerSnapshots(Snapshots);
    TestEqual(TEXT("one attribute is tracked"), Snapshots.Num(), 1);
    if (Snapshots.Num() == 1) {
        TestTrue(TEXT("snapshot exposes the post-progression base"),
                 FMath::IsNearlyEqual(Snapshots[0].NonEquipmentBase, 110.0f));
        TestTrue(TEXT("snapshot exposes the post-equipment base"),
                 FMath::IsNearlyEqual(Snapshots[0].EquipmentBase, 220.0f));
    }

    Algo::Reverse(Desired);
    TestTrue(TEXT("reconciling the same sources in another order succeeds"),
             Ledger.ReconcileTransactional(*AbilitySystem, Desired, Result));
    TestTrue(TEXT("source reconciliation is idempotent and order independent"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 220.0f));

    FMythicPermanentStatContribution *Progression = Desired.FindByPredicate(
        [ProgressionSource](const FMythicPermanentStatContribution &Contribution) {
            return Contribution.SourceGuid == ProgressionSource;
        });
    if (!TestNotNull(TEXT("progression source remains addressable"), Progression)) {
        return false;
    }
    Progression->Magnitude = 15.0f;
    TestTrue(TEXT("replacing one source updates rather than stacks it"),
             Ledger.ReconcileTransactional(*AbilitySystem, Desired, Result));
    TestTrue(TEXT("replacement recomposes from the retained baseline"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 230.0f));

    TArray<FMythicPermanentStatContribution> DuplicateIdentity = Desired;
    FMythicPermanentStatContribution Duplicate = DuplicateIdentity[0];
    Duplicate.SourceGuid = DuplicateIdentity[1].SourceGuid;
    DuplicateIdentity.Add(Duplicate);
    TestFalse(TEXT("duplicate source identity rejects the whole transaction"),
              Ledger.ReconcileTransactional(
                  *AbilitySystem, DuplicateIdentity, Result));
    TestTrue(TEXT("duplicate rejection leaves the live base unchanged"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 230.0f));

    AbilitySystem->SetNumericAttributeBase(Armor, 777.0f);
    TestFalse(TEXT("out-of-band mutation is rejected instead of absorbed"),
              Ledger.ReconcileTransactional(*AbilitySystem, Desired, Result));
    TestTrue(TEXT("drift rejection does not overwrite the external mutation"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 777.0f));

    // Restoring the last ledger-owned value permits a controlled clear, which proves the separately retained
    // pre-source baseline survived every replacement and rejected transaction above.
    AbilitySystem->SetNumericAttributeBase(Armor, 230.0f);
    TestTrue(TEXT("clearing the complete typed source set succeeds"),
             Ledger.ClearTransactional(*AbilitySystem, Result));
    TestTrue(TEXT("clear restores the original pre-source baseline"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Armor), 100.0f));
    TestTrue(TEXT("clear releases all ledger bookkeeping"), Ledger.IsEmpty());
    return true;
}

#endif
