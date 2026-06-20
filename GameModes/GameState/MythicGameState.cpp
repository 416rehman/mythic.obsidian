// 
#include "MythicGameState.h"
#include "Mythic.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "Net/UnrealNetwork.h"
#include "Resources/MythicResourceManagerComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"

AMythicGameState::AMythicGameState(const FObjectInitializer &ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Required for UMythicReplicatedObject
    bReplicateUsingRegisteredSubObjectList = true;

    // Ability System Component
    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UMythicAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // Resource Manager Component
    ResourceManagerComponent = CreateDefaultSubobject<UMythicResourceManagerComponent>(TEXT("ResourceManagerComponent"));
    ResourceManagerComponent->SetIsReplicated(true);

    // Initialize the World Tier Attributes
    WorldTierAttributes = CreateDefaultSubobject<UWorldTierAttributes>(TEXT("WorldTierAttributes"));
}

void AMythicGameState::PostInitializeComponents() {
    Super::PostInitializeComponents();

    check(AbilitySystemComponent);
    AbilitySystemComponent->InitAbilityActorInfo(/*Owner=*/ this, /*Avatar=*/ nullptr);
}

void AMythicGameState::OnRep_ReplicatedWorldTimeSecondsDouble() {
    Super::OnRep_ReplicatedWorldTimeSecondsDouble();

    if (this->IsSessionJoinTimeInitialized) {
        return;
    }

    // Clients only - Seed the World Time from the Server
    // GameState can either be a SimulatedProxy or Authority
    if (GetLocalRole() == ROLE_SimulatedProxy) {
        this->GetWorld()->TimeSeconds = ReplicatedWorldTimeSecondsDouble;
    }

    // Mark the SessionJoinTime as initialized
    IsSessionJoinTimeInitialized = true;
}

void AMythicGameState::SetWorldTier(uint8 NewWorldTier) {
    // Clamp to 1 - WorldTierAttributesCurve Row Handle max keys
    NewWorldTier = FMath::Clamp(NewWorldTier, 1, MaxWorldTier);

    // Remove the old World Tier Attributes
    if (ActiveWorldTierInitEffectHandle.IsValid()) {
        AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveWorldTierInitEffectHandle);
    }

    // Set the new World Tier
    WorldTier = NewWorldTier;

    // Apply the effect to initialize the World Tier Attributes
    FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
    EffectContext.AddSourceObject(this);
    FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(WorldTierAttributesInitializationEffect, NewWorldTier, EffectContext);
    if (SpecHandle.IsValid()) {
        ActiveWorldTierInitEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        UE_LOG(Myth, Log, TEXT("Initialized World Tier Attributes for World Tier %d"), NewWorldTier);
    }
    else {
        UE_LOG(Myth, Error, TEXT("Failed to create SpecHandle for WorldTierAttributesInitializationEffect in Game State"));
    }
}

void AMythicGameState::BeginPlay() {
    Super::BeginPlay();

    SetWorldTier(WorldTier);

    // Fail loud (once, at startup) if combat tuning data is missing - mitigation fails open (0 reduction), so a
    // silent miss would just make Armor do nothing with no indication why.
    if (ArmorMitigationCurveRowHandle.IsNull()) {
        UE_LOG(Myth, Error, TEXT("MythicGameState: ArmorMitigationCurveRowHandle is unset - Armor will provide no mitigation."));
    }

    // SERVER: restore saved world state. GameState::BeginPlay is the verified-safe "world ready" point -
    // it runs on authority after the level's saveable actors have begun (later than GameMode::BeginPlay), and
    // the world-load handler resolves this same GameState. LoadWorld self-guards via DoesSaveGameExist, so a
    // fresh world is a no-op. The DebugWorld slot matches the autosave write path + cheat default
    // (single source: UMythicSaveGameSubsystem::DebugWorldSlot).
    if (HasAuthority()) {
        UWorld *World = GetWorld();
        UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
        if (UMythicSaveGameSubsystem *SaveSys = GI ? GI->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr) {
            SaveSys->LoadWorld(UMythicSaveGameSubsystem::DebugWorldSlot);
        }
    }
}

UAbilitySystemComponent *AMythicGameState::GetAbilitySystemComponent() const {
    return this->AbilitySystemComponent;
}

void AMythicGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicGameState, AbilitySystemComponent);
    DOREPLIFETIME(AMythicGameState, ResourceManagerComponent);
}

TArray<FTrackedDestructibleData> AMythicGameState::GetTrackedDestructibles() const {
    return this->ResourceManagerComponent->GetTrackedDestructibles();
}
