
#include "MythicFactionStandingComponent.h"

#include "Mythic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

UMythicFactionStandingComponent::UMythicFactionStandingComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicFactionStandingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicFactionStandingComponent, Standings, COND_OwnerOnly);
}

int32 UMythicFactionStandingComponent::FindEntryIndex(FMythicFactionId Faction) const {
    for (int32 i = 0; i < Standings.Num(); ++i) {
        if (Standings[i].Faction == Faction) {
            return i;
        }
    }
    return INDEX_NONE;
}

float UMythicFactionStandingComponent::GetStanding(FMythicFactionId Faction) const {
    if (!Faction.IsValid()) {
        return 0.0f;
    }
    const int32 Index = FindEntryIndex(Faction);
    return Index != INDEX_NONE ? Standings[Index].Value : 0.0f;
}

void UMythicFactionStandingComponent::ServerAdjustStanding(FMythicFactionId Faction, float Delta) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Faction.IsValid() || Delta == 0.0f) {
        return;
    }

    int32 Index = FindEntryIndex(Faction);
    if (Index == INDEX_NONE) {
        FMythicFactionStandingEntry NewEntry;
        NewEntry.Faction = Faction;
        NewEntry.Value = 0.0f;
        Index = Standings.Add(NewEntry);
    }

    const float OldValue = Standings[Index].Value;
    Standings[Index].Value = FMath::Clamp(Standings[Index].Value + Delta, MinStanding, MaxStanding);
    UE_LOG(Myth, Log, TEXT("FactionStanding: %s standing toward faction %d -> %.1f (delta %.1f)"),
           *GetNameSafe(Owner), Faction.Index, Standings[Index].Value, Delta);
    NotifyStandingTierChange(Faction, OldValue, Standings[Index].Value);
}

float FMythicKillStandingPropagation::FactorForRelation(EMythicFactionRelation Relation) const {
    switch (Relation) {
    case EMythicFactionRelation::Allied:
        return AlliedFactor;
    case EMythicFactionRelation::Friendly:
        return FriendlyFactor;
    case EMythicFactionRelation::Hostile:
        return -HostileFactor;
    case EMythicFactionRelation::Unfriendly:
        return -UnfriendlyFactor;
    default:
        return 0.0f;
    }
}

void UMythicFactionStandingComponent::ServerApplyKillStanding(FMythicFactionId VictimFaction) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !VictimFaction.IsValid()) {
        return;
    }

    const float BasePenalty = KillStandingPenalty;
    ServerAdjustStanding(VictimFaction, -BasePenalty);

    if (KillStandingPropagation.IsDisabled()) {
        return;
    }
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    UMythicFactionDatabase *FDB = LWS ? LWS->GetFactionDatabase() : nullptr;
    if (!FDB) {
        return;
    }

    TArray<FMythicFactionId, TInlineAllocator<32>> AliveFactions;
    FDB->ForEachAliveFaction([&AliveFactions](FMythicFactionId Id, const FMythicFactionData &) {
        AliveFactions.Add(Id);
    });

    for (const FMythicFactionId Id : AliveFactions) {
        if (!Id.IsValid() || Id == VictimFaction) {
            continue;
        }
        const EMythicFactionRelation Relation = FDB->GetRelationship(VictimFaction, Id);
        const float Factor = KillStandingPropagation.FactorForRelation(Relation);
        if (Factor != 0.0f) {
            ServerAdjustStanding(Id, -BasePenalty * Factor);
        }
    }
}

void UMythicFactionStandingComponent::SetStanding(FMythicFactionId Faction, float NewValue) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Faction.IsValid()) {
        return;
    }

    int32 Index = FindEntryIndex(Faction);
    if (Index == INDEX_NONE) {
        FMythicFactionStandingEntry NewEntry;
        NewEntry.Faction = Faction;
        Index = Standings.Add(NewEntry);
    }

    const float OldValue = Standings[Index].Value;
    Standings[Index].Value = FMath::Clamp(NewValue, MinStanding, MaxStanding);
    NotifyStandingTierChange(Faction, OldValue, Standings[Index].Value);
}

EMythicStandingTier UMythicFactionStandingComponent::TierForStanding(float Value) const {
    if (Value <= HostileStandingThreshold) {
        return EMythicStandingTier::Hostile;
    }
    if (Value >= FriendlyStandingThreshold) {
        return EMythicStandingTier::Friendly;
    }
    return EMythicStandingTier::Neutral;
}

void UMythicFactionStandingComponent::NotifyStandingTierChange(FMythicFactionId Faction, float OldValue, float NewValue) {
    const EMythicStandingTier OldTier = TierForStanding(OldValue);
    const EMythicStandingTier NewTier = TierForStanding(NewValue);
    if (OldTier == NewTier) {
        return;
    }
    FString FactionName = FString::Printf(TEXT("Faction %d"), Faction.Index);
    if (UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                if (UMythicFactionDatabase *FDB = LWS->GetFactionDatabase()) {
                    FMythicFactionData Data;
                    if (FDB->GetFaction(Faction, Data)) {
                        FactionName = Data.DisplayName.ToString();
                    }
                }
            }
        }
    }
    ClientNotifyStandingTier(FactionName, NewTier);
}

void UMythicFactionStandingComponent::ClientNotifyStandingTier_Implementation(const FString &FactionName, EMythicStandingTier NewTier) {
}
