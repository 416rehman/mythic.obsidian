#include "MythicSkillComponent.h"
#include "MythicAbilityCatalog.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/PlatformTime.h"
#include "Mythic/Input/MythicTags_Input.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicStatLedgerComponent.h"
#include "Mythic/Progression/MythicTags_MetaProgression.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "System/MythicAssetManager.h"
#include "Net/UnrealNetwork.h"

void FMythicSkillProgressArray::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters&) {
    if (Owner) { Owner->RebuildProgressIndex(); Owner->OnSkillsChanged.Broadcast(); }
}

UMythicSkillComponent::UMythicSkillComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    for (int32 Slot=0; Slot<SlotCount; ++Slot) SlotInputTags.Add(CanonicalSlotInputTag(Slot));
    Progress.Owner = this;
}
void UMythicSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, EquippedSkills, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, UnlockedSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, Progress, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, ModifierCapacity, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, bStateReady, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, StateRevision, COND_OwnerOnly);
}
UMythicAbilityCatalog* UMythicSkillComponent::GetCatalog() const {
    const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UMythicAbilityCatalog>() : nullptr;
}
bool UMythicSkillComponent::IsCatalogued(const UMythicSkillDefinition* Skill) const {
    const auto* Catalog = GetCatalog();
    return Catalog && Catalog->Contains(Skill);
}
bool UMythicSkillComponent::IsReady() const {
    const auto* Catalog = GetCatalog();
    return bStateReady && !bRestoring && PendingRestoreLoads == 0 && Catalog && Catalog->IsReady();
}
void UMythicSkillComponent::BeginPlay() {
    Super::BeginPlay();
    Progress.Owner = this;
    MaxSlots = UnlockedSlots = SlotCount;
    SlotInputTags.Reset(SlotCount);
    for (int32 Slot=0; Slot<SlotCount; ++Slot) SlotInputTags.Add(CanonicalSlotInputTag(Slot));
    if (GetOwner()->HasAuthority()) {
        SizeSlots();
        if (auto* PS = Cast<AMythicPlayerState>(GetOwner())) {
            if (auto* A = PS->GetAchievementComponent()) A->OnAchievementUnlocked.AddUniqueDynamic(this, &ThisClass::HandlePermanentTag);
            if (auto* N = PS->GetNarrativeState()) N->OnStoryTagEarned.AddUniqueDynamic(this, &ThisClass::HandlePermanentTag);
        }
    }
    if (auto* Catalog = GetCatalog()) {
        Catalog->OnReady.AddUObject(this, &ThisClass::ReconcilePrerequisites);
        if (Catalog->IsReady()) ReconcilePrerequisites();
    }
}
void UMythicSkillComponent::EndPlay(const EEndPlayReason::Type Reason) {
    ++ServerActionSerial;
    ++RestoreGeneration;
    if (auto* Catalog = GetCatalog()) Catalog->OnReady.RemoveAll(this);
    if (auto* PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (auto* A = PS->GetAchievementComponent()) A->OnAchievementUnlocked.RemoveDynamic(this, &ThisClass::HandlePermanentTag);
        if (auto* N = PS->GetNarrativeState()) N->OnStoryTagEarned.RemoveDynamic(this, &ThisClass::HandlePermanentTag);
    }
    if (GetOwner() && GetOwner()->HasAuthority()) for (int32 i = 0; i < GrantedHandles.Num(); ++i) ClearSlotAbility(i);
    Super::EndPlay(Reason);
}
UAbilitySystemComponent* UMythicSkillComponent::ResolveASC() const {
    const auto* ASI = Cast<IAbilitySystemInterface>(GetOwner());
    return ASI ? ASI->GetAbilitySystemComponent() : nullptr;
}
void UMythicSkillComponent::SizeSlots() {
    MaxSlots = UnlockedSlots = SlotCount;
    EquippedSkills.SetNum(SlotCount);
    GrantedHandles.SetNum(SlotCount);
}
FGameplayTag UMythicSkillComponent::CanonicalSlotInputTag(int32 Slot) {
    switch (Slot) {
    case 0: return INPUT_ACTION_SKILL_Q;
    case 1: return INPUT_ACTION_SKILL_E;
    case 2: return INPUT_ACTION_SKILL_R;
    case 3: return INPUT_ACTION_SKILL_T;
    default: return FGameplayTag();
    }
}
void UMythicSkillComponent::ClearSlotAbility(int32 Slot) {
    if (!GrantedHandles.IsValidIndex(Slot)) return;
    if (auto* ASC = ResolveASC(); ASC && GrantedHandles[Slot].IsValid()) ASC->ClearAbility(GrantedHandles[Slot]);
    GrantedHandles[Slot] = {};
}
FGameplayAbilitySpecHandle UMythicSkillComponent::GrantSlotAbility(UAbilitySystemComponent* ASC, UMythicSkillDefinition* Skill, int32 Slot) {
    if (!ASC || !Skill || !Skill->GetLoadedAbility() || !GetSlotInputTag(Slot).IsValid()) return {};
    FGameplayAbilitySpec Spec(Skill->GetLoadedAbility(), 1, INDEX_NONE, Skill);
    Spec.GetDynamicSpecSourceTags().AddTag(GetSlotInputTag(Slot));
    Spec.GetDynamicSpecSourceTags().AppendTags(GetEffectiveClassification(Skill));
    return ASC->GiveAbility(Spec);
}
FGameplayTag UMythicSkillComponent::GetSlotInputTag(int32 Slot) const { return CanonicalSlotInputTag(Slot); }
bool UMythicSkillComponent::IsSlotUnlocked(int32 Slot) const { return Slot >= 0 && Slot < GetUnlockedSlots(); }
UMythicSkillDefinition* UMythicSkillComponent::GetSkillInSlot(int32 Slot) const { return EquippedSkills.IsValidIndex(Slot) ? EquippedSkills[Slot].Get() : nullptr; }
bool UMythicSkillComponent::HasPermanentPrerequisite(const UMythicSkillDefinition* Skill) const {
    return Skill && HasPermanentFact(Skill->RequiredTag);
}
bool UMythicSkillComponent::HasPermanentFact(FGameplayTag Tag) const {
    if (!Tag.IsValid()) return false;
    if (const auto* PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (auto* A = PS->GetAchievementComponent(); A && A->IsAchievementUnlocked(Tag)) return true;
        if (auto* U = PS->GetUnlockComponent(); U && U->HasUnlockTag(Tag)) return true;
        if (auto* N = PS->GetNarrativeState(); N && N->HasStoryTag(Tag)) return true;
    }
    return false;
}
bool UMythicSkillComponent::IsSkillUnlocked(const UMythicSkillDefinition* Skill) const {
    if (!IsCatalogued(Skill)) return false;
    const auto* Entry = FindProgress(FSoftObjectPath(Skill));
    return Entry && Entry->bLearned;
}
void UMythicSkillComponent::ReconcilePrerequisites() {
    auto* Catalog = GetCatalog();
    if (!Catalog || !Catalog->IsReady()) return;
    PrerequisiteIndex.Reset();
    RefinementPrerequisiteTags.Reset();
    bool bChanged = false;
    for (const auto& Skill : Catalog->GetDefinitions()) {
        for (const auto& Modifier:Skill->Modifiers) if (Modifier.RequiredUnlockTag.IsValid()) RefinementPrerequisiteTags.Add(Modifier.RequiredUnlockTag);
        if (Skill->Acquisition == EMythicAbilityAcquisition::Prerequisite)
            PrerequisiteIndex.FindOrAdd(Skill->RequiredTag).Add(Skill);
        if (GetOwner()->HasAuthority() && !bRestoring && !IsSkillUnlocked(Skill)
            && (Skill->Acquisition == EMythicAbilityAcquisition::Starter
                || (Skill->Acquisition == EMythicAbilityAcquisition::Prerequisite && HasPermanentPrerequisite(Skill))))
            bChanged |= LearnSkill(Skill, false);
    }
    if (bChanged || (GetOwner()->HasAuthority() && !bRestoring && !bStateReady && PendingRestoreLoads==0)) Changed(); else OnSkillsChanged.Broadcast();
}
void UMythicSkillComponent::HandlePermanentTag(FGameplayTag Tag) {
    if (!GetOwner()->HasAuthority() || bRestoring) return;
    FGameplayTagContainer Parents = Tag.GetSingleTagContainer().GetGameplayTagParents();
    bool bChanged = false;
    for (auto Parent:Parents) bChanged |= RefinementPrerequisiteTags.Contains(Parent);
    for (const FGameplayTag& Parent : Parents) if (const auto* Entries = PrerequisiteIndex.Find(Parent))
        for (const auto& Skill : *Entries) if (Skill.IsValid() && !IsSkillUnlocked(Skill.Get()) && HasPermanentPrerequisite(Skill.Get()))
            bChanged |= LearnSkill(Skill.Get(), false);
    if (bChanged) Changed();
}
bool UMythicSkillComponent::LearnSkill(UMythicSkillDefinition* Skill, bool bNotify) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsCatalogued(Skill)) return false;
    auto& Entry = FindOrAddProgress(Skill);
    if (Entry.bLearned) return true;
    Entry.bLearned = true; Entry.SchemaVersion = 1;
    MarkProgress(FSoftObjectPath(Skill));
    if (bNotify) Changed();
    return true;
}
bool UMythicSkillComponent::AcceptRewardSkill(UMythicSkillDefinition* Skill) {
    auto* Catalog=GetCatalog();
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Catalog || !Skill) return false;
    if (Catalog->IsReady()) return LearnSkill(Skill);
    if (!Catalog->IsConfigured(Skill)) return false;
    if (auto* Staged=UnavailableProgress.FindByPredicate([&](const auto& Row) { return Row.Skill.ToSoftObjectPath()==FSoftObjectPath(Skill); })) {
        if (!Staged->bLearned) { Staged->bLearned=true; Changed(); }
        return true;
    }
    auto& Entry=FindOrAddProgress(Skill);
    if (!Entry.bLearned) { Entry.bLearned=true; Entry.SchemaVersion=1; MarkProgress(FSoftObjectPath(Skill)); Changed(); }
    return true;
}
void UMythicSkillComponent::Changed() {
    if (GetOwner() && GetOwner()->HasAuthority()) bStateReady=!bRestoring && PendingRestoreLoads==0 && GetCatalog() && GetCatalog()->IsReady();
    if (bRestoring) return;
    ++StateRevision;
    OnSkillsChanged.Broadcast();
    if (GetOwner()) GetOwner()->ForceNetUpdate();
}
void UMythicSkillComponent::OnRep_Skills() { OnSkillsChanged.Broadcast(); }
void UMythicSkillComponent::RebuildProgressIndex() {
    ProgressIndex.Reset();
    for (int32 i = 0; i < Progress.Items.Num(); ++i) ProgressIndex.Add(Progress.Items[i].Data.Skill.ToSoftObjectPath(), i);
}
FMythicSkillProgress* UMythicSkillComponent::FindProgress(const FSoftObjectPath& Path) {
    const int32* Index = ProgressIndex.Find(Path);
    return Index && Progress.Items.IsValidIndex(*Index) ? &Progress.Items[*Index].Data : nullptr;
}
const FMythicSkillProgress* UMythicSkillComponent::FindProgress(const FSoftObjectPath& Path) const { return const_cast<ThisClass*>(this)->FindProgress(Path); }
FMythicSkillProgress& UMythicSkillComponent::FindOrAddProgress(UMythicSkillDefinition* Skill) {
    const FSoftObjectPath Path(Skill);
    if (auto* Existing = FindProgress(Path)) return *Existing;
    const int32 Index = Progress.Items.AddDefaulted();
    auto& Item = Progress.Items[Index];
    Item.Data.Skill = Skill; Item.Data.SchemaVersion = 1;
    ProgressIndex.Add(Path, Index);
    Progress.MarkItemDirty(Item);
    return Item.Data;
}
void UMythicSkillComponent::MarkProgress(const FSoftObjectPath& Path) {
    if (const int32* Index = ProgressIndex.Find(Path)) Progress.MarkItemDirty(Progress.Items[*Index]);
}
TArray<FMythicSkillProgress> UMythicSkillComponent::GetSkillProgress() const {
    TArray<FMythicSkillProgress> Result = UnavailableProgress;
    Result.Reserve(Result.Num() + Progress.Items.Num());
    for (const auto& Item : Progress.Items) Result.Add(Item.Data);
    return Result;
}
void UMythicSkillComponent::GrantSlot() {
    // Preserve legacy unlock receipts without changing the four-socket baseline.
    if (!GetOwner() || !GetOwner()->HasAuthority() || UnlockedSlots == SlotCount) return;
    UnlockedSlots = SlotCount; Changed();
}
int32 UMythicSkillComponent::GetModifierCapacity() const { return FMath::Clamp(ModifierCapacity, FMath::Clamp(BaseModifierCapacity,1,8), FMath::Clamp(MaxModifierCapacity,1,8)); }
void UMythicSkillComponent::GrantModifierCapacity() {
    if (!GetOwner() || !GetOwner()->HasAuthority() || GetModifierCapacity() >= FMath::Clamp(MaxModifierCapacity,1,8)) return;
    ModifierCapacity = GetModifierCapacity() + 1; Changed();
}
int32 UMythicSkillComponent::PointCostOf(const UMythicSkillDefinition* Skill, int32 Index) { return Skill && Skill->Modifiers.IsValidIndex(Index) ? FMath::Clamp(Skill->Modifiers[Index].PointCost,1,50) : 0; }
int32 UMythicSkillComponent::GetMaxSkillLevel(const UMythicSkillDefinition* Skill) const {
    if (!Skill) return 0;
    if (Skill->bUseLegacyModifierProgression) return FMath::Clamp(Skill->Modifiers.Num(),1,FMath::Clamp(MaxSkillLevel,1,50));
    return Skill->bHasMasteryTrack ? FMath::Clamp(Skill->MasteryUseThresholds.Num()+1,1,50) : 1;
}
int32 UMythicSkillComponent::GetSkillLevel(const UMythicSkillDefinition* Skill) const {
    const auto* Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    return Skill ? FMath::Clamp(Entry ? Entry->Level : 1,1,GetMaxSkillLevel(Skill)) : 0;
}
int32 UMythicSkillComponent::GetSkillUses(const UMythicSkillDefinition* Skill) const {
    const auto* Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    return Entry ? FMath::Max(Entry->Uses,0) : 0;
}
int32 UMythicSkillComponent::GetUsesForNextLevel(const UMythicSkillDefinition* Skill) const {
    if (!Skill || GetSkillLevel(Skill) >= GetMaxSkillLevel(Skill)) return 0;
    TArray<int32> Thresholds; float Growth; ResolveUseLadder(Skill,Thresholds,Growth);
    const int64 Needed = UsesToReachLevel(Thresholds,Growth,GetSkillLevel(Skill)+1);
    return Needed >= MAX_int32 ? 0 : static_cast<int32>(Needed);
}
TArray<int32> UMythicSkillComponent::GetActiveModifiers(const UMythicSkillDefinition* Skill) const {
    TArray<int32> Result;
    if (const auto* Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr)
        for (FName Id : Entry->ActiveModifierIds) { const int32 Index = Skill->FindModifier(Id); if (Index != INDEX_NONE) Result.Add(Index); }
    return Result;
}
bool UMythicSkillComponent::IsModifierActive(const UMythicSkillDefinition* Skill, int32 Index) const {
    const auto* Entry = Skill ? FindProgress(FSoftObjectPath(Skill)) : nullptr;
    return Entry && Skill->Modifiers.IsValidIndex(Index) && Entry->ActiveModifierIds.Contains(Skill->Modifiers[Index].ModifierId);
}
int32 UMythicSkillComponent::GetGrantedPoints(const UMythicSkillDefinition* Skill) const { return Skill && Skill->bUseLegacyModifierProgression ? GetSkillLevel(Skill) : 0; }
int32 UMythicSkillComponent::GetSpentPoints(const UMythicSkillDefinition* Skill) const {
    int32 Spent = 0; for (int32 Index : GetActiveModifiers(Skill)) Spent += PointCostOf(Skill,Index); return Spent;
}
int32 UMythicSkillComponent::GetAvailablePoints(const UMythicSkillDefinition* Skill) const { return FMath::Max(0,GetGrantedPoints(Skill)-GetSpentPoints(Skill)); }
FGameplayTagContainer UMythicSkillComponent::GetEffectiveClassification(const UMythicSkillDefinition* Skill) const {
    FGameplayTagContainer Result = Skill ? Skill->ClassificationTags : FGameplayTagContainer();
    for (int32 Index : GetActiveModifiers(Skill)) {
        Result.RemoveTags(Skill->Modifiers[Index].RemoveClassificationTags);
        Result.AppendTags(Skill->Modifiers[Index].AddClassificationTags);
    }
    return Result;
}
void UMythicSkillComponent::ResolveUseLadder(const UMythicSkillDefinition* Skill,TArray<int32> &OutThresholds, float &OutTailGrowth) {
    OutThresholds.Reset();
    OutTailGrowth = 1.0f;
    if (Skill && !Skill->bUseLegacyModifierProgression) {
        if (Skill->bHasMasteryTrack) OutThresholds=Skill->MasteryUseThresholds;
        return;
    }
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


void UMythicSkillComponent::RecordSkillUse(UMythicSkillDefinition* Skill) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsSkillUnlocked(Skill)) return;
    TArray<int32> Thresholds; float Growth; ResolveUseLadder(Skill,Thresholds,Growth);
    auto& Entry = FindOrAddProgress(Skill);
    Entry.Uses = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Entry.Uses)+1,MAX_int32));
    Entry.Level = FMath::Max(Entry.Level,LevelFromUses(Thresholds,Growth,GetMaxSkillLevel(Skill),Entry.Uses));
    MarkProgress(FSoftObjectPath(Skill));
    if (auto* Ledger = ResolveStatLedger()) Ledger->RecordStat(STAT_SKILL_USED);
    Changed();
}
void UMythicSkillComponent::GrantSkillLevel(UMythicSkillDefinition* Skill, int32 Levels) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || Levels <= 0 || !IsSkillUnlocked(Skill)) return;
    auto& Entry = FindOrAddProgress(Skill);
    Entry.Level = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Entry.Level)+Levels,GetMaxSkillLevel(Skill)));
    TArray<int32> Thresholds; float Growth; ResolveUseLadder(Skill,Thresholds,Growth);
    const int64 Uses = UsesToReachLevel(Thresholds,Growth,Entry.Level);
    if (Uses < TNumericLimits<int64>::Max()) Entry.Uses = FMath::Max(Entry.Uses,static_cast<int32>(FMath::Min<int64>(Uses,MAX_int32)));
    MarkProgress(FSoftObjectPath(Skill)); Changed();
}
bool UMythicSkillComponent::IsActive(const UMythicSkillDefinition* Skill) const {
    if (!Skill) return false;
    auto* ASC = ResolveASC(); if (!ASC) return false;
    if (GetOwner() && !GetOwner()->HasAuthority()) for (const auto& Spec:ASC->GetActivatableAbilities()) if (Spec.SourceObject.Get()==Skill && Spec.IsActive()) return true;
    for (int32 i = 0; i < EquippedSkills.Num(); ++i) if (EquippedSkills[i].ToSoftObjectPath() == FSoftObjectPath(Skill) && GrantedHandles.IsValidIndex(i))
        if (auto* Spec = ASC->FindAbilitySpecFromHandle(GrantedHandles[i]); Spec && Spec->IsActive()) return true;
    return false;
}
bool UMythicSkillComponent::OwnsGrantedSpec(const UMythicSkillDefinition* Skill, FGameplayAbilitySpecHandle Handle) const {
    for (int32 i = 0; i < GrantedHandles.Num(); ++i)
        if (GrantedHandles[i] == Handle && EquippedSkills.IsValidIndex(i) && EquippedSkills[i].Get() == Skill) return true;
    // Clients receive GAS specs independently of private component state.
    if (GetOwner() && !GetOwner()->HasAuthority()) if (auto* ASC = ResolveASC())
        if (const auto* Spec = ASC->FindAbilitySpecFromHandle(Handle))
            return Spec->SourceObject.Get() == Skill && IsSkillUnlocked(Skill);
    return false;
}
EMythicAbilityResult UMythicSkillComponent::Preflight(EMythicAbilityAction Action,int32 Slot,const UMythicSkillDefinition* Skill,FName ModifierId,bool bActive) const {
    if (Action!=EMythicAbilityAction::Equip && Action!=EMythicAbilityAction::Unequip && Action!=EMythicAbilityAction::Modifier) return EMythicAbilityResult::InvalidAbility;
    if (!IsReady()) return EMythicAbilityResult::NotReady;
    if (Action != EMythicAbilityAction::Modifier) {
        if (Slot < 0 || Slot >= SlotCount) return EMythicAbilityResult::InvalidSlot;
        if (!IsSlotUnlocked(Slot)) return EMythicAbilityResult::LockedSlot;
        if (!GetSlotInputTag(0).IsValid() || !GetSlotInputTag(1).IsValid() || GetSlotInputTag(0) == GetSlotInputTag(1)) return EMythicAbilityResult::MissingInput;
        if (IsActive(GetSkillInSlot(Slot))) return EMythicAbilityResult::ActiveAbility;
        if (Action == EMythicAbilityAction::Unequip) return EMythicAbilityResult::Success;
    }
    if (!IsCatalogued(Skill) || !Skill->HasPayload()) return EMythicAbilityResult::InvalidAbility;
    if (!IsSkillUnlocked(Skill)) return EMythicAbilityResult::LockedAbility;
    if (IsActive(Skill)) return EMythicAbilityResult::ActiveAbility;
    if (Action == EMythicAbilityAction::Equip) {
        for (int32 i = 0; i < EquippedSkills.Num(); ++i) if (i != Slot && EquippedSkills[i].ToSoftObjectPath() == FSoftObjectPath(Skill)) return EMythicAbilityResult::EquippedElsewhere;
        return EMythicAbilityResult::Success;
    }
    const int32 Index = Skill->FindModifier(ModifierId);
    if (Index == INDEX_NONE || !Skill->Modifiers[Index].HasEffect()) return EMythicAbilityResult::InvalidModifier;
    if (!bActive || IsModifierActive(Skill,Index)) return EMythicAbilityResult::Success;
    const auto& Modifier = Skill->Modifiers[Index];
    if (Modifier.RequiredUnlockTag.IsValid() && !HasPermanentFact(Modifier.RequiredUnlockTag)) return EMythicAbilityResult::LockedAbility;
    for (int32 Active : GetActiveModifiers(Skill))
        if (!Modifier.ExclusiveBehaviorGroup.IsNone() && Skill->Modifiers[Active].ExclusiveBehaviorGroup == Modifier.ExclusiveBehaviorGroup)
            return EMythicAbilityResult::IncompatibleModifier;
    if (Skill->bUseLegacyModifierProgression) {
        if (GetActiveModifiers(Skill).Num() >= GetModifierCapacity()) return EMythicAbilityResult::Capacity;
        if (PointCostOf(Skill,Index) > GetAvailablePoints(Skill)) return EMythicAbilityResult::Points;
    } else if (Skill->RefinementSelectionLimit>0 && GetActiveModifiers(Skill).Num()>=Skill->RefinementSelectionLimit) return EMythicAbilityResult::Capacity;
    return EMythicAbilityResult::Success;
}
EMythicAbilityResult UMythicSkillComponent::ExecuteAction(EMythicAbilityAction Action,int32 Slot,UMythicSkillDefinition* Skill,FName ModifierId,bool bActive) {
    const auto Result = Preflight(Action,Slot,Skill,ModifierId,bActive);
    if (Result != EMythicAbilityResult::Success) return Result;
    SizeSlots();
    if (Action == EMythicAbilityAction::Modifier) {
        auto& Entry = FindOrAddProgress(Skill);
        const bool Had = Entry.ActiveModifierIds.Contains(ModifierId);
        if (Had == bActive) return Result;
        if (bActive) Entry.ActiveModifierIds.Add(ModifierId); else Entry.ActiveModifierIds.Remove(ModifierId);
        MarkProgress(FSoftObjectPath(Skill));
        if (auto* ASC = ResolveASC()) for (int32 i = 0; i < EquippedSkills.Num(); ++i)
            if (GetSkillInSlot(i) == Skill) if (auto* Spec = ASC->FindAbilitySpecFromHandle(GrantedHandles[i])) {
                Spec->GetDynamicSpecSourceTags().Reset();
                Spec->GetDynamicSpecSourceTags().AddTag(GetSlotInputTag(i));
                Spec->GetDynamicSpecSourceTags().AppendTags(GetEffectiveClassification(Skill));
                ASC->MarkAbilitySpecDirty(*Spec);
            }
    } else if (Action == EMythicAbilityAction::Unequip) {
        if (EquippedSkills[Slot].IsNull()) return Result;
        ClearSlotAbility(Slot); EquippedSkills[Slot].Reset();
    } else {
        if (GetSkillInSlot(Slot) == Skill) return Result;
        auto* ASC = ResolveASC();
        const auto Handle = GrantSlotAbility(ASC,Skill,Slot);
        if (!Handle.IsValid()) return EMythicAbilityResult::GrantFailed;
        ClearSlotAbility(Slot); GrantedHandles[Slot] = Handle; EquippedSkills[Slot] = Skill;
    }
    Changed(); return Result;
}
void UMythicSkillComponent::ServerRequestAbilityAction_Implementation(FGuid Id,EMythicAbilityAction Action,int32 Slot,UMythicSkillDefinition* Skill,FName ModifierId,bool bActive) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    const auto Result = Preflight(Action,Slot,Skill,ModifierId,bActive);
    if (Result != EMythicAbilityResult::Success) { ClientAbilityActionResult(Id,Result,StateRevision); return; }
    const uint32 Serial = ++ServerActionSerial;
    if (Action == EMythicAbilityAction::Equip && Skill && !Skill->RuntimeAbility.IsNull() && !Skill->RuntimeAbility.IsValid()) {
        UMythicAssetManager::LoadAsync(this,Skill->RuntimeAbility,[this,Id,Action,Slot,Skill,ModifierId,bActive,Serial](TSubclassOf<UGameplayAbility> Loaded) {
            const auto Reply = Serial != ServerActionSerial ? EMythicAbilityResult::NotReady : ExecuteAction(Action,Slot,Skill,ModifierId,bActive);
            ClientAbilityActionResult(Id,Reply,StateRevision);
        });
        return;
    }
    ClientAbilityActionResult(Id,ExecuteAction(Action,Slot,Skill,ModifierId,bActive),StateRevision);
}
void UMythicSkillComponent::ServerEquipSkill_Implementation(int32 Slot,UMythicSkillDefinition* Skill) { ServerRequestAbilityAction(FGuid::NewGuid(),EMythicAbilityAction::Equip,Slot,Skill,NAME_None,false); }
void UMythicSkillComponent::ServerUnequipSkill_Implementation(int32 Slot) { ServerRequestAbilityAction(FGuid::NewGuid(),EMythicAbilityAction::Unequip,Slot,nullptr,NAME_None,false); }
void UMythicSkillComponent::ServerSetModifierActive_Implementation(UMythicSkillDefinition* Skill,int32 Index,bool bActive) {
    ServerRequestAbilityAction(FGuid::NewGuid(),EMythicAbilityAction::Modifier,INDEX_NONE,Skill,Skill && Skill->Modifiers.IsValidIndex(Index) ? Skill->Modifiers[Index].ModifierId : NAME_None,bActive);
}
void UMythicSkillComponent::ClientAbilityActionResult_Implementation(FGuid Id,EMythicAbilityResult Result,int32 Revision) { OnActionResult.Broadcast(Id,Result,Revision); }

