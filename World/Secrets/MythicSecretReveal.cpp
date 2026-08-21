
#include "World/Secrets/MythicSecretReveal.h"

#include "World/Secrets/MythicSecretTypes.h"
#include "World/Secrets/MythicTags_Secrets.h"
#include "Player/MythicPlayerState.h"
#include "Narrative/MythicNarrativeStateComponent.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

bool FMythicSecretReveal::TryRevealSecret(APlayerController *PC, const FMythicSecretDef &Def, const FVector &RevealLocation) {
    if (!PC || !PC->HasAuthority()) {
        return false;
    }
    AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return false;
    }
    UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState();
    if (!Narrative) {
        return false;
    }
    const FGameplayTagContainer &OwnedTags = Narrative->GetOwnedTags();

    const bool bAlreadyFound = Def.FoundTag.IsValid() && OwnedTags.HasTagExact(Def.FoundTag);

    if (!FMythicSecretRules::CanReveal(bAlreadyFound, Def.RequireCondition, OwnedTags)) {
        return false;
    }


    Def.Rewards.Give(PC, true, 0, RevealLocation);

    if (Def.LoreTermTag.IsValid()) {
        if (UMythicCodexComponent *Codex = PS->GetCodexComponent()) {
            Codex->ServerDiscoverTerm(Def.LoreTermTag);
        }
    }

    if (Def.AchievementStoryTag.IsValid()) {
        Narrative->ServerSetStoryTag(Def.AchievementStoryTag);
    }

    if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
        Ledger->RecordStat(TAG_Stat_Secrets_Found, 1);
    }

    if (Def.FoundTag.IsValid()) {
        Narrative->ServerSetStoryTag(Def.FoundTag);
    }

    if (UMythicAbilitySystemComponent *ASC = PS->GetMythicAbilitySystemComponent()) {
        FGameplayCueParameters CueParams;
        CueParams.Location = RevealLocation.IsZero() && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : RevealLocation;
        CueParams.Instigator = PC->GetPawn();
        const FGameplayTag CueTag = Def.RevealCueTag.IsValid() ? Def.RevealCueTag : TAG_GameplayCue_World_SecretRevealed;
        ASC->ExecuteGameplayCueMulticast(CueTag, CueParams);
    }

    return true;
}
