

#include "AttributeReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mythic.h"
#include "GameFramework/PlayerController.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Misc/DataValidation.h"
#include "Stats/MythicStatDefinition.h"
#include "UI/ViewModels/MythicStatDisplay.h"

bool UAttributeReward::Give(FRewardContext &Context) const {
    APlayerController *PlayerController = Context.PlayerController;
    if (!PlayerController || !PlayerController->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT("Attribute reward requires an authoritative player controller."));
        return false;
    }

    UAbilitySystemComponent *ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerController);
    if (!ASC || !ASC->IsOwnerActorAuthoritative()) {
        UE_LOG(Myth, Error, TEXT("Attribute reward could not resolve an authoritative Ability System Component."));
        return false;
    }

    UGameInstance *GameInstance = PlayerController->GetWorld()
        ? PlayerController->GetWorld()->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    const FPrimaryAssetId StatId = TargetStat.GetPrimaryAssetId();
    const UMythicStatDefinition *AuthoredDefinition = TargetStat.GetAsset();
    const UMythicStatDefinition *RegisteredDefinition =
        Registry && Registry->IsCoreSemanticReady() && StatId.IsValid()
            ? Registry->FindStat(StatId) : nullptr;
    if (!RegisteredDefinition || RegisteredDefinition != AuthoredDefinition
        || !RegisteredDefinition->Attribute.IsValid()
        || !ASC->HasAttributeSetForAttribute(RegisteredDefinition->Attribute)) {
        UE_LOG(Myth, Error,
               TEXT("Attribute reward '%s' rejected an unavailable, unregistered, replaced, or uninstalled Stat Definition."),
               *GetName());
        return false;
    }

    UMythicAffixApplicationComponent *AffixApplication = nullptr;
    if (AActor *OwnerActor = ASC->GetOwnerActor()) {
        AffixApplication = OwnerActor->FindComponentByClass<UMythicAffixApplicationComponent>();
    }
    if (!AffixApplication) {
        if (AActor *AvatarActor = ASC->GetAvatarActor()) {
            AffixApplication = AvatarActor->FindComponentByClass<UMythicAffixApplicationComponent>();
        }
    }
    if (!AffixApplication) {
        UE_LOG(Myth, Error,
               TEXT("Attribute reward '%s' requires the authoritative permanent-stat application component."),
               *GetName());
        return false;
    }

    return AffixApplication->SetPermanentStatSourceTransactional(
        PermanentSourceGuid, TargetStat, Modifier, Magnitude);
}

FText UAttributeReward::GetPreviewText() const {
    const UMythicStatDefinition *Definition = TargetStat.GetAsset();
    if (!Definition) {
        return MythicStatDisplay::GetUnknownStatDiagnostic();
    }
    const FText Label = Definition->DisplayName;
    const FMythicStatNumberPresentation Presentation =
        MythicStatDisplay::ResolveModifierPresentation(*Definition, Modifier);

    switch (Modifier) {
        case EGameplayModOp::Additive: {
            const FText Bonus = MythicStatDisplay::FormatBonus(Magnitude, Presentation);
            return Bonus.IsEmpty()
                       ? FText::GetEmpty()
                       : FText::Format(NSLOCTEXT("Mythic", "AttrRewardAdd", "{0} {1}"), Label, Bonus);
        }
        case EGameplayModOp::Multiplicitive:
            return FText::Format(NSLOCTEXT("Mythic", "AttrRewardMul", "{0} {1}"), Label,
                                 MythicStatDisplay::FormatValue(Magnitude, Presentation));
        case EGameplayModOp::Override:
            return FText::Format(NSLOCTEXT("Mythic", "AttrRewardSet", "{0} set to {1}"), Label,
                                 MythicStatDisplay::FormatValue(Magnitude, Presentation));
        default: {
            const FText Bonus = MythicStatDisplay::FormatBonus(Magnitude, Presentation);
            return Bonus.IsEmpty()
                       ? FText::GetEmpty()
                       : FText::Format(NSLOCTEXT("Mythic", "AttrRewardAdd", "{0} {1}"), Label, Bonus);
        }
    }
}

#if WITH_EDITOR
EDataValidationResult UAttributeReward::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (!PermanentSourceGuid.IsValid()) {
        Error(NSLOCTEXT("MythicAttributeReward", "InvalidSourceGuid",
                        "Permanent Source Guid is required for idempotent replay and save restore."));
    }

    const UMythicStatDefinition *Definition = TargetStat.Asset.LoadSynchronous();
    if (!Definition || !TargetStat.IsValid() || !Definition->Attribute.IsValid()) {
        Error(NSLOCTEXT("MythicAttributeReward", "InvalidTargetStat",
                        "Target Stat must directly reference a registered Stat Definition with a valid GAS attribute."));
    }

    const EGameplayModOp::Type Operation = Modifier.GetValue();
    if (!MythicAffix::IsSupportedModifierOp(Operation)) {
        Error(NSLOCTEXT("MythicAttributeReward", "InvalidModifier",
                        "Modifier must be an operation supported by the permanent-stat ledger."));
    }
    if (!FMath::IsFinite(Magnitude)) {
        Error(NSLOCTEXT("MythicAttributeReward", "NonFiniteMagnitude",
                        "Magnitude must be finite."));
    }
    else if (MythicAffix::ModifierRequiresNonZeroMagnitude(Operation)
             && FMath::IsNearlyZero(Magnitude)) {
        Error(NSLOCTEXT("MythicAttributeReward", "ZeroMultiplicativeMagnitude",
                        "Multiplicative and divisive permanent rewards require a non-zero magnitude."));
    }

    return Result;
}
#endif
