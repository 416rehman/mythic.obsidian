#include "World/Entity/MythicEntityIdentityDefinition.h"

#include "Engine/AssetManager.h"
#include "Misc/DataValidation.h"
#include "World/Entity/MythicEntityPresentationComponent.h"

const FPrimaryAssetType UMythicEntityIdentityDefinition::PrimaryAssetType(TEXT("MythicEntityIdentity"));

FPrimaryAssetId UMythicEntityIdentityDefinition::GetPrimaryAssetId() const {
    return IdentityTag.IsValid()
        ? FPrimaryAssetId(PrimaryAssetType, IdentityTag.GetTagName())
        : FPrimaryAssetId();
}

FPrimaryAssetId UMythicEntityIdentityDefinition::ResolvePrimaryAssetId(
    const TSoftObjectPtr<UMythicEntityIdentityDefinition> &Definition) {
    if (const UMythicEntityIdentityDefinition *Loaded = Definition.Get()) {
        return Loaded->GetPrimaryAssetId();
    }
    const FSoftObjectPath Path = Definition.ToSoftObjectPath();
    if (!Path.IsValid()) {
        return FPrimaryAssetId();
    }
    const UAssetManager *AssetManager = UAssetManager::GetIfInitialized();
    if (!AssetManager) {
        return FPrimaryAssetId();
    }
    const FPrimaryAssetId RegisteredId =
        AssetManager->GetPrimaryAssetIdForPath(Path);
    return RegisteredId.PrimaryAssetType == PrimaryAssetType
        ? RegisteredId : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicEntityIdentityDefinition::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Invalidate = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    const FGameplayTag IdentityRoot = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Entity.Identity")), false);
    if (!IdentityTag.IsValid()
        || !IdentityRoot.IsValid()
        || !IdentityTag.MatchesTag(IdentityRoot)) {
        Invalidate(NSLOCTEXT("MythicEntityIdentity", "InvalidIdentityTag",
                            "IdentityTag must be a registered Entity.Identity.* tag."));
    }

    FMythicPublicIdentitySnapshot Candidate;
    Candidate.PublicKindTag = PublicKindTag;
    FMythicPublicIdentitySnapshot Sanitized;
    if (!UMythicEntityPresentationComponent::BuildSanitizedPublicIdentity(
            Candidate, Sanitized)) {
        Invalidate(NSLOCTEXT("MythicEntityIdentity", "InvalidPublicKind",
                            "PublicKindTag must be one of the native visible Entity.Kind tags."));
    }

    if (bNameVisibleOnSight && PublicDisplayName.IsEmpty()) {
        Invalidate(NSLOCTEXT("MythicEntityIdentity", "MissingVisibleName",
                            "bNameVisibleOnSight requires a localized PublicDisplayName."));
    }

    if (PresentedFactionTag.IsValid()) {
        const FGameplayTag FactionRoot = FGameplayTag::RequestGameplayTag(
            FName(TEXT("Faction")), false);
        if (!FactionRoot.IsValid()
            || !PresentedFactionTag.MatchesTag(FactionRoot)) {
            Invalidate(NSLOCTEXT("MythicEntityIdentity", "InvalidPresentedFaction",
                                "PresentedFactionTag must be a registered Faction.* tag."));
        }
    }
    return Result;
}
#endif
