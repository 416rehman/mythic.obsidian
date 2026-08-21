
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FMythicTitleRules {
    static bool CanSelectTitle(FGameplayTag TitleTag, const FGameplayTagContainer &Granted) {
        return TitleTag.IsValid() && Granted.HasTagExact(TitleTag);
    }
};
