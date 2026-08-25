#include "CharacterData.h"
#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/MythicPlayerController.h"
#include "Mythic/Player/MythicFactionStandingComponent.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Narrative/MythicQuestJournalComponent.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/MythicAchievementComponent.h"
#include "Progression/MythicUnlockComponent.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Mythic/GAS/Progression/MythicRenownComponent.h"
#include "Mythic/GAS/Mounts/MythicMountRosterComponent.h"
#include "Mythic/World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "Mythic/World/LivingWorld/Chronicle/MythicDossierComponent.h"
#include "Mythic/World/Trading/MythicTradeContractComponent.h"
#include "Mythic/World/LivingWorld/LivingWorldTypes.h"
#include "Mythic/Objectives/ObjectiveTracker.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace {
    void ResolveCharacterActors(AActor *InActor, APlayerState *&OutPS, APlayerController *&OutPC) {
        OutPS = Cast<APlayerState>(InActor);
        OutPC = Cast<APlayerController>(InActor);
        if (OutPS && !OutPC) {
            OutPC = OutPS->GetPlayerController();
        }
        else if (OutPC && !OutPS) {
            OutPS = OutPC->PlayerState;
        }
    }
}

bool FSerializedCharacterData::Serialize(AActor *SourceActor, FSerializedCharacterData &OutData) {
    if (!SourceActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Serialize: Invalid SourceActor"));
        return false;
    }

    APlayerState *PS = nullptr;
    APlayerController *PC = nullptr;
    ResolveCharacterActors(SourceActor, PS, PC);
    AActor *ProfHost = PC ? static_cast<AActor *>(PC) : SourceActor;
    AActor *InvHost = PC ? static_cast<AActor *>(PC) : SourceActor;

    if (PS) {
        OutData.CharacterName = PS->GetPlayerName();
    }

    if (PC) {
        if (const APawn *Pawn = PC->GetPawn()) {
            OutData.SavedTransform = Pawn->GetActorTransform();
            OutData.bHasSavedTransform = true;
        }
    }

    UProficiencyComponent *ProfComp = nullptr;
    UObjectiveTracker *Tracker = nullptr;
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        ProfComp = MythicPC->GetProficiencyComponent();
        Tracker = MythicPC->GetObjectiveTracker();
    }
    else {
        ProfComp = ProfHost->FindComponentByClass<UProficiencyComponent>();
        Tracker = ProfHost->FindComponentByClass<UObjectiveTracker>();
    }

    if (ProfComp) {
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found ProficiencyComponent, serializing %d proficiencies..."),
               ProfComp->Proficiencies.Num());
        FSerializedProficiencyHelper::Serialize(ProfComp, OutData.Proficiencies);
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d proficiencies"), OutData.Proficiencies.Num());
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: No ProficiencyComponent found!"));
    }

    if (Tracker) {
        Tracker->SaveObjectives(OutData.Objectives);
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d objectives"), OutData.Objectives.Num());
    }

    if (const AMythicPlayerState *MythPS = Cast<AMythicPlayerState>(PS)) {
        if (const UMythicFactionStandingComponent *Faction = MythPS->GetFactionStanding()) {
            for (const FMythicFactionStandingEntry &Entry : Faction->GetStandings()) {
                if (FSerializedFactionStandingHelper::ShouldPersist(Entry.Faction.Index, Entry.Value)) {
                    FSerializedFactionStandingData &Out = OutData.FactionStandings.AddDefaulted_GetRef();
                    Out.FactionIndex = Entry.Faction.Index;
                    Out.Value = Entry.Value;
                }
            }
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d faction standings"),
                   OutData.FactionStandings.Num());
        }

        if (const UMythicNarrativeStateComponent *Narrative = MythPS->GetNarrativeState()) {
            OutData.StoryTags = Narrative->GetOwnedTags().GetGameplayTagArray();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d story tags"),
                   OutData.StoryTags.Num());
        }

        if (const UMythicStatLedgerComponent *Ledger = MythPS->GetStatLedgerComponent()) {
            OutData.StatCounters = Ledger->GetCharacterCounters();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d stat counters"),
                   OutData.StatCounters.Num());
        }

        if (const UMythicAchievementComponent *Achievements = MythPS->GetAchievementComponent()) {
            OutData.UnlockedAchievements = Achievements->GetUnlockedAchievements();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d unlocked achievements"),
                   OutData.UnlockedAchievements.Num());
        }

        if (const UMythicUnlockComponent *Unlocks = MythPS->GetUnlockComponent()) {
            OutData.GrantedUnlockTags = Unlocks->GetGrantedUnlockTags();
            OutData.AppliedUnlockRules = Unlocks->GetAppliedUnlockRules();
            OutData.ActiveTitle = Unlocks->GetActiveTitleTag();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d unlock tags (%d applied rules)"),
                   OutData.GrantedUnlockTags.Num(), OutData.AppliedUnlockRules.Num());
        }

        if (const UMythicRuneComponent *RuneComp = MythPS->GetRuneComponent()) {
            for (const TSoftObjectPtr<UMythicRuneDefinition> &Rune : RuneComp->GetEquippedRunes()) {
                OutData.EquippedRunes.Add(Rune.ToSoftObjectPath());
            }
            OutData.UnlockedRuneSlots = RuneComp->GetUnlockedSlots();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d rune sockets (%d open)"),
                   OutData.EquippedRunes.Num(), OutData.UnlockedRuneSlots);
        }

        if (const UMythicSkillComponent *SkillComp = MythPS->GetSkillComponent()) {
            for (const TSoftObjectPtr<UMythicSkillDefinition> &Skill : SkillComp->GetEquippedSkills()) {
                OutData.EquippedSkills.Add(Skill.ToSoftObjectPath());
            }
            OutData.UnlockedSkillSlots = SkillComp->GetUnlockedSlots();
            OutData.SkillProgress = SkillComp->GetSkillProgress();
            OutData.SkillModifierCapacity = SkillComp->GetModifierCapacity();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d skill slots (%d open), %d levelled skills (%d modifiers at once)"),
                   OutData.EquippedSkills.Num(), OutData.UnlockedSkillSlots, OutData.SkillProgress.Num(),
                   OutData.SkillModifierCapacity);
        }

        if (const UMythicCodexComponent *CodexComp = MythPS->GetCodexComponent()) {
            OutData.CodexBestiary = CodexComp->GetAllBestiaryRecords();
            OutData.CodexTerms = CodexComp->GetDiscoveredTerms().GetGameplayTagArray();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d bestiary records, %d codex terms"),
                   OutData.CodexBestiary.Num(), OutData.CodexTerms.Num());
        }

        if (const UMythicRenownComponent *RenownComp = MythPS->GetRenownComponent()) {
            OutData.RenownEntries = RenownComp->GetRenownEntries();
            OutData.GlobalRenown = RenownComp->GetGlobalRenown();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d renown scopes (global %.1f)"),
                   OutData.RenownEntries.Num(), OutData.GlobalRenown);
        }

        if (const UMythicMountRosterComponent *Mounts = MythPS->GetMountRosterComponent()) {
            OutData.MountRoster = Mounts->GetRoster();
            OutData.ActiveMountId = Mounts->GetActiveMountId();
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d mounts (active %s)"),
                   OutData.MountRoster.Num(), *OutData.ActiveMountId.ToString(EGuidFormats::Short));
        }

        if (const UMythicAcquaintanceComponent *Acquaintance = MythPS->GetAcquaintanceComponent()) {
            OutData.NpcRelations = Acquaintance->GetRelations();
        }
        if (const UMythicDossierComponent *DossierComp = MythPS->GetDossierComponent()) {
            OutData.NpcDossiers = DossierComp->GetDossiers();
        }
        if (OutData.NpcRelations.Num() > 0 || OutData.NpcDossiers.Num() > 0) {
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d NPC relations, %d dossiers"),
                   OutData.NpcRelations.Num(), OutData.NpcDossiers.Num());
        }

        if (const UMythicTradeContractComponent *Trade = MythPS->GetTradeContractComponent()) {
            OutData.TradeContracts = Trade->GetContracts();
            if (OutData.TradeContracts.Num() > 0) {
                UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d trade contracts"),
                       OutData.TradeContracts.Num());
            }
        }

        if (const UMythicQuestJournalComponent *Journal = MythPS->GetQuestJournal()) {
            Journal->GetSerializableJournal(OutData.QuestJournal, OutData.ActiveStorylines, OutData.CompletedStorylines);
            if (OutData.QuestJournal.Num() > 0 || OutData.ActiveStorylines.Num() > 0 || OutData.CompletedStorylines.Num() > 0) {
                UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d journal quests, %d active / %d completed storylines"),
                       OutData.QuestJournal.Num(), OutData.ActiveStorylines.Num(), OutData.CompletedStorylines.Num());
            }
        }
    }

    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(InvHost)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found %d inventory components"), InventoryComponents.Num());

        for (UMythicInventoryComponent *InventoryComp : InventoryComponents) {
            if (InventoryComp) {
                FSerializedInventoryData InvData;
                FSerializedInventoryData::Serialize(InventoryComp, InvData);
                OutData.Inventories.Add(InvData);
                UE_LOG(MythSaveLoad, Log, TEXT("  - Serialized inventory '%s' with %d slots"), *InventoryComp->GetName(), InvData.Slots.Num());
            }
        }
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: SourceActor does not implement IInventoryProviderInterface!"));
    }


    OutData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);

    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized character '%s'"), *OutData.CharacterName);
    return true;
}

