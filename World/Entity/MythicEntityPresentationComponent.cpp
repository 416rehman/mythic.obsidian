#include "World/Entity/MythicEntityPresentationComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/Entity/MythicEntityPresentationTags.h"

namespace {
constexpr int32 MaxPublicStatusesPerSubject = 8;

int32 StatusCategoryRank(const EMythicStatusPresentationCategory Category) {
    switch (Category) {
        case EMythicStatusPresentationCategory::HardControl: return 0;
        case EMythicStatusPresentationCategory::Damage: return 1;
        case EMythicStatusPresentationCategory::Control: return 2;
        case EMythicStatusPresentationCategory::Debuff: return 3;
        case EMythicStatusPresentationCategory::Buff: return 4;
        case EMythicStatusPresentationCategory::Cosmetic: return 5;
        default: return 6;
    }
}

double GetAuthoritativeServerTimeSeconds(const UWorld *World) {
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return static_cast<double>(World->GetTimeSeconds());
}
}

UMythicEntityPresentationComponent::UMythicEntityPresentationComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    ObservableFacts.SetOwner(this);
    PublicStatuses.SetOwner(this);
}

void UMythicEntityPresentationComponent::BeginPlay() {
    Super::BeginPlay();
    ObservableFacts.SetOwner(this);
    PublicStatuses.SetOwner(this);

    if (PublicIdentity.IsActive()) {
        RegisterCurrentInstance();
    }
}

void UMythicEntityPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    const FMythicEntityPresentationInstance PreviousInstance = PublicIdentity.Instance;
    if (bRegisteredCurrentInstance && PreviousInstance.IsValid()) {
        OnPresentationDeactivated.Broadcast(*this, PreviousInstance);
        UnregisterInstance(PreviousInstance);
    }

    UnbindAbilitySystem();
    if (GetOwner() && GetOwner()->HasAuthority() && PreviousInstance.IsValid()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicEntityPresentationRegistry *Registry =
                    World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
                Registry->ReleaseAuthorityInstance(PreviousInstance);
            }
        }
    }

    OnPresentationActivated.Clear();
    OnPresentationDeactivated.Clear();
    OnPresentationRevision.Clear();
    Super::EndPlay(EndPlayReason);
}

void UMythicEntityPresentationComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ThisClass, PublicIdentity);
    DOREPLIFETIME(ThisClass, ObservableFacts);
    DOREPLIFETIME(ThisClass, PublicStatuses);
    DOREPLIFETIME(ThisClass, PublicVitality);
}

TArray<FMythicObservableFactItem>
UMythicEntityPresentationComponent::GetObservableFacts() const {
    TArray<FMythicObservableFactItem> Result;
    for (const FMythicObservableFactItem &Item : ObservableFacts.GetItems()) {
        if (IsCurrentFact(Item)) {
            Result.Add(Item);
        }
    }
    return Result;
}

TArray<FMythicPublicStatusPresentationItem>
UMythicEntityPresentationComponent::GetPublicStatuses() const {
    TArray<FMythicPublicStatusPresentationItem> Result;
    for (const FMythicPublicStatusPresentationItem &Item : PublicStatuses.GetItems()) {
        if (IsCurrentStatus(Item)) {
            Result.Add(Item);
        }
    }
    return Result;
}

uint8 UMythicEntityPresentationComponent::QuantizePublicHealthFraction(
    const float HealthFraction) {
    if (!FMath::IsFinite(HealthFraction)) {
        return 0;
    }
    return static_cast<uint8>(FMath::RoundToInt(
        FMath::Clamp(HealthFraction, 0.0f, 1.0f) * 255.0f));
}

float UMythicEntityPresentationComponent::DequantizePublicHealthFraction(
    const uint8 QuantizedFraction) {
    return static_cast<float>(QuantizedFraction) / 255.0f;
}

bool UMythicEntityPresentationComponent::BuildSanitizedPublicIdentity(
    const FMythicPublicIdentitySnapshot &Candidate,
    FMythicPublicIdentitySnapshot &OutSanitizedIdentity) {
    using namespace MythicEntityPresentationTags;
    OutSanitizedIdentity.Reset();
    const bool bAllowedKind = Candidate.PublicKindTag == EntityKindHumanoid
        || Candidate.PublicKindTag == EntityKindCreature
        || Candidate.PublicKindTag == EntityKindPlayer
        || Candidate.PublicKindTag == EntityKindConstruct
        || Candidate.PublicKindTag == EntityKindWorldObject;
    if (!bAllowedKind) {
        return false;
    }

    OutSanitizedIdentity.PublicKindTag = Candidate.PublicKindTag;
    if (Candidate.PublicIdentityDefinitionId.IsValid()
        && Candidate.PublicIdentityDefinitionId.PrimaryAssetType
               == UMythicEntityIdentityDefinition::PrimaryAssetType) {
        OutSanitizedIdentity.PublicIdentityDefinitionId =
            Candidate.PublicIdentityDefinitionId;
    }
    return true;
}

