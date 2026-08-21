

#include "MythicResourceManagerComponent.h"

#include "Mythic.h"
#include "MythicResourceISM.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TimerManager.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "World/Gathering/MythicGatherRules.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#endif

void FTrackedDestructibleDataArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PreReplicatedRemove: Removed %d items"), RemovedIndices.Num());

    if (RemovedIndices.Num() <= 0) {
        return;
    }

    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : RemovedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceRespawn(itemsToSync);
}

void FTrackedDestructibleDataArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PostReplicatedAdd: Added %d items"), AddedIndices.Num());

    if (AddedIndices.Num() <= 0) {
        return;
    }

    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : AddedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync);
}

void FTrackedDestructibleDataArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PostReplicatedChange: Changed %d items"), ChangedIndices.Num());

    if (ChangedIndices.Num() <= 0) {
        return;
    }

    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : ChangedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync);
}


void UMythicResourceManagerComponent::ProcessBatchRespawn() {
    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Checking for resources to respawn"));
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn called on non-authority"));
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();

    UMythicRegionalPressureSubsystem *Pressure =
        GetWorld() ? GetWorld()->GetSubsystem<UMythicRegionalPressureSubsystem>() : nullptr;

    auto indicesToRemove = TArray<int32>();
    auto DestroyedItems = *DestroyedResources.GetItems();
    for (int32 i = 0; i < DestroyedItems.Num(); i++) {
        auto &item = DestroyedItems[i];
        if (ShouldRespawnDestructible(item.HitsTillDestruction, item.RespawnTime, CurrentTime)) {
            if (Pressure && Pressure->IsHarvestRespawnGated(item.Transform.GetLocation())) {
                continue;
            }
            indicesToRemove.Add(i);
        }
    }

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Found %d resources to respawn"), indicesToRemove.Num());

    if (indicesToRemove.Num() <= 0) {
        return;
    }

    DestroyedResources.RemoveItems(indicesToRemove);

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Removed %d resources from destroyed list"), indicesToRemove.Num());
}

void UMythicResourceManagerComponent::OnRep_DestroyedResources() {
    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources"));

    if (GetWorld()) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: Handling resource destruction after delay"));
        auto Items = DestroyedResources.GetItems();
        HandleResourceDestruction(*Items);
    }
    else {
        UE_LOG(Myth, Warning,
               TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: World is null; deferring resource sync to the next replication update."));
    }
}

UMythicResourceManagerComponent::UMythicResourceManagerComponent() {
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
}

namespace {
FGameplayTagContainer GetEquippedToolProbe(APlayerController *PlayerController) {
    FGameplayTagContainer Probe;
    const IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(PlayerController);
    if (!Provider) {
        return Probe;
    }
    static const FGameplayTag EquipmentGroup = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Group.Equipment"));

    FGameplayTagContainer Single;
    for (UMythicInventoryComponent *Inventory : Provider->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inventory->GetAllSlots()) {
            if (!Slot.GroupTag.MatchesTag(EquipmentGroup)) {
                continue;
            }
            if (UMythicItemInstance *Item = Slot.SlottedItemInstance) {
                Single.Reset();
                Item->GetTypeProbe(Single);
                Probe.AppendTags(Single);
            }
        }
    }
    return Probe;
}
}

