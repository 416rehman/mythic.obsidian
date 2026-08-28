#include "Itemization/Inventory/MythicItemFactorySubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicItemizationRuleset.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"

namespace {
bool GatherExactGemGrants(const UItemDefinition *ItemDefinition,
                          TArray<FMythicAffixGrantSpec> &OutGrants,
                          FName &OutDiagnosticCode) {
    OutGrants.Reset();
    const UMythicGemFragment *FoundGem = nullptr;
    if (!ItemDefinition) return false;
    for (const UItemFragment *Fragment : ItemDefinition->Fragments) {
        if (const UMythicGemFragment *Gem = Cast<UMythicGemFragment>(Fragment)) {
            if (FoundGem) {
                OutDiagnosticCode = TEXT("MultipleGemFragments");
                return false;
            }
            FoundGem = Gem;
        }
    }
    if (!FoundGem) return true;
    if (FoundGem->GrantSpecs.IsEmpty()
        || FoundGem->GrantSpecs.Num() > MythicAffixSerialization::MaxAffixesPerContainer) {
        OutDiagnosticCode = TEXT("InvalidGemGrantCount");
        return false;
    }
    for (const FMythicAffixGrantSpec &Grant : FoundGem->GrantSpecs) {
        if (!Grant.GrantGuid.IsValid() || !Grant.AffixDefinition.IsValid()
            || Grant.TierMode != EMythicAffixGrantTierMode::ExactTier
            || Grant.ExactTierRank < 1) {
            OutDiagnosticCode = TEXT("InvalidGemGrantSpec");
            return false;
        }
    }
    OutGrants = FoundGem->GrantSpecs;
    return true;
}

bool IsProfileEnabledByActiveRuleset(
    const UMythicItemizationDataRegistrySubsystem *Registry,
    const FPrimaryAssetId &ProfileId) {
    const UMythicItemizationRuleset *Ruleset = Registry ? Registry->GetActiveRuleset() : nullptr;
    if (!Registry || !Registry->IsActiveRulesetReady() || !Ruleset || !ProfileId.IsValid()) {
        return false;
    }

    return Ruleset->Profiles.ContainsByPredicate(
        [&ProfileId](const TSoftObjectPtr<UMythicAffixProfile> &ProfileRef) {
            const UMythicAffixProfile *Profile = ProfileRef.Get();
            return Profile && Profile->GetPrimaryAssetId() == ProfileId;
        });
}
}

void UMythicItemFactorySubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Collection.InitializeDependency<UMythicItemizationDataRegistrySubsystem>();
    Super::Initialize(Collection);
}

bool UMythicItemFactorySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Outer ? Outer->GetWorld() : nullptr;
    return !World || World->WorldType == EWorldType::None || World->GetNetMode() < NM_Client;
}

UMythicItemFactorySubsystem::EProfileRequirement UMythicItemFactorySubsystem::ResolveProfileRequirement(
    const UItemDefinition *ItemDefinition,
    FPrimaryAssetId &OutProfileId,
    FName &OutDiagnosticCode) {
    OutProfileId = FPrimaryAssetId();
    OutDiagnosticCode = NAME_None;
    if (!ItemDefinition) {
        OutDiagnosticCode = TEXT("MissingItemDefinition");
        return EProfileRequirement::Invalid;
    }

    const UAffixesFragment *FoundAffixes = nullptr;
    for (const UItemFragment *Fragment : ItemDefinition->Fragments) {
        if (!Fragment) {
            OutDiagnosticCode = TEXT("NullDefinitionFragment");
            return EProfileRequirement::Invalid;
        }
        if (const UAffixesFragment *Affixes = Cast<UAffixesFragment>(Fragment)) {
            if (FoundAffixes) {
                OutDiagnosticCode = TEXT("MultipleAffixesFragments");
                return EProfileRequirement::Invalid;
            }
            FoundAffixes = Affixes;
        }
    }

    if (!FoundAffixes) {
        return EProfileRequirement::None;
    }
    if (!FoundAffixes->AffixesConfig.AffixProfile.IsValid()) {
        OutDiagnosticCode = TEXT("MissingAffixProfile");
        return EProfileRequirement::Invalid;
    }

    OutProfileId = FoundAffixes->AffixesConfig.AffixProfile.GetPrimaryAssetId();
    if (!OutProfileId.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidAffixProfileReference");
        return EProfileRequirement::Invalid;
    }
    return EProfileRequirement::Valid;
}

EMythicCreateItemStatus UMythicItemFactorySubsystem::EvaluateReadyState(
    const FMythicCreateItemRequest &Request,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    TSharedPtr<const FCompiledAffixProfile> &OutCompiledProfile,
    FName &OutDiagnosticCode) {
    OutCompiledProfile.Reset();
    OutDiagnosticCode = NAME_None;

    if (!Request.ItemDefinition || !Request.OwningActor || !Request.OwningActor->HasAuthority()
        || !Request.OwningActor->IsUsingRegisteredSubObjectList()
        || Request.ItemLevel < 1 || Request.Quantity < 1) {
        OutDiagnosticCode = TEXT("InvalidCreateRequest");
        return EMythicCreateItemStatus::InvalidData;
    }

    return EvaluateDefinitionReadyState(
        Request.ItemDefinition, Registry, OutCompiledProfile, OutDiagnosticCode);
}

