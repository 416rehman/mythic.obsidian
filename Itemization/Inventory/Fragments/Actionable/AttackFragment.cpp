

#include "AttackFragment.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifyQueue.h"
#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/Passive/HarvestToolFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Input/MythicTags_Input.h"

#if WITH_EDITOR
bool UAttackFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!InputTag.MatchesTagExact(INPUT_ACTION_ATTACK)) {
        OutErrorMessage = FText::FromString(
            "AttackFragment: InputTag must be the canonical Input.Action.Attack; exact readied-source routing owns activation");
        return false;
    }

    if (!this->AttackConfig.TriggerAbility) {
        OutErrorMessage = FText::FromString("AttackFragment: TriggerAbility is not set");
        return false;
    }

    if (AttackConfig.TriggerAbility->HasAnyClassFlags(
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) {
        OutErrorMessage = FText::FromString(
            "AttackFragment: TriggerAbility must be a concrete, current MythicWeaponAttackAbility class");
        return false;
    }

    const UMythicWeaponAttackAbility *AbilityCDO =
        AttackConfig.TriggerAbility->GetDefaultObject<UMythicWeaponAttackAbility>();
    if (!AbilityCDO
        || !AbilityCDO->IsCanonicalWeaponAttackConfiguration(&OutErrorMessage)) {
        if (OutErrorMessage.IsEmpty()) {
            OutErrorMessage = FText::FromString(
                "AttackFragment: TriggerAbility has no valid canonical weapon-attack CDO");
        }
        return false;
    }

    if (!this->AttackConfig.AttackMontage) {
        OutErrorMessage = FText::FromString("AttackFragment: AttackMontage is not set");
        return false;
    }

    if (!IsAttackMontageContractValid(AttackConfig.AttackMontage,
                                      &OutErrorMessage)) {
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif

bool UAttackFragment::ShouldBindAbilityToGenericInput(
    const UMythicItemInstance * /*ItemInstance*/) {
    // A harvesting tool may not carry an Attack Fragment, so the equipped weapon is the only item that grants an
    // attack. With nothing to disambiguate, its ability binds the attack input directly.
    return true;
}

bool UAttackFragment::HasCanonicalMontageRateScale(
    const UAnimMontage *AttackMontage) {
    return AttackMontage
        && FMath::IsFinite(AttackMontage->RateScale)
        && AttackMontage->RateScale == 1.0f;
}

float UAttackFragment::GetNominalAttackCycleDuration(const UAnimMontage *AttackMontage) {
    if (!HasCanonicalMontageRateScale(AttackMontage)) {
        return 0.0f;
    }

    const int32 SectionCount = AttackMontage ? AttackMontage->GetNumSections() : 0;
    if (SectionCount <= 0) {
        return 0.0f;
    }

    double TotalDuration = 0.0;
    for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex) {
        const float SectionDuration = AttackMontage->GetSectionLength(SectionIndex);
        if (!FMath::IsFinite(SectionDuration) || SectionDuration <= KINDA_SMALL_NUMBER
            || AttackMontage->GetSectionName(SectionIndex).IsNone()
            || !AttackMontage->GetAnimCompositeSection(SectionIndex).NextSectionName.IsNone()) {
            return 0.0f;
        }
        TotalDuration += SectionDuration;
    }

    const double MeanDuration = TotalDuration / static_cast<double>(SectionCount);
    return FMath::IsFinite(MeanDuration) ? static_cast<float>(MeanDuration) : 0.0f;
}

bool UAttackFragment::IsAttackMontageContractValid(
    const UAnimMontage *AttackMontage, FText *OutError) {
    if (!AttackMontage) {
        if (OutError) {
            *OutError = NSLOCTEXT(
                "AttackFragment", "MissingAttackMontage",
                "AttackFragment: AttackMontage is not set");
        }
        return false;
    }
    if (!HasCanonicalMontageRateScale(AttackMontage)) {
        if (OutError) {
            *OutError = NSLOCTEXT(
                "AttackFragment", "InvalidMontageRateScale",
                "AttackFragment: AttackMontage global Rate Scale must be finite and exactly 1.0; GAS AttackSpeed exclusively owns weapon cadence");
        }
        return false;
    }
    if (GetNominalAttackCycleDuration(AttackMontage) <= KINDA_SMALL_NUMBER) {
        if (OutError) {
            *OutError = FText::FromString(
                "AttackFragment: AttackMontage must contain one or more named, non-empty, standalone sections; each section is one complete attack variant");
        }
        return false;
    }

    for (int32 SectionIndex = 0;
         SectionIndex < AttackMontage->GetNumSections(); ++SectionIndex) {
        TArray<const UMythicAnimNotify_SphereOverlap *> HitSamples;
        FText SectionError;
        if (FindCanonicalHitNotifiesForSection(
                AttackMontage, AttackMontage->GetSectionName(SectionIndex),
                HitSamples, &SectionError)
            && !HitSamples.IsEmpty()) {
            continue;
        }
        if (OutError) {
            *OutError = SectionError.IsEmpty()
                ? FText::Format(
                    NSLOCTEXT(
                        "AttackFragment", "MissingHitSamples",
                        "AttackFragment: montage section '{0}' must contain at least one usable Mythic Sphere Overlap temporal sample sending GAS.Event.Hitbox"),
                    FText::FromName(AttackMontage->GetSectionName(SectionIndex)))
                : SectionError;
        }
        return false;
    }
    return true;
}

bool UAttackFragment::IsCanonicalHitSampleUsable(
    const UMythicAnimNotify_SphereOverlap *HitSample, FText *OutError) {
    auto Reject = [OutError](const FText &Error) {
        if (OutError) {
            *OutError = Error;
        }
        return false;
    };

    if (!HitSample) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "NullHitSample",
            "AttackFragment: a canonical temporal hit sample is null"));
    }
    if (HitSample->GetClass()
        != UMythicAnimNotify_SphereOverlap::StaticClass()) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "SubclassedHitSample",
            "AttackFragment: every temporal hit sample must use the exact sealed native Mythic Sphere Overlap class"));
    }
    if (HitSample->SendToEventWithTag != GAS_EVENT_HITBOX) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "InvalidHitSampleTag",
            "AttackFragment: every Mythic Sphere Overlap temporal sample must send the exact GAS.Event.Hitbox tag"));
    }
    if (!FMath::IsFinite(HitSample->HitboxRadius)
        || HitSample->HitboxRadius <= 0.0f) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "InvalidHitSampleRadius",
            "AttackFragment: every Mythic Sphere Overlap temporal sample requires a finite radius greater than zero"));
    }
    if (HitSample->HitboxLocationOffset.ContainsNaN()) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "InvalidHitSampleOffset",
            "AttackFragment: every Mythic Sphere Overlap temporal sample requires a finite local-space offset"));
    }
    if (HitSample->MaxTargets < 0) {
        return Reject(NSLOCTEXT(
            "AttackFragment", "InvalidHitSampleCap",
            "AttackFragment: every Mythic Sphere Overlap temporal sample requires Max Targets to be zero or greater"));
    }
    return true;
}

