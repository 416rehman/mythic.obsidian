
#include "MythicAchievementComponent.h"

#include "MythicAchievementSet.h"
#include "MythicAchievementDefinition.h"
#include "MythicAchievementCondition.h"
#include "MythicStatLedgerComponent.h"

#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "Mythic/Mythic.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UMythicAchievementComponent::UMythicAchievementComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicAchievementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicAchievementComponent, UnlockedAchievements, COND_OwnerOnly);
}


void UMythicAchievementComponent::BeginPlay() {
    Super::BeginPlay();
    ResolveSet();
    BuildIndex();

    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    if (UMythicStatLedgerComponent *Ledger = ResolveLedger()) {
        Ledger->OnCounterChanged.RemoveDynamic(this, &UMythicAchievementComponent::HandleCounterChanged);
        Ledger->OnCounterChanged.AddDynamic(this, &UMythicAchievementComponent::HandleCounterChanged);
    }
    if (UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Narrative->OnStoryTagEarned.RemoveDynamic(this, &UMythicAchievementComponent::HandleStoryTagEarned);
        Narrative->OnStoryTagEarned.AddDynamic(this, &UMythicAchievementComponent::HandleStoryTagEarned);
    }
}

void UMythicAchievementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UMythicStatLedgerComponent *Ledger = ResolveLedger()) {
        Ledger->OnCounterChanged.RemoveDynamic(this, &UMythicAchievementComponent::HandleCounterChanged);
    }
    if (UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Narrative->OnStoryTagEarned.RemoveDynamic(this, &UMythicAchievementComponent::HandleStoryTagEarned);
    }
    Super::EndPlay(EndPlayReason);
}


UMythicStatLedgerComponent *UMythicAchievementComponent::ResolveLedger() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetStatLedgerComponent();
    }
    return nullptr;
}

UMythicNarrativeStateComponent *UMythicAchievementComponent::ResolveNarrative() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetNarrativeState();
    }
    return nullptr;
}

APlayerController *UMythicAchievementComponent::ResolvePC() const {
    if (const APlayerState *PS = Cast<APlayerState>(GetOwner())) {
        return PS->GetPlayerController();
    }
    return nullptr;
}

UMythicAchievementSet *UMythicAchievementComponent::ResolveSet() {
    if (ResolvedSet) {
        return ResolvedSet;
    }
    if (!AchievementSet.IsNull()) {
        ResolvedSet = AchievementSet.LoadSynchronous();
    }
    if (!ResolvedSet) {
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            if (!Settings->DefaultAchievementSet.IsNull()) {
                ResolvedSet = Settings->DefaultAchievementSet.LoadSynchronous();
            }
        }
    }
    return ResolvedSet;
}


void UMythicAchievementComponent::BuildIndex() {
    ExactStatIndex.Reset();
    HierStatIndex.Reset();
    TagTriggerIndex.Reset();
    bIndexBuilt = true;

    if (!ResolvedSet) {
        return;
    }
    for (int32 i = 0; i < ResolvedSet->Achievements.Num(); ++i) {
        const UMythicAchievementDefinition *Def = ResolvedSet->Achievements[i];
        if (!Def || !Def->AchievementTag.IsValid()) {
            continue;
        }
        const FMythicAchievementCondition &Cond = Def->Condition;
        for (const FMythicStatRequirement &Req : Cond.StatRequirements) {
            if (!Req.StatTag.IsValid()) {
                continue;
            }
            (Req.bHierarchical ? HierStatIndex : ExactStatIndex).FindOrAdd(Req.StatTag).Add(i);
        }
        auto IndexTags = [this, i](const FGameplayTagContainer &Tags) {
            for (const FGameplayTag &T : Tags.GetGameplayTagArray()) {
                if (T.IsValid()) {
                    TagTriggerIndex.FindOrAdd(T).Add(i);
                }
            }
        };
        IndexTags(Cond.TagCondition.RequireAll);
        IndexTags(Cond.TagCondition.RequireAny);
    }
}


