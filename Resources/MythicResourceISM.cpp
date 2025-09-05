// 


#include "MythicResourceISM.h"
#include "Mythic.h"

FRewardsToGive UMythicResourceISM::GetOnKillRewards(AActor *Killer) {
    return this->OnKillRewards;
}

void UMythicResourceISM::BeginPlay() {
    Super::BeginPlay();

    auto IsStablyNamed = this->IsNameStableForNetworking();
    auto IsStablyNamedFull = this->IsFullNameStableForNetworking();
    UE_LOG(Mythic, Log, TEXT("UMythicResourceISM::BeginPlay: IsNameStableForNetworking=%d, IsFullNameStableForNetworking=%d"),
           IsStablyNamed, IsStablyNamedFull);

    auto Owner = GetOwner();
    auto OwnerName = *Owner->GetName();
    auto OwnerIsStablyNamed = Owner->IsNameStableForNetworking();
    auto OwnerIsStablyNamedFull = Owner->IsFullNameStableForNetworking();

    UE_LOG(Mythic, Log, TEXT("UMythicResourceISM::BeginPlay: Owner=%s, OwnerIsNameStableForNetworking=%d, OwnerIsFullNameStableForNetworking=%d"),
           OwnerName, OwnerIsStablyNamed, OwnerIsStablyNamedFull);

    // If the tag is not set, log a warning
    if (!HealthConfig.HealthPerZUnit || HealthConfig.MinHealth < 0 || HealthConfig.MaxHealth < 0) {
        UE_LOG(Mythic, Error,
               TEXT(
                   "UMythicResourceISM::BeginPlay: HealthConfig is not set properly on %s. Please set HealthPerZUnit, MinHealth, and MaxHealth."
               ),
               *GetName());
    }

    // Show an in-editor error dialogue if resourcetype is not set.
    if (!ResourceType.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceISM::BeginPlay: ResourceType is not set on %s. Please set a valid GameplayTag."),
               *GetName());

        // In editor, show a message box
#if WITH_EDITOR

        FMessageDialog::Open(EAppMsgType::Ok,
                             FText::FromString(FString::Printf(TEXT("ResourceType is not set on %s. Please set a valid GameplayTag."), *GetName())));
#endif

    }
}

void UMythicResourceISM::DestroyResource(int32 InstanceId, FTransform Transform, bool UpdateRender) {
    UE_LOG(Mythic, Log, TEXT("MarkResourceAsDestroyed: InstanceId=%d, Component=%s, Owner=%s"), InstanceId, *GetName(), *GetOwner()->GetName());

    // Convert InstanceId to InstanceIndex
    int32 InstanceIndex = GetInstanceIndexForId(FPrimitiveInstanceId(InstanceId));
    if (InstanceIndex < 0) {
        UE_LOG(Mythic, Warning, TEXT("DestroyResource: Invalid InstanceId %d"), InstanceId);
        return;
    }

    // Draw Debug sphere at this instance index location
    auto World = this->GetWorld();
    GetInstanceTransform(InstanceIndex, Transform, true);
    DrawDebugSphere(World, Transform.GetTranslation(), 4, 12, FColor::Red, true);


    // Move it +Z 100 units
    FTransform HiddenTransform = Transform;
    HiddenTransform.AddToTranslation(FVector(0, 0, 100));
    UpdateInstanceTransform(InstanceIndex, HiddenTransform, true, true);
}

// Function to restore hidden instances - useful for respawning
void UMythicResourceISM::RestoreResource(int32 InstanceIndex, FTransform OriginalTransform, bool ShouldUpdateRender) {
    UpdateInstanceTransform(InstanceIndex, OriginalTransform, true, ShouldUpdateRender);
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
        UE_LOG(Mythic, Log, TEXT("Capping health to MaxHealth=%d"), HealthConfig.MaxHealth);
    }

    UE_LOG(Mythic, Log, TEXT("UMythicDestructiblesManagerComponent::CalculateHealthFromTransform: Z=%.2f, HealthPerZ=%.2f, calculated health=%d"), ZValue,
           HealthConfig.HealthPerZUnit, CalculatedHealth);

    return CalculatedHealth;
}
