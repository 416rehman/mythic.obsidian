
#include "MythicUnlockComponent.h"

#include "MythicUnlockRuleSet.h"
#include "MythicUnlockRule.h"
#include "MythicUnlockEngine.h"
#include "MythicAchievementComponent.h"
#include "Runes/MythicRuneComponent.h"
#include "Skills/MythicSkillComponent.h"

#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "Mythic/Rewards/RewardBase.h"
#include "Mythic/Mythic.h"

#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/Itemization/MythicTags_Conversion.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

namespace {
    FGameplayTag RuleKey(const UMythicUnlockRule &Rule) {
        return Rule.RuleId.IsValid() ? Rule.RuleId : Rule.EffectPayloadTag;
    }
    bool RewardIsSet(const FRewardsToGive &R) {
        return R.XPReward || R.ItemReward || R.LootReward || R.AbilityReward || R.AttributeReward
            || R.RenownReward;
    }
}

UMythicUnlockComponent::UMythicUnlockComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicUnlockComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicUnlockComponent, GrantedUnlockTags, COND_OwnerOnly);
    DOREPLIFETIME(UMythicUnlockComponent, ActiveTitle);
}


void UMythicUnlockComponent::BeginPlay() {
    Super::BeginPlay();
    ResolveRuleSet();
    BuildRuleIndex();

    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    if (UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Narrative->OnStoryTagEarned.RemoveDynamic(this, &UMythicUnlockComponent::HandleStoryTagEarned);
        Narrative->OnStoryTagEarned.AddDynamic(this, &UMythicUnlockComponent::HandleStoryTagEarned);
    }
    if (UMythicAchievementComponent *Achievements = ResolveAchievements()) {
        Achievements->OnAchievementUnlocked.RemoveDynamic(this, &UMythicUnlockComponent::HandleAchievementUnlocked);
        Achievements->OnAchievementUnlocked.AddDynamic(this, &UMythicUnlockComponent::HandleAchievementUnlocked);
    }
}

void UMythicUnlockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Narrative->OnStoryTagEarned.RemoveDynamic(this, &UMythicUnlockComponent::HandleStoryTagEarned);
    }
    if (UMythicAchievementComponent *Achievements = ResolveAchievements()) {
        Achievements->OnAchievementUnlocked.RemoveDynamic(this, &UMythicUnlockComponent::HandleAchievementUnlocked);
    }
    Super::EndPlay(EndPlayReason);
}


UMythicNarrativeStateComponent *UMythicUnlockComponent::ResolveNarrative() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetNarrativeState();
    }
    return nullptr;
}

UMythicAchievementComponent *UMythicUnlockComponent::ResolveAchievements() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetAchievementComponent();
    }
    return nullptr;
}

APlayerController *UMythicUnlockComponent::ResolvePC() const {
    if (const APlayerState *PS = Cast<APlayerState>(GetOwner())) {
        return PS->GetPlayerController();
    }
    return nullptr;
}

UMythicUnlockRuleSet *UMythicUnlockComponent::ResolveRuleSet() {
    if (ResolvedRuleSet) {
        return ResolvedRuleSet;
    }
    if (!UnlockRuleSet.IsNull()) {
        ResolvedRuleSet = UnlockRuleSet.LoadSynchronous();
    }
    if (!ResolvedRuleSet) {
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            if (!Settings->DefaultUnlockRuleSet.IsNull()) {
                ResolvedRuleSet = Settings->DefaultUnlockRuleSet.LoadSynchronous();
            }
        }
    }
    return ResolvedRuleSet;
}

void UMythicUnlockComponent::BuildRuleIndex() {
    RulePreconditions.Reset();
    RuleIndexById.Reset();
    bIndexBuilt = true;
    if (!ResolvedRuleSet) {
        return;
    }
    for (int32 i = 0; i < ResolvedRuleSet->Rules.Num(); ++i) {
        const UMythicUnlockRule *Rule = ResolvedRuleSet->Rules[i];
        RulePreconditions.Add(Rule ? Rule->Precondition : FMythicStoryCondition());
        if (Rule) {
            const FGameplayTag Key = RuleKey(*Rule);
            if (Key.IsValid()) {
                RuleIndexById.Add(Key, i);
            }
        }
    }
}


FGameplayTagContainer UMythicUnlockComponent::GatherOwnedTags() const {
    FGameplayTagContainer Owned;
    if (const UMythicNarrativeStateComponent *Narrative = ResolveNarrative()) {
        Owned.AppendTags(Narrative->GetOwnedTags());
    }
    if (const UMythicAchievementComponent *Achievements = ResolveAchievements()) {
        Owned.AppendTags(Achievements->GetUnlockedAchievements());
    }
    Owned.AppendTags(GrantedUnlockTags);
    return Owned;
}

void UMythicUnlockComponent::HandleStoryTagEarned(FGameplayTag Tag) {
    EnqueueAndDrain(Tag);
}

void UMythicUnlockComponent::HandleAchievementUnlocked(FGameplayTag AchievementTag) {
    EnqueueAndDrain(AchievementTag);
}

