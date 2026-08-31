#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicContextActions, Log, All);

void FMythicReplicatedContextActionGrantArray::PreReplicatedRemove(
    const TArrayView<int32> &RemovedIndices, const int32 FinalSize) {
    (void)RemovedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedContextActionGrantArray::PostReplicatedAdd(
    const TArrayView<int32> &AddedIndices, const int32 FinalSize) {
    (void)AddedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedContextActionGrantArray::PostReplicatedChange(
    const TArrayView<int32> &ChangedIndices, const int32 FinalSize) {
    (void)ChangedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

UMythicEntityActionGrantComponent::UMythicEntityActionGrantComponent() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    ReplicatedGrants.SetOwner(this);
}

void UMythicEntityActionGrantComponent::BeginPlay() {
    Super::BeginPlay();
    ReplicatedGrants.SetOwner(this);
    if (IsAuthority()) {
        EnsurePresentationRegistryBinding();
        ScheduleAuthorityExpiryTimer();
    }
}

void UMythicEntityActionGrantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ExpiryTimerHandle);
    }
    RemovePresentationRegistryBinding();
    AuthorityGrantLedger.Reset();
    ReplicatedGrants.SetOwner(nullptr);
    Super::EndPlay(EndPlayReason);
}

void UMythicEntityActionGrantComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicEntityActionGrantComponent, ReplicatedGrants, COND_OwnerOnly);
}

TArray<FMythicReplicatedContextActionGrant>
UMythicEntityActionGrantComponent::GetActionGrantsForSubject(
    const FMythicEntityPresentationInstance Subject) const {
    TArray<FMythicReplicatedContextActionGrant> Result;
    GatherCurrentActionGrantsForSubject(Subject, Result);
    return Result;
}

