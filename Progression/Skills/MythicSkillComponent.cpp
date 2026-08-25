
#include "MythicSkillComponent.h"

#include "MythicSkillDefinition.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic/Input/MythicTags_Input.h"
#include "Mythic/Mythic.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicStatLedgerComponent.h"
#include "Mythic/Progression/MythicTags_MetaProgression.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "Net/UnrealNetwork.h"

UMythicSkillComponent::UMythicSkillComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    SlotInputTags = {INPUT_ACTION_SKILL_Q.GetTag(), INPUT_ACTION_SKILL_E.GetTag()};
}

void UMythicSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, EquippedSkills, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, UnlockedSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, SkillProgress, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, ModifierCapacity, COND_OwnerOnly);
}

void UMythicSkillComponent::BeginPlay() {
    Super::BeginPlay();

    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        SizeSlots();
    }
}

void UMythicSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        for (int32 SlotIndex = 0; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
            ClearSlotAbility(SlotIndex);
        }
    }
    Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent *UMythicSkillComponent::ResolveASC() const {
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UMythicSkillComponent::SizeSlots() {
    const int32 Slots = FMath::Max(MaxSlots, 1);
    // Lowering MaxSlots drops slots: hand their abilities back before the entries go.
    for (int32 SlotIndex = Slots; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
    }
    if (EquippedSkills.Num() != Slots) {
        EquippedSkills.SetNum(Slots);
    }
    if (GrantedHandles.Num() != Slots) {
        GrantedHandles.SetNum(Slots);
    }
}

void UMythicSkillComponent::ClearSlotAbility(int32 SlotIndex) {
    if (!GrantedHandles.IsValidIndex(SlotIndex) || !GrantedHandles[SlotIndex].IsValid()) {
        return;
    }
    if (UAbilitySystemComponent *ASC = ResolveASC()) {
        ASC->ClearAbility(GrantedHandles[SlotIndex]);
    }
    GrantedHandles[SlotIndex] = FGameplayAbilitySpecHandle();
}

FGameplayTag UMythicSkillComponent::GetSlotInputTag(int32 SlotIndex) const {
    return SlotInputTags.IsValidIndex(SlotIndex) ? SlotInputTags[SlotIndex] : FGameplayTag();
}

FGameplayAbilitySpecHandle UMythicSkillComponent::GrantSlotAbility(UAbilitySystemComponent *ASC, UMythicSkillDefinition *Skill, int32 SlotIndex) {
    FGameplayAbilitySpec Spec(Skill->Ability, 1, INDEX_NONE, Skill);

    // The slot IS the key binding: AbilityInputTagPressed activates whatever spec carries the pressed tag.
    const FGameplayTag InputTag = GetSlotInputTag(SlotIndex);
    if (InputTag.IsValid()) {
        Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("Skills: slot %d has no input tag — '%s' is granted but no key will fire it."), SlotIndex,
               *GetNameSafe(Skill));
    }

    return ASC->GiveAbility(Spec);
}

bool UMythicSkillComponent::IsSlotUnlocked(int32 SlotIndex) const {
    return SlotIndex >= 0 && SlotIndex < GetUnlockedSlots();
}

bool UMythicSkillComponent::IsSkillUnlocked(const UMythicSkillDefinition *Skill) const {
    if (!Skill) {
        return false;
    }
    if (!Skill->RequiredTag.IsValid()) {
        return true;
    }
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (const UMythicAchievementComponent *Achievements = PS->GetAchievementComponent()) {
            if (Achievements->IsAchievementUnlocked(Skill->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent()) {
            if (Unlocks->HasUnlockTag(Skill->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState()) {
            if (Narrative->HasStoryTag(Skill->RequiredTag)) {
                return true;
            }
        }
    }
    const UAbilitySystemComponent *ASC = ResolveASC();
    return ASC && ASC->HasMatchingGameplayTag(Skill->RequiredTag);
}

UMythicSkillDefinition *UMythicSkillComponent::GetSkillInSlot(int32 SlotIndex) const {
    return EquippedSkills.IsValidIndex(SlotIndex) ? EquippedSkills[SlotIndex].LoadSynchronous() : nullptr;
}

void UMythicSkillComponent::GrantSlot() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UnlockedSlots >= MaxSlots) {
        UE_LOG(Myth, Verbose, TEXT("Skills: slot grant ignored — all %d slots are already open."), MaxSlots);
        return;
    }
    UnlockedSlots++;
    OnSkillsChanged.Broadcast();
}

int32 UMythicSkillComponent::GetModifierCapacity() const {
    const int32 Ceiling = FMath::Max(MaxModifierCapacity, 1);
    const int32 Floor = FMath::Clamp(BaseModifierCapacity, 1, Ceiling);
    return FMath::Clamp(ModifierCapacity, Floor, Ceiling);
}

void UMythicSkillComponent::GrantModifierCapacity() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    const int32 Ceiling = FMath::Max(MaxModifierCapacity, 1);
    const int32 Current = GetModifierCapacity();
    if (Current >= Ceiling) {
        UE_LOG(Myth, Verbose, TEXT("Skills: modifier capacity grant ignored — already carrying %d at once."), Ceiling);
        return;
    }
    ModifierCapacity = Current + 1;
    OnSkillsChanged.Broadcast();
}

