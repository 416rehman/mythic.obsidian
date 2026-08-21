

#include "MythicResourceISM.h"
#include "Mythic.h"

FRewardsToGive UMythicResourceISM::GetOnKillRewards(AActor *Killer) {
    return this->OnKillRewards;
}

void UMythicResourceISM::BeginPlay() {
    Super::BeginPlay();

    if (!UE_LOG_ACTIVE(Myth, Verbose)) {
    }
    else {
        const AActor *Owner = GetOwner();
        UE_LOG(Myth, Verbose,
               TEXT("UMythicResourceISM::BeginPlay: stable=%d, fullStable=%d, owner='%s', ownerStable=%d, ownerFullStable=%d"),
               IsNameStableForNetworking(), IsFullNameStableForNetworking(),
               Owner ? *Owner->GetName() : TEXT("<none>"),
               Owner ? Owner->IsNameStableForNetworking() : false,
               Owner ? Owner->IsFullNameStableForNetworking() : false);
    }

    if (!HealthConfig.HealthPerZUnit || HealthConfig.MinHealth < 0 || HealthConfig.MaxHealth < 0) {
        UE_LOG(Myth, Error,
               TEXT(
                   "UMythicResourceISM::BeginPlay: HealthConfig is not set properly on %s. Please set HealthPerZUnit, MinHealth, and MaxHealth."
               ),
               *GetName());
    }

    if (!ResourceType.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceISM::BeginPlay: ResourceType is not set on %s. Please set a valid GameplayTag."),
               *GetName());
    }
}


void UMythicResourceISM::DestroyResource(int32 InstanceId) {
    UE_LOG(Myth, Log, TEXT("DestroyResource: InstanceId=%d, Component=%s, Owner=%s"),
           InstanceId, *GetName(), *GetOwner()->GetName());

    int32 InstanceIndex = GetInstanceIndexForId(FPrimitiveInstanceId(InstanceId));
    if (InstanceIndex < 0) {
        UE_LOG(Myth, Warning, TEXT("DestroyResource: Invalid InstanceId %d"), InstanceId);
        return;
    }

    if (IsInstanceDestroyed(InstanceIndex)) {
        UE_LOG(Myth, Warning, TEXT("DestroyResource: Instance %d (InstanceId=%d) is already destroyed"),
               InstanceIndex, InstanceId);
        return;
    }

    FTransform CurrentTransform;
    GetInstanceTransform(InstanceIndex, CurrentTransform, true);


    FTransform HiddenTransform = CurrentTransform;
    HiddenTransform.AddToTranslation(FVector(0, 0, -999999));
    UpdateInstanceTransform(InstanceIndex, HiddenTransform, true);

    DestroyedInstances.Add(InstanceIndex);

    UE_LOG(Myth, Log, TEXT("DestroyResource: Successfully destroyed InstanceIndex %d. Total destroyed: %d"),
           InstanceIndex, DestroyedInstances.Num());
}

void UMythicResourceISM::RestoreResource(int32 InstanceId, FTransform OriginalTransform, bool MarkRenderStateDirty) {
    int32 InstanceIndex = GetInstanceIndexForId(FPrimitiveInstanceId(InstanceId));
    if (InstanceIndex < 0) {
        UE_LOG(Myth, Warning, TEXT("RestoreResource: Invalid InstanceId %d"), InstanceId);
        return;
    }

    UpdateInstanceTransform(InstanceIndex, OriginalTransform, true, MarkRenderStateDirty);

    if (DestroyedInstances.Remove(InstanceIndex)) {
        UE_LOG(Myth, Log, TEXT("RestoreResource: Restored InstanceIndex %d (InstanceId=%d). Total destroyed: %d"),
               InstanceIndex, InstanceId, DestroyedInstances.Num());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("RestoreResource: InstanceIndex %d (InstanceId=%d) was not in destroyed tracking"), InstanceIndex, InstanceId);
    }
}

int32 UMythicResourceISM::CalculateHealthFromTransform(const FTransform &Transform) const {
    float ZValue = Transform.GetScale3D().Z;

    int32 CalculatedHealth = FMath::Max(HealthConfig.MinHealth, FMath::RoundToInt(ZValue * HealthConfig.HealthPerZUnit));

    if (HealthConfig.MaxHealth > 0) {
        CalculatedHealth = FMath::Min(CalculatedHealth, HealthConfig.MaxHealth);
        UE_LOG(Myth, Log, TEXT("Capping health to MaxHealth=%d"), HealthConfig.MaxHealth);
    }

    UE_LOG(Myth, Log, TEXT("UMythicDestructiblesManagerComponent::CalculateHealthFromTransform: Z=%.2f, HealthPerZ=%.2f, calculated health=%d"), ZValue,
           HealthConfig.HealthPerZUnit, CalculatedHealth);

    return CalculatedHealth;
}