FVector UMythicEntityPresentationComponent::GetPresentationAnchorWorldLocation() const {
    if (IsValid(PresentationAnchor)) {
        return PresentationAnchor->GetComponentTransform().TransformPosition(
            PresentationAnchorOffsetCentimeters);
    }
    if (const AActor *Owner = GetOwner()) {
        return Owner->GetActorLocation() + PresentationAnchorOffsetCentimeters;
    }
    return PresentationAnchorOffsetCentimeters;
}

void UMythicEntityPresentationComponent::SetPresentationAnchor(
    USceneComponent *InAnchor, const FVector InLocalOffsetCentimeters) {
    PresentationAnchor = InAnchor;
    PresentationAnchorOffsetCentimeters = InLocalOffsetCentimeters;
    PublishLocalRevision();
}

bool UMythicEntityPresentationComponent::AuthorityPrepareEmbodiment(
    const FMythicEntityId &EntityId,
    const FMythicPublicIdentitySnapshot &SafeIdentity) {
    AActor *Owner = GetOwner();
    UWorld *World = GetWorld();
    if (!Owner || !Owner->HasAuthority() || !World || !EntityId.IsValid()) {
        return false;
    }

    AuthorityDeactivateEmbodiment();

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!Registry) {
        return false;
    }

    const FMythicEntityPresentationInstance Instance =
        Registry->AllocateAuthorityInstance(EntityId);
    if (!Instance.IsValid()) {
        return false;
    }

    FMythicPublicIdentitySnapshot SanitizedIdentity;
    if (!BuildSanitizedPublicIdentity(SafeIdentity, SanitizedIdentity)) {
        Registry->ReleaseAuthorityInstance(Instance);
        return false;
    }

    AuthorityEntityId = EntityId;
    PublicIdentity = MoveTemp(SanitizedIdentity);
    PublicIdentity.Instance = Instance;
    PublicIdentity.bActive = false;
    FactRevisionCounter = 0;
    StatusRevisionCounter = 0;
    VitalityRevisionCounter = 0;
    ClearPresentationState(true);
    PublishLocalRevision();
    return true;
}

bool UMythicEntityPresentationComponent::AuthorityActivateEmbodiment() {
    AActor *Owner = GetOwner();
    UWorld *World = GetWorld();
    if (!Owner || !Owner->HasAuthority() || !World
        || !AuthorityEntityId.IsValid() || !PublicIdentity.Instance.IsValid()
        || PublicIdentity.bActive) {
        return false;
    }

    WakeOwnerForReplication();
    PublicIdentity.bActive = true;
    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!Registry || !Registry->RegisterPresentationComponent(
                         PublicIdentity.Instance, this)) {
        PublicIdentity.bActive = false;
        return false;
    }

    bRegisteredCurrentInstance = true;
    AuthorityRefreshPublicStatuses();
    AuthorityRefreshPublicVitality();
    OnPresentationActivated.Broadcast(*this, PublicIdentity.Instance);
    PublishLocalRevision();
    FinishOwnerReplicationBatch();
    return true;
}

void UMythicEntityPresentationComponent::AuthorityDeactivateEmbodiment() {
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    const FMythicEntityPresentationInstance PreviousInstance =
        PublicIdentity.Instance;
    if (!PreviousInstance.IsValid() && !AuthorityEntityId.IsValid()) {
        UnbindAbilitySystem();
        ClearPresentationState(false);
        PublicIdentity.Reset();
        return;
    }

    WakeOwnerForReplication();
    PublicIdentity.bActive = false;

    // Subscribers revoke viewer grants while the authority registry can still resolve the old instance.
    OnPresentationDeactivated.Broadcast(*this, PreviousInstance);
    if (bRegisteredCurrentInstance && PreviousInstance.IsValid()) {
        UnregisterInstance(PreviousInstance);
    }

    UnbindAbilitySystem();
    ClearPresentationState(true);
    PublicIdentity.Reset();

    if (PreviousInstance.IsValid()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicEntityPresentationRegistry *Registry =
                    World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
                Registry->ReleaseAuthorityInstance(PreviousInstance);
            }
        }
    }

    AuthorityEntityId.Reset();
    PublishLocalRevision();
    FinishOwnerReplicationBatch();
}

