#include "MythicAbilitySystemComponent.h"
#include "Player/MythicPlayerController.h"

#include "Mythic.h"
#include "MythicTags_GAS.h"
#include "Settings/MythicDeveloperSettings.h"
#include "System/MythicAssetManager.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "GAS/Abilities/MythicGA_AbilityWindow.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Settings/MythicCombatSettings.h"
#include "TimerManager.h"

namespace {
bool IsValidActivationGroup(const EMythicAbilityActivationGroup Group) {
    return static_cast<uint8>(Group) < static_cast<uint8>(EMythicAbilityActivationGroup::MAX);
}
}

void UMythicAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer &AbilityTags,
                                                                           FGameplayTagContainer &OutActivationRequired,
                                                                           FGameplayTagContainer &OutActivationBlocked) const {
    if (AbilityTagRelationshipMapping) {
        AbilityTagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
    }
}

void UMythicAbilitySystemComponent::BeginPlay() {
    Super::BeginPlay();

    ReconcileAllBoundedAttributes();

    OnAbilityProjectionChanged.Broadcast();

    if (!AbilityTagRelationshipMapping) {
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            UMythicAssetManager::LoadAsync(this, Settings->DefaultAbilityTagRelationshipMapping,
                                           [this](UMythicAbilityTagRelationshipMapping *Mapping) {
                                               if (Mapping && !AbilityTagRelationshipMapping) {
                                                   AbilityTagRelationshipMapping = Mapping;
                                                   UE_LOG(Myth, Verbose, TEXT("ASC [%s]: Applied AbilityTagRelationshipMapping"), *GetName());
                                               }
                                           });
        }
    }
}

void UMythicAbilitySystemComponent::InitAbilityActorInfo(AActor *InOwnerActor, AActor *InAvatarActor) {
    if (GetAvatarActor() && GetAvatarActor() != InAvatarActor) {
        CancelAbilityWindows();
        TArray<FGuid> Handles;
        for (const auto& Shield : OwnedShields) Handles.Add(Shield.Handle);
        for (FGuid Handle : Handles) RemoveOwnedShield(Handle);
    }
    Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

    ReconcileAllBoundedAttributes();
    OnAbilityProjectionChanged.Broadcast();

    if (!InOwnerActor || !InOwnerActor->HasAuthority()) {
        return;
    }
    UMythicAffixApplicationComponent *Application =
        InOwnerActor->FindComponentByClass<UMythicAffixApplicationComponent>();
    if (!Application && InAvatarActor) {
        Application = InAvatarActor->FindComponentByClass<UMythicAffixApplicationComponent>();
    }
    if (Application) {
        Application->NotifyAbilitySystemActorInfoChanged(this);
    }
}

void UMythicAbilitySystemComponent::NotifyAbilityActivated(
    const FGameplayAbilitySpecHandle Handle, UGameplayAbility *Ability) {
    // Register first: activation delegates are allowed to query groups or synchronously end the ability.
    if (UMythicGameplayAbility *MythicAbility = Cast<UMythicGameplayAbility>(Ability)) {
        AddAbilityToActivationGroup(MythicAbility->GetActivationGroup(), MythicAbility);
    }

    Super::NotifyAbilityActivated(Handle, Ability);
}

void UMythicAbilitySystemComponent::NotifyAbilityEnded(
    const FGameplayAbilitySpecHandle Handle, UGameplayAbility *Ability, const bool bWasCancelled) {
    // Unregister first so end delegates can immediately activate abilities that the ending group blocked.
    if (UMythicGameplayAbility *MythicAbility = Cast<UMythicGameplayAbility>(Ability)) {
        RemoveAbilityFromActivationGroup(MythicAbility->GetActivationGroup(), MythicAbility);
    }

    Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
}

void UMythicAbilitySystemComponent::SetTagRelationshipMapping(UMythicAbilityTagRelationshipMapping *NewMapping) {
    AbilityTagRelationshipMapping = NewMapping;
    OnAbilityProjectionChanged.Broadcast();
}

bool UMythicAbilitySystemComponent::CanAcceptAbilityInput() const {
    if (bAbilityInputSuppressed || HasMatchingGameplayTag(GAS_INPUT_BLOCKED)) return false;
    const auto* Controller=AbilityActorInfo.IsValid() ? Cast<AMythicPlayerController>(AbilityActorInfo->PlayerController.Get()) : nullptr;
    return !Controller || !Controller->IsGameplayAbilityInputBlocked();
}

void UMythicAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag) {
    if (!InputTag.IsValid() || !CanAcceptAbilityInput() || PressedInputOwners.Contains(InputTag)) return;
    auto& Owners=PressedInputOwners.Add(InputTag);
    ABILITYLIST_SCOPE_LOCK();
    for (const auto& Spec:ActivatableAbilities.Items) {
        if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) Owners.Add(Spec.Handle);
    }
    // Copy before callbacks/overflow cleanup can invalidate the map entry.
    const auto Handles=Owners;
    for (auto Handle:Handles) AbilityInputSpecPressed(Handle);
}

void UMythicAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag) {
    TArray<FGameplayAbilitySpecHandle> Handles;
    if (!PressedInputOwners.RemoveAndCopyValue(InputTag,Handles)) return;
    for (auto Handle:Handles) AbilityInputSpecReleased(Handle);
}

void UMythicAbilitySystemComponent::AbilityInputTagCanceled(const FGameplayTag& InputTag) {
    TArray<FGameplayAbilitySpecHandle> Handles;
    if (!PressedInputOwners.RemoveAndCopyValue(InputTag,Handles)) return;
    for (auto Handle:Handles) AbilityInputSpecCanceled(Handle);
}

void UMythicAbilitySystemComponent::AbilityInputSpecPressed(FGameplayAbilitySpecHandle Handle) {
    if (!CanAcceptAbilityInput() || !FindAbilitySpecFromHandle(Handle) || InputHeldSpecHandles.Contains(Handle)) return;
    if (AbilityInputEdges.Num()>=128) { ClearAbilityInput(); return; }
    InputHeldSpecHandles.Add(Handle);
    AbilityInputEdges.Add({Handle,true,InputCancelEpochs.FindRef(Handle)});
}

void UMythicAbilitySystemComponent::AbilityInputSpecReleased(FGameplayAbilitySpecHandle Handle) {
    if (!InputHeldSpecHandles.Remove(Handle)) return;
    if (!CanAcceptAbilityInput() || AbilityInputEdges.Num()>=128) { AbilityInputSpecCanceled(Handle); return; }
    AbilityInputEdges.Add({Handle,false,InputCancelEpochs.FindRef(Handle)});
}

void UMythicAbilitySystemComponent::CancelActiveAbilityInput(FGameplayAbilitySpecHandle Handle,const FPredictionKey* RequiredKey) {
    ABILITYLIST_SCOPE_LOCK();
    auto* Spec=FindAbilitySpecFromHandle(Handle);
    if (!Spec || !Spec->IsActive()) return;
    // GAS windows disallow arbitrary client termination. This narrow interruption route
    // checks the exact active prediction key and uses the authored family cancel policy.
    const auto Instances=Spec->GetAbilityInstances();
    for (auto* Instance:Instances) {
        auto* Ability=Cast<UMythicGameplayAbility>(Instance);
        if (!Ability || !Ability->IsActive()) continue;
        const auto Info=Ability->GetCurrentActivationInfo();
        const auto Key=Info.GetActivationPredictionKey();
        if (RequiredKey && Key!=*RequiredKey) continue;
        Spec->InputPressed=false;
        if (!RequiredKey && AbilityActorInfo.IsValid() && AbilityActorInfo->IsLocallyControlled() && !IsOwnerActorAuthoritative() && Key.IsValidKey()) {
            ServerAbilityInputCanceled(Handle,Key);
        }
        if (Ability->ShouldCancelOnInputInterrupted()) Ability->CancelAbility(Handle,AbilityActorInfo.Get(),Info,IsOwnerActorAuthoritative());
    }
}

void UMythicAbilitySystemComponent::ServerAbilityInputCanceled_Implementation(FGameplayAbilitySpecHandle Handle,FPredictionKey ActivationKey) {
    if (!ActivationKey.IsValidKey()) return;
    CancelActiveAbilityInput(Handle,&ActivationKey);
}

