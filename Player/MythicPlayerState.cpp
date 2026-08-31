#include "MythicPlayerState.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Survival.h"
#include "World/Survival/MythicSurvivalComponent.h"
#include "MythicFactionStandingComponent.h"
#include "Narrative/MythicNarrativeStateComponent.h"
#include "Narrative/MythicQuestJournalComponent.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/MythicAchievementComponent.h"
#include "Progression/MythicUnlockComponent.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Knowledge/MythicCodexComponent.h"
#include "GAS/Progression/MythicRenownComponent.h"
#include "GAS/Mounts/MythicMountRosterComponent.h"
#include "Narrative/Dialogue/MythicDialogueComponent.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"
#include "World/Trading/MythicTradeContractComponent.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "World/Harvesting/MythicHarvestReceiptLedgerComponent.h"
#include "World/Harvesting/MythicHarvestRewardEscrowComponent.h"
#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "World/Entity/MythicEntityViewerKnowledgeComponent.h"
#include "MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AMythicPlayerState::AMythicPlayerState() {
    MythicAbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("MythicAbilitySystemComponent"));
    MythicAbilitySystemComponent->SetIsReplicated(true);
    MythicAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    UE_LOG(LogTemp, Verbose, TEXT("MythicAbilitySystemComponent %s created"), *MythicAbilitySystemComponent->GetName());

    AffixApplicationComponent =
        CreateDefaultSubobject<UMythicAffixApplicationComponent>(TEXT("AffixApplicationComponent"));

    SetNetUpdateFrequency(30);

    LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
    OffenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Offense>(TEXT("OffenseAttributes"));
    DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>(TEXT("DefenseAttributes"));
    UtilityAttributes = CreateDefaultSubobject<UMythicAttributeSet_Utility>(TEXT("UtilityAttributes"));
    ProficiencyAttributes = CreateDefaultSubobject<UMythicAttributeSet_Proficiencies>(TEXT("ProficiencyAttributes"));

    SurvivalAttributes = CreateDefaultSubobject<UMythicAttributeSet_Survival>(TEXT("SurvivalAttributes"));

    FactionStanding = CreateDefaultSubobject<UMythicFactionStandingComponent>(TEXT("FactionStanding"));

    NarrativeState = CreateDefaultSubobject<UMythicNarrativeStateComponent>(TEXT("NarrativeState"));

    QuestJournal = CreateDefaultSubobject<UMythicQuestJournalComponent>(TEXT("QuestJournal"));


    SurvivalComponent = CreateDefaultSubobject<UMythicSurvivalComponent>(TEXT("SurvivalComponent"));

    StatLedger = CreateDefaultSubobject<UMythicStatLedgerComponent>(TEXT("StatLedger"));

    Achievements = CreateDefaultSubobject<UMythicAchievementComponent>(TEXT("Achievements"));

    Unlocks = CreateDefaultSubobject<UMythicUnlockComponent>(TEXT("Unlocks"));

    Runes = CreateDefaultSubobject<UMythicRuneComponent>(TEXT("Runes"));

    Skills = CreateDefaultSubobject<UMythicSkillComponent>(TEXT("Skills"));

    Codex = CreateDefaultSubobject<UMythicCodexComponent>(TEXT("Codex"));

    Renown = CreateDefaultSubobject<UMythicRenownComponent>(TEXT("Renown"));

    MountRoster = CreateDefaultSubobject<UMythicMountRosterComponent>(TEXT("MountRoster"));

    DialogueComponent = CreateDefaultSubobject<UMythicDialogueComponent>(TEXT("DialogueComponent"));

    Acquaintance = CreateDefaultSubobject<UMythicAcquaintanceComponent>(TEXT("Acquaintance"));

    Dossier = CreateDefaultSubobject<UMythicDossierComponent>(TEXT("Dossier"));

    TradeContracts = CreateDefaultSubobject<UMythicTradeContractComponent>(TEXT("TradeContracts"));

    HarvestReceiptLedger =
        CreateDefaultSubobject<UMythicHarvestReceiptLedgerComponent>(
            TEXT("HarvestReceiptLedger"));
    HarvestRewardEscrow =
        CreateDefaultSubobject<UMythicHarvestRewardEscrowComponent>(
            TEXT("HarvestRewardEscrow"));
    EntityActionGrants =
        CreateDefaultSubobject<UMythicEntityActionGrantComponent>(
            TEXT("EntityActionGrants"));
    EntityCombatPresentation =
        CreateDefaultSubobject<UMythicEntityCombatPresentationComponent>(
            TEXT("EntityCombatPresentation"));
    EntityViewerKnowledge =
        CreateDefaultSubobject<UMythicEntityViewerKnowledgeComponent>(
            TEXT("EntityViewerKnowledge"));
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
    DOREPLIFETIME(AMythicPlayerState, SurvivalAttributes);
    DOREPLIFETIME_CONDITION(AMythicPlayerState, PersistentCharacterId, COND_OwnerOnly);
}

void AMythicPlayerState::SetPersistentCharacterId(const FString &InCharacterId) {
    if (!HasAuthority()) {
        return;
    }
    PersistentCharacterId = InCharacterId;
    if (UWorld *World = GetWorld()) {
        if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
            AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(GetPlayerController());
            Registry->RegisterPlayer(GetCanonicalPlayerKey(), this, MythicPC);
        }
    }
}

bool AMythicPlayerState::AuthoritySetPersistentEntityId(
    const FMythicEntityId &InEntityId) {
    if (!HasAuthority() || !InEntityId.IsValid()
        || InEntityId.GetDomain()
               != EMythicEntityDomain::PlayerCharacter) {
        return false;
    }
    if (PersistentEntityId.IsValid()) {
        return PersistentEntityId == InEntityId;
    }

    PersistentEntityId = InEntityId;
    PersistentEntityIdentityReady.Broadcast(PersistentEntityId);
    return true;
}

FString AMythicPlayerState::ResolveCanonicalPlayerKey(const FString &PersistentId, int32 SessionPlayerId) {
    return PersistentId.IsEmpty() ? FString::Printf(TEXT("session:%d"), SessionPlayerId) : PersistentId;
}

FString AMythicPlayerState::GetCanonicalPlayerKey() const {
    return ResolveCanonicalPlayerKey(PersistentCharacterId, GetPlayerId());
}


void AMythicPlayerState::BeginPlay() {
    Super::BeginPlay();

    if (GetLocalRole() == ROLE_Authority) {
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
                const FActiveGameplayEffectHandle Applied = this->MythicAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

                const bool bInstant = Effect->GetDefaultObject<UGameplayEffect>()->DurationPolicy == EGameplayEffectDurationType::Instant;
                if (bInstant || Applied.IsValid()) {
                    UE_LOG(LogTemp, Log, TEXT("Applied default gameplay effect %s to %s"), *Effect->GetName(), *GetOwner()->GetName());
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
                if (this->MythicAbilitySystemComponent->FindAbilitySpecFromClass(Ability)) {
                    UE_LOG(LogTemp, Verbose, TEXT("Default ability %s already granted to %s; skipping duplicate"), *Ability->GetName(),
                           *GetOwner()->GetName());
                    continue;
                }
                if (this->MythicAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability.GetDefaultObject(), 1, INDEX_NONE, this)).IsValid()) {
                    UE_LOG(LogTemp, Verbose, TEXT("Gave default ability %s to %s"), *Ability->GetName(), *GetOwner()->GetName());
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
        UE_LOG(LogTemp, Verbose, TEXT("Not server, not initializing default abilities and effects"));
    }
}

void AMythicPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (HasAuthority()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
                Registry->UnregisterObject(this);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}


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