bool UMythicEntityPresentationComponent::SetObservableFact(
    const FGameplayTag FactSlotTag, const FGameplayTag ValueTag,
    const FMythicPresentationHandle RelatedSubject) {
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !PublicIdentity.Instance.IsValid()
        || !IsFactAllowed(FactSlotTag, ValueTag)) {
        return false;
    }

    for (FMythicObservableFactItem &Item : ObservableFacts.Items) {
        if (Item.FactSlotTag != FactSlotTag) {
            continue;
        }

        if (Item.Subject == PublicIdentity.Instance.Handle
            && Item.EmbodimentGeneration ==
                   PublicIdentity.Instance.EmbodimentGeneration
            && Item.ValueTag == ValueTag
            && Item.RelatedSubject == RelatedSubject) {
            return true;
        }

        WakeOwnerForReplication();
        Item.Subject = PublicIdentity.Instance.Handle;
        Item.EmbodimentGeneration =
            PublicIdentity.Instance.EmbodimentGeneration;
        Item.ValueTag = ValueTag;
        Item.RelatedSubject = RelatedSubject;
        Item.Revision = AdvanceNonzeroRevision(FactRevisionCounter);
        ObservableFacts.MarkItemDirty(Item);
        PublishLocalRevision();
        FinishOwnerReplicationBatch();
        return true;
    }

    WakeOwnerForReplication();
    FMythicObservableFactItem &NewItem = ObservableFacts.Items.AddDefaulted_GetRef();
    NewItem.Subject = PublicIdentity.Instance.Handle;
    NewItem.EmbodimentGeneration =
        PublicIdentity.Instance.EmbodimentGeneration;
    NewItem.FactSlotTag = FactSlotTag;
    NewItem.ValueTag = ValueTag;
    NewItem.RelatedSubject = RelatedSubject;
    NewItem.Revision = AdvanceNonzeroRevision(FactRevisionCounter);
    ObservableFacts.MarkItemDirty(NewItem);
    PublishLocalRevision();
    FinishOwnerReplicationBatch();
    return true;
}

void UMythicEntityPresentationComponent::ClearObservableFact(
    const FGameplayTag FactSlotTag) {
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    const int32 Removed = ObservableFacts.Items.RemoveAll(
        [FactSlotTag](const FMythicObservableFactItem &Item) {
            return Item.FactSlotTag == FactSlotTag;
        });
    if (Removed <= 0) {
        return;
    }

    WakeOwnerForReplication();
    ObservableFacts.MarkArrayDirty();
    PublishLocalRevision();
    FinishOwnerReplicationBatch();
}

void UMythicEntityPresentationComponent::AuthorityBindAbilitySystem(
    UAbilitySystemComponent *InAbilitySystem) {
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    UnbindAbilitySystem();
    BoundAbilitySystem = InAbilitySystem;
    if (!InAbilitySystem) {
        if (PublicIdentity.IsActive()) {
            AuthorityRefreshPublicStatuses();
            AuthorityRefreshPublicVitality();
        }
        return;
    }

    HealthAttributeDelegateHandle = InAbilitySystem
        ->GetGameplayAttributeValueChangeDelegate(
            UMythicAttributeSet_Life::GetHealthAttribute())
        .AddUObject(this, &ThisClass::HandleVitalityAttributeChanged);
    MaximumHealthAttributeDelegateHandle = InAbilitySystem
        ->GetGameplayAttributeValueChangeDelegate(
            UMythicAttributeSet_Life::GetMaxHealthAttribute())
        .AddUObject(this, &ThisClass::HandleVitalityAttributeChanged);

    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicStatusRegistry *StatusRegistry =
        GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!StatusRegistry) {
        if (PublicIdentity.IsActive()) {
            AuthorityRefreshPublicStatuses();
            AuthorityRefreshPublicVitality();
        }
        return;
    }

    for (UMythicStatusEffectDefinition *Definition :
         StatusRegistry->GetAllStatuses()) {
        if (!Definition || !Definition->GrantedStateTag.IsValid()
            || Definition->WorldVisibility ==
                   EMythicStatusWorldVisibility::Hidden
            || StatusTagDelegateHandles.Contains(
                Definition->GrantedStateTag)) {
            continue;
        }

        ProjectedStatusStateTags.AddTag(Definition->GrantedStateTag);

        FDelegateHandle Handle = InAbilitySystem
                                     ->RegisterGameplayTagEvent(
                                         Definition->GrantedStateTag,
                                         EGameplayTagEventType::NewOrRemoved)
                                     .AddUObject(
                                         this,
                                         &ThisClass::HandleStatusTagChanged);
        StatusTagDelegateHandles.Add(Definition->GrantedStateTag, Handle);
    }

    ActiveGameplayEffectAddedDelegateHandle =
        InAbilitySystem->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(
            this, &ThisClass::HandleActiveGameplayEffectAdded);
    ActiveGameplayEffectRemovedDelegateHandle =
        InAbilitySystem->OnAnyGameplayEffectRemovedDelegate().AddUObject(
            this, &ThisClass::HandleActiveGameplayEffectRemoved);
    BindExistingProjectedStatusEffects(*InAbilitySystem);

    if (PublicIdentity.IsActive()) {
        AuthorityRefreshPublicStatuses();
        AuthorityRefreshPublicVitality();
    }
}