bool UAttackFragment::FindCanonicalHitNotifiesForSection(
    const UAnimMontage *AttackMontage, const FName SectionName,
    TArray<const UMythicAnimNotify_SphereOverlap *> &OutNotifies,
    FText *OutError) {
    OutNotifies.Reset();
    const int32 SectionIndex = AttackMontage
        ? AttackMontage->GetSectionIndex(SectionName)
        : INDEX_NONE;
    if (!AttackMontage || SectionIndex == INDEX_NONE) {
        if (OutError) {
            *OutError = FText::Format(
                NSLOCTEXT("AttackFragment", "UnknownHitSampleSection",
                          "AttackFragment: montage section '{0}' does not exist"),
                FText::FromName(SectionName));
        }
        return false;
    }

    float SectionStart = 0.0f;
    float SectionEnd = 0.0f;
    AttackMontage->GetSectionStartAndEndTime(SectionIndex, SectionStart,
                                             SectionEnd);
    FAnimNotifyContext NotifyContext;
    AttackMontage->GetAnimNotifiesFromDeltaPositions(SectionStart, SectionEnd,
                                                      NotifyContext);
    for (const FSlotAnimationTrack &SlotTrack : AttackMontage->SlotAnimTracks) {
        SlotTrack.AnimTrack.GetAnimNotifiesFromTrackPositions(
            SectionStart, SectionEnd, NotifyContext);
    }

    TSet<const UMythicAnimNotify_SphereOverlap *> SeenNotifies;
    for (const FAnimNotifyEventReference &Reference : NotifyContext.ActiveNotifies) {
        const FAnimNotifyEvent *NotifyEvent = Reference.GetNotify();
        const UMythicAnimNotify_SphereOverlap *Candidate = NotifyEvent
            ? Cast<UMythicAnimNotify_SphereOverlap>(NotifyEvent->Notify.Get())
            : nullptr;
        if (!Candidate || SeenNotifies.Contains(Candidate)) {
            continue;
        }
        SeenNotifies.Add(Candidate);

        FText SampleError;
        if (!IsCanonicalHitSampleUsable(Candidate, &SampleError)) {
            OutNotifies.Reset();
            if (OutError) {
                *OutError = FText::Format(
                    NSLOCTEXT(
                        "AttackFragment", "InvalidSectionHitSample",
                        "AttackFragment: montage section '{0}' contains an unusable Mythic Sphere Overlap temporal sample: {1}"),
                    FText::FromName(SectionName), SampleError);
            }
            return false;
        }
        OutNotifies.Add(Candidate);
    }
    return !OutNotifies.IsEmpty();
}

