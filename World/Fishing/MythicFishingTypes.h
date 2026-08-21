#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "MythicFishingTypes.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct FMythicCatchTableEntry {
    GENERATED_BODY()

    // The item granted when this entry is picked.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    TObjectPtr<UItemDefinition> Item = nullptr;

    // Inclusive stack range rolled when granted (stackables honour it; non-stackables effectively grant 1).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    FInt32Interval StackRange = FInt32Interval(1, 1);

    // Relative pick weight among eligible entries. <= 0 ⇒ never picked (unless OverrideChance is positive).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    // Minimum fishing proficiency level required for this entry to be eligible (a rarer fish gated behind skill).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing", meta = (ClampMin = "0"))
    int32 MinProficiency = 0;

    // Tool tags the fisher must own for this entry (e.g. a specific rod). Empty ⇒ any/none. Matched with HasAll.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    FGameplayTagContainer RequiredTool;

    // Bait tags the fisher must own for this entry. Empty ⇒ no bait requirement. Matched with HasAll.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    FGameplayTagContainer RequiredBait;

    // Locations this entry appears in (the spot's location tag(s)). Empty ⇒ appears anywhere. Matched with HasAny.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    FGameplayTagContainer Location;

    // Positive ⇒ use THIS as the entry's pick weight instead of Weight (the override-wins rule). 0 ⇒ use Weight.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing", meta = (ClampMin = "0.0"))
    float OverrideChance = 0.0f;


    // World-condition gate: Environment.Weather.* / Environment.Time.* / Environment.Season.* tags that must ALL be
    // current for this entry to be eligible (a night-only fish, a winter-run salmon). EMPTY ⇒ always eligible (the
    // backward-compatible default — P1i's RequiredConditions contract). Matched with HasAll against the catch
    // context's Conditions (built by the ability from UMythicEnvironmentSubsystem at cast time).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Wave P")
    FGameplayTagContainer RequiredConditions;

    // TRASH tier (boots, kelp, rusty cans): the ONLY entries an EXHAUSTED spot still yields (P3i degraded table), and
    // the tier the auto-resolve mastery valve skips the minigame for (P1i). Default false = a real catch.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Wave P")
    bool bTrashTier = false;

    // Size range (design units, e.g. cm) rolled per catch for the records ledger. Max <= 0 ⇒ no size is rolled
    // (trash/non-fish entries). P6i.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Wave P")
    FFloatInterval SizeRange = FFloatInterval(0.0f, 0.0f);

    // Per-species record ledger key (e.g. Stat.Fish.Record.Trout). Valid + a rolled size ⇒ the catch is scored via
    // FMythicStatLedger::ApplyMax (record-if-greater). Unset ⇒ no record tracking for this entry. P6i.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Wave P", meta = (Categories = "Stat"))
    FGameplayTag RecordStatTag;

    // Trophy item DEF minted (in addition to the catch) when this entry sets a NEW personal size record. Inert-but-
    // tradable now; K's trophy wall consumes Trophy.* later (C13 forward contract). Unset ⇒ no trophy. P6i.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing|Wave P")
    TObjectPtr<UItemDefinition> TrophyItem = nullptr;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicFishCatchTable : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fishing")
    TArray<FMythicCatchTableEntry> Entries;
};

USTRUCT(BlueprintType)
struct FMythicCatchContext {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    int32 ProficiencyLevel = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    FGameplayTagContainer Tool;

    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    FGameplayTagContainer Bait;

    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    FGameplayTagContainer Location;

    // Current world conditions (Environment.Weather.* / Time.* / Season.* tags) matched against each entry's
    // RequiredConditions. Empty ⇒ only condition-less entries pass their (empty) gate — which is all legacy entries.
    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    FGameplayTagContainer Conditions;

    // P3i degraded table: TRUE at an EXHAUSTED spot — only bTrashTier entries stay eligible (soggy boots until the
    // stock regenerates). Default false = the full table (backward-compatible).
    UPROPERTY(BlueprintReadWrite, Category = "Fishing")
    bool bTrashOnly = false;
};

namespace MythicFishing {
    inline float ResolveEntryWeight(const FMythicCatchTableEntry &Entry) {
        const float W = (Entry.OverrideChance > 0.0f) ? Entry.OverrideChance : Entry.Weight;
        return (W > 0.0f) ? W : 0.0f;
    }

    inline bool IsEntryEligible(const FMythicCatchTableEntry &Entry, const FMythicCatchContext &Ctx) {
        if (Ctx.ProficiencyLevel < Entry.MinProficiency) {
            return false;
        }
        if (Ctx.bTrashOnly && !Entry.bTrashTier) {
            return false;
        }
        if (!Entry.RequiredTool.IsEmpty() && !Ctx.Tool.HasAll(Entry.RequiredTool)) {
            return false;
        }
        if (!Entry.RequiredBait.IsEmpty() && !Ctx.Bait.HasAll(Entry.RequiredBait)) {
            return false;
        }
        if (!Entry.Location.IsEmpty() && !Ctx.Location.HasAny(Entry.Location)) {
            return false;
        }
        if (!Entry.RequiredConditions.IsEmpty() && !Ctx.Conditions.HasAll(Entry.RequiredConditions)) {
            return false;
        }
        return true;
    }

    inline int32 PickCatchIndex(TConstArrayView<FMythicCatchTableEntry> Entries, const FMythicCatchContext &Ctx, float Roll01) {
        float TotalWeight = 0.0f;
        for (const FMythicCatchTableEntry &E : Entries) {
            if (IsEntryEligible(E, Ctx)) {
                TotalWeight += ResolveEntryWeight(E);
            }
        }
        if (TotalWeight <= 0.0f) {
            return -1;
        }

        const float Clamped = FMath::Clamp(Roll01, 0.0f, 1.0f - KINDA_SMALL_NUMBER);
        const float Target = Clamped * TotalWeight;

        float Accum = 0.0f;
        int32 LastEligible = -1;
        for (int32 i = 0; i < Entries.Num(); ++i) {
            const FMythicCatchTableEntry &E = Entries[i];
            if (!IsEntryEligible(E, Ctx)) {
                continue;
            }
            const float W = ResolveEntryWeight(E);
            if (W <= 0.0f) {
                continue;
            }
            LastEligible = i;
            Accum += W;
            if (Target < Accum) {
                return i;
            }
        }
        return LastEligible;
    }
}
