// 


#include "MythicResourceISM.h"
#include "Mythic.h"
#include "Misc/MessageDialog.h"

FRewardsToGive UMythicResourceISM::GetOnKillRewards(AActor *Killer) {
    return this->OnKillRewards;
}

void UMythicResourceISM::BeginPlay() {
    Super::BeginPlay();

    auto IsStablyNamed = this->IsNameStableForNetworking();
    auto IsStablyNamedFull = this->IsFullNameStableForNetworking();
    UE_LOG(Myth, Log, TEXT("UMythicResourceISM::BeginPlay: IsNameStableForNetworking=%d, IsFullNameStableForNetworking=%d"),
           IsStablyNamed, IsStablyNamedFull);

    auto Owner = GetOwner();
    auto OwnerName = *Owner->GetName();
    auto OwnerIsStablyNamed = Owner->IsNameStableForNetworking();
    auto OwnerIsStablyNamedFull = Owner->IsFullNameStableForNetworking();

    UE_LOG(Myth, Log, TEXT("UMythicResourceISM::BeginPlay: Owner=%s, OwnerIsNameStableForNetworking=%d, OwnerIsFullNameStableForNetworking=%d"),
           OwnerName, OwnerIsStablyNamed, OwnerIsStablyNamedFull);

    // If the tag is not set, log a warning
    if (!HealthConfig.HealthPerZUnit || HealthConfig.MinHealth < 0 || HealthConfig.MaxHealth < 0) {
        UE_LOG(Myth, Error,
               TEXT(
                   "UMythicResourceISM::BeginPlay: HealthConfig is not set properly on %s. Please set HealthPerZUnit, MinHealth, and MaxHealth."
               ),
               *GetName());
    }

    // Show an in-editor error dialogue if resourcetype is not set.
    if (!ResourceType.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceISM::BeginPlay: ResourceType is not set on %s. Please set a valid GameplayTag."),
               *GetName());

        // In editor, show a message box
#if WITH_EDITOR

        FMessageDialog::Open(EAppMsgType::Ok,
                             FText::FromString(FString::Printf(TEXT("ResourceType is not set on %s. Please set a valid GameplayTag."), *GetName())));
#endif

    }
}


// Updated MythicResourceISM.cpp implementation
void UMythicResourceISM::DestroyResource(int32 InstanceId) {
    UE_LOG(Myth, Log, TEXT("DestroyResource: InstanceId=%d, Component=%s, Owner=%s"),
           InstanceId, *GetName(), *GetOwner()->GetName());

    // Convert InstanceId to InstanceIndex
    int32 InstanceIndex = GetInstanceIndexForId(FPrimitiveInstanceId(InstanceId));
    if (InstanceIndex < 0) {
        UE_LOG(Myth, Warning, TEXT("DestroyResource: Invalid InstanceId %d"), InstanceId);
        return;
    }

    // Check if already destroyed
    if (IsInstanceDestroyed(InstanceIndex)) {
        UE_LOG(Myth, Warning, TEXT("DestroyResource: Instance %d (InstanceId=%d) is already destroyed"),
               InstanceIndex, InstanceId);
        return;
    }

    // Get current transform
    FTransform CurrentTransform;
    GetInstanceTransform(InstanceIndex, CurrentTransform, true);

    // (Removed a persistent DrawDebugSphere here: bPersistentLines=true leaked debug primitives on every
    //  resource destruction in shipping multiplayer.)

    // Move it under landscape
    FTransform HiddenTransform = CurrentTransform;
    HiddenTransform.AddToTranslation(FVector(0, 0, -999999));
    UpdateInstanceTransform(InstanceIndex, HiddenTransform, true);

    // Mark as destroyed in our tracking
    DestroyedInstances.Add(InstanceIndex);

    UE_LOG(Myth, Log, TEXT("DestroyResource: Successfully destroyed InstanceIndex %d. Total destroyed: %d"),
           InstanceIndex, DestroyedInstances.Num());
}

void UMythicResourceISM::RestoreResource(int32 InstanceId, FTransform OriginalTransform, bool MarkRenderStateDirty) {
    // Convert InstanceId -> InstanceIndex. The caller passes the network-stable Id (FTrackedDestructibleData.InstanceId);
    // DestroyResource converts the same way, but RestoreResource previously used the Id directly as an index — once
    // Id != index (after any ISM instance add/remove) that un-hid the WRONG instance AND left the real destroyed INDEX
    // leaked in DestroyedInstances forever (so it could never be re-destroyed).
    int32 InstanceIndex = GetInstanceIndexForId(FPrimitiveInstanceId(InstanceId));
    if (InstanceIndex < 0) {
        UE_LOG(Myth, Warning, TEXT("RestoreResource: Invalid InstanceId %d"), InstanceId);
        return;
    }

    // Restore the transform
    UpdateInstanceTransform(InstanceIndex, OriginalTransform, true, MarkRenderStateDirty);

    // Remove from destroyed tracking
    if (DestroyedInstances.Remove(InstanceIndex)) {
        UE_LOG(Myth, Log, TEXT("RestoreResource: Restored InstanceIndex %d (InstanceId=%d). Total destroyed: %d"),
               InstanceIndex, InstanceId, DestroyedInstances.Num());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("RestoreResource: InstanceIndex %d (InstanceId=%d) was not in destroyed tracking"), InstanceIndex, InstanceId);
    }
}

// Helper function to get max health based on transform Z value and destructible type
int32 UMythicResourceISM::CalculateHealthFromTransform(const FTransform &Transform) const {
    // Calculate health based on Z scale/height
    float ZValue = Transform.GetScale3D().Z; // or Transform.GetLocation().Z if using world position

    // Convert Z value to health using type-specific multiplier and minimum
    int32 CalculatedHealth = FMath::Max(HealthConfig.MinHealth, FMath::RoundToInt(ZValue * HealthConfig.HealthPerZUnit));

    // Cap at type-specific maximum health
    if (HealthConfig.MaxHealth > 0) {
        CalculatedHealth = FMath::Min(CalculatedHealth, HealthConfig.MaxHealth);
        UE_LOG(Myth, Log, TEXT("Capping health to MaxHealth=%d"), HealthConfig.MaxHealth);
    }

    UE_LOG(Myth, Log, TEXT("UMythicDestructiblesManagerComponent::CalculateHealthFromTransform: Z=%.2f, HealthPerZ=%.2f, calculated health=%d"), ZValue,
           HealthConfig.HealthPerZUnit, CalculatedHealth);

    return CalculatedHealth;
}
