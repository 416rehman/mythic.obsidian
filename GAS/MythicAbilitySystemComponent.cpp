#include "MythicAbilitySystemComponent.h"

#include "Mythic.h"
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