void UMythicEntityPresentationComponent::
    AuthorityBeginAbilitySystemProjectionBatch() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    ++AbilitySystemProjectionBatchDepth;
}

void UMythicEntityPresentationComponent::
    AuthorityEndAbilitySystemProjectionBatch() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()
        || AbilitySystemProjectionBatchDepth <= 0) {
        return;
    }
    --AbilitySystemProjectionBatchDepth;
    if (AbilitySystemProjectionBatchDepth > 0) {
        return;
    }

    const bool bRefreshStatuses = bStatusProjectionDirty;
    const bool bRefreshVitality = bVitalityProjectionDirty;
    bStatusProjectionDirty = false;
    bVitalityProjectionDirty = false;
    if (bRefreshStatuses) {
        AuthorityRefreshPublicStatuses();
    }
    if (bRefreshVitality) {
        AuthorityRefreshPublicVitality();
    }
}

void UMythicEntityPresentationComponent::HandleReplicatedFactsReceived() {
    PublishLocalRevision();
}

void UMythicEntityPresentationComponent::HandleReplicatedStatusesReceived() {
    PublishLocalRevision();
}

void UMythicEntityPresentationComponent::OnRep_PublicVitality() {
    PublishLocalRevision();
}

void UMythicEntityPresentationComponent::OnRep_PublicIdentity(
    FMythicPublicIdentitySnapshot PreviousIdentity) {
    const bool bInstanceChanged =
        PreviousIdentity.Instance != PublicIdentity.Instance;
    if (PreviousIdentity.IsActive()
        && (bInstanceChanged || !PublicIdentity.IsActive())) {
        OnPresentationDeactivated.Broadcast(*this, PreviousIdentity.Instance);
        UnregisterInstance(PreviousIdentity.Instance);
    }

    if (PublicIdentity.IsActive()
        && (bInstanceChanged || !PreviousIdentity.IsActive())) {
        RegisterCurrentInstance();
    }
    PublishLocalRevision();
}

void UMythicEntityPresentationComponent::RegisterCurrentInstance() {
    if (!PublicIdentity.IsActive() || bRegisteredCurrentInstance) {
        return;
    }
    if (UWorld *World = GetWorld()) {
        if (UMythicEntityPresentationRegistry *Registry =
                World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
            if (Registry->RegisterPresentationComponent(PublicIdentity.Instance,
                                                        this)) {
                bRegisteredCurrentInstance = true;
                OnPresentationActivated.Broadcast(*this,
                                                  PublicIdentity.Instance);
            }
        }
    }
}

void UMythicEntityPresentationComponent::UnregisterInstance(
    const FMythicEntityPresentationInstance &Instance) {
    if (!Instance.IsValid()) {
        return;
    }
    if (UWorld *World = GetWorld()) {
        if (UMythicEntityPresentationRegistry *Registry =
                World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
            Registry->UnregisterPresentationComponent(Instance, this);
        }
    }
    bRegisteredCurrentInstance = false;
}

void UMythicEntityPresentationComponent::PublishLocalRevision() {
    ++LocalRevision;
    if (LocalRevision == 0) {
        LocalRevision = 1;
    }
    OnPresentationRevision.Broadcast(LocalRevision);
}

void UMythicEntityPresentationComponent::ClearPresentationState(
    const bool bMarkReplicationDirty) {
    ObservableFacts.Items.Reset();
    PublicStatuses.Items.Reset();
    PublicVitality.Reset();
    if (bMarkReplicationDirty) {
        ObservableFacts.MarkArrayDirty();
        PublicStatuses.MarkArrayDirty();
    }
}

