
#include "GAS/Progression/MythicTitleTypes.h"

#include "Mythic/Player/MythicPlayerState.h"
#include "Progression/MythicUnlockComponent.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"

#include "GameFramework/PlayerState.h"

FText UMythicTitleRegistry::GetTitleDisplayText(const UMythicTitleRegistry *Registry, FGameplayTag TitleTag) {
    if (!TitleTag.IsValid()) {
        return FText::GetEmpty();
    }
    if (Registry) {
        if (const FMythicTitleDef *Def = Registry->Find(TitleTag)) {
            return Def->Display;
        }
    }
    const FString Full = TitleTag.ToString();
    int32 LastDot = INDEX_NONE;
    Full.FindLastChar(TEXT('.'), LastDot);
    return FText::FromString(LastDot != INDEX_NONE ? Full.Mid(LastDot + 1) : Full);
}

FText UMythicTitleRegistry::GetActiveTitleText(const APlayerState *PlayerState) {
    const AMythicPlayerState *MythPS = Cast<AMythicPlayerState>(PlayerState);
    const UMythicUnlockComponent *Unlocks = MythPS ? MythPS->GetUnlockComponent() : nullptr;
    const FGameplayTag Active = Unlocks ? Unlocks->GetActiveTitleTag() : FGameplayTag();
    if (!Active.IsValid()) {
        return FText::GetEmpty();
    }

    const UMythicTitleRegistry *Registry = nullptr;
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        if (!Settings->DefaultTitleRegistry.IsNull()) {
            Registry = Settings->DefaultTitleRegistry.Get();
        }
    }
    return GetTitleDisplayText(Registry, Active);
}