void UMythicResourceManagerComponent::AddOrUpdateResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController,
                                                          UMythicResourceISM *ResourceISM, int32 index) {
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource called on non-authority"));
        return;
    }

    if (DamageAmount <= 0) {
        UE_LOG(Myth, Warning, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource: DamageAmount is <= 0, ignoring"));
        return;
    }

    if (ResourceISM && !FMythicGatherRules::CanGather(GetEquippedToolProbe(PlayerController), ResourceISM->RequiredToolTag)) {
        UE_LOG(Myth, Verbose, TEXT("AddOrUpdateResource: gather refused — %s requires tool %s"),
               *GetNameSafe(ResourceISM), *ResourceISM->RequiredToolTag.ToString());
        return;
    }

#if ENABLE_DRAW_DEBUG
    static const auto CVarResourceDebugDraw = IConsoleManager::Get().RegisterConsoleVariable(
        TEXT("Mythic.Resources.DebugDraw"), 0, TEXT("Draw a marker on each resource hit (dev only)."), ECVF_Default);
    if (ResourceISM && CVarResourceDebugDraw->GetInt() > 0) {
        auto Trans = FTransform();
        ResourceISM->GetInstanceTransform(index, Trans, true);
        const auto start = Trans.GetLocation();
        const auto end = start + FVector3d(0, 0, 1000);
        DrawDebugLine(GetWorld(), start, end, FColor::Black, false, 20, 1, 5.0f);
    }
#endif

    int32 ScaledDamage = DamageAmount;
    if (ResourceISM && ResourceISM->ResourceType.IsValid()) {
        int32 ProfLevel = GetGathererProficiencyLevel(PlayerController, ResourceISM->ResourceType);
        if (ProfLevel > 0) {
            float Multiplier = 1.0f + static_cast<float>(ProfLevel) * GatheringConfig.BonusDamagePerLevel;
            ScaledDamage = FMath::Max(1, FMath::RoundToInt(static_cast<float>(DamageAmount) * Multiplier));
        }
    }

    FTrackedDestructibleData *ExistingResource = TrackedResources.FindByPredicate([&](const FTrackedDestructibleData &TrackedResource) {
        return TrackedResource.ResourceISM == ResourceISM && TrackedResource.InstanceId == ResourceISM->InstanceIndexToId(index).Id;
    });

    int32 HitsRemaining;
    if (ExistingResource) {
        HitsRemaining = ApplyDamageToResource(*ExistingResource, ScaledDamage, PlayerController);
    }
    else {
        HitsRemaining = AddNewResource(Transform, ScaledDamage, PlayerController, ResourceISM, index);
    }

    if (HitsRemaining >= 0) {
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PlayerController)) {
            if (HitsRemaining > 0) {
                MythicPC->ClientShowGatherProgress(Transform.GetLocation(), HitsRemaining);
            }
            else {
                MythicPC->ClientShowGatherDepleted(Transform.GetLocation());
            }
        }
    }
}

void UMythicResourceManagerComponent::LoadDestroyedResource(UMythicResourceISM *ResourceISM, int32 InstanceId, FTransform Transform, double RemainingSeconds) {
    if (!GetOwner()->HasAuthority()) {
        return;
    }

    if (!ResourceISM) {
        return;
    }

    FTrackedDestructibleData NewDestructible;
    NewDestructible.ResourceISM = ResourceISM;
    NewDestructible.InstanceId = InstanceId;
    NewDestructible.Transform = Transform;
    {
        const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        NewDestructible.RespawnTime = Now + FMath::Max(0.0, RemainingSeconds);
    }
    NewDestructible.HitsTillDestruction = 0;

    bool bExists = DestroyedResources.GetItems()->ContainsByPredicate([&](const FTrackedDestructibleData &Existing) {
        return Existing.ResourceISM == ResourceISM && Existing.InstanceId == InstanceId;
    });

    if (!bExists) {
        DestroyedResources.AddItem(NewDestructible);

        ResourceISM->DestroyResource(InstanceId);
    }
}

int32 UMythicResourceManagerComponent::ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount, APlayerController *PlayerController) {
    int32 PreviousHits = Resource.HitsTillDestruction;
    Resource.HitsTillDestruction = FMath::Max(0, Resource.HitsTillDestruction - DamageAmount);
    const int32 HitsRemaining = Resource.HitsTillDestruction;

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Applied %d damage, HitsTillDestruction: %d -> %d"),
           DamageAmount, PreviousHits, Resource.HitsTillDestruction);

    if (Resource.HitsTillDestruction <= 0 && PreviousHits > 0) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Resource destroyed!"));

        auto NumRemoved = TrackedResources.RemoveAll([&](const FTrackedDestructibleData &TrackedResource) {
            return TrackedResource == Resource;
        });

        if (NumRemoved <= 0) {
            UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Could not remove resource from tracked resources"));
            return HitsRemaining;
        }

        AddToDestroyedResources(Resource, PlayerController);
    }
    return HitsRemaining;
}

