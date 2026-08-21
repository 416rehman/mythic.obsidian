
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Conversion/ConversionRecipe.h"
#include "MythicCookingRecipe.generated.h"

class UMythicItemInstance;

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicCookingRecipe : public UConversionRecipe {
    GENERATED_BODY()

public:
    // ── Dish identity ──────────────────────────────────────────────────────────────────────────────────────────
    /** Content ladder position (1 = campfire snack … 4 = grand-kitchen signature). UI/sorting only — gates are tags. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Dish", meta = (ClampMin = "0"))
    int32 DishTier = 1;

    /** Base nourishment of the dish. Written to SetByCaller.Food.Nourish scaled by potency; <= 0 writes nothing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Dish", meta = (ClampMin = "0.0"))
    float NourishValue = 0.0f;

    // ── SetByCaller potency mapping ────────────────────────────────────────────────────────────────────────────
    /** SetByCaller tag the cooked potency is written under (default SetByCaller.Food.Potency). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (Categories = "SetByCaller"))
    FGameplayTag PotencySetByCallerTag;

    /** SetByCaller tag the potency-scaled NourishValue is written under (default SetByCaller.Food.Nourish). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (Categories = "SetByCaller"))
    FGameplayTag NourishSetByCallerTag;

    /** Extra potency per Cooking level (the recipe's CraftingProficiency). 0 = level-neutral (inert default).
     *  The 2.0× hard clamp applies regardless. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (ClampMin = "0.0"))
    float PotencyPerCookingLevel = 0.0f;

    /** Potency factor at zero freshness (stale floor). 0.75 = a stale pot still cooks at 3/4 strength. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFreshnessPotencyFactor = 0.75f;

    // ── Portion crit (an extra plated portion; inert by default) ───────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float PortionCritBaseChance = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (ClampMin = "0.0"))
    float PortionCritChancePerLevel = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Potency", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float PortionCritMaxChance = 0.25f;

    // ── Discovery (M4) ─────────────────────────────────────────────────────────────────────────────────────────
    /** Hidden from station lists until the player owns TaughtSchematicTag (learned via schematic or experiment). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Discovery")
    bool bHiddenUntilDiscovered = false;

    /** TRUE only on the ONE always-available experiment recipe ("Questionable Stew"): loose TypeQuery inputs
     *  (any ≥2 ingredients — the MatchesInputs fallback path), lowest Priority. Starting it first tries to match the
     *  instigator's ingredients against hidden cooking recipes; a hit is DISCOVERED and cooked instead. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Discovery")
    bool bExperimentFallback = false;

    /** The schematic tag learning this recipe grants (and that its InstigatorTagQuery should require). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Discovery", meta = (Categories = "Itemization.Schematic"))
    FGameplayTag TaughtSchematicTag;

    /** Codex recipe page discovered on learn (a UMythicGlossaryEntry keyed under Codex.Term.Recipe.*). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Discovery", meta = (Categories = "Codex.Term"))
    FGameplayTag CodexTermKey;

    // ── Signature-dish gate (M5) ───────────────────────────────────────────────────────────────────────────────
    /** When set: cooking requires a FULL bestiary page (EMythicCodexTier::Full) for this Codex.Bestiary.* archetype.
     *  Co-op-generous: ANY online member's Full page satisfies it server-side. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Gates", meta = (Categories = "Codex.Bestiary"))
    FGameplayTag RequiredBestiaryFullKey;

    UMythicCookingRecipe();

    virtual void PostProcessProduct(UMythicItemInstance *ProductInstance, const FMythicConversionProductContext &Context) const override;
    virtual bool IsVisibleTo(const FGameplayTagContainer &InstigatorOwnedTags) const override;
    virtual bool PassesDynamicGates(AController *Instigator, FText &OutReason) const override;

    float ComputePotencyForContext(const FMythicConversionProductContext &Context) const;

    bool GrantDiscovery(AController *Instigator) const;

    static UMythicCookingRecipe *PickBestExperimentCandidate(TArrayView<UMythicCookingRecipe *const> Candidates);

    static UMythicCookingRecipe *PickExperimentMatch(const TArray<TObjectPtr<UConversionRecipe>> &AllRecipes,
                                                     const FGameplayTagContainer &StationTags,
                                                     const TArray<UMythicItemInstance *> &Inputs);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;
#endif
};
