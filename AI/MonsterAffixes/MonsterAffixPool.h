
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MonsterAffixTypes.h"
#include "MonsterAffixPool.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMonsterAffixPool : public UDataAsset {
    GENERATED_BODY()

public:
    // Authored affix definitions. When empty, callers fall back to GetDefaultPool().
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    TArray<FMonsterAffixDef> Defs;

    static const TArray<FMonsterAffixDef> &GetDefaultPool();
};
