
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"

#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UMythicDossierComponent::UMythicDossierComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void UMythicDossierComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicDossierComponent, Dossiers, COND_OwnerOnly);
}

void UMythicDossierComponent::BeginPlay() {
    Super::BeginPlay();
    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        if (UMythicAcquaintanceComponent *Acquaintance = Owner->FindComponentByClass<UMythicAcquaintanceComponent>()) {
            RelationChangedHandle = Acquaintance->OnRelationChangedNative.AddUObject(
                this, &UMythicDossierComponent::HandleRelationChanged);
        }
    }
}

void UMythicDossierComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (RelationChangedHandle.IsValid()) {
        if (const AActor *Owner = GetOwner()) {
            if (UMythicAcquaintanceComponent *Acquaintance = Owner->FindComponentByClass<UMythicAcquaintanceComponent>()) {
                Acquaintance->OnRelationChangedNative.Remove(RelationChangedHandle);
            }
        }
        RelationChangedHandle.Reset();
    }
    Super::EndPlay(EndPlayReason);
}

double UMythicDossierComponent::NowSeconds() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

void UMythicDossierComponent::HandleRelationChanged(const FMythicNpcRelation &Relation, EMythicNpcInteraction Interaction) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (FMythicNpcDossier *Row = FMythicDossierRules::Upsert(Dossiers, Relation.NpcNameHash, NowSeconds(), MaxDossiers)) {
        FMythicDossierRules::ApplyRelationEvent(*Row, Relation, Interaction);
        OnDossiersChanged.Broadcast();
    }
}

void UMythicDossierComponent::ServerObserveNpc(uint32 NameHash, const FText &DisplayName, FGameplayTag Faction,
                                               FGameplayTag RoleTag) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || NameHash == 0) {
        return;
    }
    if (FMythicNpcDossier *Row = FMythicDossierRules::Upsert(Dossiers, NameHash, NowSeconds(), MaxDossiers)) {
        if (!DisplayName.IsEmpty()) {
            Row->DisplayName = DisplayName;
        }
        if (Faction.IsValid()) {
            Row->Faction = Faction;
        }
        if (RoleTag.IsValid()) {
            Row->RoleTag = RoleTag;
        }
        OnDossiersChanged.Broadcast();
    }
}

void UMythicDossierComponent::ServerRecordNpcDeath(uint32 NameHash, uint32 KillerNameHash, const FText &DisplayName,
                                                   FGameplayTag Faction, FGameplayTag RoleTag, bool bUpsertIfMissing) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || NameHash == 0) {
        return;
    }
    FMythicNpcDossier *Row = FMythicDossierRules::Find(Dossiers, NameHash);
    if (!Row) {
        if (!bUpsertIfMissing) {
            return;
        }
        Row = FMythicDossierRules::Upsert(Dossiers, NameHash, NowSeconds(), MaxDossiers);
        if (!Row) {
            return;
        }
    }
    else {
        Row->LastUpdateTime = NowSeconds();
    }
    Row->bDead = true;
    Row->KillerNameHash = KillerNameHash;
    if (!DisplayName.IsEmpty()) {
        Row->DisplayName = DisplayName;
    }
    if (Faction.IsValid()) {
        Row->Faction = Faction;
    }
    if (RoleTag.IsValid()) {
        Row->RoleTag = RoleTag;
    }
    OnDossiersChanged.Broadcast();
}

void UMythicDossierComponent::RestoreDossiers(const TArray<FMythicNpcDossier> &InDossiers) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || InDossiers.Num() == 0) {
        return;
    }
    Dossiers = InDossiers;
    const int32 Cap = FMath::Max(1, MaxDossiers);
    while (Dossiers.Num() > Cap) {
        int32 OldestIdx = 0;
        for (int32 i = 1; i < Dossiers.Num(); ++i) {
            if (Dossiers[i].LastUpdateTime < Dossiers[OldestIdx].LastUpdateTime) {
                OldestIdx = i;
            }
        }
        Dossiers.RemoveAt(OldestIdx, 1, EAllowShrinking::No);
    }
    OnDossiersChanged.Broadcast();
}

bool UMythicDossierComponent::GetDossier(uint32 NameHash, FMythicNpcDossier &Out) const {
    if (const FMythicNpcDossier *Row = FMythicDossierRules::Find(Dossiers, NameHash)) {
        Out = *Row;
        return true;
    }
    return false;
}

void UMythicDossierComponent::OnRep_Dossiers() {
    OnDossiersChanged.Broadcast();
}