EMythicCreateItemStatus UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
    const UItemDefinition *ItemDefinition,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    TSharedPtr<const FCompiledAffixProfile> &OutCompiledProfile,
    FName &OutDiagnosticCode) {
    OutCompiledProfile.Reset();
    OutDiagnosticCode = NAME_None;
    FPrimaryAssetId ProfileId;
    TArray<FMythicAffixGrantSpec> GemGrants;
    if (!GatherExactGemGrants(ItemDefinition, GemGrants, OutDiagnosticCode)) {
        return EMythicCreateItemStatus::InvalidData;
    }
    const EProfileRequirement Requirement = ResolveProfileRequirement(
        ItemDefinition, ProfileId, OutDiagnosticCode);
    if (Requirement == EProfileRequirement::Invalid) {
        return EMythicCreateItemStatus::InvalidData;
    }
    if (Requirement == EProfileRequirement::Valid) {
        if (!Registry || !Registry->IsActiveRulesetReady()) {
            OutDiagnosticCode = TEXT("ActiveRulesetNotReady");
            return EMythicCreateItemStatus::NotReady;
        }
        if (!IsProfileEnabledByActiveRuleset(Registry, ProfileId)) {
            OutDiagnosticCode = TEXT("ProfileNotEnabledByActiveRuleset");
            return EMythicCreateItemStatus::InvalidData;
        }
        OutCompiledProfile = Registry->FindCompiledProfile(ProfileId);
        if (!OutCompiledProfile) {
            OutDiagnosticCode = TEXT("CompiledProfileUnavailable");
            return EMythicCreateItemStatus::NotReady;
        }
    }

    if (!GemGrants.IsEmpty()) {
        if (!Registry) {
            OutDiagnosticCode = TEXT("GemGrantClosureNotReady");
            return EMythicCreateItemStatus::NotReady;
        }
        for (const FMythicAffixGrantSpec &Grant : GemGrants) {
            if (!Registry->FindCompiledGrant(Grant).IsValid()) {
                OutDiagnosticCode = TEXT("GemGrantClosureNotReady");
                return EMythicCreateItemStatus::NotReady;
            }
        }
    }
    return EMythicCreateItemStatus::Success;
}

FMythicCreateItemResult UMythicItemFactorySubsystem::CreateItemReady(
    const FMythicCreateItemRequest &Request) {
    FMythicCreateItemResult Result;
    UMythicItemizationDataRegistrySubsystem *Registry = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    TSharedPtr<const FCompiledAffixProfile> CompiledProfile;
    Result.Status = EvaluateReadyState(Request, Registry, CompiledProfile, Result.DiagnosticCode);
    if (Result.Status != EMythicCreateItemStatus::Success) {
        return Result;
    }

    // The item is deliberately not registered with its replication owner until InitializeTransactional commits.
    UMythicItemInstance *Candidate = NewObject<UMythicItemInstance>(Request.OwningActor);
    if (!Candidate) {
        Result.Status = EMythicCreateItemStatus::GenerationFailed;
        Result.DiagnosticCode = TEXT("ItemAllocationFailed");
        return Result;
    }

    const FMythicItemInitializeResult InitializeResult = Candidate->InitializeTransactional(
        Request, CompiledProfile.Get());
    if (!InitializeResult.IsSuccess()) {
        Candidate->MarkAsGarbage();
        Result.Status = InitializeResult.Status == EMythicItemInitializeStatus::InvalidData
            ? EMythicCreateItemStatus::InvalidData
            : EMythicCreateItemStatus::GenerationFailed;
        Result.DiagnosticCode = InitializeResult.DiagnosticCode;
        return Result;
    }

    Result.Status = EMythicCreateItemStatus::Success;
    Result.DiagnosticCode = NAME_None;
    Result.Item = Candidate;
    return Result;
}

