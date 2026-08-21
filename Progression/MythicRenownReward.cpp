
#include "GAS/Progression/MythicRenownReward.h"

#include "GAS/Progression/MythicRenownComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Mythic.h"

#include "GameFramework/PlayerController.h"

bool URenownReward::Give(FRewardContext &Context) const {
    if (!Context.PlayerController) {
        UE_LOG(Myth, Error, TEXT("RenownReward::Give: no PlayerController in context"));
        return false;
    }
    const AMythicPlayerState *PS = Context.PlayerController->GetPlayerState<AMythicPlayerState>();
    UMythicRenownComponent *Renown = PS ? PS->GetRenownComponent() : nullptr;
    if (!Renown) {
        UE_LOG(Myth, Error, TEXT("RenownReward::Give: no RenownComponent on %s's PlayerState"),
               *GetNameSafe(Context.PlayerController));
        return false;
    }
    Renown->ServerGrantRenown(Scope, Amount);
    return true;
}

FText URenownReward::GetPreviewText() const {
    const FString Full = Scope.ToString();
    int32 LastDot = INDEX_NONE;
    Full.FindLastChar(TEXT('.'), LastDot);
    const FString Leaf = LastDot != INDEX_NONE ? Full.Mid(LastDot + 1) : Full;
    return FText::FromString(FString::Printf(TEXT("%+.0f %s Renown"), Amount, *Leaf));
}