void UMythicAbilitySystemComponent::AbilityInputSpecCanceled(FGameplayAbilitySpecHandle Handle) {
    ++InputCancelEpochs.FindOrAdd(Handle);
    InputHeldSpecHandles.Remove(Handle);
    AbilityInputEdges.RemoveAll([Handle](const auto& Edge){ return Edge.Handle==Handle; });
    if (auto* Spec=FindAbilitySpecFromHandle(Handle)) Spec->InputPressed=false;
    CancelActiveAbilityInput(Handle);
}

void UMythicAbilitySystemComponent::SetAbilityInputSuppressed(bool bSuppressed) {
    if (bAbilityInputSuppressed==bSuppressed) return;
    bAbilityInputSuppressed=bSuppressed;
    if (bSuppressed) ClearAbilityInput();
    OnAbilityProjectionChanged.Broadcast();
}

void UMythicAbilitySystemComponent::ProcessAbilityInput(float DeltaTime,bool bGamePaused) {
    if (bProcessingAbilityInput) return;
    TGuardValue<bool> Processing(bProcessingAbilityInput,true);
    if (bGamePaused || !CanAcceptAbilityInput()) {
        if (!bInputWasBlocked || !AbilityInputEdges.IsEmpty() || !InputHeldSpecHandles.IsEmpty()) ClearAbilityInput();
        bInputWasBlocked=true; return;
    }
    bInputWasBlocked=false;
    const auto Edges=MoveTemp(AbilityInputEdges);
    AbilityInputEdges.Reset();
    const uint32 ClearEpoch=InputClearEpoch;
    for (const auto& Edge:Edges) ProcessingInputOwners.AddUnique(Edge.Handle);
    TSet<FGameplayAbilitySpecHandle> Attempted;
    for (const auto& Edge:Edges) {
        if (ClearEpoch!=InputClearEpoch) break;
        if (Edge.CancelEpoch!=InputCancelEpochs.FindRef(Edge.Handle)) continue;
        if (!CanAcceptAbilityInput()) { ClearAbilityInput(); break; }
        auto* Spec=FindAbilitySpecFromHandle(Edge.Handle);
        if (!Spec || !Spec->Ability) continue;
        Spec->InputPressed=Edge.bPressed;
        if (Spec->IsActive()) {
            if (Edge.bPressed) AbilitySpecInputPressed(*Spec);
            else AbilitySpecInputReleased(*Spec);
        } else if (Edge.bPressed) {
            const auto* Ability=Cast<UMythicGameplayAbility>(Spec->Ability);
            if (Ability && (Ability->GetActivationPolicy()==EMythicAbilityActivationPolicy::OnInputTriggered || Ability->GetActivationPolicy()==EMythicAbilityActivationPolicy::WhileInputActive)) {
                Attempted.Add(Edge.Handle);
                TryActivateAbility(Edge.Handle);
            }
        }
    }
    ProcessingInputOwners.Reset();
    TArray<FAbilityInputEdge,TInlineAllocator<8>> Held;
    for (auto Handle:InputHeldSpecHandles) Held.Add({Handle,false,InputCancelEpochs.FindRef(Handle)});
    for (const auto& Retry:Held) {
        const auto Handle=Retry.Handle;
        if (ClearEpoch!=InputClearEpoch) break;
        if (!InputHeldSpecHandles.Contains(Handle) || Retry.CancelEpoch!=InputCancelEpochs.FindRef(Handle)) continue;
        if (!CanAcceptAbilityInput()) { ClearAbilityInput(); break; }
        auto* Spec=FindAbilitySpecFromHandle(Handle);
        const auto* Ability=Spec ? Cast<UMythicGameplayAbility>(Spec->Ability) : nullptr;
        if (Ability && !Spec->IsActive() && !Attempted.Contains(Handle) && Ability->GetActivationPolicy()==EMythicAbilityActivationPolicy::WhileInputActive) TryActivateAbility(Handle);
    }
}