void UMythicAchievementComponent::HandleCounterChanged(FGameplayTag Tag, int64) {
    QueueAffectedByStat(Tag);
    DrainPending();
}

void UMythicAchievementComponent::HandleStoryTagEarned(FGameplayTag Tag) {
    QueueAffectedByTag(Tag);
    DrainPending();
}

void UMythicAchievementComponent::QueueAffectedByStat(const FGameplayTag &StatTag) {
    if (!StatTag.IsValid()) {
        return;
    }
    if (const TArray<int32> *Arr = ExactStatIndex.Find(StatTag)) {
        PendingIndices.Append(*Arr);
    }
    FGameplayTag Cur = StatTag;
    while (Cur.IsValid()) {
        if (const TArray<int32> *Arr = HierStatIndex.Find(Cur)) {
            PendingIndices.Append(*Arr);
        }
        Cur = Cur.RequestDirectParent();
    }
}

void UMythicAchievementComponent::QueueAffectedByTag(const FGameplayTag &Tag) {
    if (!Tag.IsValid()) {
        return;
    }
    FGameplayTag Cur = Tag;
    while (Cur.IsValid()) {
        if (const TArray<int32> *Arr = TagTriggerIndex.Find(Cur)) {
            PendingIndices.Append(*Arr);
        }
        Cur = Cur.RequestDirectParent();
    }
}

void UMythicAchievementComponent::DrainPending() {
    const AActor *Owner = GetOwner();
    if (bIsRestoring || !Owner || !Owner->HasAuthority()) {
        PendingIndices.Reset();
        return;
    }
    if (bEvaluating) {
        return;
    }
    bEvaluating = true;
    int32 Safety = 0;
    while (PendingIndices.Num() > 0 && Safety++ < 128) {
        TSet<int32> Batch = MoveTemp(PendingIndices);
        PendingIndices.Reset();
        EvaluateBatch(Batch);
    }
    bEvaluating = false;
}


void UMythicAchievementComponent::EvaluateBatch(const TSet<int32> &Indices) {
    if (!ResolvedSet || Indices.Num() == 0) {
        return;
    }
    UMythicStatLedgerComponent *Ledger = ResolveLedger();
    UMythicNarrativeStateComponent *Narrative = ResolveNarrative();

    FGameplayTagContainer Owned;
    if (Narrative) {
        Owned.AppendTags(Narrative->GetOwnedTags());
    }
    Owned.AppendTags(UnlockedAchievements);

    auto StatLookup = [Ledger](FGameplayTag Tag, bool bHierarchical) -> int64 {
        if (!Ledger) {
            return 0;
        }
        return bHierarchical ? Ledger->GetCounterRollup(Tag) : Ledger->GetCounter(Tag);
    };

    for (int32 Idx : Indices) {
        if (!ResolvedSet->Achievements.IsValidIndex(Idx)) {
            continue;
        }
        const UMythicAchievementDefinition *Def = ResolvedSet->Achievements[Idx];
        if (!Def || !Def->AchievementTag.IsValid()) {
            continue;
        }
        if (UnlockedAchievements.HasTagExact(Def->AchievementTag)) {
            continue;
        }
        if (FMythicAchievementCondition::Evaluate(Def->Condition, Owned, StatLookup)) {
            UnlockAchievement(*Def);
        }
    }
}

void UMythicAchievementComponent::UnlockAchievement(const UMythicAchievementDefinition &Def) {
    UnlockedAchievements.AddTag(Def.AchievementTag);
    UE_LOG(Myth, Log, TEXT("Achievements: %s unlocked '%s'"), *GetNameSafe(GetOwner()), *Def.AchievementTag.ToString());

    if (UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Narrative->ServerSetStoryTag(Def.AchievementTag);
    }

    if (!bIsRestoring) {
        if (APlayerController *PC = ResolvePC()) {
            Def.Reward.Give(PC);
        }
    }

    OnAchievementUnlocked.Broadcast(Def.AchievementTag);
}


void UMythicAchievementComponent::RestoreUnlockedAchievements(const FGameplayTagContainer &Saved) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    UnlockedAchievements = Saved;
}