void UMythicSkillComponent::SetRestoring(bool bValue) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    bRestoring = bValue; bStateReady = !bValue && PendingRestoreLoads==0 && GetCatalog() && GetCatalog()->IsReady();
    GetOwner()->ForceNetUpdate();
    if (!bValue) { ReconcilePrerequisites(); Changed(); }
}
void UMythicSkillComponent::DeferCatalogueRestore() {
    auto* Catalog=GetCatalog();
    if (!Catalog || Catalog->IsReady() || PendingCatalogueReadyHandle.IsValid()) return;
    ++PendingRestoreLoads; bStateReady=false;
    // The callback unregisters itself. A member binding keeps its execution independent of
    // the delegate storage, which Remove may destroy before restoration completes.
    PendingCatalogueReadyHandle=Catalog->OnReady.AddUObject(this,&ThisClass::HandleDeferredCatalogueReady);
}
void UMythicSkillComponent::HandleDeferredCatalogueReady() {
    if (auto* Ready=GetCatalog()) Ready->OnReady.Remove(PendingCatalogueReadyHandle);
    PendingCatalogueReadyHandle.Reset();
    // Multicast callback order is unspecified. All three parts materialize in one explicit dependency order.
    if (bPendingProgressMaterialization) { bPendingProgressMaterialization=false; RestoreSkillProgress({},ModifierCapacity); }
    if (bPendingCooldownMaterialization) { bPendingCooldownMaterialization=false; RestoreAbilityCooldowns(CaptureAbilityCooldowns()); }
    if (bHasPendingEquippedRestore) { auto Deferred=PendingEquippedSkills; RestoreSkills(Deferred,UnlockedSlots); }
    --PendingRestoreLoads; Changed();
}
void UMythicSkillComponent::RestoreSkillProgress(const TArray<FMythicSkillProgress>& Saved,int32 Capacity) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    auto* Catalog = GetCatalog();
    const bool bCataloguePending=Catalog && !Catalog->IsReady();
    if (bCataloguePending) { bPendingProgressMaterialization=true; DeferCatalogueRestore(); }
    ModifierCapacity = FMath::Clamp(Capacity,FMath::Clamp(BaseModifierCapacity,1,8),FMath::Clamp(MaxModifierCapacity,1,8));
    TArray<FMythicSkillProgress> Combined = Saved;
    // Reward replay and a repeated load cannot revoke an ownership already accepted by the server.
    for (auto Current:GetSkillProgress()) {
        if (Saved.ContainsByPredicate([&Current](const auto& Row){ return Row.Skill==Current.Skill; })) { Current.ActiveModifiers.Reset(); Current.ActiveModifierIds.Reset(); }
        Combined.Add(Current);
    }
    Progress.Items.Reset(); ProgressIndex.Reset(); UnavailableProgress.Reset();
    for (auto Row : Combined) {
        if (Row.Skill.IsNull()) continue;
        UMythicSkillDefinition* Skill = Catalog && !bCataloguePending ? Catalog->Find(Row.Skill.ToSoftObjectPath()) : nullptr;
        if (!Skill) {
            auto* Existing = UnavailableProgress.FindByPredicate([&Row](const auto& E){ return E.Skill == Row.Skill; });
            if (!Existing) UnavailableProgress.Add(Row);
            else { Existing->bLearned |= Row.bLearned; Existing->Uses = FMath::Max(Existing->Uses,Row.Uses); Existing->Level = FMath::Max(Existing->Level,Row.Level); }
            continue;
        }
        if (Row.SchemaVersion < 1) {
            for (int32 Index : Row.ActiveModifiers) if (Skill->Modifiers.IsValidIndex(Index)) Row.ActiveModifierIds.AddUnique(Skill->Modifiers[Index].ModifierId);
            Row.bLearned |= Skill->Acquisition == EMythicAbilityAcquisition::Starter || HasPermanentPrerequisite(Skill);
        }
        auto& Entry = FindOrAddProgress(Skill);
        Entry.bLearned |= Row.bLearned;
        Entry.Uses = FMath::Max(Entry.Uses,FMath::Max(0,Row.Uses));
        TArray<int32> Thresholds; float Growth; ResolveUseLadder(Skill,Thresholds,Growth);
        Entry.Level = FMath::Clamp(FMath::Max3(Entry.Level,Row.Level,LevelFromUses(Thresholds,Growth,GetMaxSkillLevel(Skill),Entry.Uses)),1,Skill->bUseLegacyModifierProgression ? GetMaxSkillLevel(Skill) : 50);
        int32 Budget = Entry.Level;
        for (FName Id : Entry.ActiveModifierIds) Budget -= PointCostOf(Skill,Skill->FindModifier(Id));
        for (FName Id : Row.ActiveModifierIds) {
            int32 Index = Skill->FindModifier(Id);
            if (Index == INDEX_NONE || Entry.ActiveModifierIds.Contains(Id) || !Skill->Modifiers[Index].HasEffect()) continue;
            const auto& M = Skill->Modifiers[Index];
            bool Conflict = false;
            for (FName ActiveId : Entry.ActiveModifierIds) {
                const int32 ActiveIndex = Skill->FindModifier(ActiveId);
                Conflict |= ActiveIndex != INDEX_NONE && !M.ExclusiveBehaviorGroup.IsNone() && M.ExclusiveBehaviorGroup == Skill->Modifiers[ActiveIndex].ExclusiveBehaviorGroup;
            }
            const int32 Cost = PointCostOf(Skill,Index);
            const bool Eligible=!M.RequiredUnlockTag.IsValid() || HasPermanentFact(M.RequiredUnlockTag);
            const bool HasCapacity=Skill->bUseLegacyModifierProgression ? Entry.ActiveModifierIds.Num()<GetModifierCapacity() && Cost<=Budget
                : Skill->RefinementSelectionLimit==0 || Entry.ActiveModifierIds.Num()<Skill->RefinementSelectionLimit;
            if (!Conflict && Eligible && HasCapacity) { Entry.ActiveModifierIds.Add(Id); Budget-=Cost; }
        }
        Entry.ActiveModifiers.Reset(); Entry.SchemaVersion = 1;
        MarkProgress(FSoftObjectPath(Skill));
    }
    Progress.MarkArrayDirty(); Changed();
}
void UMythicSkillComponent::RestoreSkills(const TArray<TSoftObjectPtr<UMythicSkillDefinition>>& Saved,int32 Slots) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    PendingEquippedSkills=Saved; PendingEquippedSkills.SetNum(SlotCount); bHasPendingEquippedRestore=true;
    auto* Catalog = GetCatalog();
    if (Catalog && !Catalog->IsReady()) {
        UnlockedSlots=SlotCount; bStateReady=false;
        DeferCatalogueRestore();
        return;
    }
    ++ServerActionSerial; const uint32 Serial = ++RestoreGeneration;
    UnlockedSlots = SlotCount; SizeSlots();
    for (int32 i=0;i<SlotCount;++i) { ClearSlotAbility(i); EquippedSkills[i].Reset(); }
    TArray<FSoftObjectPath> Classes;
    TArray<TObjectPtr<UMythicSkillDefinition>> Definitions;
    Definitions.SetNum(SlotCount);
    for (int32 i=0;i<FMath::Min(Saved.Num(),GetUnlockedSlots());++i) {
        auto* Skill = Catalog ? Catalog->Find(Saved[i].ToSoftObjectPath()) : nullptr;
        if (!Skill || !IsSkillUnlocked(Skill) || !Skill->HasPayload() || !GetSlotInputTag(i).IsValid() || Definitions.Contains(Skill)) continue;
        Definitions[i]=Skill;
        if (!Skill->RuntimeAbility.IsNull()) Classes.AddUnique(Skill->RuntimeAbility.ToSoftObjectPath());
    }
    ++PendingRestoreLoads;
    bStateReady=false;
    UMythicAssetManager::LoadAsync(this,Classes,[this,Definitions,Serial](const TArray<UObject*>&) {
        --PendingRestoreLoads;
        if (Serial != RestoreGeneration) { Changed(); return; }
        for (int32 i=0;i<SlotCount;++i) if (Definitions[i]) {
            const auto Handle = GrantSlotAbility(ResolveASC(),Definitions[i],i);
            if (Handle.IsValid()) { GrantedHandles[i]=Handle; EquippedSkills[i]=Definitions[i]; }
        }
        PendingEquippedSkills.Reset(); bHasPendingEquippedRestore=false;
        Changed();
    });
}
float UMythicSkillComponent::GetCooldownRemaining(const UMythicSkillDefinition* Skill) const {
    if (!Skill || !Skill->HasTimedCooldown()) return 0;
    float Remaining = FMath::Max(0.0, CooldownExpiries.FindRef(FSoftObjectPath(Skill))-FPlatformTime::Seconds());
    if (auto* ASC=ResolveASC(); ASC && Skill->CooldownTag.IsValid()) {
        const auto Times = ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Skill->CooldownTag.GetSingleTagContainer()));
        for (float Time : Times) Remaining = FMath::Max(Remaining,Time);
    }
    return Remaining;
}
void UMythicSkillComponent::GetCooldownDisplay(const UMythicSkillDefinition* Skill,float& Remaining,float& Duration) const {
    Remaining=Duration=0;
    if (!Skill || !Skill->HasTimedCooldown()) return;
    Remaining=FMath::Max(0.0,CooldownExpiries.FindRef(FSoftObjectPath(Skill))-FPlatformTime::Seconds());
    if (auto* ASC=ResolveASC(); ASC && Skill->CooldownTag.IsValid()) {
        const auto Times=ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Skill->CooldownTag.GetSingleTagContainer()));
        float LongestEffect=-1;
        for (const auto& Time:Times) if (FMath::IsFinite(Time.Key) && FMath::IsFinite(Time.Value) && Time.Key>0 && Time.Value>0) {
            if (Time.Key>LongestEffect) { LongestEffect=Time.Key; Duration=Time.Value; }
        }
        Remaining=FMath::Max(Remaining,LongestEffect);
    }
    Duration=FMath::Max(Duration,Remaining);
}

