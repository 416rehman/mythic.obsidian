#include "MythicPlayerState.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "MythicFactionStandingComponent.h"
#include "MythicPlayerRegistrySubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AMythicPlayerState::AMythicPlayerState() {
    MythicAbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("MythicAbilitySystemComponent"));
    MythicAbilitySystemComponent->SetIsReplicated(true);
    MythicAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    UE_LOG(LogTemp, Warning, TEXT("MythicAbilitySystemComponent %s created"), *MythicAbilitySystemComponent->GetName());

    // Increased NetUpdateFrequency because PlayerState's default is very slow
    SetNetUpdateFrequency(30);

    // Attribute Sets
    LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
    OffenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Offense>(TEXT("OffenseAttributes"));
    DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>(TEXT("DefenseAttributes"));
    UtilityAttributes = CreateDefaultSubobject<UMythicAttributeSet_Utility>(TEXT("UtilityAttributes"));
    ProficiencyAttributes = CreateDefaultSubobject<UMythicAttributeSet_Proficiencies>(TEXT("ProficiencyAttributes"));

    // Per-player faction standing (replicated component).
    FactionStanding = CreateDefaultSubobject<UMythicFactionStandingComponent>(TEXT("FactionStanding"));
}

UAbilitySystemComponent *AMythicPlayerState::GetAbilitySystemComponent() const {
    return MythicAbilitySystemComponent;
}

UMythicAbilitySystemComponent *AMythicPlayerState::GetMythicAbilitySystemComponent() const {
    return MythicAbilitySystemComponent;
}

void AMythicPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicPlayerState, MythicAbilitySystemComponent);
    DOREPLIFETIME(AMythicPlayerState, LifeAttributes);
    DOREPLIFETIME(AMythicPlayerState, OffenseAttributes);
    DOREPLIFETIME(AMythicPlayerState, DefenseAttributes);
    DOREPLIFETIME(AMythicPlayerState, UtilityAttributes);
    DOREPLIFETIME(AMythicPlayerState, ProficiencyAttributes);
    DOREPLIFETIME(AMythicPlayerState, PersistentCharacterId);
}

void AMythicPlayerState::SetPersistentCharacterId(const FString &InCharacterId) {
    if (!HasAuthority()) {
        return; // server-authoritative identity; clients receive it via replication
    }
    PersistentCharacterId = InCharacterId;
    // The canonical key just changed (session fallback -> persistent CharacterID); re-register under the new key.
    if (UWorld *World = GetWorld()) {
        if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
            AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(GetPlayerController());
            Registry->RegisterPlayer(GetCanonicalPlayerKey(), this, MythicPC);
        }
    }
}

FString AMythicPlayerState::ResolveCanonicalPlayerKey(const FString &PersistentId, int32 SessionPlayerId) {
    // Persistent id wins when present (stable across save/load + reconnect); otherwise a session-stable fallback so a
    // not-yet-loaded / transient player still has a usable, unique-within-session key.
    return PersistentId.IsEmpty() ? FString::Printf(TEXT("session:%d"), SessionPlayerId) : PersistentId;
}

FString AMythicPlayerState::GetCanonicalPlayerKey() const {
    return ResolveCanonicalPlayerKey(PersistentCharacterId, GetPlayerId());
}


// --- Save System Interface Removed ---

void AMythicPlayerState::BeginPlay() {
    Super::BeginPlay();

    // If server, initialize default abilities and effects
    if (GetLocalRole() == ROLE_Authority) {
        // Register in the player registry under the current canonical key (session fallback until a character loads,
        // then re-registered under the persistent CharacterID by SetPersistentCharacterId).
        if (UWorld *World = GetWorld()) {
            if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
                AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(GetPlayerController());
                Registry->RegisterPlayer(GetCanonicalPlayerKey(), this, MythicPC);
            }
        }

        for (TSubclassOf<UGameplayEffect> Effect : DefaultGameplayEffects) {
            if (Effect) {
                FGameplayEffectContextHandle EffectContext = this->MythicAbilitySystemComponent->MakeEffectContext();
                FGameplayEffectSpecHandle SpecHandle = this->MythicAbilitySystemComponent->MakeOutgoingSpec(Effect, 1, EffectContext);
                if (this->MythicAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()).IsValid()) {
                    UE_LOG(LogTemp, Warning, TEXT("Applied default gameplay effect %s to %s"), *Effect->GetName(), *GetOwner()->GetName());
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to apply default gameplay effect %s to %s"), *Effect->GetName(), *GetOwner()->GetName());
                }
            }
            else {
                UE_LOG(LogTemp, Warning, TEXT("Default gameplay effect is null"));
            }
        }

        for (TSubclassOf<UMythicGameplayAbility> Ability : DefaultAbilities) {
            if (Ability) {
                // Idempotent grant: GiveAbility does NOT dedupe by class — it appends a fresh spec (and input binding)
                // every call — so if this grant ever re-runs on the same live ASC (a re-init path), it would stack
                // duplicate default abilities. Skip if already present, mirroring UAbilityReward::Give's guard.
                if (this->MythicAbilitySystemComponent->FindAbilitySpecFromClass(Ability)) {
                    UE_LOG(LogTemp, Verbose, TEXT("Default ability %s already granted to %s; skipping duplicate"), *Ability->GetName(),
                           *GetOwner()->GetName());
                    continue;
                }
                if (this->MythicAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability.GetDefaultObject(), 1, INDEX_NONE, this)).IsValid()) {
                    UE_LOG(LogTemp, Warning, TEXT("Gave default ability %s to %s"), *Ability->GetName(), *GetOwner()->GetName());
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("Failed to give default ability %s to %s"), *Ability->GetName(), *GetOwner()->GetName());
                }
            }
            else {
                UE_LOG(LogTemp, Warning, TEXT("Default ability is null"));
            }
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("Not server, not initializing default abilities and effects"));
    }
}

void AMythicPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    // Drop this player from the registry so a departed player's key never resolves to a stale entry.
    if (HasAuthority()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
                Registry->UnregisterObject(this);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

// ============================================================================
// IInventoryProviderInterface - Delegates to PlayerController
// ============================================================================

TArray<UMythicInventoryComponent *> AMythicPlayerState::GetAllInventoryComponents() const {
    if (APlayerController *PC = GetPlayerController()) {
        if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            return InvProvider->GetAllInventoryComponents();
        }
    }
    return TArray<UMythicInventoryComponent *>();
}

UAbilitySystemComponent *AMythicPlayerState::GetSchematicsASC() const {
    if (APlayerController *PC = GetPlayerController()) {
        if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            return InvProvider->GetSchematicsASC();
        }
    }
    return nullptr;
}

UMythicInventoryComponent *AMythicPlayerState::GetInventoryForItemType(const FGameplayTag &ItemType) const {
    if (APlayerController *PC = GetPlayerController()) {
        if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            return InvProvider->GetInventoryForItemType(ItemType);
        }
    }
    return nullptr;
}
