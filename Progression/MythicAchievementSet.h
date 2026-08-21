#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicAchievementDefinition.h"
#include "MythicAchievementSet.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMythicAchievementSet : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievements")
    TArray<TObjectPtr<UMythicAchievementDefinition>> Achievements;
};
