#include "MythicGameState.h"
#include "Mythic.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "Net/UnrealNetwork.h"
#include "Resources/MythicResourceManagerComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"

AMythicGameState::AMythicGameState(const FObjectInitializer &ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    bReplicateUsingRegisteredSubObjectList = true;

    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UMythicAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    ResourceManagerComponent = CreateDefaultSubobject<UMythicResourceManagerComponent>(TEXT("ResourceManagerComponent"));
    ResourceManagerComponent->SetIsReplicated(true);

    WorldTierAttributes = CreateDefaultSubobject<UWorldTierAttributes>(TEXT("WorldTierAttributes"));
}

void AMythicGameState::PostInitializeComponents() {
    Super::PostInitializeComponents();

    check(AbilitySystemComponent);
    AbilitySystemComponent->InitAbilityActorInfo( this, nullptr);
}

void AMythicGameState::OnRep_ReplicatedWorldTimeSecondsDouble() {
    Super::OnRep_ReplicatedWorldTimeSecondsDouble();

    if (this->IsSessionJoinTimeInitialized) {
        return;
    }

    if (GetLocalRole() == ROLE_SimulatedProxy) {
        this->GetWorld()->TimeSeconds = ReplicatedWorldTimeSecondsDouble;
    }

    IsSessionJoinTimeInitialized = true;
}

void AMythicGameState::SetWorldTier(uint8 NewWorldTier) {
    NewWorldTier = FMath::Clamp(NewWorldTier, 1, MaxWorldTier);

    if (ActiveWorldTierInitEffectHandle.IsValid()) {
        AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveWorldTierInitEffectHandle);
    }

    WorldTier = NewWorldTier;

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

void AMythicGameState::AdvanceWorldTier() {
    if (!HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("AdvanceWorldTier ignored on non-authority GameState"));
        return;
    }

    const uint8 NewTier = ComputeAdvancedWorldTier(WorldTier, MaxWorldTier);
    HighestWorldTier = ComputeHighestTier(HighestWorldTier, NewTier);

    SetWorldTier(NewTier);

    UE_LOG(Myth, Log, TEXT("AdvanceWorldTier: WorldTier now %d (highest reached %d)"), WorldTier, HighestWorldTier);
}

void AMythicGameState::BeginPlay() {
    Super::BeginPlay();

    SetWorldTier(WorldTier);

    if (HasAuthority()) {
        HighestWorldTier = ComputeHighestTier(HighestWorldTier, WorldTier);
    }

    if (ArmorMitigationCurveRowHandle.IsNull()) {
        UE_LOG(Myth, Error, TEXT("MythicGameState: ArmorMitigationCurveRowHandle is unset - Armor will provide no mitigation."));
    }

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
    DOREPLIFETIME(AMythicGameState, HighestWorldTier);
}

TArray<FTrackedDestructibleData> AMythicGameState::GetTrackedDestructibles() const {
    return this->ResourceManagerComponent->GetTrackedDestructibles();
}

float AMythicGameState::EvaluateArmorMitigation(const UObject *WorldContextObject, float Armor) {
    const UWorld *World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    const AMythicGameState *GS = World ? World->GetGameState<AMythicGameState>() : nullptr;
    if (!GS) {
        return 0.0f;
    }
    if (const FRealCurve *Curve = GS->ArmorMitigationCurveRowHandle.GetCurve(TEXT("EvaluateArmorMitigation"))) {
        return FMath::Clamp(Curve->Eval(Armor), 0.0f, 0.85f);
    }
    return 0.0f;
}