bool UMythicSkillComponent::StartCooldown(UMythicSkillDefinition* Skill,float Seconds) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsCatalogued(Skill) || !Skill->HasTimedCooldown() || !FMath::IsFinite(Seconds) || Seconds <= 0 || Seconds > 3600) return false;
    auto* ASC=ResolveASC(); if (!ASC || !Skill->CooldownEffect || !Skill->CooldownTag.IsValid()) return false;
    const FSoftObjectPath Path(Skill);
    const float Duration = FMath::Max(Seconds,GetCooldownRemaining(Skill));
    auto Spec=ASC->MakeOutgoingSpec(Skill->CooldownEffect,1,ASC->MakeEffectContext());
    if (!Spec.IsValid()) return false;
    Spec.Data->SetDuration(Duration,true);
    Spec.Data->DynamicGrantedTags.AddTag(Skill->CooldownTag);
    const auto Handle=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    if (!Handle.IsValid()) return false;
    if (const auto* Previous=CooldownHandles.Find(Path); Previous && Previous->IsValid() && *Previous!=Handle) ASC->RemoveActiveGameplayEffect(*Previous);
    CooldownHandles.Add(Path,Handle); CooldownExpiries.Add(Path,FPlatformTime::Seconds()+Duration);
    return true;
}
TArray<FMythicAbilityCooldownSave> UMythicSkillComponent::CaptureAbilityCooldowns() const {
    TArray<FMythicAbilityCooldownSave> Saved;
    const double Now=FPlatformTime::Seconds();
    for (const auto& Pair:CooldownExpiries) {
        // An authored policy change can retire a former timed debt after normal catalogue validation.
        if (const auto* Catalog=GetCatalog(); Catalog && Catalog->IsReady())
            if (const auto* Definition=Catalog->Find(Pair.Key); Definition && !Definition->HasTimedCooldown()) continue;
        const float Remaining=static_cast<float>(Pair.Value-Now);
        if (FMath::IsFinite(Remaining) && Remaining>0) { auto& Row=Saved.AddDefaulted_GetRef(); Row.Skill=Pair.Key; Row.RemainingSeconds=FMath::Min(Remaining,3600.0f); }
    }
    if (const auto* Catalog=GetCatalog()) for (const auto& Skill:Catalog->GetDefinitions()) {
        const float Remaining=GetCooldownRemaining(Skill);
        if (Remaining>0) {
            auto* Existing=Saved.FindByPredicate([&](const auto& Row) { return Row.Skill==FSoftObjectPath(Skill); });
            if (Existing) Existing->RemainingSeconds=FMath::Max(Existing->RemainingSeconds,Remaining);
            else { auto& Row=Saved.AddDefaulted_GetRef(); Row.Skill=FSoftObjectPath(Skill); Row.RemainingSeconds=Remaining; }
        }
    }
    return Saved;
}
void UMythicSkillComponent::RestoreAbilityCooldowns(const TArray<FMythicAbilityCooldownSave>& Saved) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    auto* Catalog=GetCatalog();
    if (Catalog && !Catalog->IsReady()) {
        const double Now=FPlatformTime::Seconds();
        for (const auto& Row:Saved) if (!Row.Skill.IsNull() && FMath::IsFinite(Row.RemainingSeconds) && Row.RemainingSeconds>0 && Row.RemainingSeconds<=3600)
            CooldownExpiries.FindOrAdd(Row.Skill)=FMath::Max(CooldownExpiries.FindRef(Row.Skill),Now+Row.RemainingSeconds);
        bStateReady=false;
        bPendingCooldownMaterialization=true; DeferCatalogueRestore();
        return;
    }
    for (const auto& Row:Saved) if (auto* Skill=Catalog ? Catalog->Find(Row.Skill) : nullptr)
        if (FMath::IsFinite(Row.RemainingSeconds) && Row.RemainingSeconds>0 && Row.RemainingSeconds<=3600) StartCooldown(Skill,Row.RemainingSeconds);
}
FText UMythicSkillComponent::DescribeResult(EMythicAbilityResult Result) {
    switch(Result) {
        case EMythicAbilityResult::Success:return FText::GetEmpty();
        case EMythicAbilityResult::NotReady:return NSLOCTEXT("Mythic","AbilityNotReady","Abilities are still loading.");
        case EMythicAbilityResult::LockedAbility:return NSLOCTEXT("Mythic","AbilityLocked","Earn this ability before equipping it.");
        case EMythicAbilityResult::LockedSlot:return NSLOCTEXT("Mythic","AbilitySlotLocked","This ability socket is still sealed.");
        case EMythicAbilityResult::EquippedElsewhere:return NSLOCTEXT("Mythic","AbilityElsewhere","Unequip this ability from the other socket first.");
        case EMythicAbilityResult::ActiveAbility:return NSLOCTEXT("Mythic","AbilityActive","Wait for the active ability to end.");
        case EMythicAbilityResult::IncompatibleModifier:return NSLOCTEXT("Mythic","AbilityIncompatible","Remove the other behavior modifier first.");
        case EMythicAbilityResult::Capacity:return NSLOCTEXT("Mythic","AbilityCapacity","No modifier capacity remains.");
        case EMythicAbilityResult::Points:return NSLOCTEXT("Mythic","AbilityPoints","More practice is needed for this modifier.");
        default:return NSLOCTEXT("Mythic","AbilityUnavailable","This ability choice is unavailable.");
    }
}
