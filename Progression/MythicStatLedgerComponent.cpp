
#include "MythicStatLedgerComponent.h"

#include "Settings/MythicDeveloperSettings.h"

#include "MythicStatLedger.h"
#include "MythicTags_MetaProgression.h"
#include "Mythic/GAS/MythicTags_GAS.h"
#include "Mythic/AI/MythicTags_AI.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UMythicStatLedgerComponent::UMythicStatLedgerComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicStatLedgerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicStatLedgerComponent, CharacterCounters, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicStatLedgerComponent, AccountCounters, COND_OwnerOnly);
}


int64 UMythicStatLedgerComponent::ApplyAndMarkDirty(FMythicStatCounterArray &Array, const FGameplayTag &Tag, int64 Delta) {
    const int64 NewValue = FMythicStatLedger::ApplyDelta(Array.Items, Tag, Delta);
    for (FMythicStatCounter &C : Array.Items) {
        if (C.Tag == Tag) {
            Array.MarkItemDirty(C);
            break;
        }
    }
    return NewValue;
}

void UMythicStatLedgerComponent::RecordStat(FGameplayTag Tag, int64 Delta, bool bAccountToo) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Tag.IsValid() || bIsRestoring) {
        return;
    }
    const int64 NewCharValue = ApplyAndMarkDirty(CharacterCounters, Tag, Delta);
    if (bAccountToo) {
        ApplyAndMarkDirty(AccountCounters, Tag, Delta);
    }
    OnCounterChanged.Broadcast(Tag, NewCharValue);
}

bool UMythicStatLedgerComponent::RecordStatMax(FGameplayTag Tag, int64 Value, bool bAccountToo) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Tag.IsValid() || bIsRestoring) {
        return false;
    }
    bool bNewRecord = false;
    const int64 NewCharValue = FMythicStatLedger::ApplyMax(CharacterCounters.Items, Tag, Value, &bNewRecord);
    if (bNewRecord) {
        for (FMythicStatCounter &C : CharacterCounters.Items) {
            if (C.Tag == Tag) {
                CharacterCounters.MarkItemDirty(C);
                break;
            }
        }
        OnCounterChanged.Broadcast(Tag, NewCharValue);
    }
    if (bAccountToo) {
        bool bAccountRaised = false;
        FMythicStatLedger::ApplyMax(AccountCounters.Items, Tag, Value, &bAccountRaised);
        if (bAccountRaised) {
            for (FMythicStatCounter &C : AccountCounters.Items) {
                if (C.Tag == Tag) {
                    AccountCounters.MarkItemDirty(C);
                    break;
                }
            }
        }
    }
    return bNewRecord;
}

int64 UMythicStatLedgerComponent::GetCounter(FGameplayTag Tag) const {
    return FMythicStatLedger::FindValue(CharacterCounters.Items, Tag);
}

int64 UMythicStatLedgerComponent::GetCounterRollup(FGameplayTag PrefixTag) const {
    return FMythicStatLedger::SumByPrefix(CharacterCounters.Items, PrefixTag);
}

void UMythicStatLedgerComponent::RestoreCharacterCounters(const TArray<FMythicStatCounter> &Saved) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    bIsRestoring = true;
    CharacterCounters.Items.Reset();
    for (const FMythicStatCounter &S : Saved) {
        if (!S.Tag.IsValid()) {
            continue;
        }
        FMythicStatCounter &New = CharacterCounters.Items.AddDefaulted_GetRef();
        New.Tag = S.Tag;
        New.Value = FMythicStatLedger::ClampFloor(S.Value);
        CharacterCounters.MarkItemDirty(New);
    }
    bIsRestoring = false;
}

void UMythicStatLedgerComponent::ResyncCurrencyBaseline() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    LastKnownCurrency = ComputeTotalCurrency();
}


void UMythicStatLedgerComponent::BeginPlay() {
    Super::BeginPlay();
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    BindGameplayEvents();
    BindInventoryDelegates();

    if (bDistanceOdometerEnabled) {
        if (UWorld *World = GetWorld()) {
            World->GetTimerManager().SetTimer(DistanceTimerHandle, this, &UMythicStatLedgerComponent::SampleDistance,
                                              DistanceSampleInterval,true);
        }
    }
}

void UMythicStatLedgerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UAbilitySystemComponent *ASC = ResolveASC()) {
        if (KillEventHandle.IsValid()) {
            if (FGameplayEventMulticastDelegate *Del = ASC->GenericGameplayEventCallbacks.Find(GAS_EVENT_KILL)) {
                Del->Remove(KillEventHandle);
            }
        }
        if (DeathEventHandle.IsValid()) {
            if (FGameplayEventMulticastDelegate *Del = ASC->GenericGameplayEventCallbacks.Find(GAS_EVENT_DEATH)) {
                Del->Remove(DeathEventHandle);
            }
        }
    }
    UnbindInventoryDelegates();
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(DistanceTimerHandle);
        World->GetTimerManager().ClearTimer(InventoryBindRetryTimer);
    }
    Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent *UMythicStatLedgerComponent::ResolveASC() const {
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UMythicStatLedgerComponent::BindGameplayEvents() {
    UAbilitySystemComponent *ASC = ResolveASC();
    if (!ASC) {
        return;
    }
    KillEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_KILL).AddUObject(
        this, &UMythicStatLedgerComponent::HandleKillEvent);
    DeathEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH).AddUObject(
        this, &UMythicStatLedgerComponent::HandleDeathEvent);
    ItemAcquiredEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_ITEM_ACQUIRED).AddUObject(
        this, &UMythicStatLedgerComponent::HandleItemAcquiredEvent);
}

