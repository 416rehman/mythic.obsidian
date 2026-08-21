#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicAffixTierTypes.h"
#include "MythicAffixPoolDataAsset.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixPoolDataAsset : public UDataAsset {
    GENERATED_BODY()

public:
    // The tiered affix definitions this pool can roll. Prefix/suffix budget, item-level tier gating, applicability
    // (over the item's TypeProbe) and per-tier weights are all read from the entries by UAffixesFragment.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<FMythicTieredAffixDef> Defs;
};