int32 UMythicResourceManagerComponent::AddNewResource(FTransform Transform, int32 DamageAmount,
                                                      APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32 Index) {
    FTrackedDestructibleData NewResource = FTrackedDestructibleData();
    NewResource.ResourceISM = ResourceISM;
    NewResource.Transform = Transform;


    auto ISMComponent = NewResource.ResourceISM;
    if (!ISMComponent) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Could not find ISM component for class %s"), *ISMComponent->GetName());
        return -1;
    }

    int32 MaxHealth = ISMComponent->CalculateHealthFromTransform(Transform);
    if (MaxHealth <= 0) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Calculated MaxHealth is <= 0 for ISM %s at location %s"),
               *ISMComponent->GetName(), *Transform.GetLocation().ToString());
        return -1;
    }

    NewResource.InstanceId = ISMComponent->InstanceIndexToId(Index).Id;
    NewResource.HitsTillDestruction = FMath::Max(0, MaxHealth - DamageAmount);

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: New resource with MaxHealth %d, taking %d damage, HitsTillDestruction: %d"),
           MaxHealth, DamageAmount, NewResource.HitsTillDestruction);

    auto AlreadyDestroyed = DestroyedResources.GetItems()->FindByPredicate([&NewResource](const FTrackedDestructibleData &DestroyedResource) {
        return DestroyedResource == NewResource;
    });
    if (AlreadyDestroyed) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Resource already destroyed, ignoring"));
        return -1;
    }

    if (NewResource.HitsTillDestruction <= 0) {
        AddToDestroyedResources(NewResource, PlayerController);
    }
    else {
        TrackedResources.Add(NewResource);
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Added to tracked resources"));
    }
    return NewResource.HitsTillDestruction;
}

void UMythicResourceManagerComponent::AddToDestroyedResources(FTrackedDestructibleData DestroyedResource, APlayerController *PlayerController) {
    const FVector NodeLocation = DestroyedResource.Transform.GetLocation();
    const int32 ResourceTier = DestroyedResource.ResourceISM ? DestroyedResource.ResourceISM->ResourceTier : 0;

    UMythicRegionalPressureSubsystem *Pressure =
        GetWorld() ? GetWorld()->GetSubsystem<UMythicRegionalPressureSubsystem>() : nullptr;
    if (Pressure) {
        Pressure->ServerRegisterHarvest(NodeLocation);
    }

    float RespawnDelay = FMythicGatherRules::ScaledRespawnDelay(DefaultRespawnDelay, ResourceTier);
    if (Pressure) {
        RespawnDelay = Pressure->ScaledHarvestRespawnDelay(NodeLocation, RespawnDelay, ResourceTier);
    }
    DestroyedResource.RespawnTime = GetWorld()->GetTimeSeconds() + RespawnDelay;

    DestroyedResources.AddItem(DestroyedResource);

    UE_LOG(Myth, Log,
           TEXT("UMythicResourceManagerComponent::AddToDestroyedResources: Resource %d added to destroyed resources, will respawn in %.1f seconds"),
           DestroyedResource.InstanceId, RespawnDelay);

    DestroyedResource.ResourceISM->OnKillRewards.Give(PlayerController, false, 0, NodeLocation);

    {
        const float RawYieldMult = Pressure ? Pressure->QueryHarvestYieldMultiplier(NodeLocation, ResourceTier)
                                            : FMythicGatherRules::TierYieldMultiplier(ResourceTier);
        const int32 YieldMult = FMath::Max(1, FMath::RoundToInt(RawYieldMult));
        for (int32 i = 1; i < YieldMult; ++i) {
            DestroyedResource.ResourceISM->OnKillRewards.Give(PlayerController, false, 0, NodeLocation);
        }
    }

    if (DestroyedResource.ResourceISM->ResourceType.IsValid()) {
        const FGameplayTag &ResourceType = DestroyedResource.ResourceISM->ResourceType;
        int32 ProfLevel = GetGathererProficiencyLevel(PlayerController, ResourceType);
        if (ProfLevel > 0) {
            float BonusChance = static_cast<float>(ProfLevel) * GatheringConfig.BonusYieldChancePerLevel;
            if (MythicCombat::RollSucceeds(BonusChance, FMath::FRand())) {
                DestroyedResource.ResourceISM->OnKillRewards.Give(PlayerController, false, 0, DestroyedResource.Transform.GetLocation());
                UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent: bonus yield triggered (level %d, chance %.2f)"), ProfLevel, BonusChance);
            }
        }
    }
}