bool UMythicStatLedgerComponent::IsGatheredAcquisition(const FGameplayTagContainer &ItemTags, const FGameplayTagContainer &GatheredTypes) {
    if (GatheredTypes.IsEmpty() || ItemTags.IsEmpty()) {
        return false;
    }
    return ItemTags.HasAny(GatheredTypes);
}

int64 UMythicStatLedgerComponent::QuantityFromEvent(float EventMagnitude) {
    return FMath::Max<int64>(1, static_cast<int64>(FMath::RoundToInt(EventMagnitude)));
}

void UMythicStatLedgerComponent::HandleItemAcquiredEvent(const FGameplayEventData *Payload) {
    if (!Payload) {
        return;
    }
    // The acquisition event stamps the item's type onto TargetTags, so the ledger classifies without reloading
    // the definition.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bGathered = Settings && IsGatheredAcquisition(Payload->TargetTags, Settings->GatheredItemTypes);
    RecordStat(bGathered ? STAT_ITEM_GATHERED : STAT_ITEM_LOOTED, QuantityFromEvent(Payload->EventMagnitude));
}

void UMythicStatLedgerComponent::HandleKillEvent(const FGameplayEventData *Payload) {
    RecordStat(STAT_KILL_GENERIC, 1);

    if (!Payload) {
        return;
    }
    bool bBoss = Payload->TargetTags.HasTag(AI_TIER_BOSS);
    if (!bBoss) {
        if (const AActor *Victim = Payload->Target.Get()) {
            if (const UAbilitySystemComponent *VictimASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Victim)) {
                bBoss = VictimASC->HasMatchingGameplayTag(AI_TIER_BOSS);
            }
        }
    }
    if (bBoss) {
        RecordStat(STAT_KILL_BOSS, 1);
    }
}

void UMythicStatLedgerComponent::HandleDeathEvent(const FGameplayEventData *) {
    RecordStat(STAT_DEATH, 1);
}


TArray<UMythicInventoryComponent *> UMythicStatLedgerComponent::GetOwnerInventories() const {
    if (IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(GetOwner())) {
        return Provider->GetAllInventoryComponents();
    }
    return TArray<UMythicInventoryComponent *>();
}

int32 UMythicStatLedgerComponent::ComputeTotalCurrency() const {
    int32 Total = 0;
    for (const UMythicInventoryComponent *Inv : GetOwnerInventories()) {
        if (Inv) {
            Total += Inv->GetTotalCurrency();
        }
    }
    return Total;
}

void UMythicStatLedgerComponent::BindInventoryDelegates() {
    UnbindInventoryDelegates();
    for (UMythicInventoryComponent *Inv : GetOwnerInventories()) {
        if (!Inv) {
            continue;
        }
        Inv->OnSlotUpdated.AddDynamic(this, &UMythicStatLedgerComponent::HandleInventorySlotUpdated);
        BoundInventories.Add(Inv);
    }

    if (BoundInventories.Num() > 0) {
        LastKnownCurrency = ComputeTotalCurrency();
        if (UWorld *World = GetWorld()) {
            World->GetTimerManager().ClearTimer(InventoryBindRetryTimer);
        }
    }
    else if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(InventoryBindRetryTimer, this, &UMythicStatLedgerComponent::BindInventoryDelegates,
                                          5.0f,true);
    }
}

void UMythicStatLedgerComponent::UnbindInventoryDelegates() {
    for (TWeakObjectPtr<UMythicInventoryComponent> &Weak : BoundInventories) {
        if (UMythicInventoryComponent *Inv = Weak.Get()) {
            Inv->OnSlotUpdated.RemoveDynamic(this, &UMythicStatLedgerComponent::HandleInventorySlotUpdated);
        }
    }
    BoundInventories.Reset();
}

void UMythicStatLedgerComponent::HandleInventorySlotUpdated(int32) {
    const int32 Now = ComputeTotalCurrency();
    if (Now > LastKnownCurrency) {
        RecordStat(STAT_GOLD_EARNED, static_cast<int64>(Now - LastKnownCurrency));
    }
    LastKnownCurrency = Now;
}


void UMythicStatLedgerComponent::SampleDistance() {
    const APlayerState *PS = Cast<APlayerState>(GetOwner());
    const APawn *Pawn = PS ? PS->GetPawn() : nullptr;
    if (!Pawn) {
        return;
    }
    const FVector Loc = Pawn->GetActorLocation();
    if (!bHasOdometerBaseline) {
        LastOdometerLocation = Loc;
        bHasOdometerBaseline = true;
        return;
    }
    const double DistCm = FVector::Dist(LastOdometerLocation, Loc);
    LastOdometerLocation = Loc;
    if (DistCm >= 1.0) {
        RecordStat(STAT_DISTANCE_TRAVELED, static_cast<int64>(DistCm));
    }
}
