#include "MythicPlayerState.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Exp.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
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
    ExperienceAttributes = CreateDefaultSubobject<UMythicAttributeSet_Exp>(TEXT("ExperienceAttributes"));
    ProficiencyAttributes = CreateDefaultSubobject<UMythicAttributeSet_Proficiencies>(TEXT("ProficiencyAttributes"));
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
    DOREPLIFETIME(AMythicPlayerState, ExperienceAttributes);
    DOREPLIFETIME(AMythicPlayerState, ProficiencyAttributes);
}


// --- Save System Interface Removed ---

void AMythicPlayerState::BeginPlay() {
    Super::BeginPlay();

    // If server, initialize default abilities and effects
    if (GetLocalRole() == ROLE_Authority) {
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
                FGameplayAbilitySpec Spec(Ability.GetDefaultObject(), 1);
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