FMythicSkillProgress *UMythicSkillComponent::FindProgress(const FSoftObjectPath &SkillPath) {
    if (SkillPath.IsNull()) {
        return nullptr;
    }
    return SkillProgress.FindByPredicate([&SkillPath](const FMythicSkillProgress &Entry) {
        return Entry.Skill.ToSoftObjectPath() == SkillPath;
    });
}

const FMythicSkillProgress *UMythicSkillComponent::FindProgress(const FSoftObjectPath &SkillPath) const {
    return const_cast<UMythicSkillComponent *>(this)->FindProgress(SkillPath);
}

FMythicSkillProgress &UMythicSkillComponent::FindOrAddProgress(UMythicSkillDefinition *Skill) {
    const FSoftObjectPath SkillPath(Skill);
    if (FMythicSkillProgress *Existing = FindProgress(SkillPath)) {
        return *Existing;
    }
    FMythicSkillProgress &Added = SkillProgress.AddDefaulted_GetRef();
    Added.Skill = Skill;
    return Added;
}

void UMythicSkillComponent::PruneProgress(const FSoftObjectPath &SkillPath) {
    SkillProgress.RemoveAll([&SkillPath](const FMythicSkillProgress &Entry) {
        return Entry.Skill.ToSoftObjectPath() == SkillPath && Entry.Level <= 1 && Entry.ActiveModifiers.Num() == 0
            && Entry.Uses <= 0;
    });
}

// A stored index the definition no longer has costs nothing: the modifier it paid for is gone with it.
int32 UMythicSkillComponent::PointCostOf(const UMythicSkillDefinition *Skill, int32 ModifierIndex) {
    return Skill && Skill->Modifiers.IsValidIndex(ModifierIndex)
               ? FMath::Max(Skill->Modifiers[ModifierIndex].PointCost, 1)
               : 0;
}

int32 UMythicSkillComponent::GetMaxSkillLevel(const UMythicSkillDefinition *Skill) const {
    if (!Skill) {
        return 0;
    }
    return FMath::Clamp(Skill->Modifiers.Num(), 1, FMath::Max(MaxSkillLevel, 1));
}

int32 UMythicSkillComponent::GetSkillLevel(const UMythicSkillDefinition *Skill) const {
    if (!Skill) {
        return 0;
    }
    const FMythicSkillProgress *Entry = FindProgress(FSoftObjectPath(Skill));
    return Entry ? FMath::Clamp(Entry->Level, 1, GetMaxSkillLevel(Skill)) : 1;
}

int32 UMythicSkillComponent::GetSkillUses(const UMythicSkillDefinition *Skill) const {
    const FMythicSkillProgress *Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    return Entry ? FMath::Max(Entry->Uses, 0) : 0;
}

int32 UMythicSkillComponent::GetUsesForNextLevel(const UMythicSkillDefinition *Skill) const {
    if (!Skill || GetSkillLevel(Skill) >= GetMaxSkillLevel(Skill)) {
        return 0;
    }
    TArray<int32> Thresholds;
    float TailGrowth = 1.0f;
    ResolveUseLadder(Thresholds, TailGrowth);

    const int64 Needed = UsesToReachLevel(Thresholds, TailGrowth, GetSkillLevel(Skill) + 1);
    return Needed >= MAX_int32 ? 0 : static_cast<int32>(Needed);
}