bool FSerializedCharacterData::Deserialize(AActor *TargetActor, const FSerializedCharacterData &InData) {
    if (!TargetActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Deserialize: Invalid TargetActor"));
        return false;
    }

    if (!TargetActor->HasAuthority()) {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Deserialize: ignored on non-authority actor [%s]"),
               *GetNameSafe(TargetActor));
        return false;
    }

    APlayerState *PS = nullptr;
    APlayerController *PC = nullptr;
    ResolveCharacterActors(TargetActor, PS, PC);
    AActor *ProfHost = PC ? static_cast<AActor *>(PC) : TargetActor;
    AActor *InvHost = PC ? static_cast<AActor *>(PC) : TargetActor;

    // A save with no name must not blank one the manifest already set. Saves written before a character had a
    // name carry an empty string, and applying it would undo the name every load.
    if (PS && !InData.CharacterName.IsEmpty()) {
        PS->SetPlayerName(InData.CharacterName);
    }

    if (InData.bHasSavedTransform && PC) {
        if (APawn *Pawn = PC->GetPawn()) {
            Pawn->SetActorTransform(InData.SavedTransform,false,nullptr, ETeleportType::TeleportPhysics);
        }
    }

    UProficiencyComponent *ProfComp = nullptr;
    UObjectiveTracker *Tracker = nullptr;
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        ProfComp = MythicPC->GetProficiencyComponent();
        Tracker = MythicPC->GetObjectiveTracker();
    }
    else {
        ProfComp = ProfHost->FindComponentByClass<UProficiencyComponent>();
        Tracker = ProfHost->FindComponentByClass<UObjectiveTracker>();
    }

    if (ProfComp) {
        FSerializedProficiencyHelper::Deserialize(ProfComp, InData.Proficiencies);
        ProfComp->ApplyLoadedProficiencies();
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d proficiencies"), ProfComp->Proficiencies.Num());
    }

    if (Tracker) {
        Tracker->RestoreObjectives(InData.Objectives);
    }

    if (AMythicPlayerState *MythPS = Cast<AMythicPlayerState>(PS)) {
        UMythicAchievementComponent *AchievementsToRestore = MythPS->GetAchievementComponent();
        UMythicUnlockComponent *UnlocksToRestore = MythPS->GetUnlockComponent();
        if (AchievementsToRestore) {
            AchievementsToRestore->SetRestoring(true);
        }
        if (UnlocksToRestore) {
            UnlocksToRestore->SetRestoring(true);
        }

        if (UMythicFactionStandingComponent *Faction = MythPS->GetFactionStanding()) {
            for (const FSerializedFactionStandingData &Data : InData.FactionStandings) {
                FMythicFactionId Id;
                Id.Index = Data.FactionIndex;
                Faction->SetStanding(Id, Data.Value);
            }
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d faction standings"),
                   InData.FactionStandings.Num());
        }

        if (UMythicNarrativeStateComponent *Narrative = MythPS->GetNarrativeState()) {
            for (const FGameplayTag &Tag : InData.StoryTags) {
                Narrative->ServerSetStoryTag(Tag);
            }
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d story tags"),
                   InData.StoryTags.Num());
        }

        if (UMythicStatLedgerComponent *Ledger = MythPS->GetStatLedgerComponent()) {
            Ledger->RestoreCharacterCounters(InData.StatCounters);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d stat counters"),
                   InData.StatCounters.Num());
        }

        if (UMythicCodexComponent *CodexComp = MythPS->GetCodexComponent()) {
            CodexComp->RestoreCodex(InData.CodexBestiary, InData.CodexTerms);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d bestiary records, %d codex terms"),
                   InData.CodexBestiary.Num(), InData.CodexTerms.Num());
        }

        if (UMythicRenownComponent *RenownComp = MythPS->GetRenownComponent()) {
            RenownComp->RestoreRenown(InData.RenownEntries, InData.GlobalRenown);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d renown scopes (global %.1f)"),
                   InData.RenownEntries.Num(), InData.GlobalRenown);
        }

        if (UMythicMountRosterComponent *Mounts = MythPS->GetMountRosterComponent()) {
            Mounts->RestoreRoster(InData.MountRoster, InData.ActiveMountId);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d mounts"),
                   InData.MountRoster.Num());
        }

        if (UMythicAcquaintanceComponent *Acquaintance = MythPS->GetAcquaintanceComponent()) {
            Acquaintance->RestoreRelations(InData.NpcRelations);
        }
        if (UMythicDossierComponent *DossierComp = MythPS->GetDossierComponent()) {
            DossierComp->RestoreDossiers(InData.NpcDossiers);
        }
        if (InData.NpcRelations.Num() > 0 || InData.NpcDossiers.Num() > 0) {
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d NPC relations, %d dossiers"),
                   InData.NpcRelations.Num(), InData.NpcDossiers.Num());
        }

        if (UMythicTradeContractComponent *Trade = MythPS->GetTradeContractComponent()) {
            Trade->RestoreContracts(InData.TradeContracts);
            if (InData.TradeContracts.Num() > 0) {
                UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d trade contracts"),
                       InData.TradeContracts.Num());
            }
        }

        if (UMythicQuestJournalComponent *Journal = MythPS->GetQuestJournal()) {
            Journal->RestoreQuests(InData.QuestJournal, InData.ActiveStorylines, InData.CompletedStorylines);
            if (InData.QuestJournal.Num() > 0 || InData.ActiveStorylines.Num() > 0 || InData.CompletedStorylines.Num() > 0) {
                UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d journal quests, %d active / %d completed storylines"),
                       InData.QuestJournal.Num(), InData.ActiveStorylines.Num(), InData.CompletedStorylines.Num());
            }
        }

        if (AchievementsToRestore) {
            AchievementsToRestore->RestoreUnlockedAchievements(InData.UnlockedAchievements);
            AchievementsToRestore->SetRestoring(false);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d unlocked achievements"),
                   InData.UnlockedAchievements.Num());
        }
        if (UnlocksToRestore) {
            UnlocksToRestore->RestoreUnlockState(InData.GrantedUnlockTags, InData.AppliedUnlockRules, InData.ActiveTitle);
            UnlocksToRestore->SetRestoring(false);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d unlock tags (%d applied rules)"),
                   InData.GrantedUnlockTags.Num(), InData.AppliedUnlockRules.Num());
        }

        // Last, after the story/achievement/unlock ledgers above: IsRuneUnlocked reads all three, so a rune gated on a
        // deed the player has actually earned would otherwise be dropped as unearned.
        UMythicRuneComponent *RuneComp = MythPS->GetRuneComponent();
        if (RuneComp && InData.UnlockedRuneSlots > 0) {
            TArray<TSoftObjectPtr<UMythicRuneDefinition>> SavedRunes;
            SavedRunes.Reserve(InData.EquippedRunes.Num());
            for (const FSoftObjectPath &RunePath : InData.EquippedRunes) {
                SavedRunes.Emplace(RunePath);
            }
            RuneComp->RestoreRunes(SavedRunes, InData.UnlockedRuneSlots);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d rune sockets (%d open)"),
                   InData.EquippedRunes.Num(), InData.UnlockedRuneSlots);
        }

        // Beside the runes, and for the same reason: IsSkillUnlocked reads the achievement, unlock and story ledgers
        // restored above, so a skill gated on an earned deed would otherwise be dropped as unearned.
        UMythicSkillComponent *SkillComp = MythPS->GetSkillComponent();
        if (SkillComp && InData.UnlockedSkillSlots > 0) {
            TArray<TSoftObjectPtr<UMythicSkillDefinition>> SavedSkills;
            SavedSkills.Reserve(InData.EquippedSkills.Num());
            for (const FSoftObjectPath &SkillPath : InData.EquippedSkills) {
                SavedSkills.Emplace(SkillPath);
            }
            SkillComp->RestoreSkills(SavedSkills, InData.UnlockedSkillSlots);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d skill slots (%d open)"),
                   InData.EquippedSkills.Num(), InData.UnlockedSkillSlots);
        }

        // Growth is keyed on the skill, not the slot, so it restores whether or not the bar above filled.
        if (SkillComp && InData.SkillModifierCapacity > 0) {
            SkillComp->RestoreSkillProgress(InData.SkillProgress, InData.SkillModifierCapacity);
            UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d levelled skills (%d modifiers at once)"),
                   InData.SkillProgress.Num(), InData.SkillModifierCapacity);
        }
    }

    UMythicStatLedgerComponent *LedgerToGuard = nullptr;
    if (AMythicPlayerState *LedgerPS = Cast<AMythicPlayerState>(PS)) {
        LedgerToGuard = LedgerPS->GetStatLedgerComponent();
    }
    if (LedgerToGuard) {
        LedgerToGuard->SetRestoring(true);
    }

    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(InvHost)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        for (int32 i = 0; i < InventoryComponents.Num() && i < InData.Inventories.Num(); ++i) {
            if (InventoryComponents[i]) {
                FSerializedInventoryData::Deserialize(InventoryComponents[i], InData.Inventories[i]);
            }
        }
    }

    if (LedgerToGuard) {
        LedgerToGuard->SetRestoring(false);
        LedgerToGuard->ResyncCurrencyBaseline();
    }


    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Applied character '%s'"), *InData.CharacterName);
    return true;
}