void UMythicEntityPresentationComponent::UnbindAbilitySystem() {
    bStatusRefreshQueued = false;
    AbilitySystemProjectionBatchDepth = 0;
    bStatusProjectionDirty = false;
    bVitalityProjectionDirty = false;
    if (UAbilitySystemComponent *AbilitySystem = BoundAbilitySystem.Get()) {
        if (HealthAttributeDelegateHandle.IsValid()) {
            AbilitySystem->GetGameplayAttributeValueChangeDelegate(
                UMythicAttributeSet_Life::GetHealthAttribute())
                .Remove(HealthAttributeDelegateHandle);
        }
        if (MaximumHealthAttributeDelegateHandle.IsValid()) {
            AbilitySystem->GetGameplayAttributeValueChangeDelegate(
                UMythicAttributeSet_Life::GetMaxHealthAttribute())
                .Remove(MaximumHealthAttributeDelegateHandle);
        }
        for (const TPair<FGameplayTag, FDelegateHandle> &Pair :
             StatusTagDelegateHandles) {
            AbilitySystem
                ->RegisterGameplayTagEvent(Pair.Key,
                                           EGameplayTagEventType::NewOrRemoved)
                .Remove(Pair.Value);
        }
        if (ActiveGameplayEffectAddedDelegateHandle.IsValid()) {
            AbilitySystem->OnActiveGameplayEffectAddedDelegateToSelf.Remove(
                ActiveGameplayEffectAddedDelegateHandle);
        }
        if (ActiveGameplayEffectRemovedDelegateHandle.IsValid()) {
            AbilitySystem->OnAnyGameplayEffectRemovedDelegate().Remove(
                ActiveGameplayEffectRemovedDelegateHandle);
        }
        for (const TPair<FActiveGameplayEffectHandle,
                         FMythicProjectedStatusEffectDelegateHandles> &Pair :
             StatusEffectDelegateHandles) {
            if (FOnActiveGameplayEffectStackChange *Delegate =
                    AbilitySystem->OnGameplayEffectStackChangeDelegate(
                        Pair.Key);
                Delegate && Pair.Value.StackChanged.IsValid()) {
                Delegate->Remove(Pair.Value.StackChanged);
            }
            if (FOnActiveGameplayEffectTimeChange *Delegate =
                    AbilitySystem->OnGameplayEffectTimeChangeDelegate(Pair.Key);
                Delegate && Pair.Value.TimeChanged.IsValid()) {
                Delegate->Remove(Pair.Value.TimeChanged);
            }
            if (FOnActiveGameplayEffectInhibitionChanged *Delegate =
                    AbilitySystem->OnGameplayEffectInhibitionChangedDelegate(
                        Pair.Key);
                Delegate && Pair.Value.InhibitionChanged.IsValid()) {
                Delegate->Remove(Pair.Value.InhibitionChanged);
            }
        }
    }
    HealthAttributeDelegateHandle.Reset();
    MaximumHealthAttributeDelegateHandle.Reset();
    ActiveGameplayEffectAddedDelegateHandle.Reset();
    ActiveGameplayEffectRemovedDelegateHandle.Reset();
    ProjectedStatusStateTags.Reset();
    StatusTagDelegateHandles.Reset();
    StatusEffectDelegateHandles.Reset();
    BoundAbilitySystem.Reset();
}

bool UMythicEntityPresentationComponent::BindProjectedStatusEffectDelegates(
    UAbilitySystemComponent &AbilitySystem,
    const FActiveGameplayEffectHandle EffectHandle,
    const FGameplayEffectSpec &EffectSpec) {
    if (!EffectHandle.IsValid()
        || StatusEffectDelegateHandles.Contains(EffectHandle)
        || !GameplayEffectGrantsProjectedStatus(
            EffectSpec, ProjectedStatusStateTags)) {
        return false;
    }

    FMythicProjectedStatusEffectDelegateHandles Handles;
    if (FOnActiveGameplayEffectStackChange *Delegate =
            AbilitySystem.OnGameplayEffectStackChangeDelegate(EffectHandle)) {
        Handles.StackChanged = Delegate->AddUObject(
            this, &ThisClass::HandleStatusEffectStackChanged);
    }
    if (FOnActiveGameplayEffectTimeChange *Delegate =
            AbilitySystem.OnGameplayEffectTimeChangeDelegate(EffectHandle)) {
        Handles.TimeChanged = Delegate->AddUObject(
            this, &ThisClass::HandleStatusEffectTimeChanged);
    }
    if (FOnActiveGameplayEffectInhibitionChanged *Delegate =
            AbilitySystem.OnGameplayEffectInhibitionChangedDelegate(
                EffectHandle)) {
        Handles.InhibitionChanged = Delegate->AddUObject(
            this, &ThisClass::HandleStatusEffectInhibitionChanged);
    }
    StatusEffectDelegateHandles.Add(EffectHandle, Handles);
    return true;
}

bool UMythicEntityPresentationComponent::GameplayEffectGrantsProjectedStatus(
    const FGameplayEffectSpec &EffectSpec,
    const FGameplayTagContainer &ProjectedStateTags) {
    if (ProjectedStateTags.IsEmpty()) {
        return false;
    }
    FGameplayTagContainer GrantedTags;
    EffectSpec.GetAllGrantedTags(GrantedTags);
    return GrantedTags.HasAny(ProjectedStateTags);
}

