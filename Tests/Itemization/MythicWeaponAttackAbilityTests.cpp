#include "Misc/AutomationTest.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Actor.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Resources/MythicResourceISM.h"
#include "Components/StaticMeshComponent.h"
#include "Tests/Itemization/MythicWeaponAttackTestTypes.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackTypedConfigTest,
    "Mythic.Itemization.WeaponAttackAbility.TypedLiveConfiguration",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackTypedConfigTest::RunTest(const FString &Parameters) {
    const FProperty *AttackConfigProperty = FindFProperty<FProperty>(
        UAttackFragment::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UAttackFragment, AttackConfig));
    TestNotNull(TEXT("AttackConfig remains reflected"), AttackConfigProperty);
    if (AttackConfigProperty) {
        TestTrue(TEXT("AttackConfig remains owner-replicated"),
                 AttackConfigProperty->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("immutable AttackConfig is not revived from a save"),
                  AttackConfigProperty->HasAnyPropertyFlags(CPF_SaveGame));
        TestTrue(TEXT("Blueprint can inspect AttackConfig"),
                 AttackConfigProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
        TestTrue(TEXT("Blueprint cannot mutate AttackConfig"),
                 AttackConfigProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
    }

    const UScriptStruct *ConfigStruct = FAttackConfig::StaticStruct();
    const FClassProperty *TriggerAbilityProperty =
        FindFProperty<FClassProperty>(
            ConfigStruct, GET_MEMBER_NAME_CHECKED(FAttackConfig, TriggerAbility));
    TestNotNull(TEXT("TriggerAbility remains reflected"), TriggerAbilityProperty);
    if (TriggerAbilityProperty) {
        TestEqual(TEXT("TriggerAbility accepts only the sealed weapon base"),
                  TriggerAbilityProperty->MetaClass.Get(),
                  UMythicWeaponAttackAbility::StaticClass());
        TestFalse(TEXT("TriggerAbility is live definition configuration"),
                  TriggerAbilityProperty->HasAnyPropertyFlags(CPF_SaveGame));
        TestTrue(TEXT("Blueprint cannot replace TriggerAbility at runtime"),
                 TriggerAbilityProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
    }

    const FProperty *MontageProperty = FindFProperty<FProperty>(
        ConfigStruct, GET_MEMBER_NAME_CHECKED(FAttackConfig, AttackMontage));
    TestNotNull(TEXT("AttackMontage remains reflected"), MontageProperty);
    if (MontageProperty) {
        TestFalse(TEXT("AttackMontage is live definition configuration"),
                  MontageProperty->HasAnyPropertyFlags(CPF_SaveGame));
        TestTrue(TEXT("Blueprint cannot replace AttackMontage at runtime"),
                 MontageProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
    }

    const UMythicWeaponAttackAbility *AbilityCDO =
        GetDefault<UMythicWeaponAttackAbility>();
    TestEqual(TEXT("weapon attacks are locally predicted"),
              AbilityCDO->GetNetExecutionPolicy(),
              EGameplayAbilityNetExecutionPolicy::LocalPredicted);
    TestEqual(TEXT("the server exclusively owns attack termination"),
              AbilityCDO->GetNetSecurityPolicy(),
              EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination);
    TestEqual(TEXT("one reusable instance owns each active attack"),
              AbilityCDO->GetInstancingPolicy(),
              EGameplayAbilityInstancingPolicy::InstancedPerActor);
    TestEqual(TEXT("weapon attacks activate only from their bound input"),
              AbilityCDO->GetActivationPolicy(),
              EMythicAbilityActivationPolicy::OnInputTriggered);
    TestEqual(TEXT("weapon attacks preserve the complete authored recovery cycle"),
              AbilityCDO->GetActivationGroup(),
              EMythicAbilityActivationGroup::Exclusive_Blocking);

    const FBoolProperty *RetriggerProperty = FindFProperty<FBoolProperty>(
        UGameplayAbility::StaticClass(), TEXT("bRetriggerInstancedAbility"));
    const FBoolProperty *RemoteCancelProperty = FindFProperty<FBoolProperty>(
        UGameplayAbility::StaticClass(),
        TEXT("bServerRespectsRemoteAbilityCancellation"));
    TestNotNull(TEXT("retrigger policy remains reflected"), RetriggerProperty);
    TestNotNull(TEXT("remote cancellation policy remains reflected"),
                RemoteCancelProperty);
    if (RetriggerProperty) {
        TestFalse(TEXT("input spam cannot restart an active section"),
                  RetriggerProperty->GetPropertyValue_InContainer(AbilityCDO));
    }
    if (RemoteCancelProperty) {
        TestFalse(TEXT("a client cannot release-to-cancel server cadence"),
                  RemoteCancelProperty->GetPropertyValue_InContainer(AbilityCDO));
    }
    const FArrayProperty *TriggerArrayProperty = FindFProperty<FArrayProperty>(
        UGameplayAbility::StaticClass(), TEXT("AbilityTriggers"));
    TestNotNull(TEXT("ability trigger policy remains reflected"),
                TriggerArrayProperty);
    if (TriggerArrayProperty) {
        const FScriptArrayHelper_InContainer TriggerArray(TriggerArrayProperty,
                                                           AbilityCDO);
        TestEqual(TEXT("external begin/end events cannot bypass input cadence"),
                  TriggerArray.Num(), 0);
    }

    const FClassProperty *CostEffectProperty = FindFProperty<FClassProperty>(
        UGameplayAbility::StaticClass(), TEXT("CostGameplayEffectClass"));
    const FClassProperty *CooldownEffectProperty = FindFProperty<FClassProperty>(
        UGameplayAbility::StaticClass(), TEXT("CooldownGameplayEffectClass"));
    const FArrayProperty *AdditionalCostsProperty = FindFProperty<FArrayProperty>(
        UMythicGameplayAbility::StaticClass(), TEXT("AdditionalCosts"));
    const FIntProperty *MaxChargesProperty = FindFProperty<FIntProperty>(
        UMythicGameplayAbility::StaticClass(), TEXT("MaxCharges"));
    const FFloatProperty *RechargeSecondsProperty = FindFProperty<FFloatProperty>(
        UMythicGameplayAbility::StaticClass(), TEXT("RechargeSeconds"));
    const FStructProperty *CooldownCategoryProperty = FindFProperty<FStructProperty>(
        UMythicGameplayAbility::StaticClass(), TEXT("CooldownCategoryTag"));

    TestNotNull(TEXT("engine cost configuration remains reflected"),
                CostEffectProperty);
    TestNotNull(TEXT("engine cooldown configuration remains reflected"),
                CooldownEffectProperty);
    TestNotNull(TEXT("Mythic additional costs remain reflected"),
                AdditionalCostsProperty);
    TestNotNull(TEXT("Mythic charge count remains reflected"),
                MaxChargesProperty);
    TestNotNull(TEXT("Mythic recharge time remains reflected"),
                RechargeSecondsProperty);
    TestNotNull(TEXT("Mythic shared cooldown category remains reflected"),
                CooldownCategoryProperty);

    if (CostEffectProperty) {
        TestNull(TEXT("weapon attacks cannot author a gameplay-effect cost"),
                 CostEffectProperty->GetPropertyValue_InContainer(AbilityCDO));
    }
    if (CooldownEffectProperty) {
        TestNull(TEXT("montage cadence cannot acquire a second GE cooldown"),
                 CooldownEffectProperty->GetPropertyValue_InContainer(AbilityCDO));
    }
    if (AdditionalCostsProperty) {
        const FScriptArrayHelper_InContainer AdditionalCosts(
            AdditionalCostsProperty, AbilityCDO);
        TestEqual(TEXT("weapon attacks cannot author hidden additional costs"),
                  AdditionalCosts.Num(), 0);
    }
    if (MaxChargesProperty) {
        TestEqual(TEXT("weapon attacks cannot acquire a second charge cadence"),
                  MaxChargesProperty->GetPropertyValue_InContainer(AbilityCDO),
                  1);
    }
    if (RechargeSecondsProperty) {
        TestEqual(TEXT("weapon attacks cannot acquire a recharge timer"),
                  RechargeSecondsProperty->GetPropertyValue_InContainer(
                      AbilityCDO),
                  0.0f);
    }
    if (CooldownCategoryProperty) {
        const FGameplayTag *CooldownCategory =
            CooldownCategoryProperty->ContainerPtrToValuePtr<FGameplayTag>(
                AbilityCDO);
        TestTrue(TEXT("weapon attacks cannot gate a shared cooldown category"),
                 CooldownCategory && !CooldownCategory->IsValid());
    }

    TestNull(TEXT("native cost lookup is permanently disabled"),
             AbilityCDO->GetCostGameplayEffect());
    TestNull(TEXT("native cooldown lookup is permanently disabled"),
             AbilityCDO->GetCooldownGameplayEffect());
    TestTrue(TEXT("native cost checks cannot introduce a second attack gate"),
             AbilityCDO->CheckCost(FGameplayAbilitySpecHandle(), nullptr));
    TestTrue(TEXT("native cooldown checks cannot introduce a second attack gate"),
             AbilityCDO->CheckCooldown(FGameplayAbilitySpecHandle(), nullptr));
    TestFalse(TEXT("input-only weapon attacks never opt into event activation"),
              AbilityCDO->ShouldAbilityRespondToEvent(nullptr, nullptr));

    FText CanonicalError;
    TestTrue(TEXT("the native CDO satisfies the complete sealed policy"),
             AbilityCDO->IsCanonicalWeaponAttackConfiguration(
                 &CanonicalError));
    if (!CanonicalError.IsEmpty()) {
        AddError(FString::Printf(TEXT("Unexpected canonical-policy error: %s"),
                                 *CanonicalError.ToString()));
    }

    FName ForbiddenLifecycleFunction = NAME_None;
    TestFalse(TEXT("the native class does not implement a forbidden BP hook"),
              UMythicWeaponAttackAbility::HasForbiddenBlueprintHookOverride(
                  UMythicWeaponAttackAbility::StaticClass(),
                  &ForbiddenLifecycleFunction));
    TestTrue(TEXT("no forbidden hook is reported for the native class"),
             ForbiddenLifecycleFunction.IsNone());

#if WITH_EDITOR
    const FString HiddenOverrides =
        UMythicWeaponAttackAbility::StaticClass()->GetMetaData(
            TEXT("KismetHideOverrides"));
    const TCHAR *ForbiddenBlueprintHooks[] = {
        TEXT("K2_ShouldAbilityRespondToEvent"),
        TEXT("K2_CanActivateAbility"),
        TEXT("K2_ActivateAbility"),
        TEXT("K2_ActivateAbilityFromEvent"),
        TEXT("K2_CommitExecute"),
        TEXT("K2_OnEndAbility"),
        TEXT("BP_EditSpecValues"),
        TEXT("K2_OnAbilityAdded"),
        TEXT("K2_OnAbilityRemoved"),
        TEXT("K2_OnAvatarSet")};
    for (const TCHAR *ForbiddenBlueprintHook : ForbiddenBlueprintHooks) {
        TestTrue(
            *FString::Printf(TEXT("%s is hidden from Blueprint Overrides"),
                             ForbiddenBlueprintHook),
            HiddenOverrides.Contains(ForbiddenBlueprintHook));
    }

    const FProperty *SealedProperties[] = {
        CostEffectProperty,
        CooldownEffectProperty,
        AdditionalCostsProperty,
        MaxChargesProperty,
        RechargeSecondsProperty,
        CooldownCategoryProperty};
    for (const FProperty *SealedProperty : SealedProperties) {
        if (SealedProperty) {
            TestFalse(
                *FString::Printf(TEXT("%s is locked in weapon attack Details"),
                                 *SealedProperty->GetName()),
                AbilityCDO->CanEditChange(SealedProperty));
        }
    }
#endif
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackSingleImpactGateTest,
    "Mythic.Itemization.WeaponAttackAbility.OneImpactPerSection",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackSingleImpactGateTest::RunTest(
    const FString &Parameters) {
    UMythicAnimNotify_SphereOverlap *ExpectedNotify =
        NewObject<UMythicAnimNotify_SphereOverlap>(GetTransientPackage());
    UMythicAnimNotify_SphereOverlap *ForeignNotify =
        NewObject<UMythicAnimNotify_SphereOverlap>(GetTransientPackage());
    const UObject *ExpectedActivation =
        NewObject<UAttackFragment>(GetTransientPackage());
    const UObject *ForeignActivation =
        NewObject<UAttackFragment>(GetTransientPackage());
    TArray<const UMythicAnimNotify_SphereOverlap *> AuthorizedSamples{
        ExpectedNotify};
    FGameplayEventData Event;
    Event.EventTag = GAS_EVENT_HITBOX;
    Event.OptionalObject = ForeignNotify;
    Event.OptionalObject2 = ExpectedActivation;

    bool bConsumed = false;
    TestFalse(TEXT("foreign same-tag traffic is ignored"),
              UMythicWeaponAttackAbility::TryConsumeExpectedHitEvent(
                  AuthorizedSamples, ExpectedActivation, Event, bConsumed));
    TestFalse(TEXT("foreign traffic does not spend the section hit"), bConsumed);

    Event.OptionalObject = ExpectedNotify;
    Event.OptionalObject2 = ForeignActivation;
    TestFalse(TEXT("another activation cannot consume this section's sample"),
              UMythicWeaponAttackAbility::TryConsumeExpectedHitEvent(
                  AuthorizedSamples, ExpectedActivation, Event, bConsumed));
    TestFalse(TEXT("foreign activation identity does not spend the section hit"),
              bConsumed);

    Event.OptionalObject2 = ExpectedActivation;
    TestTrue(TEXT("the first exact section impact is consumed"),
             UMythicWeaponAttackAbility::TryConsumeExpectedHitEvent(
                 AuthorizedSamples, ExpectedActivation, Event, bConsumed));
    TestTrue(TEXT("claiming the first impact records consumption"), bConsumed);
    TestFalse(TEXT("a later authorized sample cannot spend a second damage budget"),
              UMythicWeaponAttackAbility::TryConsumeExpectedHitEvent(
                  AuthorizedSamples, ExpectedActivation, Event, bConsumed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackSourceDomainTest,
    "Mythic.Itemization.WeaponAttackAbility.ExactSourceAndTargetDomains",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackSourceDomainTest::RunTest(
    const FString &Parameters) {
    using EDomain = EMythicAttackSourceDomain;

    AMythicStorageContainer *ItemOwner =
        NewObject<AMythicStorageContainer>(GetTransientPackage());
    UMythicItemInstance *Item = NewObject<UMythicItemInstance>(ItemOwner);
    Item->SetOwner(ItemOwner);
    UItemDefinition *Definition = NewObject<UItemDefinition>(ItemOwner);
    Definition->StackSizeMax = 1;
    Definition->ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD;
    Item->InitializeFixtureForTests(Definition, 1, 1);
    UAttackFragment *AttackFragment = NewObject<UAttackFragment>(Item);
    AttackFragment->SetOwnerItemInstance(Item);

    TestEqual(TEXT("a weapon child tag resolves from the granting item"),
              UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                  AttackFragment),
              EDomain::Weapon);

    Definition->ItemType = ITEMIZATION_TYPE_EQUIPMENT_TOOL_PICKAXE;
    TestEqual(TEXT("a tool resolves as its own harvest source, so a weaponless player can still work a node"),
              UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                  AttackFragment),
              EDomain::HarvestTool);

    Item->AddTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD);
    TestEqual(TEXT("an item claiming both source domains is rejected"),
              UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                  AttackFragment),
              EDomain::Invalid);
    Item->RemoveTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD);

    FGameplayTagContainer UnrelatedProbe;
    UnrelatedProbe.AddTag(ITEMIZATION_TYPE_EQUIPMENT_GEAR_HEAD);
    TestEqual(TEXT("a non-attack item domain is rejected"),
              UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                  UnrelatedProbe),
              EDomain::Invalid);
    TestEqual(TEXT("a fragment without live item provenance is rejected"),
              UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                  NewObject<UAttackFragment>(GetTransientPackage())),
              EDomain::Invalid);

    TestTrue(TEXT("weapons accept a living ASC target"),
             UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                 EDomain::Weapon, true, false));
    TestTrue(TEXT("the weapon is the only attacker, so it reaches a destructible-only target"),
             UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                 EDomain::Weapon, false, true));
    TestTrue(TEXT("weapons preserve a harvestable contact for Requires Tool feedback"),
             UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                 EDomain::Weapon, false, true, true));
    TestTrue(TEXT("a target that is both living and destructible still accepts the weapon"),
             UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                 EDomain::Weapon, true, true));
    TestFalse(TEXT("an ambiguous source can never accept a target"),
              UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                  EDomain::Invalid, true, true));

    TestTrue(TEXT("a tool reaches a harvestable node"),
             UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                 EDomain::HarvestTool, false, false, true));
    TestFalse(TEXT("a tool can never reach a living target"),
              UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                  EDomain::HarvestTool, true, false, false));
    TestFalse(TEXT("a tool can never reach a plain destructible"),
              UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                  EDomain::HarvestTool, false, true, false));
    TestFalse(TEXT("a tool contacting a living target beside a node is still refused the kill"),
              UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
                  EDomain::HarvestTool, true, true, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackRuntimeQueryGuardTest,
    "Mythic.Itemization.WeaponAttackAbility.RuntimeQueryFailsClosed",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackRuntimeQueryGuardTest::RunTest(
    const FString &Parameters) {
    const FCollisionObjectQueryParams ObjectTypes =
        UMythicAnimNotify_SphereOverlap::BuildRuntimeObjectQueryParams();
    TestTrue(TEXT("canonical overlap mask includes Destructible resources"),
             (ObjectTypes.GetQueryBitfield64()
              & static_cast<int64>(ECC_TO_BITFIELD(ECC_Destructible))) != 0);
    TestTrue(TEXT("finite positive query data is accepted"),
             UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
                 100.0f, FVector::ZeroVector, 0));
    TestFalse(TEXT("a zero radius is rejected"),
              UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
                  0.0f, FVector::ZeroVector, 0));
    TestFalse(TEXT("a non-finite radius is rejected"),
              UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
                  std::numeric_limits<float>::infinity(),
                  FVector::ZeroVector, 0));
    TestFalse(TEXT("a non-finite offset is rejected"),
              UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
                  100.0f,
                  FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0),
                  0));
    TestFalse(TEXT("a corrupt negative target cap is rejected"),
              UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
                  100.0f, FVector::ZeroVector, -1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackTargetNormalizationTest,
    "Mythic.Itemization.WeaponAttackAbility.TargetsAreCanonical",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackTargetNormalizationTest::RunTest(
    const FString &Parameters) {
    AActor *Avatar = NewObject<AActor>(GetTransientPackage());
    AActor *Target = NewObject<AActor>(GetTransientPackage());
    AActor *SecondTarget = NewObject<AActor>(GetTransientPackage());

    TArray<FHitResult> Hits;
    Hits.Emplace(Target, nullptr, FVector(1.0, 0.0, 0.0), FVector::ForwardVector);
    Hits.Emplace(Target, nullptr, FVector(2.0, 0.0, 0.0), FVector::ForwardVector);
    Hits.Emplace(Avatar, nullptr, FVector::ZeroVector, FVector::ForwardVector);
    Hits.AddDefaulted();
    Hits.Emplace(SecondTarget, nullptr, FVector(3.0, 0.0, 0.0),
                 FVector::ForwardVector);

    UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
        Hits, Avatar, EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("invalid, self, and duplicate actors are removed"), Hits.Num(),
              2);
    if (Hits.Num() == 2) {
        TestEqual(TEXT("the first target occurrence wins"), Hits[0].GetActor(),
                  Target);
        TestEqual(TEXT("stable source ordering is preserved"),
                  Hits[1].GetActor(), SecondTarget);
    }

    AActor *FabricatedTarget = NewObject<AActor>(GetTransientPackage());
    TArray<FHitResult> RequestedHits;
    RequestedHits.Emplace(FabricatedTarget, nullptr, FVector::ZeroVector,
                          FVector::ForwardVector);
    RequestedHits.Add(Hits[0]);
    TArray<FHitResult> IntersectedHits;
    UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
        Hits, RequestedHits, IntersectedHits, Avatar,
        EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("Blueprint filtering cannot expand the native query"),
              IntersectedHits.Num(), 1);
    if (IntersectedHits.Num() == 1) {
        TestEqual(TEXT("the original canonical hit data is retained"),
                  IntersectedHits[0].GetActor(), Target);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackInstanceIdentityTest,
    "Mythic.Itemization.WeaponAttackAbility.DestructibleInstanceIdentity",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackInstanceIdentityTest::RunTest(
    const FString &Parameters) {
    AActor *Avatar = NewObject<AActor>(GetTransientPackage());
    AActor *ResourceOwner = NewObject<AActor>(GetTransientPackage());
    UMythicResourceISM *FirstISM =
        NewObject<UMythicResourceISM>(ResourceOwner);
    UMythicResourceISM *SecondISM =
        NewObject<UMythicResourceISM>(ResourceOwner);

    const auto MakeResourceHit =
        [ResourceOwner](UMythicResourceISM *Component,
                        const int32 InstanceIndex,
                        const double X) {
            FHitResult Hit(ResourceOwner, Component, FVector(X, 0.0, 0.0),
                           FVector::ForwardVector);
            Hit.Item = InstanceIndex;
            return Hit;
        };

    TArray<FHitResult> ResourceHits{
        MakeResourceHit(FirstISM, 4, 1.0),
        MakeResourceHit(FirstISM, 4, 2.0),
        MakeResourceHit(FirstISM, 5, 3.0),
        MakeResourceHit(SecondISM, 4, 4.0)};
    UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
        ResourceHits, Avatar, EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("only the same component and instance index deduplicate"),
              ResourceHits.Num(), 3);
    if (ResourceHits.Num() == 3) {
        TestEqual(TEXT("the first occurrence of an instance wins"),
                  ResourceHits[0].ImpactPoint.X, 1.0);
        TestEqual(TEXT("another instance on the same ISM is preserved"),
                  ResourceHits[1].Item, 5);
        TestEqual(TEXT("the same index on another ISM is preserved"),
                  ResourceHits[2].GetComponent(),
                  static_cast<UPrimitiveComponent *>(SecondISM));
    }

    TArray<FHitResult> RequestedHits;
    if (ResourceHits.Num() >= 2) {
        RequestedHits.Add(ResourceHits[1]);
    }
    RequestedHits.Add(MakeResourceHit(FirstISM, 99, 5.0));
    TArray<FHitResult> IntersectedHits;
    UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
        ResourceHits, RequestedHits, IntersectedHits, Avatar,
        EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("Blueprint filtering selects one exact resource instance"),
              IntersectedHits.Num(), 1);
    if (IntersectedHits.Num() == 1) {
        TestEqual(TEXT("the selected canonical instance index is retained"),
                  IntersectedHits[0].Item, 5);
        TestEqual(TEXT("the selected canonical geometry is retained"),
                  IntersectedHits[0].ImpactPoint.X, 3.0);
    }

    AActor *OrdinaryActor = NewObject<AActor>(GetTransientPackage());
    TArray<FHitResult> DestructibleFilterCandidates{
        MakeResourceHit(FirstISM, 7, 1.0),
        FHitResult(OrdinaryActor, nullptr, FVector::ZeroVector,
                   FVector::ForwardVector)};
    UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
        DestructibleFilterCandidates, EMythicAttackSourceDomain::Weapon, Avatar);
    TestEqual(TEXT("the runtime filter retains the destructible contact"),
              DestructibleFilterCandidates.Num(), 1);
    if (DestructibleFilterCandidates.Num() == 1) {
        TestEqual(TEXT("the exact ISM hit survives domain filtering"),
                  DestructibleFilterCandidates[0].GetComponent(),
                  static_cast<UPrimitiveComponent *>(FirstISM));
        TestEqual(TEXT("the exact resource instance survives domain filtering"),
                  DestructibleFilterCandidates[0].Item, 7);
    }

    TArray<FHitResult> WeaponCandidates{
        MakeResourceHit(FirstISM, 8, 1.0),
        FHitResult(OrdinaryActor, nullptr, FVector::ZeroVector,
                   FVector::ForwardVector)};
    UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
        WeaponCandidates, EMythicAttackSourceDomain::Weapon, Avatar);
    TestEqual(TEXT("a weapon retains the exact harvestable contact for rejection feedback"),
              WeaponCandidates.Num(), 1);
    if (WeaponCandidates.Num() == 1) {
        TestEqual(TEXT("weapon rejection routing preserves the resource instance"),
                  WeaponCandidates[0].Item, 8);
    }

    TArray<FHitResult> ExactSecondComponentRequest{
        MakeResourceHit(SecondISM, 4, 9.0)};
    TArray<FHitResult> ExactSecondComponentIntersection;
    UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
        ResourceHits, ExactSecondComponentRequest,
        ExactSecondComponentIntersection, Avatar,
        EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("an exact request cannot alias another destructible component"),
              ExactSecondComponentIntersection.Num(), 1);
    if (ExactSecondComponentIntersection.Num() == 1) {
        TestEqual(TEXT("the requested destructible component identity wins"),
                  ExactSecondComponentIntersection[0].GetComponent(),
                  static_cast<UPrimitiveComponent *>(SecondISM));
        TestEqual(TEXT("canonical geometry is retained for that component"),
                  ExactSecondComponentIntersection[0].ImpactPoint.X, 4.0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackUnrelatedProxyTest,
    "Mythic.Itemization.WeaponAttackAbility.UnrelatedDestructibleProxyRejected",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackUnrelatedProxyTest::RunTest(
    const FString &Parameters) {
    AActor *Avatar = NewObject<AActor>(GetTransientPackage());
    AActor *ResourceOwner = NewObject<AActor>(GetTransientPackage());
    UMythicResourceISM *DestructibleComponent =
        NewObject<UMythicResourceISM>(ResourceOwner);
    UStaticMeshComponent *UnrelatedProxy =
        NewObject<UStaticMeshComponent>(ResourceOwner);
    ResourceOwner->AddOwnedComponent(DestructibleComponent);
    ResourceOwner->AddOwnedComponent(UnrelatedProxy);

    FHitResult ExactDestructibleHit(
        ResourceOwner, DestructibleComponent, FVector(1.0, 0.0, 0.0),
        FVector::ForwardVector);
    ExactDestructibleHit.Item = 12;
    FHitResult UnrelatedProxyHit(
        ResourceOwner, UnrelatedProxy, FVector(2.0, 0.0, 0.0),
        FVector::ForwardVector);
    UnrelatedProxyHit.Item = 12;
    TestFalse(TEXT("the typed resolver rejects an unrelated proxy"),
              FMythicDestructibleTargetIdentity::Resolve(UnrelatedProxyHit)
                  .IsValid());

    TArray<FHitResult> DomainCandidates{UnrelatedProxyHit};
    UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
        DomainCandidates, EMythicAttackSourceDomain::Weapon, Avatar);
    TestTrue(TEXT("a sibling destructible cannot authorize an unrelated proxy hit"),
             DomainCandidates.IsEmpty());

    // Normalization only canonicalizes identity; rejecting the proxy is the domain filter's job asserted above. What
    // must hold here is that the proxy keeps its own actor identity and never inherits the resource's instance.
    TArray<FHitResult> NormalizedCandidates{UnrelatedProxyHit};
    UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
        NormalizedCandidates, Avatar, EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("an unrelated proxy normalizes to itself"),
              NormalizedCandidates.Num(), 1);
    if (NormalizedCandidates.Num() == 1) {
        TestEqual(TEXT("the proxy never adopts the resource instance index"),
                  NormalizedCandidates[0].Item, UnrelatedProxyHit.Item);
        TestEqual(TEXT("the proxy keeps its own component"),
                  NormalizedCandidates[0].GetComponent(),
                  static_cast<UPrimitiveComponent *>(UnrelatedProxy));
    }

    TArray<FHitResult> IntersectedHits;
    UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
        TArray<FHitResult>{ExactDestructibleHit},
        TArray<FHitResult>{UnrelatedProxyHit}, IntersectedHits, Avatar,
        EMythicAttackSourceDomain::Weapon);
    TestTrue(TEXT("Blueprint cannot select a destructible through an unrelated proxy"),
             IntersectedHits.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackActorDestructibleIdentityTest,
    "Mythic.Itemization.WeaponAttackAbility.ActorDestructibleIgnoresHitItem",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackActorDestructibleIdentityTest::RunTest(
    const FString &Parameters) {
    AActor *Avatar = NewObject<AActor>(GetTransientPackage());
    AMythicWeaponAttackActorDestructibleTestFixture *DestructibleActor =
        NewObject<AMythicWeaponAttackActorDestructibleTestFixture>(
            GetTransientPackage());

    const auto MakeActorHit =
        [DestructibleActor](const int32 Item, const double X) {
            FHitResult Hit(DestructibleActor, nullptr, FVector(X, 0.0, 0.0),
                           FVector::ForwardVector);
            Hit.Item = Item;
            return Hit;
        };

    const FMythicDestructibleTargetIdentity ActorIdentity =
        FMythicDestructibleTargetIdentity::Resolve(MakeActorHit(3, 1.0));
    TestEqual(TEXT("the typed resolver selects the destructible actor"),
              ActorIdentity.TargetObject,
              static_cast<const UObject *>(DestructibleActor));
    TestEqual(TEXT("actor-backed destructibles have no instance index"),
              ActorIdentity.InstanceIndex, INDEX_NONE);
    TestFalse(TEXT("actor-backed destructibles are not per-instance"),
              ActorIdentity.bPerInstance);

    TArray<FHitResult> ActorHits{
        MakeActorHit(3, 1.0), MakeActorHit(47, 2.0)};
    UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
        ActorHits, EMythicAttackSourceDomain::Weapon, Avatar);
    TestEqual(TEXT("actor-backed destructible hits are valid tool targets"),
              ActorHits.Num(), 2);
    UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
        ActorHits, Avatar, EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("Hit.Item cannot split one actor-backed destructible"),
              ActorHits.Num(), 1);
    if (ActorHits.Num() == 1) {
        TestEqual(TEXT("the first actor hit geometry wins"),
                  ActorHits[0].ImpactPoint.X, 1.0);
    }

    TArray<FHitResult> IntersectedHits;
    UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
        TArray<FHitResult>{MakeActorHit(3, 1.0)},
        TArray<FHitResult>{MakeActorHit(99, 9.0)}, IntersectedHits, Avatar,
        EMythicAttackSourceDomain::Weapon);
    TestEqual(TEXT("actor selection ignores component-only Item variance"),
              IntersectedHits.Num(), 1);
    if (IntersectedHits.Num() == 1) {
        TestEqual(TEXT("actor selection retains canonical hit data"),
                  IntersectedHits[0].ImpactPoint.X, 1.0);
        TestEqual(TEXT("actor selection retains the canonical Item payload"),
                  IntersectedHits[0].Item, 3);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDamageContainerExactDestructibleRoutingTest,
    "Mythic.Itemization.WeaponAttackAbility.DamageContainerExactDestructibleRouting",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicDamageContainerExactDestructibleRoutingTest::RunTest(
    const FString &Parameters) {
    AActor *ResourceOwner = NewObject<AActor>(GetTransientPackage());
    UMythicResourceISM *ExactDestructible =
        NewObject<UMythicResourceISM>(ResourceOwner);
    UMythicResourceISM *SiblingDestructible =
        NewObject<UMythicResourceISM>(ResourceOwner);
    UStaticMeshComponent *UnrelatedProxy =
        NewObject<UStaticMeshComponent>(ResourceOwner);
    ResourceOwner->AddOwnedComponent(ExactDestructible);
    ResourceOwner->AddOwnedComponent(SiblingDestructible);
    ResourceOwner->AddOwnedComponent(UnrelatedProxy);

    FHitResult ExactHit(
        ResourceOwner, ExactDestructible, FVector(11.0, 12.0, 13.0),
        FVector::ForwardVector);
    ExactHit.Item = 23;
    const FMythicDestructibleTargetIdentity ExactIdentity =
        FMythicDestructibleTargetIdentity::Resolve(ExactHit);
    TestEqual(TEXT("the typed resolver selects the exact hit component"),
              ExactIdentity.TargetObject,
              static_cast<const UObject *>(ExactDestructible));
    TestEqual(TEXT("the typed resolver retains the component instance"),
              ExactIdentity.InstanceIndex, 23);
    TestTrue(TEXT("component-backed destructibles are per-instance"),
             ExactIdentity.bPerInstance);
    FMythicDamageContainerSpec ExactSpec;
    ExactSpec.AddTargets(TArray<FHitResult>{ExactHit}, TArray<AActor *>());
    TestEqual(TEXT("an exact component hit enters destructible dispatch"),
              ExactSpec.DestructibleTargetsHandle.Num(), 1);
    TestEqual(TEXT("an exact component hit never enters living dispatch"),
              ExactSpec.TargetsHandle.Num(), 0);
    const FGameplayAbilityTargetData *ExactTargetData =
        ExactSpec.DestructibleTargetsHandle.Num() > 0
            ? ExactSpec.DestructibleTargetsHandle.Get(0)
            : nullptr;
    const FHitResult *RoutedExactHit =
        ExactTargetData ? ExactTargetData->GetHitResult() : nullptr;
    if (TestNotNull(TEXT("destructible dispatch retains single-hit target data"),
                    RoutedExactHit)) {
        TestEqual(TEXT("destructible dispatch retains the exact component"),
                  RoutedExactHit->GetComponent(),
                  static_cast<UPrimitiveComponent *>(ExactDestructible));
        TestEqual(TEXT("destructible dispatch retains the exact instance"),
                  RoutedExactHit->Item, 23);
        TestEqual(TEXT("destructible dispatch retains canonical geometry"),
                  RoutedExactHit->ImpactPoint, FVector(11.0, 12.0, 13.0));
    }

    FHitResult ProxyHit(
        ResourceOwner, UnrelatedProxy, FVector(21.0, 22.0, 23.0),
        FVector::ForwardVector);
    ProxyHit.Item = 23;
    FMythicDamageContainerSpec ProxySpec;
    ProxySpec.AddTargets(TArray<FHitResult>{ProxyHit}, TArray<AActor *>());
    TestEqual(TEXT("a sibling destructible cannot hijack proxy dispatch"),
              ProxySpec.DestructibleTargetsHandle.Num(), 0);
    TestEqual(TEXT("an unrelated proxy remains ordinary hit target data"),
              ProxySpec.TargetsHandle.Num(), 1);
    const FGameplayAbilityTargetData *ProxyTargetData =
        ProxySpec.TargetsHandle.Num() > 0
            ? ProxySpec.TargetsHandle.Get(0)
            : nullptr;
    const FHitResult *RoutedProxyHit =
        ProxyTargetData ? ProxyTargetData->GetHitResult() : nullptr;
    if (TestNotNull(TEXT("ordinary dispatch also retains exact hit data"),
                    RoutedProxyHit)) {
        TestEqual(TEXT("ordinary dispatch retains the proxy component"),
                  RoutedProxyHit->GetComponent(),
                  static_cast<UPrimitiveComponent *>(UnrelatedProxy));
        TestEqual(TEXT("ordinary dispatch retains the proxy Item payload"),
                  RoutedProxyHit->Item, 23);
    }

    AMythicWeaponAttackActorDestructibleTestFixture *ActorDestructible =
        NewObject<AMythicWeaponAttackActorDestructibleTestFixture>(
            GetTransientPackage());
    FHitResult ActorHit(
        ActorDestructible, nullptr, FVector(31.0, 32.0, 33.0),
        FVector::ForwardVector);
    ActorHit.Item = 41;
    FMythicDamageContainerSpec ActorSpec;
    ActorSpec.AddTargets(TArray<FHitResult>{ActorHit}, TArray<AActor *>());
    TestEqual(TEXT("an actor implementation enters destructible dispatch"),
              ActorSpec.DestructibleTargetsHandle.Num(), 1);
    const FGameplayAbilityTargetData *ActorTargetData =
        ActorSpec.DestructibleTargetsHandle.Num() > 0
            ? ActorSpec.DestructibleTargetsHandle.Get(0)
            : nullptr;
    const FHitResult *RoutedActorHit =
        ActorTargetData ? ActorTargetData->GetHitResult() : nullptr;
    if (TestNotNull(TEXT("actor destructible retains its canonical hit"),
                    RoutedActorHit)) {
        TestEqual(TEXT("actor destructible hit payload is not rewritten"),
                  RoutedActorHit->Item, 41);
        TestEqual(TEXT("actor destructible geometry is not rewritten"),
                  RoutedActorHit->ImpactPoint, FVector(31.0, 32.0, 33.0));
    }

    FMythicDamageContainerSpec ActorOnlySpec;
    ActorOnlySpec.AddTargets(TArray<FHitResult>(),
                             TArray<AActor *>{ResourceOwner});
    TestEqual(TEXT("actor-only data cannot infer a component destructible"),
              ActorOnlySpec.DestructibleTargetsHandle.Num(), 0);
    TestEqual(TEXT("actor-only data retains its exact actor semantics"),
              ActorOnlySpec.TargetsHandle.Num(), 1);
    return true;
}

#endif
