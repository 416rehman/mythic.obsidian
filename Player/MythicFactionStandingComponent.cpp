//

#include "MythicFactionStandingComponent.h"

#include "Mythic.h"
#include "Net/UnrealNetwork.h"
#include "World/LivingWorld/LivingWorldSubsystem.h" // FactionDatabase (server-side faction name resolution)
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "UI/MythicDamageNumberSubsystem.h" // floating "now Hostile/Friendly" callout
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

UMythicFactionStandingComponent::UMythicFactionStandingComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicFactionStandingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Owner-only: faction standing is private per-player UI data. This component lives on the PlayerState (net-relevant
    // to EVERY client), so an unconditional DOREPLIFETIME leaked every player's full reputation list to all peers.
    // Matches the sibling progression components (Proficiency / ObjectiveTracker, both COND_OwnerOnly).
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
        return; // only announce a genuine tier crossing
    }
    // Resolve the faction's display name on the SERVER (clients carry no FactionDatabase) and push it to the owning
    // client. Reading the committed faction snapshot from the game thread is the established lock-free pattern.
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
    const APlayerState *PS = Cast<APlayerState>(GetOwner());
    const APawn *Pawn = PS ? PS->GetPawn() : nullptr;
    if (!Pawn) {
        return;
    }
    UWorld *World = Pawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        FString TierText;
        FLinearColor Color;
        switch (NewTier) {
        case EMythicStandingTier::Hostile:
            TierText = TEXT("Hostile");
            Color = FLinearColor(0.9f, 0.1f, 0.1f); // red
            break;
        case EMythicStandingTier::Friendly:
            TierText = TEXT("Friendly");
            Color = FLinearColor(0.1f, 0.9f, 0.3f); // green
            break;
        default:
            TierText = TEXT("Neutral");
            Color = FLinearColor(0.8f, 0.8f, 0.8f); // gray
            break;
        }
        const FVector Location = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
        const FString Text = FString::Printf(TEXT("%s now %s"), *FactionName, *TierText);
        DamageNumbers->AddDamageNumberCustom(Location, Text, Color, 3.0f);
    }
}
