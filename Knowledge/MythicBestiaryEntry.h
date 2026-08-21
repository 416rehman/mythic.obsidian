
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "MythicBestiaryEntry.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MYTHIC_API UMythicBestiaryEntry : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // The archetype's stable codex identity — the key kills/encounters are recorded under (must match what the
    // creature's ASC stamp / the NPC.Type derivation produces; see FMythicBestiaryRules::MakeBestiaryKeyFromOwnedTags).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Codex.Bestiary"))
    FGameplayTag CodexKey;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText DisplayName;

    // Flavor/lore paragraph — revealed at Basic tier.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
    FText Lore;

    // Sighted-tier art: the mystery silhouette shown before the player has killed enough for the real portrait.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
    TSoftObjectPtr<UTexture2D> Silhouette;

    // Basic-tier art: the full portrait.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
    TSoftObjectPtr<UTexture2D> Portrait;

    // Authored combat intel — revealed at FULL tier only (FMythicBestiaryRules::RevealsResistances). Authored (not
    // derived from live GEs) so the codex shows the designed identity of the archetype, not a snapshot of one spawn.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intel", meta = (Categories = "DamageType"))
    FGameplayTagContainer Resistances;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intel", meta = (Categories = "DamageType"))
    FGameplayTagContainer Weaknesses;

    // "Known to carry ..." hint — revealed at Basic tier.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intel", meta = (MultiLine = true))
    FText DropHint;

    // Kills needed for the Basic tier (portrait/lore/drop hints). Sanitized to >= 1 by the tier rule.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thresholds", meta = (ClampMin = "1"))
    int32 KillThresholdBasic = 1;

    // Kills needed for the Full tier (resistances/weaknesses). Sanitized to >= KillThresholdBasic by the tier rule.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thresholds", meta = (ClampMin = "1"))
    int32 KillThresholdFull = 10;
};
