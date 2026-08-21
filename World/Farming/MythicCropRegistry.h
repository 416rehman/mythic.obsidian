
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicCropRegistry.generated.h"

class UMythicCropDefinition;

UCLASS(BlueprintType)
class MYTHIC_API UMythicCropRegistry : public UDataAsset {
    GENERATED_BODY()

public:
    // seed item-type tag (Item.Seed.*) → the crop it plants.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop Registry", meta = (Categories = "Item.Seed"))
    TMap<FGameplayTag, TObjectPtr<UMythicCropDefinition>> SeedToCrop;

    // Resolve the crop a seed's effective type-probe plants: the first SeedToCrop entry whose key the probe contains.
    // Returns nullptr if the probe carries no known seed tag. Pure lookup — no engine state.
    UFUNCTION(BlueprintCallable, Category = "Crop Registry")
    UMythicCropDefinition *ResolveCropForSeedProbe(const FGameplayTagContainer &SeedTypeProbe) const {
        for (const TPair<FGameplayTag, TObjectPtr<UMythicCropDefinition>> &Pair : SeedToCrop) {
            if (Pair.Key.IsValid() && Pair.Value && SeedTypeProbe.HasTag(Pair.Key)) {
                return Pair.Value;
            }
        }
        return nullptr;
    }
};