void UMythicAbilitySystemComponent::ClearAbilityInput() {
    if (bClearingAbilityInput) return;
    TGuardValue<bool> Clearing(bClearingAbilityInput,true);
    ++InputClearEpoch;
    auto Handles=MoveTemp(InputHeldSpecHandles);
    for (const auto& Edge:AbilityInputEdges) Handles.AddUnique(Edge.Handle);
    for (auto Handle:ProcessingInputOwners) Handles.AddUnique(Handle);
    // A placement preview can outlive the physical press. Its explicit interruption
    // policy, rather than a held-key array, owns the continuing input-dependent work.
    for (const auto& Spec:ActivatableAbilities.Items) {
        const auto* Ability=Cast<UMythicGameplayAbility>(Spec.Ability);
        if (Spec.IsActive() && Ability && Ability->ShouldCancelOnInputInterrupted()) Handles.AddUnique(Spec.Handle);
    }
    ProcessingInputOwners.Reset();
    AbilityInputEdges.Reset(); InputHeldSpecHandles.Reset(); PressedInputOwners.Reset();
    for (auto Handle:Handles) {
        if (auto* Spec=FindAbilitySpecFromHandle(Handle)) Spec->InputPressed=false;
        CancelActiveAbilityInput(Handle);
    }
}

void UMythicAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& Spec) {
    Super::OnGiveAbility(Spec);
    OnAbilityProjectionChanged.Broadcast();
}

void UMythicAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& Spec) {
    InputCancelEpochs.Remove(Spec.Handle);
    AbilityInputEdges.RemoveAll([Handle=Spec.Handle](const auto& Edge){ return Edge.Handle==Handle; });
    InputHeldSpecHandles.Remove(Spec.Handle);
    for (auto& Pair:PressedInputOwners) Pair.Value.Remove(Spec.Handle);
    Spec.InputPressed=false;
    Super::OnRemoveAbility(Spec);
    OnAbilityProjectionChanged.Broadcast();
}

const TArray<UMythicAttributeSet *> &UMythicAbilitySystemComponent::GetAttributeSets() const {
    return reinterpret_cast<const TArray<UMythicAttributeSet *> &>(this->GetSpawnedAttributes());
}

void UMythicAbilitySystemComponent::ReconcileAllBoundedAttributes() {
    for (UAttributeSet *AttributeSet : GetSpawnedAttributes()) {
        if (UMythicAttributeSet *MythicSet = Cast<UMythicAttributeSet>(AttributeSet)) {
            MythicSet->ReconcileAllBoundedAttributes();
        }
    }
}

void UMythicAbilitySystemComponent::ExecuteGameplayCueMulticast(FGameplayTag CueTag, const FGameplayCueParameters &CueParams) {
    if (!CueTag.IsValid()) {
        return;
    }

    if (GetOwnerRole() == ROLE_Authority) {
        Multicast_ExecuteGameplayCue(CueTag, CueParams);
    }
}

void UMythicAbilitySystemComponent::Multicast_ExecuteGameplayCue_Implementation(FGameplayTag CueTag, FGameplayCueParameters CueParams) {
    ExecuteGameplayCue(CueTag, CueParams);
}

void UMythicAbilitySystemComponent::ExecuteGameplayCueOwnerOnly(FGameplayTag CueTag, const FGameplayCueParameters &CueParams) {
    if (!CueTag.IsValid()) {
        return;
    }

    if (GetOwnerRole() == ROLE_Authority) {
        Client_ExecuteGameplayCue(CueTag, CueParams);
    }
}

void UMythicAbilitySystemComponent::Client_ExecuteGameplayCue_Implementation(FGameplayTag CueTag, FGameplayCueParameters Parameters) {
    ExecuteGameplayCue(CueTag, Parameters);
}

bool UMythicAbilitySystemComponent::IsActivationGroupBlocked(EMythicAbilityActivationGroup Group) const {
    if (!ensureMsgf(IsValidActivationGroup(Group),
                    TEXT("IsActivationGroupBlocked: Invalid activation group [%d]."),
                    static_cast<uint8>(Group))) {
        return true;
    }

    bool bBlocked = false;

    switch (Group) {
    case EMythicAbilityActivationGroup::Independent:
        bBlocked = false;
        break;

    case EMythicAbilityActivationGroup::Exclusive_Replaceable:
    case EMythicAbilityActivationGroup::Exclusive_Blocking:
        bBlocked = (ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Blocking)] > 0);
        break;

    default:
        return true;
    }

    return bBlocked;
}