int32 UMythicSkillComponent::GetGrantedPoints(const UMythicSkillDefinition *Skill) const {
    return GetSkillLevel(Skill);
}

int32 UMythicSkillComponent::GetSpentPoints(const UMythicSkillDefinition *Skill) const {
    const FMythicSkillProgress *Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    if (!Entry) {
        return 0;
    }
    int32 Spent = 0;
    for (const int32 Index : Entry->ActiveModifiers) {
        Spent += PointCostOf(Skill, Index);
    }
    return Spent;
}

int32 UMythicSkillComponent::GetAvailablePoints(const UMythicSkillDefinition *Skill) const {
    return FMath::Max(GetGrantedPoints(Skill) - GetSpentPoints(Skill), 0);
}

TArray<int32> UMythicSkillComponent::GetActiveModifiers(const UMythicSkillDefinition *Skill) const {
    TArray<int32> Active;
    const FMythicSkillProgress *Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    if (!Entry) {
        return Active;
    }
    Active.Reserve(Entry->ActiveModifiers.Num());
    // Filtered on the way out, so content that lost a modifier can never make the ability read past the end.
    for (const int32 Index : Entry->ActiveModifiers) {
        if (Skill->Modifiers.IsValidIndex(Index)) {
            Active.Add(Index);
        }
    }
    return Active;
}

bool UMythicSkillComponent::IsModifierActive(const UMythicSkillDefinition *Skill, int32 ModifierIndex) const {
    if (!Skill || !Skill->Modifiers.IsValidIndex(ModifierIndex)) {
        return false;
    }
    const FMythicSkillProgress *Entry = FindProgress(FSoftObjectPath(Skill));
    return Entry && Entry->ActiveModifiers.Contains(ModifierIndex);
}

void UMythicSkillComponent::ResolveUseLadder(TArray<int32> &OutThresholds, float &OutTailGrowth) {
    OutThresholds.Reset();
    OutTailGrowth = 1.0f;
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        OutThresholds = Settings->SkillLevelUseThresholds;
        OutTailGrowth = Settings->SkillLevelUseTailGrowth;
    }
}

int64 UMythicSkillComponent::UsesToReachLevel(const TArray<int32> &Thresholds, float TailGrowth, int32 Level) {
    if (Level <= 1) {
        return 0;
    }
    if (Thresholds.Num() == 0) {
        return TNumericLimits<int64>::Max();
    }

    const double Growth = FMath::Clamp(static_cast<double>(TailGrowth), 1.0, 100.0);
    int64 Running = 0;
    for (int32 Step = 0; Step < Level - 1; Step++) {
        // +1 every rung whatever the table says, so a row authored flat or backwards still costs practice.
        const double Grown = FMath::Min(Running * Growth, static_cast<double>(MAX_int32));
        Running = Thresholds.IsValidIndex(Step)
                      ? FMath::Max<int64>(Running + 1, Thresholds[Step])
                      : FMath::Max<int64>(Running + 1, static_cast<int64>(FMath::CeilToDouble(Grown)));
    }
    return Running;
}

int32 UMythicSkillComponent::LevelFromUses(const TArray<int32> &Thresholds, float TailGrowth, int32 Ceiling, int64 Uses) {
    const int32 Top = FMath::Max(Ceiling, 1);
    int32 Level = 1;
    while (Level < Top && Uses >= UsesToReachLevel(Thresholds, TailGrowth, Level + 1)) {
        Level++;
    }
    return Level;
}

UMythicStatLedgerComponent *UMythicSkillComponent::ResolveStatLedger() const {
    const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner());
    return PS ? PS->GetStatLedgerComponent() : nullptr;
}

