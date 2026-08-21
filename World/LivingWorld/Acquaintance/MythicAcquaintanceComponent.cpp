
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_HOSTILE, "Npc.Warmth.Hostile", "Active-conversation NPC holds a grudge (warmth <= -50)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_WARY, "Npc.Warmth.Wary", "Active-conversation NPC is wary (-50 < warmth <= -15)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_STRANGER, "Npc.Warmth.Stranger", "Active-conversation NPC barely knows the player (default)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_ACQUAINTANCE, "Npc.Warmth.Acquaintance", "Active-conversation NPC recognizes the player (15 <= warmth < 45)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_FRIEND, "Npc.Warmth.Friend", "Active-conversation NPC counts the player a friend (45 <= warmth < 80)")
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_NPC_WARMTH_CONFIDANT, "Npc.Warmth.Confidant", "Active-conversation NPC trusts the player fully (warmth >= 80)")

UMythicAcquaintanceComponent::UMythicAcquaintanceComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void UMythicAcquaintanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicAcquaintanceComponent, Relations, COND_OwnerOnly);
}

double UMythicAcquaintanceComponent::NowSeconds() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

float UMythicAcquaintanceComponent::ServerRecordInteraction(uint32 NpcNameHash, FGameplayTag Faction,
                                                            EMythicNpcInteraction Interaction) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || NpcNameHash == 0) {
        return 0.0f;
    }
    const float NewWarmth =
        FMythicAcquaintanceRules::ApplyInteraction(Relations, NpcNameHash, Faction, Interaction, NowSeconds(), Config);

    if (const FMythicNpcRelation *Rel = FMythicAcquaintanceRules::FindRelation(Relations, NpcNameHash)) {
        OnRelationChangedNative.Broadcast(*Rel, Interaction);
    }
    OnAcquaintanceChanged.Broadcast();
    return NewWarmth;
}

void UMythicAcquaintanceComponent::RestoreRelations(const TArray<FMythicNpcRelation> &InRelations) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || InRelations.Num() == 0) {
        return;
    }
    Relations = InRelations;
    const int32 Cap = FMath::Max(1, Config.MaxRelations);
    while (Relations.Num() > Cap) {
        int32 OldestIdx = 0;
        for (int32 i = 1; i < Relations.Num(); ++i) {
            if (Relations[i].LastInteractionTime < Relations[OldestIdx].LastInteractionTime) {
                OldestIdx = i;
            }
        }
        Relations.RemoveAt(OldestIdx, 1, EAllowShrinking::No);
    }
    OnAcquaintanceChanged.Broadcast();
}

bool UMythicAcquaintanceComponent::GetRelation(uint32 NpcNameHash, FMythicNpcRelation &Out) const {
    const FMythicNpcRelation *Rel = FMythicAcquaintanceRules::FindRelation(Relations, NpcNameHash);
    if (!Rel) {
        return false;
    }
    Out = *Rel;
    Out.Warmth = FMythicAcquaintanceRules::DecayedWarmth(Rel->Warmth, Rel->LastInteractionTime, NowSeconds(),
                                                         Config.DecayPerDay, Config.SecondsPerWorldDay);
    return true;
}

float UMythicAcquaintanceComponent::GetCurrentWarmth(uint32 NpcNameHash) const {
    FMythicNpcRelation Rel;
    return GetRelation(NpcNameHash, Rel) ? Rel.Warmth : 0.0f;
}

EMythicWarmthTier UMythicAcquaintanceComponent::GetWarmthTier(uint32 NpcNameHash) const {
    FMythicNpcRelation Rel;
    if (!GetRelation(NpcNameHash, Rel)) {
        return EMythicWarmthTier::Stranger;
    }
    return FMythicAcquaintanceRules::WarmthTier(Rel.Warmth);
}

EMythicWarmthTier UMythicAcquaintanceComponent::GetWarmthTierForActor(const AActor *Npc) const {
    return Npc ? GetWarmthTier(GetTypeHash(Npc->GetFName())) : EMythicWarmthTier::Stranger;
}

void UMythicAcquaintanceComponent::OnRep_Relations() {
    OnAcquaintanceChanged.Broadcast();
}