void UMythicAbilitySystemComponent::AddAbilityToActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility) {
    if (!ensureMsgf(MythicAbility, TEXT("AddAbilityToActivationGroup requires a valid Mythic ability."))
        || !ensureMsgf(IsValidActivationGroup(Group),
                       TEXT("AddAbilityToActivationGroup: Invalid activation group [%d]."),
                       static_cast<uint8>(Group))) {
        return;
    }

    int32 &GroupCount = ActivationGroupCounts[static_cast<uint8>(Group)];
    if (!ensureMsgf(GroupCount < INT32_MAX,
                    TEXT("AddAbilityToActivationGroup overflow for group [%d] and ability [%s]."),
                    static_cast<uint8>(Group), *GetNameSafe(MythicAbility))) {
        return;
    }

    ++GroupCount;

    constexpr bool bReplicateCancelAbility = false;

    switch (Group) {
    case EMythicAbilityActivationGroup::Independent:
        break;

    case EMythicAbilityActivationGroup::Exclusive_Replaceable:
    case EMythicAbilityActivationGroup::Exclusive_Blocking:
        CancelActivationGroupAbilities(EMythicAbilityActivationGroup::Exclusive_Replaceable, MythicAbility, bReplicateCancelAbility);
        break;

    default:
        return;
    }

    const int32 ExclusiveCount = ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Replaceable)] + ActivationGroupCounts[
        static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Blocking)];
    if (!ensure(ExclusiveCount <= 1)) {
        UE_LOG(Myth, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
    }
}

void UMythicAbilitySystemComponent::RemoveAbilityFromActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility) {
    if (!ensureMsgf(MythicAbility, TEXT("RemoveAbilityFromActivationGroup requires a valid Mythic ability."))
        || !ensureMsgf(IsValidActivationGroup(Group),
                       TEXT("RemoveAbilityFromActivationGroup: Invalid activation group [%d]."),
                       static_cast<uint8>(Group))) {
        return;
    }

    int32 &GroupCount = ActivationGroupCounts[static_cast<uint8>(Group)];
    if (!ensureMsgf(GroupCount > 0,
                    TEXT("RemoveAbilityFromActivationGroup underflow for group [%d] and ability [%s]."),
                    static_cast<uint8>(Group), *GetNameSafe(MythicAbility))) {
        return;
    }

    --GroupCount;
}

void UMythicAbilitySystemComponent::CancelActivationGroupAbilities(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *IgnoreMythicAbility,
                                                                   bool bReplicateCancelAbility) {
    auto ShouldCancelFunc = [this, Group, IgnoreMythicAbility](const UMythicGameplayAbility *MythicAbility, FGameplayAbilitySpecHandle Handle) {
        return ((MythicAbility->GetActivationGroup() == Group) && (MythicAbility != IgnoreMythicAbility));
    };

    CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UMythicAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility) {
    ABILITYLIST_SCOPE_LOCK();
    for (const FGameplayAbilitySpec &AbilitySpec : ActivatableAbilities.Items) {
        if (!AbilitySpec.IsActive()) {
            continue;
        }

        UMythicGameplayAbility *MythicAbilityCDO = Cast<UMythicGameplayAbility>(AbilitySpec.Ability);
        if (!MythicAbilityCDO) {
            UE_LOG(Myth, Error, TEXT("CancelAbilitiesByFunc: Non-MythicGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
            continue;
        }

        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced,
                   TEXT("CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
        PRAGMA_ENABLE_DEPRECATION_WARNINGS

        TArray<UGameplayAbility *> Instances = AbilitySpec.GetAbilityInstances();
        for (UGameplayAbility *AbilityInstance : Instances) {
            UMythicGameplayAbility *MythicAbilityInstance = CastChecked<UMythicGameplayAbility>(AbilityInstance);

            if (ShouldCancelFunc(MythicAbilityInstance, AbilitySpec.Handle)) {
                if (MythicAbilityInstance->CanBeCanceled()) {
                    MythicAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), MythicAbilityInstance->GetCurrentActivationInfo(),
                                                         bReplicateCancelAbility);
                }
                else {
                    UE_LOG(Myth, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."),
                           *MythicAbilityInstance->GetName());
                }
            }
        }
    }
}

void UMythicAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo,
                                                         FGameplayAbilityTargetDataHandle &OutTargetDataHandle) {
    TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(
        FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
    if (ReplicatedData.IsValid()) {
        OutTargetDataHandle = ReplicatedData->TargetData;
    }
}