void UMythicSkillComponent::RecordSkillUse(UMythicSkillDefinition *Skill) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!Skill) {
        UE_LOG(Myth, Warning, TEXT("Skills: use not recorded — no skill given."));
        return;
    }
    if (!IsSkillUnlocked(Skill)) {
        UE_LOG(Myth, Warning, TEXT("Skills: use not recorded — '%s' needs %s, which this player has not earned."),
               *GetNameSafe(Skill), *Skill->RequiredTag.ToString());
        return;
    }

    TArray<int32> Thresholds;
    float TailGrowth = 1.0f;
    ResolveUseLadder(Thresholds, TailGrowth);

    {
        FMythicSkillProgress &Entry = FindOrAddProgress(Skill);
        Entry.Uses = FMath::Max(Entry.Uses, 0) + 1;

        const int32 Earned = LevelFromUses(Thresholds, TailGrowth, GetMaxSkillLevel(Skill), Entry.Uses);
        if (Earned > Entry.Level) {
            UE_LOG(Myth, Log, TEXT("Skills: '%s' reached level %d on %d uses."), *GetNameSafe(Skill), Earned, Entry.Uses);
            Entry.Level = Earned;
        }
    }

    // Last, and outside the row's lifetime: the ledger fans out to achievements and unlocks, which grant back into
    // this component. A reference into SkillProgress must not be alive while that runs.
    if (UMythicStatLedgerComponent *Ledger = ResolveStatLedger()) {
        Ledger->RecordStat(STAT_SKILL_USED);
    }

    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::GrantSkillLevel(UMythicSkillDefinition *Skill, int32 Levels) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!Skill || Levels <= 0) {
        return;
    }

    const int32 Ceiling = GetMaxSkillLevel(Skill);
    const int32 Current = GetSkillLevel(Skill);
    if (Current >= Ceiling) {
        UE_LOG(Myth, Verbose, TEXT("Skills: level grant ignored — '%s' is already at level %d, its ceiling."),
               *GetNameSafe(Skill), Ceiling);
        return;
    }

    FMythicSkillProgress &Entry = FindOrAddProgress(Skill);
    Entry.Level = FMath::Min(Current + Levels, Ceiling);

    // The uses catch up to the granted level, so a granted rank is never re-earned and the practice bar reads true.
    TArray<int32> Thresholds;
    float TailGrowth = 1.0f;
    ResolveUseLadder(Thresholds, TailGrowth);
    const int64 Owed = UsesToReachLevel(Thresholds, TailGrowth, Entry.Level);
    if (Owed < TNumericLimits<int64>::Max()) {
        Entry.Uses = FMath::Max<int32>(Entry.Uses, static_cast<int32>(FMath::Min<int64>(Owed, MAX_int32)));
    }

    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::ServerSetModifierActive_Implementation(UMythicSkillDefinition *Skill, int32 ModifierIndex, bool bActive) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!Skill) {
        UE_LOG(Myth, Warning, TEXT("Skills: modifier change refused — no skill given."));
        return;
    }
    if (!Skill->Modifiers.IsValidIndex(ModifierIndex)) {
        UE_LOG(Myth, Warning, TEXT("Skills: modifier change refused — '%s' has no modifier %d (it has %d)."),
               *GetNameSafe(Skill), ModifierIndex, Skill->Modifiers.Num());
        return;
    }

    const FSoftObjectPath SkillPath(Skill);

    if (!bActive) {
        FMythicSkillProgress *Entry = FindProgress(SkillPath);
        if (!Entry || Entry->ActiveModifiers.Remove(ModifierIndex) == 0) {
            return;
        }
        PruneProgress(SkillPath);
        OnSkillsChanged.Broadcast();
        return;
    }

    if (!IsSkillUnlocked(Skill)) {
        UE_LOG(Myth, Warning, TEXT("Skills: modifier refused — '%s' needs %s, which this player has not earned."),
               *GetNameSafe(Skill), *Skill->RequiredTag.ToString());
        return;
    }
    if (IsModifierActive(Skill, ModifierIndex)) {
        return;
    }
    if (!Skill->Modifiers[ModifierIndex].HasEffect()) {
        UE_LOG(Myth, Error, TEXT("Skills: modifier refused — '%s' modifier %d changes nothing, so a point spent on it buys nothing."),
               *GetNameSafe(Skill), ModifierIndex);
        return;
    }

    const int32 Capacity = GetModifierCapacity();
    if (GetActiveModifiers(Skill).Num() >= Capacity) {
        UE_LOG(Myth, Warning, TEXT("Skills: modifier refused — '%s' already carries %d at once."), *GetNameSafe(Skill), Capacity);
        return;
    }

    const int32 Cost = PointCostOf(Skill, ModifierIndex);
    const int32 Available = GetAvailablePoints(Skill);
    if (Cost > Available) {
        UE_LOG(Myth, Warning, TEXT("Skills: modifier refused — '%s' modifier %d costs %d point(s), %d left."),
               *GetNameSafe(Skill), ModifierIndex, Cost, Available);
        return;
    }

    FindOrAddProgress(Skill).ActiveModifiers.Add(ModifierIndex);
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::RestoreSkillProgress(const TArray<FMythicSkillProgress> &SavedProgress, int32 SavedModifierCapacity) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    const int32 Ceiling = FMath::Max(MaxModifierCapacity, 1);
    const int32 Floor = FMath::Clamp(BaseModifierCapacity, 1, Ceiling);
    ModifierCapacity = FMath::Clamp(SavedModifierCapacity, Floor, Ceiling);

    SkillProgress.Reset();

    const int32 Capacity = GetModifierCapacity();

    TArray<int32> Thresholds;
    float TailGrowth = 1.0f;
    ResolveUseLadder(Thresholds, TailGrowth);

    for (const FMythicSkillProgress &Saved : SavedProgress) {
        const FSoftObjectPath SavedPath = Saved.Skill.ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            continue;
        }
        if (FindProgress(SavedPath)) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped a second progress row for '%s'."), *SavedPath.ToString());
            continue;
        }

        UMythicSkillDefinition *Skill = Saved.Skill.LoadSynchronous();
        if (!Skill) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved progress for '%s' — it no longer resolves to a skill."),
                   *SavedPath.ToString());
            continue;
        }

        const int32 LevelCeiling = GetMaxSkillLevel(Skill);

        FMythicSkillProgress Entry;
        Entry.Skill = Skill;
        Entry.Uses = FMath::Max(Saved.Uses, 0);

        // The practice on file is the floor, so a ladder retuned cheaper pays out on load; a level granted outright
        // sits above the practice and is kept. The skill's own modifier count is what clamps both.
        Entry.Level = FMath::Clamp(FMath::Max(Saved.Level, LevelFromUses(Thresholds, TailGrowth, LevelCeiling, Entry.Uses)),
                                   1, LevelCeiling);

        // Re-run the live gates rather than trusting the file: a ceiling that moved or content that lost a modifier
        // must not hand back a build the player could not buy today.
        int32 Budget = Entry.Level;
        for (const int32 Index : Saved.ActiveModifiers) {
            if (!Skill->Modifiers.IsValidIndex(Index) || Entry.ActiveModifiers.Contains(Index)) {
                continue;
            }
            if (!Skill->Modifiers[Index].HasEffect()) {
                UE_LOG(Myth, Warning, TEXT("Skills: dropped modifier %d on '%s' — it changes nothing, so the point it charges buys nothing."),
                       Index, *GetNameSafe(Skill));
                continue;
            }
            if (Entry.ActiveModifiers.Num() >= Capacity) {
                UE_LOG(Myth, Warning, TEXT("Skills: dropped modifier %d on '%s' — only %d can be carried at once."),
                       Index, *GetNameSafe(Skill), Capacity);
                break;
            }
            const int32 Cost = FMath::Max(Skill->Modifiers[Index].PointCost, 1);
            if (Cost > Budget) {
                UE_LOG(Myth, Warning, TEXT("Skills: dropped modifier %d on '%s' — it costs %d point(s), %d left."),
                       Index, *GetNameSafe(Skill), Cost, Budget);
                continue;
            }
            Budget -= Cost;
            Entry.ActiveModifiers.Add(Index);
        }

        if (Entry.Level > 1 || Entry.ActiveModifiers.Num() > 0 || Entry.Uses > 0) {
            SkillProgress.Add(MoveTemp(Entry));
        }
    }

    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::RestoreSkills(const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &SavedSkills, int32 SavedUnlockedSlots) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    UnlockedSlots = FMath::Clamp(SavedUnlockedSlots, 1, FMath::Max(MaxSlots, 1));
    SizeSlots();

    for (int32 SlotIndex = 0; SlotIndex < EquippedSkills.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
        EquippedSkills[SlotIndex].Reset();
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    for (int32 SlotIndex = 0; SlotIndex < SavedSkills.Num(); SlotIndex++) {
        const FSoftObjectPath SavedPath = SavedSkills[SlotIndex].ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            continue;
        }
        if (!EquippedSkills.IsValidIndex(SlotIndex) || !IsSlotUnlocked(SlotIndex)) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — slot %d is no longer open (%d of %d)."),
                   *SavedPath.ToString(), SlotIndex, GetUnlockedSlots(), MaxSlots);
            continue;
        }

        bool bAlreadyBound = false;
        for (int32 Other = 0; Other < SlotIndex && !bAlreadyBound; Other++) {
            bAlreadyBound = EquippedSkills[Other].ToSoftObjectPath() == SavedPath;
        }
        if (bAlreadyBound) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — already bound to an earlier slot."), *SavedPath.ToString());
            continue;
        }

        UMythicSkillDefinition *Skill = SavedSkills[SlotIndex].LoadSynchronous();
        if (!Skill || !Skill->HasPayload()) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — it no longer resolves to a skill with an ability."),
                   *SavedPath.ToString());
            continue;
        }
        if (!IsSkillUnlocked(Skill)) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — %s is no longer earned."), *GetNameSafe(Skill),
                   *Skill->RequiredTag.ToString());
            continue;
        }
        if (!ASC) {
            UE_LOG(Myth, Error, TEXT("Skills: restore failed — %s has no ability system."), *GetNameSafe(GetOwner()));
            break;
        }

        const FGameplayAbilitySpecHandle Handle = GrantSlotAbility(ASC, Skill, SlotIndex);
        if (!Handle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("Skills: restore failed — could not grant '%s'."), *GetNameSafe(Skill));
            continue;
        }
        GrantedHandles[SlotIndex] = Handle;
        EquippedSkills[SlotIndex] = Skill;
    }

    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::ServerEquipSkill_Implementation(int32 SlotIndex, UMythicSkillDefinition *Skill) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    SizeSlots();

    if (!EquippedSkills.IsValidIndex(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — slot %d is outside 0..%d."), SlotIndex, EquippedSkills.Num() - 1);
        return;
    }
    if (!IsSlotUnlocked(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — slot %d is still locked (%d of %d open)."), SlotIndex,
               GetUnlockedSlots(), MaxSlots);
        return;
    }
    if (!Skill) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — no skill given for slot %d."), SlotIndex);
        return;
    }
    if (!Skill->HasPayload()) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' has no ability."), *GetNameSafe(Skill));
        return;
    }
    if (!IsSkillUnlocked(Skill)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' needs %s, which this player has not earned."),
               *GetNameSafe(Skill), *Skill->RequiredTag.ToString());
        return;
    }

    // Compare by path: a skill can be bound while its asset is unloaded, and a null Get() would read as a free slot.
    const FSoftObjectPath SkillPath(Skill);
    if (EquippedSkills[SlotIndex].ToSoftObjectPath() == SkillPath) {
        return;
    }
    for (int32 Other = 0; Other < EquippedSkills.Num(); Other++) {
        if (Other != SlotIndex && EquippedSkills[Other].ToSoftObjectPath() == SkillPath) {
            UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' is already bound to slot %d."), *GetNameSafe(Skill), Other);
            return;
        }
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("Skills: equip failed — %s has no ability system."), *GetNameSafe(GetOwner()));
        return;
    }

    ClearSlotAbility(SlotIndex);

    const FGameplayAbilitySpecHandle Handle = GrantSlotAbility(ASC, Skill, SlotIndex);
    if (!Handle.IsValid()) {
        EquippedSkills[SlotIndex].Reset();
        OnSkillsChanged.Broadcast();
        UE_LOG(Myth, Error, TEXT("Skills: equip failed — could not grant '%s'."), *GetNameSafe(Skill));
        return;
    }

    GrantedHandles[SlotIndex] = Handle;
    EquippedSkills[SlotIndex] = Skill;
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::ServerUnequipSkill_Implementation(int32 SlotIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !EquippedSkills.IsValidIndex(SlotIndex)) {
        return;
    }
    if (EquippedSkills[SlotIndex].IsNull()) {
        return;
    }

    ClearSlotAbility(SlotIndex);
    EquippedSkills[SlotIndex].Reset();
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::OnRep_Skills() {
    OnSkillsChanged.Broadcast();
}