void UMythicUnlockComponent::EnqueueAndDrain(const FGameplayTag &Tag) {
    const AActor *Owner = GetOwner();
    if (bIsRestoring || !Owner || !Owner->HasAuthority()) {
        return;
    }
    PendingTriggers.Add(Tag);
    if (bEvaluating) {
        return;
    }
    bEvaluating = true;
    bool bChanged = true;
    int32 Safety = 0;
    while (bChanged && Safety++ < 128) {
        PendingTriggers.Reset();
        bChanged = EvaluatePass();
        if (PendingTriggers.Num() > 0) {
            bChanged = true;
        }
    }
    bEvaluating = false;
}

bool UMythicUnlockComponent::EvaluatePass() {
    if (!ResolvedRuleSet) {
        return false;
    }
    const FGameplayTagContainer Owned = GatherOwnedTags();
    TArray<int32> Fire;
    FMythicUnlockEngine::CollectNewlySatisfied(RulePreconditions, Owned, AppliedRuleIndices, Fire);

    for (int32 Idx : Fire) {
        AppliedRuleIndices.Add(Idx);
        UMythicUnlockRule *Rule = ResolvedRuleSet->Rules.IsValidIndex(Idx) ? ResolvedRuleSet->Rules[Idx] : nullptr;
        if (!Rule) {
            continue;
        }
        ApplyRule(*Rule);
        const FGameplayTag Key = RuleKey(*Rule);
        if (Key.IsValid()) {
            AppliedUnlockRules.AddUnique(Key);
        }
    }
    return Fire.Num() > 0;
}

void UMythicUnlockComponent::ApplyRule(const UMythicUnlockRule &Rule) {
    switch (Rule.Effect) {
        case EMythicUnlockEffect::UnlockSkill:
        case EMythicUnlockEffect::UnlockPerk:
            break;
        case EMythicUnlockEffect::GrantSkillSlot:
            if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
                if (UMythicSkillComponent *SkillComp = PS->GetSkillComponent()) {
                    SkillComp->GrantSlot();
                }
            }
            break;
        case EMythicUnlockEffect::GrantPerkSlot:
            if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
                if (UMythicRuneComponent *Runes = PS->GetRuneComponent()) {
                    Runes->GrantSlot();
                }
            }
            break;
        case EMythicUnlockEffect::GrantTitle:
        case EMythicUnlockEffect::GrantCosmetic:
            if (Rule.EffectPayloadTag.IsValid() && !GrantedUnlockTags.HasTagExact(Rule.EffectPayloadTag)) {
                GrantedUnlockTags.AddTag(Rule.EffectPayloadTag);
            }
            break;
        case EMythicUnlockEffect::UnlockRecipe:
            ServerLearnRecipe(Rule.EffectPayloadTag);
            break;
        case EMythicUnlockEffect::UnlockFastTravel:
            UE_LOG(Myth, Log, TEXT("Unlocks: UnlockFastTravel %s (integration point — wire the fast-travel graph here)"),
                   *Rule.EffectPayloadTag.ToString());
            break;
        case EMythicUnlockEffect::GrantReward:
        default:
            break;
    }

    if (!bIsRestoring && RewardIsSet(Rule.OptionalReward)) {
        if (APlayerController *PC = ResolvePC()) {
            Rule.OptionalReward.Give(PC);
        }
    }
}


void UMythicUnlockComponent::EnsureSchematicTagOnASC(const FGameplayTag &SchematicTag) const {
    if (!SchematicTag.IsValid()) {
        return;
    }
    const IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(GetOwner());
    UAbilitySystemComponent *ASC = Provider ? Provider->GetSchematicsASC() : nullptr;
    if (ASC && !ASC->HasMatchingGameplayTag(SchematicTag)) {
        ASC->AddLooseGameplayTag(SchematicTag, 1, EGameplayTagReplicationState::TagOnly);
    }
}

bool UMythicUnlockComponent::ServerLearnRecipe(FGameplayTag SchematicTag) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return false;
    }
    if (!ShouldGrantLearn(GrantedUnlockTags, SchematicTag)) {
        EnsureSchematicTagOnASC(SchematicTag);
        return false;
    }
    GrantedUnlockTags.AddTag(SchematicTag);
    EnsureSchematicTagOnASC(SchematicTag);
    EnqueueAndDrain(SchematicTag);
    return true;
}


void UMythicUnlockComponent::ServerSetActiveTitle(FGameplayTag TitleTag) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (TitleTag.IsValid() && !GrantedUnlockTags.HasTagExact(TitleTag)) {
        return;
    }
    ActiveTitle = TitleTag;
}

void UMythicUnlockComponent::RestoreUnlockState(const FGameplayTagContainer &SavedGranted,
                                                const TArray<FGameplayTag> &SavedAppliedRules, FGameplayTag SavedActiveTitle) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    GrantedUnlockTags = SavedGranted;
    AppliedUnlockRules = SavedAppliedRules;
    ActiveTitle = SavedActiveTitle;

    for (const FGameplayTag &Granted : GrantedUnlockTags) {
        if (Granted.MatchesTag(ITEMIZATION_SCHEMATIC)) {
            EnsureSchematicTagOnASC(Granted);
        }
    }

    if (!bIndexBuilt) {
        BuildRuleIndex();
    }
    AppliedRuleIndices.Reset();
    for (const FGameplayTag &Id : AppliedUnlockRules) {
        if (const int32 *Idx = RuleIndexById.Find(Id)) {
            AppliedRuleIndices.Add(*Idx);
        }
    }
}