void UMythicEntityPresentationComponent::BindExistingProjectedStatusEffects(
    UAbilitySystemComponent &AbilitySystem) {
    for (const FActiveGameplayEffectHandle EffectHandle :
         AbilitySystem.GetActiveEffects(FGameplayEffectQuery())) {
        if (const FActiveGameplayEffect *ActiveEffect =
                AbilitySystem.GetActiveGameplayEffect(EffectHandle)) {
            BindProjectedStatusEffectDelegates(
                AbilitySystem, EffectHandle, ActiveEffect->Spec);
        }
    }
}

void UMythicEntityPresentationComponent::HandleActiveGameplayEffectAdded(
    UAbilitySystemComponent *TargetAbilitySystem,
    const FGameplayEffectSpec &EffectSpec,
    const FActiveGameplayEffectHandle EffectHandle) {
    UAbilitySystemComponent *AbilitySystem = BoundAbilitySystem.Get();
    if (!AbilitySystem || TargetAbilitySystem != AbilitySystem) {
        return;
    }
    if (BindProjectedStatusEffectDelegates(
            *AbilitySystem, EffectHandle, EffectSpec)) {
        RequestAuthorityStatusRefresh();
    }
}

void UMythicEntityPresentationComponent::HandleActiveGameplayEffectRemoved(
    const FActiveGameplayEffect &RemovedEffect) {
    if (StatusEffectDelegateHandles.Remove(RemovedEffect.Handle) > 0) {
        RequestAuthorityStatusRefresh();
    }
}

void UMythicEntityPresentationComponent::HandleStatusEffectStackChanged(
    const FActiveGameplayEffectHandle EffectHandle, int32 /*NewCount*/,
    int32 /*PreviousCount*/) {
    if (StatusEffectDelegateHandles.Contains(EffectHandle)) {
        RequestAuthorityStatusRefresh();
    }
}

void UMythicEntityPresentationComponent::HandleStatusEffectTimeChanged(
    const FActiveGameplayEffectHandle EffectHandle, float /*NewStartTime*/,
    float /*NewDuration*/) {
    if (StatusEffectDelegateHandles.Contains(EffectHandle)) {
        RequestAuthorityStatusRefresh();
    }
}

void UMythicEntityPresentationComponent::HandleStatusEffectInhibitionChanged(
    const FActiveGameplayEffectHandle EffectHandle, bool /*bIsInhibited*/) {
    if (StatusEffectDelegateHandles.Contains(EffectHandle)) {
        RequestAuthorityStatusRefresh();
    }
}

void UMythicEntityPresentationComponent::HandleStatusTagChanged(
    FGameplayTag /*StateTag*/, int32 /*NewCount*/) {
    RequestAuthorityStatusRefresh();
}

void UMythicEntityPresentationComponent::HandleVitalityAttributeChanged(
    const FOnAttributeChangeData & /*ChangeData*/) {
    AuthorityRefreshPublicVitality();
}

void UMythicEntityPresentationComponent::RequestAuthorityStatusRefresh() {
    if (AbilitySystemProjectionBatchDepth > 0) {
        bStatusProjectionDirty = true;
        return;
    }
    if (bStatusRefreshQueued) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        AuthorityRefreshPublicStatuses();
        return;
    }
    bStatusRefreshQueued = true;
    World->GetTimerManager().SetTimerForNextTick(
        this, &ThisClass::FlushQueuedAuthorityStatusRefresh);
}

void UMythicEntityPresentationComponent::FlushQueuedAuthorityStatusRefresh() {
    if (!bStatusRefreshQueued) {
        return;
    }
    bStatusRefreshQueued = false;
    AuthorityRefreshPublicStatuses();
}

