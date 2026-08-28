#include "MythicGameState.h"
#include "Mythic.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"

AMythicGameState::AMythicGameState(const FObjectInitializer &ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    bReplicateUsingRegisteredSubObjectList = true;

    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UMythicAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

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

void AMythicGameState::AddPlayerState(APlayerState *PlayerState) {
    Super::AddPlayerState(PlayerState);
    if (HasAuthority()) {
        if (UGameInstance *GI = GetGameInstance()) {
            if (UMythicNPCManager *Mgr = GI->GetSubsystem<UMythicNPCManager>()) {
                Mgr->RefreshCombatScalingOnActive();
            }
        }
    }
}

void AMythicGameState::RemovePlayerState(APlayerState *PlayerState) {
    Super::RemovePlayerState(PlayerState);
    if (HasAuthority()) {
        if (UGameInstance *GI = GetGameInstance()) {
            if (UMythicNPCManager *Mgr = GI->GetSubsystem<UMythicNPCManager>()) {
                Mgr->RefreshCombatScalingOnActive();
            }
        }
    }
}

void AMythicGameState::BeginPlay() {
    Super::BeginPlay();

    if (UWorld *World = GetWorld()) {
        if (UMythicHarvestWorldSubsystem *HarvestWorld =
                World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
            HarvestWorld->RegisterPresentationCoordinator(*this);
        }
    }

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
        UMythicSaveGameSubsystem *SaveSys = GI
            ? GI->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr;
        FString AuthorityWorldSlot;
        if (SaveSys
            && UMythicSaveGameSubsystem::TryResolveAuthorityWorldSlot(
                AuthorityWorldSlot)) {
            if (UGameplayStatics::DoesSaveGameExist(
                    AuthorityWorldSlot, 0)) {
                SaveSys->LoadWorld(AuthorityWorldSlot);
            }
            else {
                UE_LOG(Myth, Display,
                       TEXT("MythicGameState: authority world slot '%s' has no snapshot; starting a new world."),
                       *AuthorityWorldSlot);
            }
        }
        else if (World && World->GetNetMode() == NM_DedicatedServer) {
#if UE_BUILD_SHIPPING
            UE_LOG(Myth, Fatal,
                   TEXT("Shipping dedicated server requires -MythicWorldSlot=<deployment-instance-id>; DebugWorld is forbidden."));
#else
            UE_LOG(Myth, Warning,
                   TEXT("Dedicated server has no -MythicWorldSlot; world persistence and autosave are disabled for this development run."));
#endif
        }
    }
}

UAbilitySystemComponent *AMythicGameState::GetAbilitySystemComponent() const {
    return this->AbilitySystemComponent;
}

void AMythicGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicGameState, AbilitySystemComponent);
    DOREPLIFETIME(AMythicGameState, HighestWorldTier);
    DOREPLIFETIME(AMythicGameState, HarvestPresentationStreamToken);
}

void AMythicGameState::OnRep_HarvestPresentationStreamToken() {
    if (UWorld *World = GetWorld()) {
        if (UMythicHarvestWorldSubsystem *HarvestWorld =
                World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
            HarvestWorld->RegisterPresentationCoordinator(*this);
        }
    }
}

bool AMythicGameState::CanSetHarvestPresentationStreamToken(
    const FMythicHarvestPresentationStreamToken &Token) const {
    if (!HasAuthority() || !Token.IsValid()) {
        return false;
    }
    if (!HarvestPresentationStreamToken.IsValid()) {
        return true;
    }
    const EMythicHarvestPresentationStreamOrder Order =
        FMythicHarvestPresentationStreamToken::Compare(
            Token, HarvestPresentationStreamToken);
    return Order == EMythicHarvestPresentationStreamOrder::Same
        || Order == EMythicHarvestPresentationStreamOrder::Newer;
}

bool AMythicGameState::SetHarvestPresentationStreamToken(
    const FMythicHarvestPresentationStreamToken &Token) {
    if (!CanSetHarvestPresentationStreamToken(Token)) {
        return false;
    }
    if (HarvestPresentationStreamToken == Token) {
        return true;
    }
    HarvestPresentationStreamToken = Token;
    ForceNetUpdate();
    return true;
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