void UMythicEntityActionGrantComponent::GatherCurrentActionGrantsForSubject(
    const FMythicEntityPresentationInstance &Subject,
    TArray<FMythicReplicatedContextActionGrant> &OutGrants) const {
    OutGrants.Reset();
    if (!Subject.IsValid()) {
        return;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    for (const FMythicReplicatedContextActionGrant &Grant : ReplicatedGrants.Items) {
        if (Grant.Subject == Subject && !Grant.IsExpired(Now)) {
            OutGrants.Add(Grant);
        }
    }
}

bool UMythicEntityActionGrantComponent::FindActionGrant(
    const FMythicEntityPresentationInstance Subject, const FGameplayTag ActionTag,
    FMythicReplicatedContextActionGrant &OutGrant) const {
    OutGrant = FMythicReplicatedContextActionGrant();
    if (!Subject.IsValid() || !IsSafeActionTag(ActionTag)) {
        return false;
    }

    const FMythicReplicatedContextActionGrant *Found =
        ReplicatedGrants.Items.FindByPredicate(
            [&Subject, &ActionTag](const FMythicReplicatedContextActionGrant &Grant) {
                return Grant.Matches(Subject, ActionTag);
            });
    if (!Found || Found->IsExpired(GetSynchronizedServerTimeSeconds())) {
        return false;
    }

    OutGrant = *Found;
    return true;
}

bool UMythicEntityActionGrantComponent::AuthorityReplaceBoundContextActionOffers(
    const FMythicEntityPresentationInstance Subject, AActor *SubjectActor,
    const TArray<FMythicAuthorityContextActionOffer> &Offers,
    const int32 MaximumOffers, const double ExpiryServerTimeSeconds) {
    struct FDesiredBoundGrant {
        FGameplayTag ActionTag;
        EMythicContextActionGrantState State =
            EMythicContextActionGrantState::UnavailableWithReason;
        FGameplayTag UnavailableReasonTag;
        TWeakObjectPtr<UObject> Provider;
        TWeakObjectPtr<UMythicContextActionDefinition> Definition;
        FMythicAuthorityContextActionDefinitionSignature
            DefinitionSignature;
        uint32 ProviderSourceRevision = 0;
        int32 PresentationPriority = 0;
    };

    if (!IsAuthority() || !Subject.IsValid()) {
        return false;
    }
    if (!IsValid(SubjectActor) || SubjectActor->GetWorld() != GetWorld()
        || !FMath::IsFinite(ExpiryServerTimeSeconds)
        || ExpiryServerTimeSeconds < 0.0 || MaximumOffers < 0) {
        AuthorityRevokeSubjectGrants(Subject);
        return false;
    }

    EnsurePresentationRegistryBinding();
    UMythicEntityPresentationRegistry *Registry =
        BoundPresentationRegistry.Get();
    UMythicEntityPresentationComponent *Presentation = Registry
        ? Registry->ResolvePresentationComponent(Subject) : nullptr;
    if (!Registry || !Presentation || Presentation->GetOwner() != SubjectActor) {
        AuthorityRevokeSubjectGrants(Subject);
        return false;
    }

    const int32 SafeMaximum = FMath::Clamp(
        MaximumOffers, 0, MaximumReplicatedGrantsPerSubject);
    const double Now = GetSynchronizedServerTimeSeconds();
    const bool bLeaseAlreadyExpired =
        ExpiryServerTimeSeconds > 0.0
        && ExpiryServerTimeSeconds <= Now;

    const int32 MaximumInputOffers =
        (FMythicContextActionProjectionRules::HardMaximumProviderComponents
         + 1)
        * FMythicContextActionProjectionRules::HardMaximumOffersPerProvider;
    if (Offers.Num() > MaximumInputOffers) {
        UE_LOG(LogMythicContextActions, Warning,
               TEXT("Rejected %d provider-bound context-action rows because the authority input bound is %d."),
               Offers.Num(), MaximumInputOffers);
        AuthorityRevokeSubjectGrants(Subject);
        return false;
    }

    TMap<FGameplayTag, FDesiredBoundGrant> DesiredByTag;
    TSet<FGameplayTag> ClaimedTags;
    TSet<FGameplayTag> AmbiguousTags;
    if (!bLeaseAlreadyExpired && SafeMaximum > 0) {
        for (int32 OfferIndex = 0; OfferIndex < Offers.Num();
             ++OfferIndex) {
            const FMythicAuthorityContextActionOffer &BoundOffer =
                Offers[OfferIndex];
            UObject *Provider = BoundOffer.Provider.Get();
            UMythicContextActionDefinition *Definition =
                BoundOffer.Offer.Definition;
            const FMythicContextActionOffer &Offer = BoundOffer.Offer;
            if (!IsValid(Provider) || !IsValid(Definition)
                || !Provider->GetClass()->ImplementsInterface(
                    UMythicContextActionProvider::StaticClass())
                || !DoesProviderBelongToSubject(Provider, SubjectActor)) {
                continue;
            }

            const FGameplayTag ActionTag = Offer.GetActionTag();
            if (!IsSafeActionTag(ActionTag)) {
                continue;
            }

            // A structurally identifiable row claims its tag before availability and the remaining authored data are
            // trusted. Hidden or malformed duplicates must revoke the whole tag, independent of provider/order.
            if (AmbiguousTags.Contains(ActionTag)) {
                continue;
            }
            if (ClaimedTags.Contains(ActionTag)) {
                DesiredByTag.Remove(ActionTag);
                AmbiguousTags.Add(ActionTag);
                UE_LOG(LogMythicContextActions, Warning,
                       TEXT("Revoked ambiguous context action %s because more than one authority provider row claimed the same tag."),
                       *ActionTag.ToString());
                continue;
            }
            ClaimedTags.Add(ActionTag);

            if (Offer.SourceRevision < 0
                || Offer.SourceRevision > static_cast<int64>(MAX_uint32)
                || Offer.Availability
                       < EMythicContextActionAvailability::Hidden
                || Offer.Availability
                       > EMythicContextActionAvailability::UnavailableWithReason
                || Definition->FocusPolicy
                       < EMythicContextActionFocusPolicy::NotRequired
                || Definition->FocusPolicy
                       > EMythicContextActionFocusPolicy::LockedSubject
                || Definition->RangePolicy
                       < EMythicContextActionRangePolicy::NotRequired
                || Definition->RangePolicy
                       > EMythicContextActionRangePolicy::DefinitionRange
                || Definition->LineOfSightPolicy
                       < EMythicContextActionLineOfSightPolicy::NotRequired
                || Definition->LineOfSightPolicy
                       > EMythicContextActionLineOfSightPolicy::ViewerInteractionOriginToSubject
                || Definition->PresentationSemantic
                       < EMythicContextActionPresentationSemantic::Other
                || Definition->PresentationSemantic
                       > EMythicContextActionPresentationSemantic::Assist
                || Definition->WorldPresentationPolicy
                       < EMythicContextActionWorldPresentationPolicy::FocusOnly
                || Definition->WorldPresentationPolicy
                       > EMythicContextActionWorldPresentationPolicy::ContextWhenAvailable
                || Definition->PresentationPriority < -1000
                || Definition->PresentationPriority > 1000
                || !FMath::IsFinite(Definition->HoldDurationSeconds)
                || !FMath::IsFinite(Definition->MaximumFocusAngleDegrees)
                || !FMath::IsFinite(Definition->MaximumRangeCentimeters)
                || !FMythicContextActionProjectionRules::IsHoldDurationValid(
                    Definition->HoldDurationSeconds)
                || Definition->MaximumFocusAngleDegrees <= 0.0f
                || Definition->MaximumFocusAngleDegrees > 90.0f
                || Definition->MaximumRangeCentimeters < 0.0f
                || (Definition->RangePolicy
                        == EMythicContextActionRangePolicy::DefinitionRange
                    && Definition->MaximumRangeCentimeters <= 0.0f)
                || Offer.Availability
                       == EMythicContextActionAvailability::Hidden
                || (Offer.Availability
                        == EMythicContextActionAvailability::UnavailableWithReason
                    && !Definition->bExplainWhenUnavailable)) {
                continue;
            }

            FDesiredBoundGrant Candidate;
            Candidate.ActionTag = ActionTag;
            Candidate.State =
                Offer.Availability == EMythicContextActionAvailability::Available
                    ? EMythicContextActionGrantState::Available
                    : EMythicContextActionGrantState::UnavailableWithReason;
            Candidate.UnavailableReasonTag =
                Candidate.State
                        == EMythicContextActionGrantState::UnavailableWithReason
                    ? SanitizeReasonTag(Offer.UnavailableReasonTag)
                    : FGameplayTag();
            Candidate.Provider = Provider;
            Candidate.Definition = Definition;
            Candidate.DefinitionSignature =
                FMythicAuthorityContextActionDefinitionSignature::Capture(
                    *Definition);
            Candidate.ProviderSourceRevision =
                static_cast<uint32>(Offer.SourceRevision);
            Candidate.PresentationPriority = Definition->PresentationPriority;
            DesiredByTag.Add(ActionTag, MoveTemp(Candidate));
        }
    }

    TArray<FDesiredBoundGrant> Desired;
    DesiredByTag.GenerateValueArray(Desired);
    Desired.Sort([](const FDesiredBoundGrant &Left,
                    const FDesiredBoundGrant &Right) {
        if (Left.PresentationPriority != Right.PresentationPriority) {
            return Left.PresentationPriority > Right.PresentationPriority;
        }
        return Left.ActionTag.GetTagName().LexicalLess(
            Right.ActionTag.GetTagName());
    });
    if (Desired.Num() > SafeMaximum) {
        Desired.SetNum(SafeMaximum, EAllowShrinking::No);
    }

    TSet<FGameplayTag> CandidateTags;
    for (const FDesiredBoundGrant &Entry : Desired) {
        CandidateTags.Add(Entry.ActionTag);
    }

    bool bChanged = RemoveGrantsByPredicate(
                        [&Subject, &CandidateTags](
                            const FMythicReplicatedContextActionGrant &Grant) {
                            return Grant.Subject == Subject
                                && !CandidateTags.Contains(Grant.ActionTag);
                        })
                    > 0;
    TSet<FAuthorityGrantLedgerKey> InstalledKeys;
    TSet<FGameplayTag> InstalledTags;
    for (const FDesiredBoundGrant &Entry : Desired) {
        const FMythicReplicatedContextActionGrant *ExistingGrant =
            ReplicatedGrants.Items.FindByPredicate(
                [&Subject, &Entry](
                    const FMythicReplicatedContextActionGrant &Grant) {
                    return Grant.Matches(Subject, Entry.ActionTag);
                });

        uint32 GrantNonce = 0;
        if (ExistingGrant && ExistingGrant->OfferRevision != 0
            && ExistingGrant->State == Entry.State
            && ExistingGrant->UnavailableReasonTag
                   == Entry.UnavailableReasonTag
            && !ExistingGrant->IsExpired(Now)) {
            const FAuthorityGrantLedgerKey ExistingKey{
                Subject, Entry.ActionTag, ExistingGrant->OfferRevision};
            const FAuthorityGrantLedgerEntry *ExistingLedger =
                AuthorityGrantLedger.Find(ExistingKey);
            if (ExistingLedger
                && ExistingLedger->SubjectActor.Get() == SubjectActor
                && ExistingLedger->Provider == Entry.Provider
                && ExistingLedger->Definition == Entry.Definition
                && ExistingLedger->DefinitionSignature
                       == Entry.DefinitionSignature
                && ExistingLedger->ProviderSourceRevision
                       == Entry.ProviderSourceRevision
                && ExistingLedger->ExpiryServerTimeSeconds
                       == ExistingGrant->ExpiryServerTimeSeconds
                && (ExistingLedger->ExpiryServerTimeSeconds <= 0.0
                    || ExistingLedger->ExpiryServerTimeSeconds > Now)) {
                GrantNonce = ExistingGrant->OfferRevision;
            }
        }
        if (GrantNonce == 0) {
            GrantNonce = AllocateAuthorityGrantNonce();
        }
        if (GrantNonce == 0) {
            UE_LOG(LogMythicContextActions, Error,
                   TEXT("Context-action grant nonce space exhausted; action %s failed closed."),
                   *Entry.ActionTag.ToString());
            continue;
        }

        FMythicReplicatedContextActionGrant Grant;
        Grant.Subject = Subject;
        Grant.ActionTag = Entry.ActionTag;
        Grant.State = Entry.State;
        Grant.UnavailableReasonTag = Entry.UnavailableReasonTag;
        Grant.OfferRevision = GrantNonce;
        Grant.ExpiryServerTimeSeconds = ExpiryServerTimeSeconds;
        const bool bGrantChanged = SetGrantInternal(Grant);
        const FMythicReplicatedContextActionGrant *InstalledGrant =
            ReplicatedGrants.Items.FindByPredicate(
                [&Subject, &Entry, GrantNonce](
                    const FMythicReplicatedContextActionGrant &Candidate) {
                    return Candidate.Matches(Subject, Entry.ActionTag)
                        && Candidate.OfferRevision == GrantNonce;
                });
        if (!InstalledGrant) {
            continue;
        }

        const FAuthorityGrantLedgerKey NewKey{
            Subject, Entry.ActionTag, GrantNonce};
        FAuthorityGrantLedgerEntry NewLedger;
        NewLedger.SubjectActor = SubjectActor;
        NewLedger.Provider = Entry.Provider;
        NewLedger.Definition = Entry.Definition;
        NewLedger.DefinitionSignature = Entry.DefinitionSignature;
        NewLedger.ProviderSourceRevision = Entry.ProviderSourceRevision;
        NewLedger.ExpiryServerTimeSeconds = ExpiryServerTimeSeconds;
        AuthorityGrantLedger.Add(NewKey, MoveTemp(NewLedger));
        InstalledKeys.Add(NewKey);
        InstalledTags.Add(Entry.ActionTag);
        bChanged |= bGrantChanged;
    }

    bChanged |= RemoveGrantsByPredicate(
                    [&Subject, &InstalledTags](
                        const FMythicReplicatedContextActionGrant &Grant) {
                        return Grant.Subject == Subject
                            && !InstalledTags.Contains(Grant.ActionTag);
                    })
                > 0;
    RemoveAuthorityLedgerEntriesByPredicate(
        [&Subject, &InstalledKeys](const FAuthorityGrantLedgerKey &Key,
                                   const FAuthorityGrantLedgerEntry &) {
            return Key.Subject == Subject && !InstalledKeys.Contains(Key);
        });

    if (bChanged) {
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
    }
    return bChanged;
}

bool UMythicEntityActionGrantComponent::AuthorityResolveActionGrantBinding(
    const FMythicEntityPresentationInstance Subject,
    const FGameplayTag ActionTag, const uint32 GrantNonce,
    UObject *&OutProvider,
    UMythicContextActionDefinition *&OutDefinition,
    uint32 &OutProviderSourceRevision) {
    return ResolveAuthorityGrantBindingInternal(
        Subject, ActionTag, GrantNonce, false, OutProvider, OutDefinition,
        OutProviderSourceRevision);
}

bool UMythicEntityActionGrantComponent::AuthorityConsumeActionGrantBinding(
    const FMythicEntityPresentationInstance Subject,
    const FGameplayTag ActionTag, const uint32 GrantNonce,
    UObject *&OutProvider,
    UMythicContextActionDefinition *&OutDefinition,
    uint32 &OutProviderSourceRevision) {
    return ResolveAuthorityGrantBindingInternal(
        Subject, ActionTag, GrantNonce, true, OutProvider, OutDefinition,
        OutProviderSourceRevision);
}

bool UMythicEntityActionGrantComponent::AuthorityRevokeActionGrant(
    const FMythicEntityPresentationInstance Subject, const FGameplayTag ActionTag) {
    if (!IsAuthority() || !Subject.IsValid() || !IsSafeActionTag(ActionTag)) {
        return false;
    }
    const int32 Removed = RemoveGrantsByPredicate(
        [&Subject, &ActionTag](
            const FMythicReplicatedContextActionGrant &Grant) {
            return Grant.Matches(Subject, ActionTag);
        });
    RemoveAuthorityLedgerEntriesByPredicate(
        [&Subject, &ActionTag](const FAuthorityGrantLedgerKey &Key,
                               const FAuthorityGrantLedgerEntry &) {
            return Key.Subject == Subject && Key.ActionTag == ActionTag;
        });
    if (Removed == 0) {
        return false;
    }
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return true;
}

int32 UMythicEntityActionGrantComponent::AuthorityRevokeSubjectGrants(
    const FMythicEntityPresentationInstance Subject) {
    if (!IsAuthority() || !Subject.IsValid()) {
        return 0;
    }
    const int32 Removed = RemoveGrantsByPredicate(
        [&Subject](const FMythicReplicatedContextActionGrant &Grant) {
            return Grant.Subject == Subject;
        });
    RemoveAuthorityLedgerEntriesByPredicate(
        [&Subject](const FAuthorityGrantLedgerKey &Key,
                   const FAuthorityGrantLedgerEntry &) {
            return Key.Subject == Subject;
        });
    if (Removed > 0) {
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
    }
    return Removed;
}

int32 UMythicEntityActionGrantComponent::AuthorityRevokeAllActionGrants() {
    if (!IsAuthority()) {
        return 0;
    }
    const int32 Removed = ReplicatedGrants.Items.Num();
    const bool bHadLedgerEntries = !AuthorityGrantLedger.IsEmpty();
    ReplicatedGrants.Items.Reset();
    AuthorityGrantLedger.Reset();
    if (Removed > 0) {
        ReplicatedGrants.MarkArrayDirty();
    }
    if (Removed > 0 || bHadLedgerEntries) {
        PublishRevision();
    }
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

int32 UMythicEntityActionGrantComponent::AuthorityPruneExpiredActionGrants(
    const double ServerTimeSeconds) {
    if (!IsAuthority() || !FMath::IsFinite(ServerTimeSeconds) || ServerTimeSeconds < 0.0) {
        return 0;
    }
    const int32 Removed = RemoveGrantsByPredicate(
        [ServerTimeSeconds](const FMythicReplicatedContextActionGrant &Grant) {
            return Grant.IsExpired(ServerTimeSeconds);
        });
    RemoveAuthorityLedgerEntriesByPredicate(
        [ServerTimeSeconds](const FAuthorityGrantLedgerKey &,
                            const FAuthorityGrantLedgerEntry &Entry) {
            return Entry.ExpiryServerTimeSeconds > 0.0
                && Entry.ExpiryServerTimeSeconds <= ServerTimeSeconds;
        });
    if (Removed > 0) {
        PublishRevision();
    }
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

bool UMythicEntityActionGrantComponent::IsAuthority() const {
    return GetOwner() && GetOwner()->HasAuthority();
}

double UMythicEntityActionGrantComponent::GetSynchronizedServerTimeSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return IsAuthority() ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

bool FMythicAuthorityContextActionDefinitionSignature::operator==(
    const FMythicAuthorityContextActionDefinitionSignature &Other) const {
    return ActionTag == Other.ActionTag
        && PresentationPriority == Other.PresentationPriority
        && HoldDurationSeconds == Other.HoldDurationSeconds
        && MaximumFocusAngleDegrees == Other.MaximumFocusAngleDegrees
        && MaximumRangeCentimeters == Other.MaximumRangeCentimeters
        && PresentationSemantic == Other.PresentationSemantic
        && WorldPresentationPolicy == Other.WorldPresentationPolicy
        && FocusPolicy == Other.FocusPolicy
        && RangePolicy == Other.RangePolicy
        && LineOfSightPolicy == Other.LineOfSightPolicy
        && bExplainWhenUnavailable == Other.bExplainWhenUnavailable;
}

uint32 UMythicEntityActionGrantComponent::AllocateAuthorityGrantNonce() {
    if (!IsAuthority() || NextAuthorityGrantNonce == 0
        || NextAuthorityGrantNonce > static_cast<uint64>(MAX_uint32)) {
        return 0;
    }
    return static_cast<uint32>(NextAuthorityGrantNonce++);
}

bool UMythicEntityActionGrantComponent::DoesProviderBelongToSubject(
    UObject *Provider, AActor *SubjectActor) {
    if (!IsValid(Provider) || !IsValid(SubjectActor)) {
        return false;
    }
    if (const AActor *ProviderActor = Cast<AActor>(Provider)) {
        return ProviderActor == SubjectActor;
    }
    if (const UActorComponent *ProviderComponent =
            Cast<UActorComponent>(Provider)) {
        return ProviderComponent->GetOwner() == SubjectActor;
    }
    return false;
}

FMythicAuthorityContextActionDefinitionSignature
FMythicAuthorityContextActionDefinitionSignature::Capture(
    const UMythicContextActionDefinition &Definition) {
    FMythicAuthorityContextActionDefinitionSignature Signature;
    Signature.ActionTag = Definition.ActionTag;
    Signature.PresentationPriority = Definition.PresentationPriority;
    Signature.HoldDurationSeconds = Definition.HoldDurationSeconds;
    Signature.MaximumFocusAngleDegrees =
        Definition.MaximumFocusAngleDegrees;
    Signature.MaximumRangeCentimeters =
        Definition.MaximumRangeCentimeters;
    Signature.PresentationSemantic = Definition.PresentationSemantic;
    Signature.WorldPresentationPolicy = Definition.WorldPresentationPolicy;
    Signature.FocusPolicy = Definition.FocusPolicy;
    Signature.RangePolicy = Definition.RangePolicy;
    Signature.LineOfSightPolicy = Definition.LineOfSightPolicy;
    Signature.bExplainWhenUnavailable = Definition.bExplainWhenUnavailable;
    return Signature;
}

bool FMythicAuthorityContextActionDefinitionSignature::Matches(
    const UMythicContextActionDefinition &Definition) const {
    return *this == Capture(Definition);
}

bool UMythicEntityActionGrantComponent::ResolveAuthorityGrantBindingInternal(
    const FMythicEntityPresentationInstance &Subject,
    const FGameplayTag ActionTag, const uint32 GrantNonce,
    const bool bConsume, UObject *&OutProvider,
    UMythicContextActionDefinition *&OutDefinition,
    uint32 &OutProviderSourceRevision) {
    OutProvider = nullptr;
    OutDefinition = nullptr;
    OutProviderSourceRevision = 0;
    if (!IsAuthority() || !Subject.IsValid() || !IsSafeActionTag(ActionTag)
        || GrantNonce == 0) {
        return false;
    }

    FMythicReplicatedContextActionGrant *Grant =
        ReplicatedGrants.Items.FindByPredicate(
            [&Subject, &ActionTag](
                const FMythicReplicatedContextActionGrant &Candidate) {
                return Candidate.Matches(Subject, ActionTag);
            });
    if (!Grant || Grant->OfferRevision != GrantNonce
        || Grant->State != EMythicContextActionGrantState::Available) {
        return false;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    if (Grant->IsExpired(Now)) {
        AuthorityRevokeActionGrant(Subject, ActionTag);
        return false;
    }

    const FAuthorityGrantLedgerKey Key{Subject, ActionTag, GrantNonce};
    const FAuthorityGrantLedgerEntry *Ledger =
        AuthorityGrantLedger.Find(Key);
    AActor *SubjectActor = Ledger ? Ledger->SubjectActor.Get() : nullptr;
    UObject *Provider = Ledger ? Ledger->Provider.Get() : nullptr;
    UMythicContextActionDefinition *Definition =
        Ledger ? Ledger->Definition.Get() : nullptr;

    EnsurePresentationRegistryBinding();
    UMythicEntityPresentationComponent *Presentation =
        BoundPresentationRegistry.IsValid()
            ? BoundPresentationRegistry->ResolvePresentationComponent(Subject)
            : nullptr;
    const bool bBindingCurrent =
        Ledger && IsValid(SubjectActor) && IsValid(Provider)
        && IsValid(Definition) && Presentation
        && Presentation->GetOwner() == SubjectActor
        && DoesProviderBelongToSubject(Provider, SubjectActor)
        && Provider->GetClass()->ImplementsInterface(
            UMythicContextActionProvider::StaticClass())
        && Definition->ActionTag == ActionTag
        && Ledger->DefinitionSignature.Matches(*Definition)
        && Ledger->ExpiryServerTimeSeconds
               == Grant->ExpiryServerTimeSeconds
        && (Ledger->ExpiryServerTimeSeconds <= 0.0
            || Ledger->ExpiryServerTimeSeconds > Now);
    if (!bBindingCurrent) {
        AuthorityRevokeActionGrant(Subject, ActionTag);
        return false;
    }

    OutProvider = Provider;
    OutDefinition = Definition;
    OutProviderSourceRevision = Ledger->ProviderSourceRevision;
    if (!bConsume) {
        return true;
    }

    const int32 Removed = RemoveGrantsByPredicate(
        [&Subject, &ActionTag, GrantNonce](
            const FMythicReplicatedContextActionGrant &Candidate) {
            return Candidate.Matches(Subject, ActionTag)
                && Candidate.OfferRevision == GrantNonce;
        });
    if (Removed != 1) {
        OutProvider = nullptr;
        OutDefinition = nullptr;
        OutProviderSourceRevision = 0;
        return false;
    }

    // Do not cross a synchronous native/Blueprint revision callback after returning raw retained issuer pointers.
    // The transport row is already gone; publish on the next game-thread tick after provider execution returns.
    QueueReplicatedRevision();
    ScheduleAuthorityExpiryTimer();
    return true;
}

bool UMythicEntityActionGrantComponent::SetGrantInternal(
    const FMythicReplicatedContextActionGrant &NewGrant) {
    FMythicReplicatedContextActionGrant *Existing =
        ReplicatedGrants.Items.FindByPredicate(
            [&NewGrant](const FMythicReplicatedContextActionGrant &Grant) {
                return Grant.Matches(NewGrant.Subject, NewGrant.ActionTag);
            });
    if (Existing) {
        if (Existing->State == NewGrant.State
            && Existing->UnavailableReasonTag == NewGrant.UnavailableReasonTag
            && Existing->OfferRevision == NewGrant.OfferRevision
            && Existing->ExpiryServerTimeSeconds == NewGrant.ExpiryServerTimeSeconds) {
            return false;
        }
        Existing->State = NewGrant.State;
        Existing->UnavailableReasonTag = NewGrant.UnavailableReasonTag;
        Existing->OfferRevision = NewGrant.OfferRevision;
        Existing->ExpiryServerTimeSeconds = NewGrant.ExpiryServerTimeSeconds;
        ReplicatedGrants.MarkItemDirty(*Existing);
        return true;
    }

    if (ReplicatedGrants.Items.Num() >= MaximumReplicatedGrants) {
        UE_LOG(LogMythicContextActions, Warning,
               TEXT("Rejected context-action grant because the owner-only bound of %d was reached."),
               MaximumReplicatedGrants);
        return false;
    }

    int32 SubjectGrantCount = 0;
    for (const FMythicReplicatedContextActionGrant &Grant : ReplicatedGrants.Items) {
        SubjectGrantCount += Grant.Subject == NewGrant.Subject ? 1 : 0;
    }
    if (SubjectGrantCount >= MaximumReplicatedGrantsPerSubject) {
        UE_LOG(LogMythicContextActions, Warning,
               TEXT("Rejected context-action grant because the per-subject bound of %d was reached."),
               MaximumReplicatedGrantsPerSubject);
        return false;
    }

    FMythicReplicatedContextActionGrant &Added = ReplicatedGrants.Items.Add_GetRef(NewGrant);
    ReplicatedGrants.MarkItemDirty(Added);
    return true;
}

int32 UMythicEntityActionGrantComponent::RemoveGrantsByPredicate(
    TFunctionRef<bool(const FMythicReplicatedContextActionGrant &)> Predicate) {
    for (const FMythicReplicatedContextActionGrant &Grant :
         ReplicatedGrants.Items) {
        if (Predicate(Grant) && Grant.OfferRevision != 0) {
            AuthorityGrantLedger.Remove(FAuthorityGrantLedgerKey{
                Grant.Subject, Grant.ActionTag, Grant.OfferRevision});
        }
    }
    const int32 Before = ReplicatedGrants.Items.Num();
    ReplicatedGrants.Items.RemoveAllSwap(Predicate, EAllowShrinking::No);
    const int32 Removed = Before - ReplicatedGrants.Items.Num();
    if (Removed > 0) {
        ReplicatedGrants.MarkArrayDirty();
    }
    return Removed;
}

int32 UMythicEntityActionGrantComponent::
    RemoveAuthorityLedgerEntriesByPredicate(
        TFunctionRef<bool(const FAuthorityGrantLedgerKey &,
                          const FAuthorityGrantLedgerEntry &)> Predicate) {
    const int32 Before = AuthorityGrantLedger.Num();
    for (auto It = AuthorityGrantLedger.CreateIterator(); It; ++It) {
        if (Predicate(It.Key(), It.Value())) {
            It.RemoveCurrent();
        }
    }
    return Before - AuthorityGrantLedger.Num();
}

void UMythicEntityActionGrantComponent::PublishRevision() {
    ++LocalRevision;
    if (LocalRevision == 0 || LocalRevision > static_cast<uint32>(MAX_int32)) {
        LocalRevision = 1;
    }
    NativeRevisionDelegate.Broadcast(LocalRevision);
    OnActionGrantsChanged.Broadcast(static_cast<int32>(LocalRevision));

    if (IsAuthority()) {
        if (AActor *OwnerActor = GetOwner()) {
            OwnerActor->FlushNetDormancy();
            OwnerActor->ForceNetUpdate();
        }
    }
}

void UMythicEntityActionGrantComponent::QueueReplicatedRevision() {
    if (bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = true;
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimerForNextTick(
            this, &UMythicEntityActionGrantComponent::PublishQueuedReplicatedRevision);
    }
    else {
        PublishQueuedReplicatedRevision();
    }
}

void UMythicEntityActionGrantComponent::PublishQueuedReplicatedRevision() {
    if (!bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = false;
    PublishRevision();
}

void UMythicEntityActionGrantComponent::ScheduleAuthorityExpiryTimer() {
    if (!IsAuthority()) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    FTimerManager &TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(ExpiryTimerHandle);

    const double Now = GetSynchronizedServerTimeSeconds();
    double EarliestExpiry = TNumericLimits<double>::Max();
    for (const FMythicReplicatedContextActionGrant &Grant : ReplicatedGrants.Items) {
        if (Grant.ExpiryServerTimeSeconds > 0.0) {
            EarliestExpiry = FMath::Min(EarliestExpiry, Grant.ExpiryServerTimeSeconds);
        }
    }
    if (EarliestExpiry == TNumericLimits<double>::Max()) {
        return;
    }

    const float DelaySeconds = static_cast<float>(FMath::Clamp(EarliestExpiry - Now, 0.01, 86400.0));
    TimerManager.SetTimer(ExpiryTimerHandle, this,
                          &UMythicEntityActionGrantComponent::HandleAuthorityExpiryTimer,
                          DelaySeconds, false);
}

void UMythicEntityActionGrantComponent::HandleAuthorityExpiryTimer() {
    AuthorityPruneExpiredActionGrants(GetSynchronizedServerTimeSeconds());
}

void UMythicEntityActionGrantComponent::EnsurePresentationRegistryBinding() {
    if (!IsAuthority() || BoundPresentationRegistry.IsValid()) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicEntityPresentationRegistry *Registry = World
        ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    if (!Registry) {
        return;
    }
    BoundPresentationRegistry = Registry;
    PresentationUnregisteredHandle =
        Registry->OnPresentationUnregistered.AddUObject(
            this,
            &UMythicEntityActionGrantComponent::HandlePresentationUnregistered);
}

void UMythicEntityActionGrantComponent::RemovePresentationRegistryBinding() {
    if (UMythicEntityPresentationRegistry *Registry =
            BoundPresentationRegistry.Get();
        Registry && PresentationUnregisteredHandle.IsValid()) {
        Registry->OnPresentationUnregistered.Remove(
            PresentationUnregisteredHandle);
    }
    PresentationUnregisteredHandle.Reset();
    BoundPresentationRegistry.Reset();
}

void UMythicEntityActionGrantComponent::HandlePresentationUnregistered(
    const FMythicEntityPresentationInstance &Subject,
    UMythicEntityPresentationComponent *Presentation) {
    (void)Presentation;
    if (IsAuthority() && Subject.IsValid()) {
        AuthorityRevokeSubjectGrants(Subject);
    }
}

bool UMythicEntityActionGrantComponent::IsSafeActionTag(const FGameplayTag Tag) {
    return Tag.IsValid() && !Tag.MatchesTagExact(CONTEXT_ACTION_ROOT)
           && Tag.MatchesTag(CONTEXT_ACTION_ROOT)
           && !Tag.MatchesTag(CONTEXT_ACTION_REASON_ROOT);
}

FGameplayTag UMythicEntityActionGrantComponent::SanitizeReasonTag(const FGameplayTag Tag) {
    return Tag.IsValid() && !Tag.MatchesTagExact(CONTEXT_ACTION_REASON_ROOT)
                   && Tag.MatchesTag(CONTEXT_ACTION_REASON_ROOT)
               ? Tag
               : FGameplayTag();
}