bool UAttackFragment::ResolveRuntimeAttackContract(FText *OutError) const {
    UAnimMontage *AttackMontage = AttackConfig.AttackMontage;
    const float MontageRateScale = AttackMontage
        ? AttackMontage->RateScale
        : 0.0f;
#if !WITH_EDITOR
    if (RuntimeContractCache.bBuilt
        && RuntimeContractCache.Montage.Get() == AttackMontage
        && RuntimeContractCache.ValidatedMontageRateScale == MontageRateScale) {
        if (OutError && !RuntimeContractCache.bValid) {
            *OutError = RuntimeContractCache.Error;
        }
        return RuntimeContractCache.bValid;
    }
#endif

    RuntimeContractCache = FAttackRuntimeContractCache();
    RuntimeContractCache.bBuilt = true;
    RuntimeContractCache.Montage = AttackMontage;
    RuntimeContractCache.ValidatedMontageRateScale = MontageRateScale;

    const int32 SectionCount = AttackMontage ? AttackMontage->GetNumSections() : 0;
    if (!AttackMontage) {
        RuntimeContractCache.Error = FText::FromString(
            "AttackFragment: AttackMontage is not set");
    }
    else if (!HasCanonicalMontageRateScale(AttackMontage)) {
        RuntimeContractCache.Error = NSLOCTEXT(
            "AttackFragment", "InvalidRuntimeMontageRateScale",
            "AttackFragment: live AttackMontage global Rate Scale must be finite and exactly 1.0; GAS AttackSpeed exclusively owns weapon cadence");
    }
    else if (SectionCount <= 0) {
        RuntimeContractCache.Error = FText::FromString(
            "AttackFragment: AttackMontage must contain at least one attack section");
    }
    else {
        double TotalDuration = 0.0;
        for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex) {
            const FName SectionName = AttackMontage->GetSectionName(SectionIndex);
            const float SectionDuration = AttackMontage->GetSectionLength(SectionIndex);
            const bool bStandalone =
                AttackMontage->GetAnimCompositeSection(SectionIndex).NextSectionName.IsNone();
            if (SectionName.IsNone() || !FMath::IsFinite(SectionDuration)
                || SectionDuration <= KINDA_SMALL_NUMBER || !bStandalone) {
                RuntimeContractCache.Error = FText::Format(
                    NSLOCTEXT("AttackFragment", "InvalidRuntimeSection",
                              "AttackFragment: montage section index {0} must be named, non-empty, and standalone"),
                    FText::AsNumber(SectionIndex));
                break;
            }

            TArray<const UMythicAnimNotify_SphereOverlap *> HitSamples;
            FText SampleError;
            if (!FindCanonicalHitNotifiesForSection(
                    AttackMontage, SectionName, HitSamples, &SampleError)
                || HitSamples.IsEmpty()) {
                RuntimeContractCache.Error = SampleError.IsEmpty()
                    ? FText::Format(
                        NSLOCTEXT("AttackFragment", "MissingRuntimeHitSamples",
                                  "AttackFragment: montage section '{0}' has no usable authorized GAS.Event.Hitbox temporal samples"),
                        FText::FromName(SectionName))
                    : SampleError;
                break;
            }

            FAttackRuntimeSectionDescriptor &Descriptor =
                RuntimeContractCache.Sections.AddDefaulted_GetRef();
            Descriptor.SectionName = SectionName;
            Descriptor.DurationSeconds = SectionDuration;
            for (const UMythicAnimNotify_SphereOverlap *HitSample : HitSamples) {
                Descriptor.AuthorizedHitSamples.Add(
                    const_cast<UMythicAnimNotify_SphereOverlap *>(HitSample));
            }
            RuntimeContractCache.SectionIndexByName.Add(
                SectionName, RuntimeContractCache.Sections.Num() - 1);
            TotalDuration += SectionDuration;
        }

        if (RuntimeContractCache.Error.IsEmpty()
            && RuntimeContractCache.Sections.Num() == SectionCount) {
            const double MeanDuration = TotalDuration / static_cast<double>(SectionCount);
            if (FMath::IsFinite(MeanDuration)
                && MeanDuration > static_cast<double>(KINDA_SMALL_NUMBER)) {
                RuntimeContractCache.NominalCycleDurationSeconds =
                    static_cast<float>(MeanDuration);
                RuntimeContractCache.bValid = true;
            }
            else {
                RuntimeContractCache.Error = FText::FromString(
                    "AttackFragment: montage has no finite positive nominal attack cycle");
            }
        }
    }

    if (OutError && !RuntimeContractCache.bValid) {
        *OutError = RuntimeContractCache.Error;
    }
    return RuntimeContractCache.bValid;
}

