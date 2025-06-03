// 


#include "MythicNPCCharacter.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/NPC/MythicAttributeSet_NPCCombat.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"


const FMythicNPCData AMythicNPCCharacter::GetNPCData() const {
    return this->NPCData;
}

void AMythicNPCCharacter::OnSpawnedFromPool(const struct FMythicNPCData &InNPCData) {
    // OnSpawnedFromPool is called when the NPC is spawned from the pool and is ready to be used, any initialization should happen here.
    // Authority only
    if (!this->HasAuthority()) {
        return;
    }
    this->NPCData = InNPCData;
    this->InitializeASC();
}

void AMythicNPCCharacter::OnReturnedToPool() {
    // TODO - Cleanup should happen here so that the NPC can be reused later.
}

const FGuid &AMythicNPCCharacter::GetNPCId() const {
    return this->NPCData.NPCId;
}

const FGameplayTag &AMythicNPCCharacter::GetNPCType() const {
    return this->NPCData.NPCType;
}

// Sets default values
AMythicNPCCharacter::AMythicNPCCharacter() {
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    AbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    // Attributes
    LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>("LifeAttributes");
    CombatAttributes = CreateDefaultSubobject<UMythicAttributeSet_NPCCombat>("CombatAttributes");
}

UAbilitySystemComponent *AMythicNPCCharacter::GetAbilitySystemComponent() const {
    return AbilitySystemComponent;
}

void AMythicNPCCharacter::InitializeASC() {
    if (auto OwnController = this->GetController()) {
        // AI Controller is the owner, and this NPC is the avatar
        AbilitySystemComponent->InitAbilityActorInfo(OwnController, this);
    }
}

void AMythicNPCCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicNPCCharacter, AbilitySystemComponent);
}

// Called when the game starts or when spawned
void AMythicNPCCharacter::BeginPlay() {
    Super::BeginPlay();

    // TODO: Initialize attributes based on the NPC's data
    // Server Only
    // if (this->HasAuthority()) {
    // AMythicGameState *GameState = GetWorld()->GetGameState<AMythicGameState>();
    // if (!GameState) {
    //     UE_LOG(Mythic, Error, TEXT("GameState not found"));
    //     return;
    // }
    //
    // UMythicAbilitySystemComponent *MythicASC = GameState->GetMythicAbilitySystemComponent();
    // if (!MythicASC) {
    //     UE_LOG(Mythic, Error, TEXT("MythicASC not found"));
    //     return;
    // }
    //
    //
    // auto LevelF = static_cast<float>(this->Config.Level);
    //
    // // Enemy Health = Random(HealthMin, HealthMax) * WorldTierAttributes->GetEnemyHealthMultiplier()
    // auto MinHealth = GameState->HealthMinCurveRowHandle.Eval(LevelF, "");
    // auto MaxHealth = GameState->HealthMaxCurveRowHandle.Eval(LevelF, "");
    // auto Health = FMath::RandRange(MinHealth, MaxHealth) * MythicASC->GetNumericAttribute(
    //     GameState->WorldTierAttributes->GetEnemyHealthMultiplierAttribute());
    // UE_LOG(Mythic, Log, TEXT("Attribute Initialized. Health: %f"), Health);
    // LifeAttributes->SetMaxHealth(Health);
    // LifeAttributes->SetHealth(Health);
    //
    // // Enemy Damage = Random(DamageMin, DamageMax) * WorldTierAttributes->GetEnemyDamageMultiplier()
    // auto MinDamage = GameState->DamageMinCurveRowHandle.Eval(LevelF, "");
    // auto MaxDamage = GameState->DamageMaxCurveRowHandle.Eval(LevelF, "");
    // auto Damage = FMath::RandRange(MinDamage, MaxDamage) * MythicASC->GetNumericAttribute(
    //     GameState->WorldTierAttributes->GetEnemyDamageMultiplierAttribute());
    // UE_LOG(Mythic, Log, TEXT("Attribute Initialized. Damage: %f"), Damage);
    // CombatAttributes->SetDamage(Damage);
    // }
}
