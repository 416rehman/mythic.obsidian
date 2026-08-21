
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicCorpseTypes.h"
#include "MythicCorpseConfig.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMythicCorpseConfig : public UDataAsset {
    GENERATED_BODY()

public:
    // Base seconds a Normal-tier corpse persists before it is destroyed. Tier scales this up via
    // FMythicCorpseRules::DecayLifetimeForTier(Tier, DecayLifetime, DecayLifetimePerTier).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Decay", meta = (ClampMin = "1.0"))
    float DecayLifetime = 300.0f;

    // Ascending ages (seconds) at which the corpse advances Fresh -> Bloated -> Decayed -> Skeletal. Three entries
    // map cleanly onto the four stages; fewer entries simply cap the reachable stage.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Decay")
    TArray<float> StageThresholds = {60.0f, 150.0f, 240.0f};

    // Extra seconds of lifetime per enemy tier above Normal (Elite/Champion/Boss corpses linger longer).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Decay", meta = (ClampMin = "0.0"))
    float DecayLifetimePerTier = 120.0f;

    // If false, the corpse cannot be opened/looted (a pure decorative/raisable body).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Loot")
    bool bLootable = true;

    // If false, the corpse can never be raised by necromancy regardless of stage.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Necromancy")
    bool bRaisable = true;

    // Latest decomp stage from which the corpse may still be raised (inclusive). Default Decayed: a Skeletal
    // body is too far gone to reanimate as flesh.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse|Necromancy")
    EMythicDecompStage MaxRaisableStage = EMythicDecompStage::Decayed;
};