void UMythicResourceManagerComponent::BeginPlay() {
    Super::BeginPlay();

    DestroyedResources.OwnerComponent = this;

    if (GetOwner()->HasAuthority()) {
        GetWorld()->GetTimerManager().SetTimer(
            BatchRespawnTimerHandle,
            this,
            &UMythicResourceManagerComponent::ProcessBatchRespawn,
            BatchRespawnInterval,
            true
            );

        UE_LOG(Myth, Log, TEXT("Batch respawn system started - checking every %.1f seconds"), BatchRespawnInterval);
    }
}

void UMythicResourceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicResourceManagerComponent, DestroyedResources);
}

TArray<FTrackedDestructibleData> UMythicResourceManagerComponent::GetTrackedDestructibles() const {
    return this->TrackedResources;
}

bool UMythicResourceManagerComponent::ShouldRespawnDestructible(int32 HitsTillDestruction, float RespawnTime, float CurrentTime) {
    return HitsTillDestruction <= 0 && RespawnTime > 0.0f && CurrentTime >= RespawnTime;
}

void UMythicResourceManagerComponent::HandleResourceDestruction(const TArray<FTrackedDestructibleData> &DestroyedResources) {
    UE_LOG(Myth, Log, TEXT("HandleResourceDestruction: Syncing destruction of %d resources"), DestroyedResources.Num());

    TSet<UMythicResourceISM *> ISMsToDirty;

    auto length = DestroyedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = DestroyedResources[i];

        auto ResourceComponent = Resource.ResourceISM;
        if (!ResourceComponent) {
            UE_LOG(Myth, Error, TEXT("HandleResourceDestruction: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Myth, Log, TEXT("HandleResourceDestruction: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        ResourceComponent->DestroyResource(Resource.InstanceId);

        ISMsToDirty.Add(ResourceComponent);
    }

    for (UMythicResourceISM *ISM : ISMsToDirty) {
        if (ISM) {
            ISM->MarkRenderStateDirty();
        }
    }
}

void UMythicResourceManagerComponent::HandleResourceRespawn(const TArray<FTrackedDestructibleData> &RespawnedResources) {
    UE_LOG(Myth, Log, TEXT("HandleResourceRespawn: Syncing respawn of %d resources"), RespawnedResources.Num());

    auto length = RespawnedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = RespawnedResources[i];

        auto ResourceComponent = Resource.ResourceISM;
        if (!ResourceComponent) {
            UE_LOG(Myth, Error, TEXT("HandleResourceRespawn: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Myth, Log, TEXT("HandleResourceRespawn: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        bool ShouldUpdateRender = i == length - 1;
        ResourceComponent->RestoreResource(Resource.InstanceId, Resource.Transform, ShouldUpdateRender);
    }
}

int32 UMythicResourceManagerComponent::GetGathererProficiencyLevel(APlayerController *PlayerController, const FGameplayTag &ResourceType) const {
    if (!PlayerController || !ResourceType.IsValid()) {
        return 0;
    }

    const TObjectPtr<UProficiencyDefinition> *FoundDef = GatheringConfig.ResourceToProficiency.Find(ResourceType);
    if (!FoundDef || !*FoundDef) {
        return 0;
    }
    UProficiencyDefinition *ProfDef = *FoundDef;

    UProficiencyComponent *ProfComp = nullptr;
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PlayerController)) {
        ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent());
    }
    if (!ProfComp) {
        return 0;
    }

    for (const FProficiency &Prof : ProfComp->Proficiencies) {
        if (Prof.Definition == ProfDef) {
            UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerController);
            if (!ASC) {
                return 0;
            }
            float CurrentXP = ASC->GetNumericAttribute(ProfDef->ProgressAttribute);
            return UProficiencyDefinition::CalcLevelAtXP(CurrentXP, ProfDef);
        }
    }

    return 0;
}
