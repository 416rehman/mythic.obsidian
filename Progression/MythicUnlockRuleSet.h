#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicUnlockRule.h"
#include "MythicUnlockRuleSet.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMythicUnlockRuleSet : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    TArray<TObjectPtr<UMythicUnlockRule>> Rules;
};