float UAttackFragment::GetRuntimeNominalAttackCycleDuration() const {
    return ResolveRuntimeAttackContract()
        ? RuntimeContractCache.NominalCycleDurationSeconds
        : 0.0f;
}

bool UAttackFragment::GetRuntimeAttackSectionNames(
    TArray<FName> &OutSectionNames, FText *OutError) const {
    OutSectionNames.Reset();
    if (!ResolveRuntimeAttackContract(OutError)) {
        return false;
    }
    OutSectionNames.Reserve(RuntimeContractCache.Sections.Num());
    for (const FAttackRuntimeSectionDescriptor &Section :
         RuntimeContractCache.Sections) {
        OutSectionNames.Add(Section.SectionName);
    }
    return !OutSectionNames.IsEmpty();
}

bool UAttackFragment::GetRuntimeAuthorizedHitSamples(
    const FName SectionName,
    TArray<const UMythicAnimNotify_SphereOverlap *> &OutSamples,
    FText *OutError) const {
    OutSamples.Reset();
    if (!ResolveRuntimeAttackContract(OutError)) {
        return false;
    }
    const int32 *SectionIndex =
        RuntimeContractCache.SectionIndexByName.Find(SectionName);
    if (!SectionIndex || !RuntimeContractCache.Sections.IsValidIndex(*SectionIndex)) {
        if (OutError) {
            *OutError = FText::Format(
                NSLOCTEXT("AttackFragment", "UnknownRuntimeSection",
                          "AttackFragment: selected montage section '{0}' is not in the compiled attack contract"),
                FText::FromName(SectionName));
        }
        return false;
    }
    for (const TWeakObjectPtr<UMythicAnimNotify_SphereOverlap> &Sample :
         RuntimeContractCache.Sections[*SectionIndex].AuthorizedHitSamples) {
        if (const UMythicAnimNotify_SphereOverlap *Resolved = Sample.Get()) {
            OutSamples.Add(Resolved);
        }
    }
    if (OutSamples.IsEmpty() && OutError) {
        *OutError = FText::Format(
            NSLOCTEXT("AttackFragment", "ExpiredRuntimeSamples",
                      "AttackFragment: selected montage section '{0}' has no live authorized hit samples"),
            FText::FromName(SectionName));
    }
    return !OutSamples.IsEmpty();
}

void UAttackFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemActivated: Fragment=%s, Item=%s, InputTag=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance),
           *InputTag.ToString());

    FAttackRuntimeServerOnlyData &Runtime = AttackRuntimeServerOnlyData;
    UMythicAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();

    // A fragment must never leave an ability on a former owner if inventory authority transfers it while active.
    if (Runtime.ASC.Get() != ASC) {
        if (Runtime.ASC && Runtime.AbilityHandle.IsValid()) {
            Runtime.ASC->ClearAbility(Runtime.AbilityHandle);
        }
        Runtime.AbilityHandle = FGameplayAbilitySpecHandle();
    }
    Runtime.ASC = ASC;

    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: ASC is null! Cannot grant ability."));
        return;
    }

    UE_LOG(Myth, Log, TEXT("  -> Got ASC: %s (Owner: %s)"),
           *GetNameSafe(ASC),
           *GetNameSafe(ASC->GetOwnerActor()));

    if (ShouldGrantAttackAbility(Runtime.AbilityHandle.IsValid())) {
        UE_LOG(Myth, Log, TEXT("  -> Granting ability %s with InputTag %s"), *GetNameSafe(AttackConfig.TriggerAbility), *InputTag.ToString());
        Runtime.AbilityHandle = GrantItemAbility(
            ASC, ItemInstance, AttackConfig.TriggerAbility,
            ShouldBindAbilityToGenericInput(ItemInstance));

        if (!Runtime.AbilityHandle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: Failed to grant attack ability %s"), *GetNameSafe(AttackConfig.TriggerAbility));
        }
        else {
            UE_LOG(Myth, Log, TEXT("  -> SUCCESS! Granted ability"));

            if (FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(Runtime.AbilityHandle)) {
                FGameplayTagContainer DynamicTags = Spec->GetDynamicSpecSourceTags();
                UE_LOG(Myth, Log, TEXT("  -> Granted ability DynamicSpecSourceTags: %s"), *DynamicTags.ToString());
            }
        }
    }
    else {
        UE_LOG(Myth, Log, TEXT("  -> Attack ability already live, skipping grant"));
    }

}

void UAttackFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemDeactivated: Fragment=%s, Item=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance));

    FAttackRuntimeServerOnlyData &Runtime = AttackRuntimeServerOnlyData;
    UMythicAbilitySystemComponent *ASC = Runtime.ASC.Get();
    if (ASC && Runtime.AbilityHandle.IsValid()) {
        ASC->ClearAbility(Runtime.AbilityHandle);
        UE_LOG(Myth, Log, TEXT("  -> Cleared ability"));
    }
    else if (!ASC && Runtime.AbilityHandle.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("  -> Attack ability handle was live without a cached ASC; clearing stale bookkeeping"));
    }

    Runtime.AbilityHandle = FGameplayAbilitySpecHandle();
    Runtime.ASC = nullptr;
}


bool UAttackFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherFragment = Cast<UAttackFragment>(Other);
    if (!OtherFragment) {
        return false;
    }

    const FAttackConfig &OtherConfig = OtherFragment->AttackConfig;

    return Super::CanBeStackedWith(Other) &&
        AttackConfig.TriggerAbility == OtherConfig.TriggerAbility &&
        AttackConfig.AttackMontage == OtherConfig.AttackMontage;
}