void UMythicEntityPresentationComponent::AuthorityRefreshPublicVitality() {
    if (AbilitySystemProjectionBatchDepth > 0) {
        bVitalityProjectionDirty = true;
        return;
    }
    bVitalityProjectionDirty = false;
    AActor *Owner = GetOwner();
    FMythicPublicVitalitySnapshot Next;

    if (Owner && Owner->HasAuthority() && PublicIdentity.Instance.IsValid()) {
        if (const UAbilitySystemComponent *AbilitySystem =
                BoundAbilitySystem.Get()) {
            const float MaximumHealth = AbilitySystem->GetNumericAttribute(
                UMythicAttributeSet_Life::GetMaxHealthAttribute());
            const float CurrentHealth = AbilitySystem->GetNumericAttribute(
                UMythicAttributeSet_Life::GetHealthAttribute());
            if (FMath::IsFinite(MaximumHealth)
                && MaximumHealth > UE_SMALL_NUMBER
                && FMath::IsFinite(CurrentHealth)) {
                Next.Instance = PublicIdentity.Instance;
                Next.bValid = true;
                Next.HealthFractionQuantized = QuantizePublicHealthFraction(
                    CurrentHealth / MaximumHealth);
            }
        }
    }

    const bool bChanged = PublicVitality.Instance != Next.Instance
        || PublicVitality.bValid != Next.bValid
        || PublicVitality.HealthFractionQuantized
               != Next.HealthFractionQuantized;
    if (!bChanged) {
        return;
    }

    if (Next.bValid) {
        Next.Revision = AdvanceNonzeroRevision(VitalityRevisionCounter);
    }
    WakeOwnerForReplication();
    PublicVitality = Next;
    PublishLocalRevision();
    FinishOwnerReplicationBatch();
}

void UMythicEntityPresentationComponent::AuthorityRefreshPublicStatuses() {
    if (AbilitySystemProjectionBatchDepth > 0) {
        bStatusProjectionDirty = true;
        return;
    }
    bStatusProjectionDirty = false;
    // A direct lifecycle/rebind refresh also consumes any deferred event burst; the queued callback then no-ops.
    bStatusRefreshQueued = false;
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !PublicIdentity.Instance.IsValid()) {
        if (!PublicStatuses.Items.IsEmpty()) {
            PublicStatuses.Items.Reset();
            PublicStatuses.MarkArrayDirty();
            PublishLocalRevision();
        }
        return;
    }

    UAbilitySystemComponent *AbilitySystem = BoundAbilitySystem.Get();
    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicStatusRegistry *StatusRegistry =
        GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;

    TArray<UMythicStatusEffectDefinition *> Selected;
    if (AbilitySystem && StatusRegistry) {
        for (UMythicStatusEffectDefinition *Definition :
             StatusRegistry->GetAllStatuses()) {
            if (Definition && Definition->StatusType.IsValid()
                && Definition->GrantedStateTag.IsValid()
                && Definition->WorldVisibility !=
                       EMythicStatusWorldVisibility::Hidden
                && AbilitySystem->GetTagCount(Definition->GrantedStateTag) > 0) {
                Selected.Add(Definition);
            }
        }
    }

    Selected.Sort([](const UMythicStatusEffectDefinition &Left,
                     const UMythicStatusEffectDefinition &Right) {
        const int32 LeftRank = StatusCategoryRank(Left.PresentationCategory);
        const int32 RightRank = StatusCategoryRank(Right.PresentationCategory);
        if (LeftRank != RightRank) {
            return LeftRank < RightRank;
        }
        if (Left.WorldPresentationPriority != Right.WorldPresentationPriority) {
            return Left.WorldPresentationPriority >
                   Right.WorldPresentationPriority;
        }
        return Left.StatusType.GetTagName().LexicalLess(
            Right.StatusType.GetTagName());
    });
    if (Selected.Num() > MaxPublicStatusesPerSubject) {
        Selected.SetNum(MaxPublicStatusesPerSubject, EAllowShrinking::No);
    }

    TSet<FGameplayTag> SelectedTags;
    bool bChanged = false;
    const double ServerNow = GetAuthoritativeServerTimeSeconds(GetWorld());
    const float EffectWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    for (const UMythicStatusEffectDefinition *Definition : Selected) {
        SelectedTags.Add(Definition->StatusType);
        double ServerEndTime = 0.0;
        int32 StackCount = 0;

        const FGameplayEffectQuery Query =
            FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
                FGameplayTagContainer(Definition->GrantedStateTag));
        for (const FActiveGameplayEffectHandle &Handle :
             AbilitySystem->GetActiveEffects(Query)) {
            const FActiveGameplayEffect *Active =
                AbilitySystem->GetActiveGameplayEffect(Handle);
            if (!Active || Active->bIsInhibited || Active->IsPendingRemove) {
                continue;
            }
            const float Remaining = Active->GetTimeRemaining(EffectWorldTime);
            if (FMath::IsFinite(Remaining) && Remaining > 0.0f) {
                ServerEndTime = FMath::Max(ServerEndTime,
                                           ServerNow + Remaining);
            }
            StackCount = FMath::Max(StackCount, Active->Spec.GetStackCount());
        }

        if (!Definition->bShowRemainingDuration) {
            ServerEndTime = 0.0;
        }
        const uint8 PublicStackCount = Definition->bShowStackCount
                                          ? static_cast<uint8>(FMath::Clamp(
                                                StackCount, 1, 255))
                                          : 0;

        FMythicPublicStatusPresentationItem *Existing =
            PublicStatuses.Items.FindByPredicate(
                [Definition](const FMythicPublicStatusPresentationItem &Item) {
                    return Item.StatusType == Definition->StatusType;
                });
        if (!Existing) {
            FMythicPublicStatusPresentationItem &NewItem =
                PublicStatuses.Items.AddDefaulted_GetRef();
            NewItem.Subject = PublicIdentity.Instance.Handle;
            NewItem.EmbodimentGeneration =
                PublicIdentity.Instance.EmbodimentGeneration;
            NewItem.StatusType = Definition->StatusType;
            NewItem.ServerEndTimeSeconds = ServerEndTime;
            NewItem.StackCount = PublicStackCount;
            NewItem.Revision = AdvanceNonzeroRevision(StatusRevisionCounter);
            PublicStatuses.MarkItemDirty(NewItem);
            bChanged = true;
            continue;
        }

        if (Existing->Subject != PublicIdentity.Instance.Handle
            || Existing->EmbodimentGeneration !=
                   PublicIdentity.Instance.EmbodimentGeneration
            || !FMath::IsNearlyEqual(Existing->ServerEndTimeSeconds,
                                     ServerEndTime, 0.05)
            || Existing->StackCount != PublicStackCount) {
            Existing->Subject = PublicIdentity.Instance.Handle;
            Existing->EmbodimentGeneration =
                PublicIdentity.Instance.EmbodimentGeneration;
            Existing->ServerEndTimeSeconds = ServerEndTime;
            Existing->StackCount = PublicStackCount;
            Existing->Revision =
                AdvanceNonzeroRevision(StatusRevisionCounter);
            PublicStatuses.MarkItemDirty(*Existing);
            bChanged = true;
        }
    }

    const int32 Removed = PublicStatuses.Items.RemoveAll(
        [&SelectedTags](const FMythicPublicStatusPresentationItem &Item) {
            return !SelectedTags.Contains(Item.StatusType);
        });
    if (Removed > 0) {
        PublicStatuses.MarkArrayDirty();
        bChanged = true;
    }

    if (bChanged) {
        WakeOwnerForReplication();
        PublishLocalRevision();
        FinishOwnerReplicationBatch();
    }
}

