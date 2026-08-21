#include "MythicCookingRecipe.h"

#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Actionable/ConsumableEffectFragment.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Itemization/Cooking/MythicCookingCore.h"
#include "Itemization/Cooking/MythicTags_Cooking.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/MythicUnlockComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UMythicCookingRecipe::UMythicCookingRecipe() {
    PotencySetByCallerTag = TAG_SetByCaller_Food_Potency;
    NourishSetByCallerTag = TAG_SetByCaller_Food_Nourish;
}

float UMythicCookingRecipe::ComputePotencyForContext(const FMythicConversionProductContext &Context) const {
    const FMythicYieldQualityRules &Rules = GetDefault<UMythicDeveloperSettings>()->YieldQuality;
    const float TierValue = Context.SnapshotAvgQualityTierValue >= 0.0f
        ? Context.SnapshotAvgQualityTierValue
        : FMythicYieldQuality::TierValue(EMythicYieldQuality::Common);
    const float QualityMult = FMythicYieldQuality::PotencyMultiplierForTierValue(Rules, TierValue);
    const float FreshFactor = FMythicCookingCore::FreshnessPotencyFactor(Context.SnapshotMinFreshnessFraction, MinFreshnessPotencyFactor);
    return FMythicCookingCore::ComputePotency(QualityMult, FreshFactor, Context.SnapshotCrafterProficiencyLevel, PotencyPerCookingLevel);
}

void UMythicCookingRecipe::PostProcessProduct(UMythicItemInstance *ProductInstance, const FMythicConversionProductContext &Context) const {
    if (!IsValid(ProductInstance)) {
        return;
    }

    const float Potency = ComputePotencyForContext(Context);

    if (const UConsumableEffectFragment *ConstFrag = ProductInstance->GetFragment<UConsumableEffectFragment>()) {
        UConsumableEffectFragment *Frag = const_cast<UConsumableEffectFragment *>(ConstFrag);
        for (FConsumableEffectEntry &Entry : Frag->ConsumableEffectConfig.Effects) {
            if (PotencySetByCallerTag.IsValid()) {
                Entry.SetByCallerMagnitudes.Add(PotencySetByCallerTag, Potency);
            }
            if (NourishValue > 0.0f && NourishSetByCallerTag.IsValid()) {
                Entry.SetByCallerMagnitudes.Add(NourishSetByCallerTag, FMythicCookingCore::QuantizePotency(NourishValue * Potency));
            }
        }
    }

    const float CritChance = FMythicCookingCore::PortionCritChance(
        Context.SnapshotCrafterProficiencyLevel, PortionCritBaseChance, PortionCritChancePerLevel, PortionCritMaxChance);
    if (CritChance > 0.0f && MythicCombat::RollSucceeds(CritChance, FMath::FRand())) {
        const UItemDefinition *Def = ProductInstance->GetItemDefinition();
        if (Def && Def->StackSizeMax > 1 && ProductInstance->GetStacks() < Def->StackSizeMax) {
            ProductInstance->SetStackSize(ProductInstance->GetStacks() + 1);
        }
    }

    if (Context.InstigatorController) {
        if (const AMythicPlayerState *PS = Context.InstigatorController->GetPlayerState<AMythicPlayerState>()) {
            if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
                Ledger->RecordStat(STAT_COOKING_DISHES_COOKED, FMath::Max(1, ProductInstance->GetStacks()));
                if (bExperimentFallback) {
                    Ledger->RecordStat(STAT_COOKING_EXPERIMENTS, 1);
                }
            }
        }
    }
}

bool UMythicCookingRecipe::IsVisibleTo(const FGameplayTagContainer &InstigatorOwnedTags) const {
    if (!bHiddenUntilDiscovered) {
        return true;
    }
    return Requirements.InstigatorTagQuery.IsEmpty() || Requirements.InstigatorTagQuery.Matches(InstigatorOwnedTags);
}