void UMythicItemFactorySubsystem::RequestItemDefinitionReadyAsync(
    UItemDefinition *ItemDefinition,
    FOnMythicItemDefinitionReady Completion) {
    FPrimaryAssetId ProfileId;
    FName DiagnosticCode;
    TArray<FMythicAffixGrantSpec> GemGrants;
    if (!GatherExactGemGrants(ItemDefinition, GemGrants, DiagnosticCode)) {
        Completion.ExecuteIfBound(false);
        return;
    }
    const EProfileRequirement Requirement = ResolveProfileRequirement(
        ItemDefinition, ProfileId, DiagnosticCode);
    if (Requirement == EProfileRequirement::Invalid) {
        Completion.ExecuteIfBound(false);
        return;
    }
    if (Requirement == EProfileRequirement::None && GemGrants.IsEmpty()) {
        Completion.ExecuteIfBound(true);
        return;
    }

    UMythicItemizationDataRegistrySubsystem *Registry = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    if (!Registry) {
        Completion.ExecuteIfBound(false);
        return;
    }
    const TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakRegistry(Registry);
    auto RequestGrants = [WeakRegistry, ProfileId, GemGrants = MoveTemp(GemGrants), Completion](
                             const bool bRulesetReady) mutable {
        UMythicItemizationDataRegistrySubsystem *ReadyRegistry = WeakRegistry.Get();
        if (!bRulesetReady || !ReadyRegistry
            || (ProfileId.IsValid() && !IsProfileEnabledByActiveRuleset(ReadyRegistry, ProfileId))) {
            Completion.ExecuteIfBound(false);
            return;
        }
        if (GemGrants.IsEmpty()) {
            Completion.ExecuteIfBound(true);
            return;
        }
        ReadyRegistry->RequestGrantClosureAsync(
            GemGrants,
            FOnMythicItemizationDataReady::CreateLambda(
                [Completion](const bool bGrantsReady) mutable {
                    Completion.ExecuteIfBound(bGrantsReady);
                }));
    };
    if (Requirement == EProfileRequirement::Valid) {
        Registry->RequestActiveRulesetAsync(
            FOnMythicItemizationDataReady::CreateLambda(MoveTemp(RequestGrants)));
    }
    else {
        RequestGrants(true);
    }
}

void UMythicItemFactorySubsystem::CreateItemAsync(
    const FMythicCreateItemRequest &Request,
    FOnMythicItemCreated Completion) {
    FPrimaryAssetId ProfileId;
    FName DiagnosticCode;
    TArray<FMythicAffixGrantSpec> GemGrants;
    const bool bGemGrantsValid = GatherExactGemGrants(
        Request.ItemDefinition, GemGrants, DiagnosticCode);
    const EProfileRequirement Requirement = ResolveProfileRequirement(
        Request.ItemDefinition, ProfileId, DiagnosticCode);
    if (!bGemGrantsValid || Requirement == EProfileRequirement::Invalid || !Request.OwningActor
        || !Request.OwningActor->HasAuthority() || Request.ItemLevel < 1 || Request.Quantity < 1) {
        FMythicCreateItemResult Result;
        Result.Status = EMythicCreateItemStatus::InvalidData;
        Result.DiagnosticCode = !bGemGrantsValid || Requirement == EProfileRequirement::Invalid
            ? DiagnosticCode : FName(TEXT("InvalidCreateRequest"));
        Completion.ExecuteIfBound(MoveTemp(Result));
        return;
    }
    TWeakObjectPtr<UMythicItemFactorySubsystem> WeakThis(this);
    TWeakObjectPtr<UItemDefinition> WeakDefinition(Request.ItemDefinition);
    TWeakObjectPtr<AActor> WeakOwner(Request.OwningActor);
    RequestItemDefinitionReadyAsync(
        Request.ItemDefinition,
        FOnMythicItemDefinitionReady::CreateLambda(
            [WeakThis, WeakDefinition, WeakOwner, Request, Completion](const bool bReady) mutable {
                if (!WeakThis.IsValid()) {
                    return;
                }
                if (!bReady || !WeakDefinition.IsValid() || !WeakOwner.IsValid()) {
                    FMythicCreateItemResult Result;
                    Result.Status = EMythicCreateItemStatus::InvalidData;
                    Result.DiagnosticCode = TEXT("CreateRequestExpired");
                    if (!bReady && WeakDefinition.IsValid() && WeakOwner.IsValid()) {
                        UMythicItemizationDataRegistrySubsystem *Registry = WeakThis->GetGameInstance()
                            ? WeakThis->GetGameInstance()->GetSubsystem<
                                UMythicItemizationDataRegistrySubsystem>()
                            : nullptr;
                        TSharedPtr<const FCompiledAffixProfile> IgnoredProfile;
                        FName ReadinessDiagnostic;
                        const EMythicCreateItemStatus Readiness = EvaluateDefinitionReadyState(
                            WeakDefinition.Get(), Registry, IgnoredProfile, ReadinessDiagnostic);
                        Result.Status = Readiness == EMythicCreateItemStatus::InvalidData
                            ? EMythicCreateItemStatus::InvalidData
                            : EMythicCreateItemStatus::GenerationFailed;
                        Result.DiagnosticCode = ReadinessDiagnostic.IsNone()
                            ? FName(TEXT("ItemClosureLoadFailed"))
                            : ReadinessDiagnostic;
                    }
                    Completion.ExecuteIfBound(MoveTemp(Result));
                    return;
                }
                Completion.ExecuteIfBound(WeakThis->CreateItemReady(Request));
            }));
}