bool UMythicEntityPresentationComponent::IsFactAllowed(
    const FGameplayTag FactSlotTag, const FGameplayTag ValueTag) const {
    using namespace MythicEntityPresentationTags;
    if (!FactSlotTag.IsValid() || !ValueTag.IsValid()) {
        return false;
    }
    if (FactSlotTag == ObservableSlotActivity) {
        const FGameplayTag ActivityRoot = FGameplayTag::RequestGameplayTag(
            FName(TEXT("NPC.Activity")), false);
        return ActivityRoot.IsValid() && ValueTag.MatchesTag(ActivityRoot);
    }
    if (FactSlotTag == ObservableSlotBehavior) {
        return ValueTag == ObservableBehaviorFighting
               || ValueTag == ObservableBehaviorFleeing
               || ValueTag == ObservableBehaviorSurrendering;
    }
    if (FactSlotTag == ObservableSlotLifeState) {
        return ValueTag == ObservableLifeDowned
               || ValueTag == ObservableLifeDying
               || ValueTag == ObservableLifeDead;
    }
    return false;
}

bool UMythicEntityPresentationComponent::IsCurrentFact(
    const FMythicObservableFactItem &Item) const {
    return PublicIdentity.IsActive()
           && Item.Subject == PublicIdentity.Instance.Handle
           && Item.EmbodimentGeneration ==
                  PublicIdentity.Instance.EmbodimentGeneration
           && Item.Revision != 0;
}

bool UMythicEntityPresentationComponent::IsCurrentStatus(
    const FMythicPublicStatusPresentationItem &Item) const {
    return PublicIdentity.IsActive()
           && Item.Subject == PublicIdentity.Instance.Handle
           && Item.EmbodimentGeneration ==
                  PublicIdentity.Instance.EmbodimentGeneration
           && Item.Revision != 0;
}

void UMythicEntityPresentationComponent::WakeOwnerForReplication() {
    if (AActor *Owner = GetOwner(); Owner && Owner->HasAuthority()) {
        Owner->FlushNetDormancy();
    }
}

void UMythicEntityPresentationComponent::FinishOwnerReplicationBatch() {
    if (AActor *Owner = GetOwner(); Owner && Owner->HasAuthority()) {
        Owner->ForceNetUpdate();
    }
}

uint32 UMythicEntityPresentationComponent::AdvanceNonzeroRevision(
    uint32 &Counter) {
    ++Counter;
    if (Counter == 0) {
        Counter = 1;
    }
    return Counter;
}