bool UMythicCookingRecipe::PassesDynamicGates(AController *Instigator, FText &OutReason) const {
    if (!RequiredBestiaryFullKey.IsValid()) {
        return true;
    }
    const UWorld *World = Instigator ? Instigator->GetWorld() : nullptr;
    const AGameStateBase *GS = World ? World->GetGameState() : nullptr;
    if (GS) {
        for (const APlayerState *PSBase : GS->PlayerArray) {
            const AMythicPlayerState *PS = Cast<AMythicPlayerState>(PSBase);
            const UMythicCodexComponent *Codex = PS ? PS->GetCodexComponent() : nullptr;
            if (Codex && Codex->GetBestiaryTier(RequiredBestiaryFullKey) == EMythicCodexTier::Full) {
                return true;
            }
        }
    }
    OutReason = NSLOCTEXT("Cooking", "NeedsBestiaryFull", "Requires complete bestiary knowledge of the creature");
    return false;
}

bool UMythicCookingRecipe::GrantDiscovery(AController *Instigator) const {
    if (!Instigator || !TaughtSchematicTag.IsValid()) {
        return false;
    }
    const AMythicPlayerState *PS = Instigator->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return false;
    }
    UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent();
    const bool bNewlyLearned = Unlocks && Unlocks->ServerLearnRecipe(TaughtSchematicTag);
    if (bNewlyLearned) {
        if (CodexTermKey.IsValid()) {
            if (UMythicCodexComponent *Codex = PS->GetCodexComponent()) {
                Codex->ServerDiscoverTerm(CodexTermKey);
            }
        }
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(STAT_COOKING_RECIPES_DISCOVERED, 1);
        }
    }
    return bNewlyLearned;
}

UMythicCookingRecipe *UMythicCookingRecipe::PickBestExperimentCandidate(TArrayView<UMythicCookingRecipe *const> Candidates) {
    UMythicCookingRecipe *Best = nullptr;
    for (UMythicCookingRecipe *R : Candidates) {
        if (!R || R->bExperimentFallback) {
            continue;
        }
        if (!Best) {
            Best = R;
            continue;
        }
        if (R->Process.Priority > Best->Process.Priority) {
            Best = R;
        }
        else if (R->Process.Priority == Best->Process.Priority && R->RecipeId.ToString() < Best->RecipeId.ToString()) {
            Best = R;
        }
    }
    return Best;
}

UMythicCookingRecipe *UMythicCookingRecipe::PickExperimentMatch(const TArray<TObjectPtr<UConversionRecipe>> &AllRecipes,
                                                                const FGameplayTagContainer &StationTags,
                                                                const TArray<UMythicItemInstance *> &Inputs) {
    TArray<UMythicCookingRecipe *> Candidates;
    for (const TObjectPtr<UConversionRecipe> &Recipe : AllRecipes) {
        UMythicCookingRecipe *Cooking = Cast<UMythicCookingRecipe>(Recipe.Get());
        if (!Cooking || Cooking->bExperimentFallback) {
            continue;
        }
        if (!Cooking->MatchesStation(StationTags) || !Cooking->MatchesInputs(Inputs)) {
            continue;
        }
        Candidates.Add(Cooking);
    }
    return PickBestExperimentCandidate(Candidates);
}

#if WITH_EDITOR
EDataValidationResult UMythicCookingRecipe::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (bHiddenUntilDiscovered && Requirements.InstigatorTagQuery.IsEmpty()) {
        Context.AddWarning(FText::FromString(TEXT(
            "Cooking recipe is bHiddenUntilDiscovered but has an EMPTY InstigatorTagQuery - it will always be visible. "
            "Gate it on its TaughtSchematicTag.")));
    }
    if (bHiddenUntilDiscovered && !TaughtSchematicTag.IsValid()) {
        Context.AddError(FText::FromString(TEXT(
            "Cooking recipe is bHiddenUntilDiscovered but has no TaughtSchematicTag - it can never be discovered.")));
        Result = EDataValidationResult::Invalid;
    }
    if (bExperimentFallback && bHiddenUntilDiscovered) {
        Context.AddError(FText::FromString(TEXT(
            "The experiment fallback recipe must not be hidden - it IS the discovery surface.")));
        Result = EDataValidationResult::Invalid;
    }
    return Result;
}
#endif
