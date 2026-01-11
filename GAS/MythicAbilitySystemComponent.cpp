#include "MythicAbilitySystemComponent.h"

#include "Mythic.h"
#include "MythicTags_GAS.h"
#include "Settings/MythicDeveloperSettings.h"
#include "System/MythicAssetManager.h"

void UMythicAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer &AbilityTags,
                                                                           FGameplayTagContainer &OutActivationRequired,
                                                                           FGameplayTagContainer &OutActivationBlocked) const {
    if (AbilityTagRelationshipMapping) {
        AbilityTagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
    }
}

void UMythicAbilitySystemComponent::BeginPlay() {
    Super::BeginPlay();

    // Apply default tag relationship mapping from developer settings if not already set
    if (!AbilityTagRelationshipMapping) {
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            // LoadAsync handles null and already-loaded cases automatically
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

void UMythicAbilitySystemComponent::SetTagRelationshipMapping(UMythicAbilityTagRelationshipMapping *NewMapping) {
    AbilityTagRelationshipMapping = NewMapping;
}

void UMythicAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag &InputTag) {
    if (InputTag.IsValid()) {
        UE_LOG(Myth, Log, TEXT("ASC::AbilityInputTagPressed: Tag=%s, ASC=%s, ActivatableAbilities.Num=%d"),
               *InputTag.ToString(),
               *GetName(),
               ActivatableAbilities.Items.Num());

        int32 MatchCount = 0;
        for (const FGameplayAbilitySpec &AbilitySpec : ActivatableAbilities.Items) {
            if (AbilitySpec.Ability) {
                FGameplayTagContainer DynamicTags = AbilitySpec.GetDynamicSpecSourceTags();
                bool bMatches = DynamicTags.HasTagExact(InputTag);
                UE_LOG(Myth, Log, TEXT("  -> Ability: %s, DynamicTags: %s, Matches: %s"),
                       *GetNameSafe(AbilitySpec.Ability),
                       *DynamicTags.ToString(),
                       bMatches ? TEXT("YES") : TEXT("NO"));

                if (bMatches) {
                    InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
                    InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
                    MatchCount++;
                }
            }
        }
        UE_LOG(Myth, Log, TEXT("  -> Total matches for InputTag %s: %d"), *InputTag.ToString(), MatchCount);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("ASC::AbilityInputTagPressed: InputTag is NOT VALID!"));
    }
}

void UMythicAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag &InputTag) {
    if (InputTag.IsValid()) {
        UE_LOG(Myth, Log, TEXT("ASC::AbilityInputTagReleased: Tag=%s, ASC=%s"),
               *InputTag.ToString(),
               *GetName());

        int32 MatchCount = 0;
        for (const FGameplayAbilitySpec &AbilitySpec : ActivatableAbilities.Items) {
            if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))) {
                InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
                InputHeldSpecHandles.Remove(AbilitySpec.Handle);
                UE_LOG(Myth, Log, TEXT("  -> Matched Ability: %s"), *GetNameSafe(AbilitySpec.Ability));
                MatchCount++;
            }
        }
        UE_LOG(Myth, Log, TEXT("  -> Total matches for InputTag %s: %d"), *InputTag.ToString(), MatchCount);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("ASC::AbilityInputTagReleased: InputTag is NOT VALID!"));
    }
}

void UMythicAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused) {
    if (HasMatchingGameplayTag(GAS_INPUT_BLOCKED)) {
        ClearAbilityInput();
        return;
    }

    static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
    AbilitiesToActivate.Reset();

    if (InputPressedSpecHandles.Num() > 0 || InputHeldSpecHandles.Num() > 0 || InputReleasedSpecHandles.Num() > 0) {
        UE_LOG(Myth, Log, TEXT("ASC::ProcessAbilityInput: Pressed=%d, Held=%d, Released=%d"),
               InputPressedSpecHandles.Num(),
               InputHeldSpecHandles.Num(),
               InputReleasedSpecHandles.Num());
    }

    //
    // Process all abilities that activate when the input is held.
    //
    for (const FGameplayAbilitySpecHandle &SpecHandle : InputHeldSpecHandles) {
        if (const FGameplayAbilitySpec *AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability && !AbilitySpec->IsActive()) {
                const UMythicGameplayAbility *MythicAbilityCDO = Cast<UMythicGameplayAbility>(AbilitySpec->Ability);
                if (MythicAbilityCDO) {
                    UE_LOG(Myth, Log, TEXT("  -> Held ability %s, ActivationPolicy=%d (WhileInputActive=%d)"),
                           *GetNameSafe(AbilitySpec->Ability),
                           (int32)MythicAbilityCDO->GetActivationPolicy(),
                           (int32)EMythicAbilityActivationPolicy::WhileInputActive);

                    if (MythicAbilityCDO->GetActivationPolicy() == EMythicAbilityActivationPolicy::WhileInputActive) {
                        AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                    }
                }
            }
        }
    }

    //
    // Process all abilities that had their input pressed this frame.
    //
    for (const FGameplayAbilitySpecHandle &SpecHandle : InputPressedSpecHandles) {
        if (FGameplayAbilitySpec *AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability) {
                AbilitySpec->InputPressed = true;

                if (AbilitySpec->IsActive()) {
                    // Ability is active so pass along the input event.
                    UE_LOG(Myth, Log, TEXT("  -> Pressed ability %s is already ACTIVE, forwarding input"), *GetNameSafe(AbilitySpec->Ability));
                    AbilitySpecInputPressed(*AbilitySpec);
                }
                else {
                    const UMythicGameplayAbility *MythicAbilityCDO = Cast<UMythicGameplayAbility>(AbilitySpec->Ability);
                    if (MythicAbilityCDO) {
                        UE_LOG(Myth, Log, TEXT("  -> Pressed ability %s, ActivationPolicy=%d (OnInputTriggered=%d)"),
                               *GetNameSafe(AbilitySpec->Ability),
                               (int32)MythicAbilityCDO->GetActivationPolicy(),
                               (int32)EMythicAbilityActivationPolicy::OnInputTriggered);

                        if (MythicAbilityCDO->GetActivationPolicy() == EMythicAbilityActivationPolicy::OnInputTriggered) {
                            AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                        }
                    }
                    else {
                        UE_LOG(Myth, Warning, TEXT("  -> Pressed ability %s is NOT a MythicGameplayAbility!"), *GetNameSafe(AbilitySpec->Ability));
                    }
                }
            }
        }
    }

    //
    // Try to activate all the abilities that are from presses and holds.
    // We do it all at once so that held inputs don't activate the ability
    // and then also send a input event to the ability because of the press.
    //
    UE_LOG(Myth, Log, TEXT("  -> AbilitiesToActivate.Num=%d"), AbilitiesToActivate.Num());
    for (const FGameplayAbilitySpecHandle &AbilitySpecHandle : AbilitiesToActivate) {
        if (FGameplayAbilitySpec *AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle)) {
            UE_LOG(Myth, Log, TEXT("  -> Attempting to activate ability %s"), *GetNameSafe(AbilitySpec->Ability));
        }
        bool bSuccess = TryActivateAbility(AbilitySpecHandle);
        UE_LOG(Myth, Log, TEXT("  -> TryActivateAbility result: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    }

    //
    // Process all abilities that had their input released this frame.
    //
    for (const FGameplayAbilitySpecHandle &SpecHandle : InputReleasedSpecHandles) {
        if (FGameplayAbilitySpec *AbilitySpec = FindAbilitySpecFromHandle(SpecHandle)) {
            if (AbilitySpec->Ability) {
                AbilitySpec->InputPressed = false;

                if (AbilitySpec->IsActive()) {
                    // Ability is active so pass along the input event.
                    AbilitySpecInputReleased(*AbilitySpec);
                }
            }
        }
    }

    //
    // Clear the cached ability handles.
    //
    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
}

void UMythicAbilitySystemComponent::ClearAbilityInput() {
    InputPressedSpecHandles.Reset();
    InputReleasedSpecHandles.Reset();
    InputHeldSpecHandles.Reset();
}

const TArray<UMythicAttributeSet *> &UMythicAbilitySystemComponent::GetAttributeSets() const {
    return reinterpret_cast<const TArray<UMythicAttributeSet *> &>(this->GetSpawnedAttributes());
}

bool UMythicAbilitySystemComponent::IsActivationGroupBlocked(EMythicAbilityActivationGroup Group) const {
    bool bBlocked = false;

    switch (Group) {
    case EMythicAbilityActivationGroup::Independent:
        // Independent abilities are never blocked.
        bBlocked = false;
        break;

    case EMythicAbilityActivationGroup::Exclusive_Replaceable:
    case EMythicAbilityActivationGroup::Exclusive_Blocking:
        // Exclusive abilities can activate if nothing is blocking.
        bBlocked = (ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Blocking)] > 0);
        break;

    default:
        checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
        break;
    }

    return bBlocked;
}

void UMythicAbilitySystemComponent::AddAbilityToActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility) {
    check(MythicAbility);
    check(ActivationGroupCounts[static_cast<uint8>(Group)] < INT32_MAX);

    ActivationGroupCounts[static_cast<uint8>(Group)]++;

    constexpr bool bReplicateCancelAbility = false;

    switch (Group) {
    case EMythicAbilityActivationGroup::Independent:
        // Independent abilities do not cancel any other abilities.
        break;

    case EMythicAbilityActivationGroup::Exclusive_Replaceable:
    case EMythicAbilityActivationGroup::Exclusive_Blocking:
        CancelActivationGroupAbilities(EMythicAbilityActivationGroup::Exclusive_Replaceable, MythicAbility, bReplicateCancelAbility);
        break;

    default:
        checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
        break;
    }

    const int32 ExclusiveCount = ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Replaceable)] + ActivationGroupCounts[
        static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Blocking)];
    if (!ensure(ExclusiveCount <= 1)) {
        UE_LOG(Myth, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
    }
}

void UMythicAbilitySystemComponent::RemoveAbilityFromActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility) {
    check(MythicAbility);
    check(ActivationGroupCounts[static_cast<uint8>(Group)] > 0);

    ActivationGroupCounts[static_cast<uint8>(Group)]--;
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

        // Cancel all the spawned instances.
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
