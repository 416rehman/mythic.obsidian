#include "MythicPlayerController.h"

#include "Mythic.h"
#include "MythicPlayerState.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "GameModes/MythicCheatManager.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "EngineUtils.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/MythicTags_Conversion.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "World/Death/MythicCorpse.h"
#include "World/Ownership/MythicOwnership.h"
#include "World/Trading/MythicPlayerStall.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Inventory/MythicLootFilter.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Proficiency/ProficiencyComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Proficiency/ProficiencyDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Objectives/MythicObjectiveEvents.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h"
#include "UI/MythicDamageNumberSubsystem.h"
#include "UI/Inventory/MythicInventoryInteractionCoordinator.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicGift.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/FastTravel/MythicFastTravelRules.h"
#include "Itemization/Inventory/MythicEncumbrance.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/IMythicInteractable.h"
#include "Interaction/MythicInteractionComponent.h"
#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"
#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "World/LivingWorld/Chronicle/MythicChronicleRelayComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "AI/NPCs/MythicRecruitRules.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "AbilitySystemGlobals.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Player/MythicFactionStandingComponent.h"
#include "World/Harvesting/MythicHarvestFocusComponent.h"

namespace {
FName MythicRarityTagName(EItemRarity Rarity) {
    switch (Rarity) {
    case EItemRarity::Rare:      return FName(TEXT("Itemization.Rarity.Rare"));
    case EItemRarity::Epic:      return FName(TEXT("Itemization.Rarity.Epic"));
    case EItemRarity::Legendary: return FName(TEXT("Itemization.Rarity.Legendary"));
    case EItemRarity::Mythic:    return FName(TEXT("Itemization.Rarity.Mythic"));
    case EItemRarity::Common:
    default:                     return FName(TEXT("Itemization.Rarity.Common"));
    }
}

void MythicStampItemIdentity(FGameplayEventData &Payload, const UItemDefinition *ItemDef) {
    if (!ItemDef) {
        return;
    }
    if (ItemDef->ItemType.IsValid()) {
        Payload.TargetTags.AddTag(ItemDef->ItemType);
    }
    Payload.TargetTags.AddTag(FGameplayTag::RequestGameplayTag(MythicRarityTagName(ItemDef->Rarity)));
    if (const UYieldQualityFragment *Q =
            UItemDefinition::GetFragment<UYieldQualityFragment>(const_cast<UItemDefinition *>(ItemDef))) {
        if (const FGameplayTag QT = Q->GetQualityTag(); QT.IsValid()) {
            Payload.TargetTags.AddTag(QT);
        }
    }
}

FGameplayTag MythicSanitizeContextActionFailure(const FGameplayTag Candidate) {
    return Candidate.IsValid()
               && Candidate.MatchesTag(CONTEXT_ACTION_REASON_ROOT)
               && !Candidate.MatchesTagExact(CONTEXT_ACTION_REASON_ROOT)
           ? Candidate
           : CONTEXT_ACTION_REASON_UNAVAILABLE;
}

bool MythicDoesContextActionProviderBelongToSubject(UObject *Provider,
                                                    AActor *Subject) {
    if (!IsValid(Provider) || !IsValid(Subject)) {
        return false;
    }
    if (const AActor *ProviderActor = Cast<AActor>(Provider)) {
        return ProviderActor == Subject;
    }
    if (const UActorComponent *ProviderComponent =
            Cast<UActorComponent>(Provider)) {
        return ProviderComponent->GetOwner() == Subject;
    }
    return false;
}

bool MythicValidateContextActionSpatialRules(AMythicPlayerController *Controller,
                                             AActor *Subject,
                                             const FVector SubjectLocation,
                                             const UMythicContextActionDefinition &Definition,
                                             const FMythicContextActionProjectionRuntimePolicy &Policy,
                                             FGameplayTag &OutFailureReason) {
    APawn *ViewerPawn = Controller ? Controller->GetPawn() : nullptr;
    UWorld *World = Controller ? Controller->GetWorld() : nullptr;
    if (!Controller || !ViewerPawn || !World || !IsValid(Subject) || !Policy.bValid
        || SubjectLocation.ContainsNaN()) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

    if (Definition.FocusPolicy == EMythicContextActionFocusPolicy::LockedSubject) {
        // A remote client's local lock state is never trusted; wire a server-owned combat lock before enabling this policy.
        OutFailureReason = CONTEXT_ACTION_REASON_NOT_FOCUSED;
        return false;
    }
    if (Definition.FocusPolicy == EMythicContextActionFocusPolicy::FocusedSubject
        || Definition.FocusPolicy == EMythicContextActionFocusPolicy::FocusedOrLockedSubject) {
        const FVector DirectionToSubject = (SubjectLocation - ViewLocation).GetSafeNormal();
        const float MinimumFocusDot =
            FMath::Cos(FMath::DegreesToRadians(Definition.MaximumFocusAngleDegrees));
        if (DirectionToSubject.IsNearlyZero()
            || FVector::DotProduct(ViewRotation.Vector(), DirectionToSubject) < MinimumFocusDot) {
            OutFailureReason = CONTEXT_ACTION_REASON_NOT_FOCUSED;
            return false;
        }
    }

    if (Definition.RangePolicy == EMythicContextActionRangePolicy::DefinitionRange) {
        if (!FMath::IsFinite(Definition.MaximumRangeCentimeters)
            || Definition.MaximumRangeCentimeters <= 0.0f
            || FVector::DistSquared(ViewerPawn->GetActorLocation(), SubjectLocation)
                   > FMath::Square(Definition.MaximumRangeCentimeters)) {
            OutFailureReason = CONTEXT_ACTION_REASON_OUT_OF_RANGE;
            return false;
        }
    }

    if (Definition.LineOfSightPolicy != EMythicContextActionLineOfSightPolicy::NotRequired) {
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MythicContextActionLineOfSight),
                                          Policy.bTraceComplex, ViewerPawn);
        QueryParams.AddIgnoredActor(ViewerPawn);
        QueryParams.AddIgnoredActor(Controller);
        QueryParams.AddIgnoredActor(Subject);
        const FVector TraceStart =
            Definition.LineOfSightPolicy
                    == EMythicContextActionLineOfSightPolicy::ViewerPawnToSubject
                ? ViewerPawn->GetActorLocation()
                : ViewLocation;
        if (World->LineTraceTestByChannel(
                TraceStart, SubjectLocation,
                Policy.DiscoveryTraceChannel, QueryParams)) {
            OutFailureReason = CONTEXT_ACTION_REASON_OBSTRUCTED;
            return false;
        }
    }

    return true;
}

bool MythicValidateContextActionDiscovery(
    AMythicPlayerController *Controller, AActor *Subject,
    const FVector SubjectLocation,
    const FMythicContextActionProjectionRuntimePolicy &Policy,
    FVector &OutViewLocation, FVector &OutViewForward,
    float &OutDistanceSquared, FGameplayTag &OutFailureReason) {
    OutViewLocation = FVector::ZeroVector;
    OutViewForward = FVector::ForwardVector;
    OutDistanceSquared = 0.0f;
    OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;

    APawn *ViewerPawn = Controller ? Controller->GetPawn() : nullptr;
    UWorld *World = Controller ? Controller->GetWorld() : nullptr;
    if (!Controller || !ViewerPawn || !World || !IsValid(Subject) || !Policy.bValid
        || Subject->GetWorld() != World || SubjectLocation.ContainsNaN()) {
        return false;
    }

    OutDistanceSquared = FVector::DistSquared(
        ViewerPawn->GetActorLocation(), SubjectLocation);
    if (OutDistanceSquared
        > FMath::Square(Policy.MaximumDiscoveryRangeCentimeters)) {
        OutFailureReason = CONTEXT_ACTION_REASON_OUT_OF_RANGE;
        return false;
    }

    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(OutViewLocation, ViewRotation);
    if (OutViewLocation.ContainsNaN() || ViewRotation.ContainsNaN()) {
        return false;
    }
    OutViewForward = ViewRotation.Vector();

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(MythicContextActionDiscoveryLineOfSight),
        Policy.bTraceComplex, ViewerPawn);
    QueryParams.AddIgnoredActor(ViewerPawn);
    QueryParams.AddIgnoredActor(Controller);
    QueryParams.AddIgnoredActor(Subject);
    if (World->LineTraceTestByChannel(
            OutViewLocation, SubjectLocation,
            Policy.DiscoveryTraceChannel, QueryParams)) {
        OutFailureReason = CONTEXT_ACTION_REASON_OBSTRUCTED;
        return false;
    }
    OutFailureReason = FGameplayTag();
    return true;
}

bool MythicPrepareProjectedContextActionOffer(
    const FMythicContextActionOffer &SourceOffer,
    const FVector ViewerLocation, const FVector ViewForward,
    const FVector SubjectLocation, const float SubjectDistanceSquared,
    FMythicContextActionOffer &OutOffer) {
    OutOffer = SourceOffer;
    const UMythicContextActionDefinition *Definition = SourceOffer.Definition;
    if (!IsValid(Definition)) {
        return false;
    }

    if (Definition->FocusPolicy == EMythicContextActionFocusPolicy::LockedSubject) {
        // Server-owned combat lock integration is required before LockedSubject definitions may project.
        return false;
    }
    if (Definition->FocusPolicy == EMythicContextActionFocusPolicy::FocusedSubject
        || Definition->FocusPolicy == EMythicContextActionFocusPolicy::FocusedOrLockedSubject) {
        if (!FMath::IsFinite(Definition->MaximumFocusAngleDegrees)
            || Definition->MaximumFocusAngleDegrees <= 0.0f
            || Definition->MaximumFocusAngleDegrees > 90.0f) {
            return false;
        }
        const FVector DirectionToSubject =
            (SubjectLocation - ViewerLocation).GetSafeNormal();
        const float MinimumFocusDot = FMath::Cos(FMath::DegreesToRadians(
            Definition->MaximumFocusAngleDegrees));
        if (DirectionToSubject.IsNearlyZero()
            || FVector::DotProduct(ViewForward, DirectionToSubject)
                   < MinimumFocusDot) {
            return false;
        }
    }

    if (Definition->RangePolicy == EMythicContextActionRangePolicy::DefinitionRange
        && (!FMath::IsFinite(Definition->MaximumRangeCentimeters)
            || Definition->MaximumRangeCentimeters <= 0.0f
            || SubjectDistanceSquared
                   > FMath::Square(Definition->MaximumRangeCentimeters))) {
        if (!Definition->bExplainWhenUnavailable) {
            return false;
        }
        OutOffer.Availability =
            EMythicContextActionAvailability::UnavailableWithReason;
        OutOffer.UnavailableReasonTag = CONTEXT_ACTION_REASON_OUT_OF_RANGE;
    }
    return true;
}

bool MythicValidateFocusedCombatPresentation(
    AMythicPlayerController *Controller, AActor *Subject,
    const FVector SubjectAnchor,
    const FMythicCombatPresentationProjectionPolicy &Policy) {
    APawn *ViewerPawn = Controller ? Controller->GetPawn() : nullptr;
    UWorld *World = Controller ? Controller->GetWorld() : nullptr;
    if (!Controller || !ViewerPawn || !World || !IsValid(Subject)
        || Subject == ViewerPawn || Subject->GetWorld() != World
        || SubjectAnchor.ContainsNaN() || !Policy.IsValid()) {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    if (ViewLocation.ContainsNaN() || ViewRotation.ContainsNaN()) {
        return false;
    }
    const FVector ToSubject = SubjectAnchor - ViewLocation;
    const FVector DirectionToSubject = ToSubject.GetSafeNormal();
    const float ViewDot = DirectionToSubject.IsNearlyZero()
        ? 1.0f : FVector::DotProduct(ViewRotation.Vector(), DirectionToSubject);
    const float DistanceSquared = FVector::DistSquared(
        ViewerPawn->GetActorLocation(), SubjectAnchor);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(MythicFocusedCombatPresentationLineOfSight),
        Policy.bTraceComplex, ViewerPawn);
    QueryParams.AddIgnoredActor(ViewerPawn);
    QueryParams.AddIgnoredActor(Controller);
    QueryParams.AddIgnoredActor(Subject);
    const bool bAnchorVisible = !World->LineTraceTestByChannel(
        ViewLocation, SubjectAnchor, Policy.LineOfSightTraceChannel,
        QueryParams);
    FVector SubjectBoundsOrigin = SubjectAnchor;
    FVector SubjectBoundsExtent = FVector::ZeroVector;
    Subject->GetActorBounds(false, SubjectBoundsOrigin, SubjectBoundsExtent);
    const FVector SubjectTorso = SubjectBoundsOrigin
        + FVector(0.0, 0.0, SubjectBoundsExtent.Z * 0.20);
    const bool bTorsoSampleValid = !SubjectTorso.ContainsNaN();
    const bool bTorsoVisible = bTorsoSampleValid
        && !World->LineTraceTestByChannel(
            ViewLocation, SubjectTorso,
            Policy.LineOfSightTraceChannel, QueryParams);
    const bool bHasLineOfSight = bAnchorVisible || bTorsoVisible;
    return FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
        DistanceSquared, ViewDot, bHasLineOfSight, Policy);
}
}


AMythicPlayerController::AMythicPlayerController() {
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bReplicateUsingRegisteredSubObjectList = true;

    CheatClass = UMythicCheatManager::StaticClass();

    ProficiencyComponent = CreateDefaultSubobject<UProficiencyComponent>(TEXT("ProficiencyComponent"));
    ProficiencyComponent->SetIsReplicated(true);

    InventoryComponent = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("InventoryComponent"));
    InventoryComponent->SetIsReplicated(true);

    ObjectiveTracker = CreateDefaultSubobject<UObjectiveTracker>(TEXT("ObjectiveTracker"));
    ObjectiveTracker->SetIsReplicated(true);

    EnvironmentHazard = CreateDefaultSubobject<UMythicEnvironmentHazardComponent>(TEXT("EnvironmentHazard"));

    ChronicleRelay = CreateDefaultSubobject<UMythicChronicleRelayComponent>(TEXT("ChronicleRelay"));

    HarvestFocusComponent = CreateDefaultSubobject<UMythicHarvestFocusComponent>(TEXT("HarvestFocusComponent"));
}


UAbilitySystemComponent *AMythicPlayerController::GetAbilitySystemComponent() const {
    auto PS = GetPlayerState<AMythicPlayerState>();
    return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

TArray<UMythicInventoryComponent *> AMythicPlayerController::GetAllInventoryComponents() const {
    return {
        InventoryComponent
    };
}

UAbilitySystemComponent *AMythicPlayerController::GetSchematicsASC() const {
    return this->GetAbilitySystemComponent();
}

UMythicInventoryComponent *AMythicPlayerController::GetInventoryForItemType(const FGameplayTag &ItemType) const {
    return IInventoryProviderInterface::GetInventoryForItemType(ItemType);
}

void AMythicPlayerController::OnPossess(APawn *InPawn) {
    Super::OnPossess(InPawn);
    LastPresentedHarvestFeedbackSequence = 0;
    BindContextActionAttention();

    if (AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>()) {
        if (UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr) {
            Registry->RegisterPlayer(PS->GetCanonicalPlayerKey(), PS, this);
        }
    }

    if (this->IsLocalPlayerController()) {
        OnPossessedOnClient();
        if (HarvestFocusComponent) {
            HarvestFocusComponent->RefreshLocalFocus();
        }
    }
}

void AMythicPlayerController::OnUnPossess() {
    if (UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr) {
        Registry->UnregisterObject(this);
    }

    if (HasAuthority()) {
        EnterContextActionAuthorityBarrier();
        ClearAuthorityCombatPresentationFocus();
    }
    else if (IsLocalController()) {
        RequestContextActionOfferRefresh(
            FMythicEntityPresentationInstance());
    }

    Super::OnUnPossess();
    if (HarvestFocusComponent) {
        HarvestFocusComponent->RefreshLocalFocus();
    }
}

void AMythicPlayerController::SeamlessTravelTo(APlayerController *NewPC) {
    if (HasAuthority()) {
        EnterContextActionAuthorityBarrier();
    }
    Super::SeamlessTravelTo(NewPC);
}

void AMythicPlayerController::PreClientTravel(
    const FString &PendingURL, const ETravelType TravelType,
    const bool bIsSeamlessTravel) {
    if (HasAuthority()) {
        EnterContextActionAuthorityBarrier();
    }
    ResetInventoryActionSubmissionState();
    if (ULocalPlayer *LocalPlayer = GetLocalPlayer()) {
        if (UMythicInventoryInteractionCoordinator *Coordinator =
                LocalPlayer->GetSubsystem<
                    UMythicInventoryInteractionCoordinator>()) {
            Coordinator->DetachFromController(this);
        }
    }
    Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}

void AMythicPlayerController::OnRep_PlayerState() {
    Super::OnRep_PlayerState();

    LastPresentedHarvestFeedbackSequence = 0;
    BindContextActionAttention();
    OnPossessedOnClient();
}

void AMythicPlayerController::BeginPlay() {
    Super::BeginPlay();

    BindContextActionAttention();

    if (ULocalPlayer *LocalPlayer = GetLocalPlayer()) {
        if (UMythicInventoryInteractionCoordinator *Coordinator =
                LocalPlayer->GetSubsystem<
                    UMythicInventoryInteractionCoordinator>()) {
            Coordinator->AttachToController(this);
        }
    }

    if (HasAuthority() && GetWorld() && ZoneCheckInterval > 0.0f) {
        GetWorld()->GetTimerManager().SetTimer(ZoneCheckTimerHandle, this, &AMythicPlayerController::CheckZoneEntry,
                                               ZoneCheckInterval, true);
    }

    if (auto LocalPlayer = this->GetLocalPlayer()) {
        auto LocalIndex = LocalPlayer->GetLocalPlayerIndex();
        this->Login(LocalIndex);
    }
}

void AMythicPlayerController::BindContextActionAttention() {
    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UMythicEntityAttentionSubsystem *Attention =
        LocalPlayer
            ? LocalPlayer->GetSubsystem<UMythicEntityAttentionSubsystem>()
            : nullptr;
    if (!Attention || BoundContextActionAttentionSubsystem.Get() == Attention) {
        return;
    }

    UnbindContextActionAttention();
    BoundContextActionAttentionSubsystem = Attention;
    ContextActionFocusChangedHandle =
        Attention->OnFocusedEntityChanged.AddUObject(
            this, &AMythicPlayerController::HandleContextActionFocusChanged);
    RequestContextActionOfferRefresh(Attention->GetFocusedInstance());
}

void AMythicPlayerController::UnbindContextActionAttention() {
    if (UMythicEntityAttentionSubsystem *Attention =
            BoundContextActionAttentionSubsystem.Get();
        Attention && ContextActionFocusChangedHandle.IsValid()) {
        Attention->OnFocusedEntityChanged.Remove(
            ContextActionFocusChangedHandle);
    }
    ContextActionFocusChangedHandle.Reset();
    BoundContextActionAttentionSubsystem.Reset();
}

void AMythicPlayerController::HandleContextActionFocusChanged(
    const FMythicEntityPresentationInstance &PreviousSubject,
    const FMythicEntityPresentationInstance &NewSubject) {
    (void)PreviousSubject;
    RequestContextActionOfferRefresh(NewSubject);
}

void AMythicPlayerController::RequestContextActionOfferRefresh(
    const FMythicEntityPresentationInstance Subject) {
    if (!IsLocalController()) {
        return;
    }
    ServerRequestContextActionOfferRefresh(Subject);
}

UMythicEntityCombatPresentationComponent *
AMythicPlayerController::ResolveEntityCombatPresentationComponent() const {
    const AMythicPlayerState *MythicPS = GetPlayerState<AMythicPlayerState>();
    return MythicPS ? MythicPS->GetEntityCombatPresentationComponent()
                    : nullptr;
}

double AMythicPlayerController::GetCombatPresentationRequestClockSeconds() const {
    return GetWorld() ? FPlatformTime::Seconds() : 0.0;
}

double AMythicPlayerController::GetCombatPresentationLeaseClockSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return HasAuthority() ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

void AMythicPlayerController::ScheduleCombatPresentationRefresh(
    const float DelaySeconds) {
    UWorld *World = GetWorld();
    if (!HasAuthority() || !World
        || !AuthorityRequestedCombatPresentationSubject.IsValid()
        || !FMath::IsFinite(DelaySeconds)) {
        return;
    }
    World->GetTimerManager().SetTimer(
        CombatPresentationRefreshTimerHandle, this,
        &ThisClass::AuthorityRefreshFocusedCombatPresentation,
        FMath::Max(0.01f, DelaySeconds), false);
}

void AMythicPlayerController::ClearAuthorityCombatPresentationFocus() {
    if (!HasAuthority()) {
        return;
    }
    if (UMythicEntityCombatPresentationComponent *CombatPresentation =
            ResolveEntityCombatPresentationComponent()) {
        CombatPresentation->AuthorityRevokeAllCombatPresentations();
    }
    AuthorityRequestedCombatPresentationSubject.Reset();
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(
            CombatPresentationRefreshTimerHandle);
    }
}

void AMythicPlayerController::AuthoritySetCombatPresentationFocus(
    const FMythicEntityPresentationInstance Subject) {
    if (!HasAuthority()) {
        return;
    }

    UWorld *World = GetWorld();
    UMythicEntityCombatPresentationComponent *CombatPresentation =
        ResolveEntityCombatPresentationComponent();
    const FMythicEntityPresentationInstance PreviousSubject =
        AuthorityRequestedCombatPresentationSubject;
    if (CombatPresentation && PreviousSubject.IsValid()
        && PreviousSubject != Subject) {
        CombatPresentation->AuthorityRevokeCombatPresentation(
            PreviousSubject);
    }

    AuthorityRequestedCombatPresentationSubject = Subject;
    if (!Subject.IsValid() || !World) {
        ClearAuthorityCombatPresentationFocus();
        return;
    }
    if (!CombatPresentationProjectionPolicy.IsValid()) {
        ClearAuthorityCombatPresentationFocus();
        if (!bCombatPresentationPolicyWarningEmitted) {
            bCombatPresentationPolicyWarningEmitted = true;
            UE_LOG(Myth, Error,
                   TEXT("%s cannot project focused combat reads because its Combat Presentation Projection Policy is invalid."),
                   *GetNameSafe(this));
        }
        return;
    }

    const double Now = GetCombatPresentationRequestClockSeconds();
    const double Delay =
        FMythicCombatPresentationProjectionRules::GetRequestThrottleDelaySeconds(
            Now, LastCombatPresentationClientRequestSeconds,
            CombatPresentationProjectionPolicy.MinimumClientRequestIntervalSeconds);
    if (!FMath::IsFinite(Delay)) {
        ClearAuthorityCombatPresentationFocus();
        return;
    }
    if (Delay > 0.0) {
        ScheduleCombatPresentationRefresh(static_cast<float>(Delay));
        return;
    }
    LastCombatPresentationClientRequestSeconds = Now;
    AuthorityRefreshFocusedCombatPresentation();
}

void AMythicPlayerController::AuthorityRefreshFocusedCombatPresentation() {
    if (!HasAuthority()) {
        return;
    }

    const FMythicEntityPresentationInstance Subject =
        AuthorityRequestedCombatPresentationSubject;
    UWorld *World = GetWorld();
    UMythicEntityCombatPresentationComponent *CombatPresentation =
        ResolveEntityCombatPresentationComponent();
    if (!World || !Subject.IsValid()
        || !CombatPresentationProjectionPolicy.IsValid()) {
        ClearAuthorityCombatPresentationFocus();
        return;
    }
    if (!CombatPresentation) {
        ScheduleCombatPresentationRefresh(
            CombatPresentationProjectionPolicy.AuthorityRefreshIntervalSeconds);
        return;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    UMythicEntityPresentationComponent *Presentation =
        Registry ? Registry->ResolvePresentationComponent(Subject) : nullptr;
    AActor *SubjectActor = Presentation ? Presentation->GetOwner() : nullptr;
    if (!Presentation || !IsValid(SubjectActor)
        || Presentation->GetPresentationInstance() != Subject) {
        CombatPresentation->AuthorityRevokeCombatPresentation(Subject);
        AuthorityRequestedCombatPresentationSubject.Reset();
        World->GetTimerManager().ClearTimer(
            CombatPresentationRefreshTimerHandle);
        return;
    }

    if (!MythicValidateFocusedCombatPresentation(
            this, SubjectActor,
            Presentation->GetPresentationAnchorWorldLocation(),
            CombatPresentationProjectionPolicy)) {
        CombatPresentation->AuthorityRevokeCombatPresentation(Subject);
        ScheduleCombatPresentationRefresh(
            CombatPresentationProjectionPolicy.AuthorityRefreshIntervalSeconds);
        return;
    }

    UAbilitySystemComponent *ViewerAbilitySystem = GetAbilitySystemComponent();
    UAbilitySystemComponent *SubjectAbilitySystem =
        FMythicCombatPresentationProjectionRules::ResolveAbilitySystem(
            SubjectActor);
    const APawn *ViewerPawn = GetPawn();
    const bool bPublicCombatCommitment =
        FMythicCombatPresentationProjectionRules::HasPublicCombatCommitment(
            SubjectActor, SubjectAbilitySystem, ViewerPawn);
    FMythicCombatPressureSnapshot ViewerSnapshot;
    FMythicCombatPressureSnapshot SubjectSnapshot;
    if (bPublicCombatCommitment) {
        ViewerSnapshot = FMythicCombatPresentationProjectionRules::
            BuildAuthorityPressureSnapshot(
                this, GetPawn(), ViewerAbilitySystem);
        SubjectSnapshot = FMythicCombatPresentationProjectionRules::
            BuildAuthorityPressureSnapshot(
                SubjectActor, SubjectActor, SubjectAbilitySystem);
    }
    const EMythicPresentedCombatRank CanonicalPresentedRank =
        FMythicCombatPresentationProjectionRules::
            ResolveAuthorityNpcPresentedCombatRank(
                Cast<AMythicNPCCharacter>(SubjectActor));

    FMythicEntityCombatPresentationAuthorityRequest Request;
    Request.Subject = Subject;
    Request.AssessmentInputs.bAssessmentPermitted =
        bPublicCombatCommitment;
    Request.AssessmentInputs.bCombatCapable =
        SubjectSnapshot.bCombatCapable;
    Request.AssessmentInputs.ViewerEffectivePressure =
        FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
            ViewerSnapshot);
    Request.AssessmentInputs.SubjectEffectivePressure =
        FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
            SubjectSnapshot);
    Request.PresentedCombatRank = CanonicalPresentedRank;
    Request.bRankPresentationPermitted = bPublicCombatCommitment
        && CanonicalPresentedRank != EMythicPresentedCombatRank::Unknown;
    Request.SubjectCombatLevel =
        FMythicCombatPresentationProjectionRules::
            ResolveAuthorityCombatLevel(
                SubjectActor, SubjectAbilitySystem);
    Request.bExactCombatLevelPermitted = bPublicCombatCommitment
        && SubjectSnapshot.bCombatCapable
        && Request.SubjectCombatLevel > 0
        && CombatPresentationProjectionPolicy.bPermitExactCombatLevelForFocus;
    CombatPresentationSourceRevision =
        FMythicCombatPresentationProjectionRules::AdvanceNonzeroRevision(
            CombatPresentationSourceRevision);
    Request.SourceRevision = CombatPresentationSourceRevision;
    Request.ExpiryServerTimeSeconds = GetCombatPresentationLeaseClockSeconds()
        + static_cast<double>(CombatPresentationProjectionPolicy.PresentationLeaseDurationSeconds);

    if (!CombatPresentation->AuthoritySetCombatPresentation(Request)) {
        CombatPresentation->AuthorityRevokeCombatPresentation(Subject);
        AuthorityRequestedCombatPresentationSubject.Reset();
        World->GetTimerManager().ClearTimer(
            CombatPresentationRefreshTimerHandle);
        return;
    }
    ScheduleCombatPresentationRefresh(
        CombatPresentationProjectionPolicy.AuthorityRefreshIntervalSeconds);
}

UMythicEntityActionGrantComponent *
AMythicPlayerController::ResolveEntityActionGrantComponent() const {
    const AMythicPlayerState *MythicPlayerState =
        GetPlayerState<AMythicPlayerState>();
    return MythicPlayerState
               ? MythicPlayerState->GetEntityActionGrantComponent()
               : nullptr;
}

double AMythicPlayerController::GetContextActionRequestClockSeconds() const {
    return GetWorld() ? FPlatformTime::Seconds() : 0.0;
}

double AMythicPlayerController::GetContextActionLeaseClockSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return HasAuthority() ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

void AMythicPlayerController::ScheduleContextActionOfferRefresh(
    const float DelaySeconds) {
    UWorld *World = GetWorld();
    if (!HasAuthority() || !World
        || !AuthorityRequestedContextActionSubject.IsValid()
        || !FMath::IsFinite(DelaySeconds)) {
        return;
    }
    World->GetTimerManager().SetTimer(
        ContextActionOfferRefreshTimerHandle, this,
        &AMythicPlayerController::AuthorityRefreshContextActionOffers,
        FMath::Max(DelaySeconds, 0.01f), false);
}

void AMythicPlayerController::ServerRequestContextActionOfferRefresh_Implementation(
    const FMythicEntityPresentationInstance Subject) {
    if (!HasAuthority()) {
        return;
    }

    // One opaque focus edge drives both independent server projections. Combat never consumes context-action policy,
    // provider output, client attributes, or UI state; sharing only the nomination avoids a second spoofable RPC path.
    AuthoritySetCombatPresentationFocus(Subject);

    UMythicEntityActionGrantComponent *Grants =
        ResolveEntityActionGrantComponent();
    const FMythicEntityPresentationInstance PreviousSubject =
        AuthorityRequestedContextActionSubject;
    if (PendingContextActionHold.IsActive()
        && PendingContextActionHold.Subject != Subject) {
        ResetPendingContextActionHold();
    }
    if (Grants && PreviousSubject.IsValid() && PreviousSubject != Subject) {
        Grants->AuthorityRevokeSubjectGrants(PreviousSubject);
    }

    AuthorityRequestedContextActionSubject = Subject;
    UWorld *World = GetWorld();
    if (!Subject.IsValid() || !World) {
        AuthorityRequestedContextActionSubject.Reset();
        if (World) {
            World->GetTimerManager().ClearTimer(
                ContextActionOfferRefreshTimerHandle);
        }
        return;
    }

    const FMythicContextActionProjectionRuntimePolicy Policy =
        FMythicContextActionProjectionRules::BuildRuntimePolicy(
            ContextActionProjectionPolicy);
    if (!Policy.bValid) {
        ResetPendingContextActionHold();
        if (Grants) {
            Grants->AuthorityRevokeSubjectGrants(Subject);
        }
        AuthorityRequestedContextActionSubject.Reset();
        World->GetTimerManager().ClearTimer(
            ContextActionOfferRefreshTimerHandle);
        if (!bContextActionPolicyWarningEmitted) {
            bContextActionPolicyWarningEmitted = true;
            UE_LOG(Myth, Error,
                   TEXT("%s cannot project context actions because its Context Action Projection Policy is missing or invalid."),
                   *GetNameSafe(this));
        }
        return;
    }

    const double Delay =
        FMythicContextActionProjectionRules::GetRequestThrottleDelaySeconds(
            GetContextActionRequestClockSeconds(),
            LastContextActionProjectionSeconds,
            Policy.MinimumClientRequestIntervalSeconds);
    if (!FMath::IsFinite(Delay)) {
        ResetPendingContextActionHold();
        if (Grants) {
            Grants->AuthorityRevokeSubjectGrants(Subject);
        }
        AuthorityRequestedContextActionSubject.Reset();
        World->GetTimerManager().ClearTimer(
            ContextActionOfferRefreshTimerHandle);
        return;
    }
    if (Delay > 0.0) {
        ScheduleContextActionOfferRefresh(static_cast<float>(Delay));
        return;
    }
    AuthorityRefreshContextActionOffers();
}

void AMythicPlayerController::AuthorityRefreshContextActionOffers() {
    if (!HasAuthority()) {
        return;
    }

    const FMythicContextActionProjectionRuntimePolicy Policy =
        FMythicContextActionProjectionRules::BuildRuntimePolicy(
            ContextActionProjectionPolicy);
    UMythicEntityActionGrantComponent *Grants =
        ResolveEntityActionGrantComponent();
    const FMythicEntityPresentationInstance Subject =
        AuthorityRequestedContextActionSubject;
    UWorld *World = GetWorld();
    if (Policy.bValid && Subject.IsValid() && World) {
        // Consuming the budget precedes all fallible dependencies, so a not-yet-ready PlayerState cannot bypass it.
        LastContextActionProjectionSeconds =
            GetContextActionRequestClockSeconds();
    }
    if (!Policy.bValid || !Grants || !Subject.IsValid() || !World) {
        if (PendingContextActionHold.IsActive()
            && PendingContextActionHold.Subject == Subject) {
            ResetPendingContextActionHold();
        }
        if (Grants && Subject.IsValid()) {
            Grants->AuthorityRevokeSubjectGrants(Subject);
        }
        if (!Policy.bValid || !Subject.IsValid() || !World) {
            AuthorityRequestedContextActionSubject.Reset();
        }
        else {
            ScheduleContextActionOfferRefresh(
                Policy.AuthorityRefreshIntervalSeconds);
        }
        return;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    UMythicEntityPresentationComponent *Presentation =
        Registry ? Registry->ResolvePresentationComponent(Subject) : nullptr;
    AActor *SubjectActor = Presentation ? Presentation->GetOwner() : nullptr;
    if (!Presentation || !IsValid(SubjectActor)
        || !FMythicContextActionProjectionRules::IsExactResolvedSubject(
            Subject, Presentation->GetPresentationInstance())) {
        if (PendingContextActionHold.Subject == Subject) {
            ResetPendingContextActionHold();
        }
        Grants->AuthorityRevokeSubjectGrants(Subject);
        AuthorityRequestedContextActionSubject.Reset();
        World->GetTimerManager().ClearTimer(
            ContextActionOfferRefreshTimerHandle);
        return;
    }

    const FVector SubjectLocation =
        Presentation->GetPresentationAnchorWorldLocation();
    FVector ViewLocation;
    FVector ViewForward;
    float SubjectDistanceSquared = 0.0f;
    FGameplayTag DiscoveryFailureReason;
    if (!MythicValidateContextActionDiscovery(
            this, SubjectActor, SubjectLocation, Policy, ViewLocation,
            ViewForward, SubjectDistanceSquared,
            DiscoveryFailureReason)) {
        if (PendingContextActionHold.Subject == Subject) {
            ResetPendingContextActionHold();
        }
        const TArray<FMythicAuthorityContextActionOffer> NoOffers;
        Grants->AuthorityReplaceBoundContextActionOffers(
            Subject, SubjectActor, NoOffers, Policy.MaximumProjectedOffers,
            GetContextActionLeaseClockSeconds()
                + static_cast<double>(Policy.OfferLeaseDurationSeconds));
        ScheduleContextActionOfferRefresh(
            Policy.AuthorityRefreshIntervalSeconds);
        return;
    }

    TArray<FMythicAuthorityContextActionOffer> ProjectedOffers;
    ProjectedOffers.Reserve(
        (Policy.MaximumProviderComponents + 1)
        * Policy.MaximumOffersPerProvider);
    TArray<FMythicContextActionOffer> ProviderOffers;
    ProviderOffers.Reserve(
        FMath::Min(Policy.MaximumOffersPerProvider, 8));

    const auto GatherProvider =
        [this, SubjectActor, SubjectLocation, ViewLocation, ViewForward,
         SubjectDistanceSquared, &Policy, &ProjectedOffers,
         &ProviderOffers](UObject *Provider) {
            if (!IsValid(Provider)
                || !Provider->GetClass()->ImplementsInterface(
                    UMythicContextActionProvider::StaticClass())) {
                return;
            }
            ProviderOffers.Reset();
            IMythicContextActionProvider::Execute_GatherContextActions(
                Provider, this, SubjectActor, ProviderOffers);
            const int32 OfferCount = FMath::Min(
                ProviderOffers.Num(), Policy.MaximumOffersPerProvider);
            for (int32 Index = 0; Index < OfferCount; ++Index) {
                FMythicContextActionOffer PreparedOffer;
                if (MythicPrepareProjectedContextActionOffer(
                        ProviderOffers[Index], ViewLocation, ViewForward,
                        SubjectLocation, SubjectDistanceSquared,
                        PreparedOffer)) {
                    ProjectedOffers.Emplace(Provider, PreparedOffer);
                }
            }
        };

    GatherProvider(SubjectActor);
    TInlineComponentArray<UActorComponent *> Components;
    if (IsValid(SubjectActor)) {
        SubjectActor->GetComponents(Components);
    }
    int32 ProviderComponentCount = 0;
    for (UActorComponent *Component : Components) {
        if (!IsValid(Component)
            || !Component->GetClass()->ImplementsInterface(
                UMythicContextActionProvider::StaticClass())) {
            continue;
        }
        if (ProviderComponentCount++ >= Policy.MaximumProviderComponents) {
            break;
        }
        GatherProvider(Component);
    }

    // Provider code is a domain boundary: prove the same embodiment, geometry, bound, and LOS again before writing.
    Presentation = Registry->ResolvePresentationComponent(Subject);
    SubjectActor = Presentation ? Presentation->GetOwner() : nullptr;
    FVector RevalidatedViewLocation;
    FVector RevalidatedViewForward;
    float RevalidatedDistanceSquared = 0.0f;
    DiscoveryFailureReason = FGameplayTag();
    if (!Presentation || !IsValid(SubjectActor)
        || !FMythicContextActionProjectionRules::IsExactResolvedSubject(
            Subject, Presentation->GetPresentationInstance())
        || !MythicValidateContextActionDiscovery(
            this, SubjectActor,
            Presentation->GetPresentationAnchorWorldLocation(), Policy,
            RevalidatedViewLocation, RevalidatedViewForward,
             RevalidatedDistanceSquared, DiscoveryFailureReason)) {
        if (PendingContextActionHold.Subject == Subject) {
            ResetPendingContextActionHold();
        }
        Grants->AuthorityRevokeSubjectGrants(Subject);
        if (!Presentation || !IsValid(SubjectActor)) {
            AuthorityRequestedContextActionSubject.Reset();
            World->GetTimerManager().ClearTimer(
                ContextActionOfferRefreshTimerHandle);
            return;
        }
        ScheduleContextActionOfferRefresh(
            Policy.AuthorityRefreshIntervalSeconds);
        return;
    }

    TArray<FMythicAuthorityContextActionOffer> RevalidatedOffers;
    RevalidatedOffers.Reserve(ProjectedOffers.Num());
    const FVector RevalidatedSubjectLocation =
        Presentation->GetPresentationAnchorWorldLocation();
    for (const FMythicAuthorityContextActionOffer &BoundOffer :
         ProjectedOffers) {
        FMythicContextActionOffer PreparedOffer;
        if (MythicPrepareProjectedContextActionOffer(
                BoundOffer.Offer, RevalidatedViewLocation,
                RevalidatedViewForward,
                RevalidatedSubjectLocation, RevalidatedDistanceSquared,
                PreparedOffer)) {
            RevalidatedOffers.Emplace(BoundOffer.Provider.Get(),
                                      PreparedOffer);
        }
    }

    Grants->AuthorityReplaceBoundContextActionOffers(
        Subject, SubjectActor, RevalidatedOffers,
        Policy.MaximumProjectedOffers,
        GetContextActionLeaseClockSeconds()
            + static_cast<double>(Policy.OfferLeaseDurationSeconds));
    if (PendingContextActionHold.IsActive()
        && PendingContextActionHold.Subject == Subject) {
        UObject *BoundProvider = nullptr;
        UMythicContextActionDefinition *BoundDefinition = nullptr;
        uint32 ProviderSourceRevision = 0;
        const bool bHoldStillOffered =
            Grants->AuthorityResolveActionGrantBinding(
                Subject, PendingContextActionHold.ActionTag,
                PendingContextActionHold.OfferRevision, BoundProvider,
                BoundDefinition, ProviderSourceRevision)
            && IsValid(BoundProvider) && IsValid(BoundDefinition)
            && FMath::IsNearlyEqual(
                BoundDefinition->HoldDurationSeconds,
                PendingContextActionHold.RequiredDurationSeconds,
                UE_KINDA_SMALL_NUMBER);
        (void)ProviderSourceRevision;
        if (!bHoldStillOffered) {
            ResetPendingContextActionHold();
        }
    }
    ScheduleContextActionOfferRefresh(
        Policy.AuthorityRefreshIntervalSeconds);
}

void AMythicPlayerController::Login(int32 LocalUserNum) {
    UE_LOG(Myth, Log, TEXT("EOS: Connecting to Online Services"));

    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    if (!OSS) {
        UE_LOG(Myth, Error, TEXT("EOS: No Online Subsystem available — skipping login"));
        return;
    }
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid()) {
        UE_LOG(Myth, Error, TEXT("EOS: Identity interface unavailable — skipping login"));
        return;
    }


    LoginDelegateHandle = Identity->
        AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::CB_LoginResponse));

    UE_LOG(Myth, Log, TEXT("EOS: Using Online Subsystem: %s"), *OSS->GetSubsystemName().ToString());

    FString AuthType;
    FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

    if (!AuthType.IsEmpty()) {
        if (!Identity->AutoLogin(LocalUserNum)) {
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start AutoLogin"));

            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            LoginDelegateHandle.Reset();
        }
    }
    else {
        FOnlineAccountCredentials Credentials("AccountPortal", "", "");

        UE_LOG(Myth, Log, TEXT("EOS: Logging in to Online service"));

        if (!Identity->Login(LocalUserNum, Credentials)) {
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start Login"));

            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            LoginDelegateHandle.Reset();
        }
    }
}

void AMythicPlayerController::CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error) {
    if (bWasSuccessful) {
        UE_LOG(Myth, Log, TEXT("EOS: Login successful - %s"), *UserId.ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT("EOS: Login failed: %s"), *Error);
    }

    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    if (!OSS) {
        UE_LOG(Myth, Error, TEXT("EOS: No Online Subsystem available — cannot clear login delegate"));
        return;
    }
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid()) {
        UE_LOG(Myth, Error, TEXT("EOS: Identity interface unavailable — cannot clear login delegate"));
        return;
    }

    Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
    LoginDelegateHandle.Reset();
}

UProficiencyComponent *AMythicPlayerController::GetProficiencyComponent() const {
    return ProficiencyComponent;
}

int32 AMythicPlayerController::GetPlayerLevel() const {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return 1;
    }

    bool bFound = false;
    int32 Level = UMythicAttributeSet_Proficiencies::GetLevel(ASC, bFound);
    return bFound ? Level : 1;
}

float AMythicPlayerController::GetPlayerLevelProgress() const {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return 0.0f;
    }

    const UMythicAttributeSet_Proficiencies *ProfSet = ASC->GetSet<UMythicAttributeSet_Proficiencies>();
    if (!ProfSet) {
        return 0.0f;
    }

    float Current = ProfSet->GetOverallXp();
    float MaxVal = ProfSet->GetOverallXpMax();
    if (MaxVal <= 0.0f) {
        return 0.0f;
    }
    int32 Level = 1;
    float IntoLevel = 0.0f;
    float LevelSpan = 0.0f;
    UMythicAttributeSet_Proficiencies::GetLevelXpWindow(Current, MaxVal, Level, IntoLevel, LevelSpan);
    if (LevelSpan > 0.0f) {
        return FMath::Clamp(IntoLevel / LevelSpan, 0.0f, 1.0f);
    }

    return FMath::Clamp(Current / MaxVal, 0.0f, 1.0f);
}

TArray<FProficiencySummary> AMythicPlayerController::GetProficiencySummaries() const {
    TArray<FProficiencySummary> Summaries;
    if (!ProficiencyComponent) {
        return Summaries;
    }

    for (int32 i = 0; i < ProficiencyComponent->Proficiencies.Num(); ++i) {
        Summaries.Add(ProficiencyComponent->GetSummary(i));
    }

    return Summaries;
}

FPlayerStatsSummary AMythicPlayerController::GetPlayerStats() const {
    FPlayerStatsSummary Stats;
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return Stats;
    }

    if (const UMythicAttributeSet_Offense *OffSet = ASC->GetSet<UMythicAttributeSet_Offense>()) {
        Stats.Power = OffSet->GetPower();
        Stats.DamagePerHit = OffSet->GetDamagePerHit();
        Stats.AttackSpeed = OffSet->GetAttackSpeed();
        Stats.CritChance = OffSet->GetCriticalHitChance();
        Stats.CritDamage = OffSet->GetCriticalHitDamage();
    }

    if (const UMythicAttributeSet_Defense *DefSet = ASC->GetSet<UMythicAttributeSet_Defense>()) {
        Stats.Armor = DefSet->GetArmor();
        Stats.DodgeChance = DefSet->GetDodgeChance();
        Stats.MaxShield = DefSet->GetMaxShield();
        Stats.ShieldRegenRate = DefSet->GetShieldRegenRate();
        Stats.HealthRegenRate = DefSet->GetHealthRegenRate();
    }

    if (const UMythicAttributeSet_Life *LifeSet = ASC->GetSet<UMythicAttributeSet_Life>()) {
        Stats.MaxHealth = LifeSet->GetMaxHealth();
    }

    if (const UMythicAttributeSet_Utility *UtilSet = ASC->GetSet<UMythicAttributeSet_Utility>()) {
        Stats.MaxStamina = UtilSet->GetMaxStamina();
        Stats.StaminaRegenRate = UtilSet->GetStaminaRegenRate();
        Stats.CooldownReduction = UtilSet->GetCooldownReduction();
        Stats.ProficiencyXPBonus = UtilSet->GetProficiencyXPBonus();
        Stats.MovementSpeedMultiplier = UtilSet->GetMovementSpeedMultiplier();
    }

    Stats.PlayerLevel = GetPlayerLevel();

    return Stats;
}


bool AMythicPlayerController::ServerOpenConversionStation_Validate(AMythicConversionStation *Station) { return Station != nullptr; }

void AMythicPlayerController::ServerOpenConversionStation_Implementation(AMythicConversionStation *Station) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_RegisterInstigator(this, GetPawn());
    }
}

bool AMythicPlayerController::ServerConversionRequestStart_Validate(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity) {
    return Station != nullptr && RecipeId.IsValid() && Quantity >= 1 && Quantity <= 999;
}

void AMythicPlayerController::ServerConversionRequestStart_Implementation(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_RequestStart(this, RecipeId, Quantity);
    }
}

bool AMythicPlayerController::ServerConversionCancelJob_Validate(AMythicConversionStation *Station, int32 JobId) {
    return Station != nullptr && JobId > 0;
}

void AMythicPlayerController::ServerConversionCancelJob_Implementation(AMythicConversionStation *Station, int32 JobId) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_CancelJob(this, JobId);
    }
}

bool AMythicPlayerController::ServerConversionSetAutoRepeat_Validate(AMythicConversionStation *Station, bool bRepeat) { return Station != nullptr; }

void AMythicPlayerController::ServerConversionSetAutoRepeat_Implementation(AMythicConversionStation *Station, bool bRepeat) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_SetAutoRepeat(this, bRepeat);
    }
}


bool AMythicPlayerController::ServerMoveItemBetweenInventories_Validate(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target,
                                                                        int32 TargetSlot) {
    return Source != nullptr && Target != nullptr && SourceSlot >= 0 && TargetSlot >= -1;
}

bool AMythicPlayerController::CanPlayerAccessInventory(UMythicInventoryComponent *Inventory) const {
    if (!Inventory) {
        return false;
    }
    if (GetAllInventoryComponents().Contains(Inventory)) {
        return true;
    }
    if (AMythicStorageContainer *Container = Cast<AMythicStorageContainer>(Inventory->GetOwner())) {
        return Container->Server_IsOpener(this) && Container->IsActorInRange(GetPawn());
    }
    if (AMythicCorpse *Corpse = Cast<AMythicCorpse>(Inventory->GetOwner())) {
        return Corpse->Server_IsOpener(this) && Corpse->IsActorInRange(GetPawn());
    }
    return false;
}

bool AMythicPlayerController::IsPlayerOwnedInventory(
    const UMythicInventoryComponent *Inventory) const {
    if (!Inventory) {
        return false;
    }
    for (const UMythicInventoryComponent *OwnedInventory : GetAllInventoryComponents()) {
        if (OwnedInventory == Inventory) {
            return true;
        }
    }
    return false;
}

int64 AMythicPlayerController::AllocateInventoryActionRequestId() {
    if (bInventoryActionRequestIdsExhausted
        || NextInventoryActionRequestId <= 0) {
        return 0;
    }
    const int64 RequestId = NextInventoryActionRequestId;
    if (RequestId == MAX_int64) {
        bInventoryActionRequestIdsExhausted = true;
        NextInventoryActionRequestId = 0;
    }
    else {
        NextInventoryActionRequestId = RequestId + 1;
    }
    return RequestId;
}

int64 AMythicPlayerController::BeginInventoryActionSubmission(
    FMythicInventoryActionSubmission &Submission) {
    if (!IsLocalController() || ActiveInventoryActionRequestId > 0) {
        return 0;
    }

    const int64 RequestId = AllocateInventoryActionRequestId();
    if (RequestId <= 0) {
        return 0;
    }

    ActiveInventoryActionRequestId = RequestId;
    Submission.RequestId = RequestId;
    OnInventoryActionSubmitted.Broadcast(Submission);
    return RequestId;
}

bool AMythicPlayerController::AcceptNewInventoryActionRequestId(
    const int64 RequestId) {
    if (RequestId <= 0 || RequestId <= HighestAcceptedInventoryActionRequestId) {
        return false;
    }
    HighestAcceptedInventoryActionRequestId = RequestId;
    return true;
}

void AMythicPlayerController::ResetInventoryActionSubmissionState() {
    ActiveInventoryActionRequestId = 0;
    ReceivedInventoryActionReceipts.Reset();
    ReceivedInventoryActionReceiptOrder.Reset();
}

bool AMythicPlayerController::ReplayCachedInventoryActionReceipt(
    const int64 RequestId) {
    if (RequestId <= 0) {
        return false;
    }
    const FMythicInventoryActionReceipt *Cached =
        InventoryActionReceiptCache.Find(RequestId);
    if (!Cached) {
        return false;
    }

    FMythicInventoryActionReceipt Replay = *Cached;
    Replay.Disposition = EMythicInventoryReceiptDisposition::Replayed;
    ClientReceiveInventoryActionReceipt(Replay);
    return true;
}

void AMythicPlayerController::CompleteInventoryActionRequest(
    FMythicInventoryActionReceipt Receipt) {
    Receipt.Disposition = Receipt.WasSuccessful()
        ? EMythicInventoryReceiptDisposition::Committed
        : EMythicInventoryReceiptDisposition::Rejected;

    if (Receipt.RequestId > 0
        && Receipt.RequestId >= HighestAcceptedInventoryActionRequestId) {
        if (InventoryActionReceiptOrder.Num()
            >= MaxInventoryActionReceiptCacheSize) {
            const int64 OldestRequestId = InventoryActionReceiptOrder[0];
            InventoryActionReceiptOrder.RemoveAt(0, 1, EAllowShrinking::No);
            InventoryActionReceiptCache.Remove(OldestRequestId);
        }
        InventoryActionReceiptCache.Add(Receipt.RequestId, Receipt);
        InventoryActionReceiptOrder.Add(Receipt.RequestId);
    }
    ClientReceiveInventoryActionReceipt(Receipt);
}

bool AMythicPlayerController::ConsumeReceivedInventoryActionReceipt(
    const int64 RequestId,
    FMythicInventoryActionReceipt &OutReceipt) {
    FMythicInventoryActionReceipt *BufferedReceipt =
        ReceivedInventoryActionReceipts.Find(RequestId);
    if (!BufferedReceipt) {
        return false;
    }

    OutReceipt = *BufferedReceipt;
    ReceivedInventoryActionReceipts.Remove(RequestId);
    ReceivedInventoryActionReceiptOrder.RemoveSingle(RequestId);
    return true;
}

int64 AMythicPlayerController::SubmitInventoryMove(
    const FMythicInventorySourceLocator &Source,
    const FMythicInventoryTargetLocator &Target) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::Move;
    Submission.Inventory = Source.Inventory;
    Submission.ItemGuid = Source.ExpectedItemGuid;
    Submission.SourceSlotIndex = Source.SlotIndex;
    Submission.TargetSlotIndex = Target.SlotIndex;
    Submission.RequestedQuantity = Source.ExpectedQuantity;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventoryMove(RequestId, Source, Target);
    return RequestId;
}

int64 AMythicPlayerController::SubmitInventoryMoveBySlots(
    UMythicInventoryComponent *SourceInventory,
    const int32 SourceSlotIndex,
    UMythicInventoryComponent *TargetInventory,
    const int32 TargetSlotIndex) {
    FMythicInventorySourceLocator Source;
    Source.Inventory = SourceInventory;
    Source.SlotIndex = SourceSlotIndex;
    if (UMythicItemInstance *SourceItem = SourceInventory
            ? SourceInventory->GetItem(SourceSlotIndex) : nullptr) {
        Source.ExpectedItemGuid = SourceItem->GetItemInstanceGuid();
        Source.ExpectedQuantity = SourceItem->GetStacks();
    }

    FMythicInventoryTargetLocator Target;
    Target.Inventory = TargetInventory;
    Target.SlotIndex = TargetSlotIndex;
    if (UMythicItemInstance *TargetItem = TargetInventory
            ? TargetInventory->GetItem(TargetSlotIndex) : nullptr) {
        Target.bExpectEmpty = false;
        Target.ExpectedOccupantGuid = TargetItem->GetItemInstanceGuid();
        Target.ExpectedOccupantQuantity = TargetItem->GetStacks();
    }
    return SubmitInventoryMove(Source, Target);
}

int64 AMythicPlayerController::SubmitInventorySplit(
    const FMythicInventorySourceLocator &Source,
    const int32 Quantity) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::Split;
    Submission.Inventory = Source.Inventory;
    Submission.ItemGuid = Source.ExpectedItemGuid;
    Submission.SourceSlotIndex = Source.SlotIndex;
    Submission.RequestedQuantity = Quantity;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventorySplit(RequestId, Source, Quantity);
    return RequestId;
}

int64 AMythicPlayerController::SubmitInventoryDropQuantity(
    const FMythicInventorySourceLocator &Source,
    const int32 Quantity) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::DropQuantity;
    Submission.Inventory = Source.Inventory;
    Submission.ItemGuid = Source.ExpectedItemGuid;
    Submission.SourceSlotIndex = Source.SlotIndex;
    Submission.RequestedQuantity = Quantity;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventoryDropQuantity(RequestId, Source, Quantity);
    return RequestId;
}

int64 AMythicPlayerController::SubmitInventoryDropWholeSlot(
    UMythicInventoryComponent *Inventory,
    const int32 SlotIndex) {
    FMythicInventorySourceLocator Source;
    Source.Inventory = Inventory;
    Source.SlotIndex = SlotIndex;
    if (UMythicItemInstance *Item = Inventory
            ? Inventory->GetItem(SlotIndex) : nullptr) {
        Source.ExpectedItemGuid = Item->GetItemInstanceGuid();
        Source.ExpectedQuantity = Item->GetStacks();
    }
    return SubmitInventoryDropQuantity(Source, Source.ExpectedQuantity);
}

int64 AMythicPlayerController::SubmitInventoryUse(
    const FMythicInventorySourceLocator &Source) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::Use;
    Submission.Inventory = Source.Inventory;
    Submission.ItemGuid = Source.ExpectedItemGuid;
    Submission.SourceSlotIndex = Source.SlotIndex;
    Submission.RequestedQuantity = Source.ExpectedQuantity;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventoryUse(RequestId, Source);
    return RequestId;
}

int64 AMythicPlayerController::SubmitInventorySetJunk(
    const FMythicInventorySourceLocator &Source,
    const bool bMarkedJunk) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::SetJunk;
    Submission.Inventory = Source.Inventory;
    Submission.ItemGuid = Source.ExpectedItemGuid;
    Submission.SourceSlotIndex = Source.SlotIndex;
    Submission.RequestedQuantity = Source.ExpectedQuantity;
    Submission.bDesiredManualJunk = bMarkedJunk;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventorySetJunk(RequestId, Source, bMarkedJunk);
    return RequestId;
}

int64 AMythicPlayerController::SubmitInventorySort(
    UMythicInventoryComponent *Inventory,
    const FGameplayTag GroupTag,
    const ESortMode SortMode) {
    FMythicInventoryActionSubmission Submission;
    Submission.Action = EMythicInventoryAction::Sort;
    Submission.Inventory = Inventory;
    Submission.GroupTag = GroupTag;
    const int64 RequestId = BeginInventoryActionSubmission(Submission);
    if (RequestId <= 0) {
        return 0;
    }
    ServerRequestInventorySort(RequestId, Inventory, GroupTag, SortMode);
    return RequestId;
}

void AMythicPlayerController::ServerRequestInventoryMove_Implementation(
    const int64 RequestId,
    const FMythicInventorySourceLocator &Source,
    const FMythicInventoryTargetLocator &Target) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::Move;
    Receipt.ItemGuid = Source.ExpectedItemGuid;
    Receipt.SourceSlotIndex = Source.SlotIndex;
    Receipt.TargetSlotIndex = Target.SlotIndex;

    if (!AcceptNewInventoryActionRequestId(RequestId)
        || !Source.IsStructurallyValid()
        || !Target.IsStructurallyValid()) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (Source.Inventory != Target.Inventory
             || !IsPlayerOwnedInventory(Source.Inventory)
             || !IsPlayerOwnedInventory(Target.Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        Receipt.Result = Source.Inventory->TryPlayerMoveItem(
            Source, Target, Receipt.QuantityProcessed);
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ServerRequestInventorySplit_Implementation(
    const int64 RequestId,
    const FMythicInventorySourceLocator &Source,
    const int32 Quantity) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::Split;
    Receipt.ItemGuid = Source.ExpectedItemGuid;
    Receipt.SourceSlotIndex = Source.SlotIndex;

    if (!AcceptNewInventoryActionRequestId(RequestId)
        || !Source.IsStructurallyValid() || Quantity <= 0) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (!IsPlayerOwnedInventory(Source.Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        Receipt.Result = Source.Inventory->TrySplitStackToFreeSlot(
            Source, Quantity, Receipt.TargetSlotIndex);
        if (Receipt.WasSuccessful()) {
            Receipt.QuantityProcessed = Quantity;
        }
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ServerRequestInventoryDropQuantity_Implementation(
    const int64 RequestId,
    const FMythicInventorySourceLocator &Source,
    const int32 Quantity) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::DropQuantity;
    Receipt.ItemGuid = Source.ExpectedItemGuid;
    Receipt.SourceSlotIndex = Source.SlotIndex;

    APawn *PlayerPawn = GetPawn();
    if (!AcceptNewInventoryActionRequestId(RequestId)
        || !Source.IsStructurallyValid() || Quantity <= 0
        || !IsValid(PlayerPawn)) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (!IsPlayerOwnedInventory(Source.Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        AMythicWorldItem *SpawnedWorldItem = nullptr;
        Receipt.Result = Source.Inventory->DropItemQuantity(
            Source, Quantity, PlayerPawn->GetActorLocation(), 100.0f, this,
            Receipt.QuantityProcessed, SpawnedWorldItem);
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ServerRequestInventoryUse_Implementation(
    const int64 RequestId,
    const FMythicInventorySourceLocator &Source) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::Use;
    Receipt.ItemGuid = Source.ExpectedItemGuid;
    Receipt.SourceSlotIndex = Source.SlotIndex;

    if (!AcceptNewInventoryActionRequestId(RequestId)
        || !Source.IsStructurallyValid()) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (!IsPlayerOwnedInventory(Source.Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        Receipt.Result = Source.Inventory->TryUseItemInSlot(
            Source, Receipt.QuantityProcessed);
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ServerRequestInventorySetJunk_Implementation(
    const int64 RequestId,
    const FMythicInventorySourceLocator &Source,
    const bool bMarkedJunk) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::SetJunk;
    Receipt.ItemGuid = Source.ExpectedItemGuid;
    Receipt.SourceSlotIndex = Source.SlotIndex;

    if (!AcceptNewInventoryActionRequestId(RequestId)
        || !Source.IsStructurallyValid()) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (!IsPlayerOwnedInventory(Source.Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        Receipt.Result = Source.Inventory->TrySetItemJunk(
            Source, bMarkedJunk);
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ServerRequestInventorySort_Implementation(
    const int64 RequestId,
    UMythicInventoryComponent *Inventory,
    const FGameplayTag GroupTag,
    const ESortMode SortMode) {
    if (ReplayCachedInventoryActionReceipt(RequestId)) {
        return;
    }

    FMythicInventoryActionReceipt Receipt;
    Receipt.RequestId = RequestId;
    Receipt.Action = EMythicInventoryAction::Sort;

    if (!AcceptNewInventoryActionRequestId(RequestId) || !Inventory) {
        Receipt.Result = EMythicInventoryActionResult::InvalidRequest;
    }
    else if (!IsPlayerOwnedInventory(Inventory)) {
        Receipt.Result = EMythicInventoryActionResult::UnauthorizedInventory;
    }
    else {
        Receipt.Result = Inventory->TrySortGroup(GroupTag, SortMode);
    }
    CompleteInventoryActionRequest(Receipt);
}

void AMythicPlayerController::ClientReceiveInventoryActionReceipt_Implementation(
    const FMythicInventoryActionReceipt &Receipt) {
    if (Receipt.RequestId > 0) {
        if (!ReceivedInventoryActionReceipts.Contains(Receipt.RequestId)) {
            if (ReceivedInventoryActionReceiptOrder.Num()
                >= MaxInventoryActionReceiptCacheSize) {
                const int64 OldestRequestId =
                    ReceivedInventoryActionReceiptOrder[0];
                ReceivedInventoryActionReceiptOrder.RemoveAt(
                    0, 1, EAllowShrinking::No);
                ReceivedInventoryActionReceipts.Remove(OldestRequestId);
            }
            ReceivedInventoryActionReceiptOrder.Add(Receipt.RequestId);
        }
        ReceivedInventoryActionReceipts.Add(Receipt.RequestId, Receipt);
    }
    if (Receipt.RequestId > 0
        && Receipt.RequestId == ActiveInventoryActionRequestId) {
        ActiveInventoryActionRequestId = 0;
    }
    OnInventoryActionReceiptReceived.Broadcast(Receipt);
}

void AMythicPlayerController::ServerMoveItemBetweenInventories_Implementation(UMythicInventoryComponent *Source, int32 SourceSlot,
                                                                              UMythicInventoryComponent *Target, int32 TargetSlot) {
    if (!HasAuthority() || !Source || !Target) {
        return;
    }

    if (!CanPlayerAccessInventory(Source) || !CanPlayerAccessInventory(Target)) {
        return;
    }

    if (!Source->CanPlayerTakeFromSlot(SourceSlot)) {
        return;
    }

    Source->SendItem(SourceSlot, Target, TargetSlot);

    if (const AActor *SourceOwnerActor = Source->GetOwner()) {
        if (const UMythicOwnershipComponent *Ownership = SourceOwnerActor->FindComponentByClass<UMythicOwnershipComponent>()) {
            if (Ownership->IsOwned() && GetAllInventoryComponents().Contains(Target)) {
                MythicTheftCrime::TrySubmitTheft(GetPawn(), const_cast<AActor *>(SourceOwnerActor), Ownership->GetOwnership());
            }
        }
    }
}

int32 AMythicPlayerController::GetCarriedCurrency() const {
    int32 Total = 0;
    for (UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (Inv) {
            Total += Inv->GetTotalCurrency();
        }
    }
    return Total;
}


bool AMythicPlayerController::ServerVendorBuy_Validate(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity) {
    return Vendor != nullptr && StockSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerVendorBuy_Implementation(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteBuy(this, StockSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
    RecordVendorAcquaintance(Vendor, Plan);
}

bool AMythicPlayerController::ServerVendorSell_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex,
                                                        int32 Quantity) {
    return Vendor != nullptr && PlayerInventory != nullptr && PlayerSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerVendorSell_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                              int32 PlayerSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteSell(this, PlayerInventory, PlayerSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
    RecordVendorAcquaintance(Vendor, Plan);
}

bool AMythicPlayerController::ServerStallBuy_Validate(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity) {
    return Stall != nullptr && StallSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerStallBuy_Implementation(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Stall) {
        return;
    }
    if (!CanPlayerAccessInventory(Stall->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Stall->Server_ExecuteStallPurchase(this, StallSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

void AMythicPlayerController::RecordVendorAcquaintance(const AMythicVendor *Vendor, const FMythicTradePlan &Plan) {
    if (!HasAuthority() || !Vendor || Plan.Quantity <= 0) {
        return;
    }
    AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>();
    UMythicAcquaintanceComponent *Acquaintance = PS ? PS->GetAcquaintanceComponent() : nullptr;
    if (!Acquaintance) {
        return;
    }

    FGameplayTag VendorFactionTag;
    if (const UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr) {
        if (const UMythicFactionDatabase *FactionDB = LW->GetFactionDatabase()) {
            FMythicFactionData FactionData;
            if (FactionDB->GetFaction(Vendor->GetVendorFaction(), FactionData)) {
                VendorFactionTag = FactionData.FactionTag;
            }
        }
    }

    Acquaintance->ServerRecordInteraction(GetTypeHash(Vendor->GetFName()), VendorFactionTag, EMythicNpcInteraction::Traded);
}

bool AMythicPlayerController::ServerVendorRepair_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex) {
    return Vendor != nullptr && PlayerInventory != nullptr && PlayerSlotIndex >= 0;
}

void AMythicPlayerController::ServerVendorRepair_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                               int32 PlayerSlotIndex) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteRepair(this, PlayerInventory, PlayerSlotIndex);
    if (Plan.Result == EMythicTradeResult::Success) {
        if (UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex)) {
            if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                ClientNotifyItemDurability(Def->Name, EMythicItemDurabilityBeat::Repaired);
            }
        }
    }
    else if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

bool AMythicPlayerController::ServerVendorRepairAll_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory) {
    return Vendor != nullptr && PlayerInventory != nullptr;
}

void AMythicPlayerController::ServerVendorRepairAll_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteRepairAll(this, PlayerInventory);
    if (Plan.Result == EMythicTradeResult::Success) {
        ClientNotifyItemDurability(FText::GetEmpty(), EMythicItemDurabilityBeat::Repaired);
    }
    else if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

bool AMythicPlayerController::ServerBuyback_Validate(AMythicVendor *Vendor, int32 BuybackIndex) {
    return Vendor != nullptr && BuybackIndex >= 0;
}

void AMythicPlayerController::ServerBuyback_Implementation(AMythicVendor *Vendor, int32 BuybackIndex) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteBuyback(this, BuybackIndex);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}


bool AMythicPlayerController::ServerSetItemJunk_Validate(UMythicItemInstance *Item, bool bJunk) {
    return Item != nullptr;
}

void AMythicPlayerController::ServerSetItemJunk_Implementation(UMythicItemInstance *Item, bool bJunk) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    UMythicInventoryComponent *Inventory = Item->GetInventoryComponent();
    if (!GetAllInventoryComponents().Contains(Inventory)) {
        return;
    }

    FMythicInventorySourceLocator Source;
    Source.Inventory = Inventory;
    Source.SlotIndex = Item->GetSlot();
    Source.ExpectedItemGuid = Item->GetItemInstanceGuid();
    Source.ExpectedQuantity = Item->GetStacks();
    Inventory->TrySetItemJunk(Source, bJunk);
}

bool AMythicPlayerController::ServerSellAllJunk_Validate(AMythicVendor *Vendor) {
    return Vendor != nullptr;
}

void AMythicPlayerController::ServerSellAllJunk_Implementation(AMythicVendor *Vendor) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }

    for (UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        const int32 NumSlots = Inv->GetNumSlots();
        for (int32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx) {
            FMythicInventorySlotEntry Entry;
            if (!Inv->GetSlotEntry(SlotIdx, Entry)) {
                continue;
            }
            UMythicItemInstance *Item = Entry.SlottedItemInstance;
            if (!IsValid(Item)) {
                continue;
            }
            const UItemDefinition *Def = Item->GetItemDefinition();
            if (!Def) {
                continue;
            }
            const bool bIsCurrency = Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
            const bool bCanTake = Inv->CanPlayerTakeFromSlot(SlotIdx);
            const bool bJunk = MythicLootFilter::IsJunk(Item->IsMarkedJunk(),
                                                        static_cast<int32>(Def->Rarity.GetValue()),
                                                        MythicLootFilter::DefaultMaxJunkRarity,
                                                        Def->Value, bIsCurrency, Entry.IsGearSlot(), bCanTake);
            if (!bJunk) {
                continue;
            }
            Vendor->Server_ExecuteSell(this, Inv, SlotIdx, Item->GetStacks());
        }
    }
}


bool AMythicPlayerController::IsWithinGiftRange(const AMythicPlayerController *Other) const {
    const APawn *MyPawn = GetPawn();
    const APawn *OtherPawn = Other ? Other->GetPawn() : nullptr;
    if (!MyPawn || !OtherPawn) {
        return false;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Range = Settings ? FMath::Max(1.0f, Settings->GiftRange) : 350.0f;
    return FVector::DistSquared(MyPawn->GetActorLocation(), OtherPawn->GetActorLocation()) <= FMath::Square(Range);
}

void AMythicPlayerController::ClearPendingGift() {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(PendingGiftTimerHandle);
    }
    PendingGiftGiver = nullptr;
    PendingGiftSourceInv = nullptr;
    PendingGiftItem = nullptr;
    PendingGiftSourceSlot = INDEX_NONE;
    PendingGiftQuantity = 0;
}

void AMythicPlayerController::OnPendingGiftExpired() {
    ClearPendingGift();
}

bool AMythicPlayerController::ServerOfferGift_Validate(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity) {
    return Recipient != nullptr && SourceInv != nullptr && SourceSlotIndex >= 0;
}

void AMythicPlayerController::ServerOfferGift_Implementation(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Recipient || !SourceInv) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(SourceInv)) {
        return;
    }
    UMythicItemInstance *Item = SourceInv->GetItem(SourceSlotIndex);
    const bool bTakeable = (Item != nullptr) && SourceInv->CanPlayerTakeFromSlot(SourceSlotIndex);
    if (!MythicGift::CanOfferGift( true, Recipient != this,
                                  IsWithinGiftRange(Recipient), bTakeable)) {
        return;
    }

    Recipient->ClearPendingGift();
    Recipient->PendingGiftGiver = this;
    Recipient->PendingGiftSourceInv = SourceInv;
    Recipient->PendingGiftItem = Item;
    Recipient->PendingGiftSourceSlot = SourceSlotIndex;
    Recipient->PendingGiftQuantity = Quantity;
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Timeout = Settings ? Settings->GiftOfferTimeoutSeconds : 20.0f;
    if (UWorld *W = GetWorld(); W && Timeout > 0.0f) {
        W->GetTimerManager().SetTimer(Recipient->PendingGiftTimerHandle, Recipient,
                                      &AMythicPlayerController::OnPendingGiftExpired, Timeout,false);
    }

    FText ItemName;
    if (const UItemDefinition *Def = Item->GetItemDefinition()) {
        ItemName = Def->Name;
    }
    Recipient->ClientReceiveGiftOffer(this, ItemName);
}

void AMythicPlayerController::ClientReceiveGiftOffer_Implementation(AMythicPlayerController *Giver, const FText &ItemName) {
    OnGiftOffered(Giver, ItemName);
}

void AMythicPlayerController::ServerRespondGift_Implementation(bool bAccept) {
    if (!HasAuthority()) {
        return;
    }
    AMythicPlayerController *Giver = PendingGiftGiver.Get();
    UMythicInventoryComponent *SourceInv = PendingGiftSourceInv.Get();
    UMythicItemInstance *OfferedItem = PendingGiftItem.Get();
    const int32 SourceSlot = PendingGiftSourceSlot;

    const bool bGiverValid = (Giver != nullptr) && (SourceInv != nullptr) && (OfferedItem != nullptr);
    const bool bInRange = bGiverValid && IsWithinGiftRange(Giver);
    const bool bItemStillThere = bGiverValid && (SourceInv->GetItem(SourceSlot) == OfferedItem);

    FText ItemName;
    if (OfferedItem) {
        if (const UItemDefinition *Def = OfferedItem->GetItemDefinition()) {
            ItemName = Def->Name;
        }
    }

    if (!MythicGift::CanCompleteGift(HasPendingGift(), bAccept, bGiverValid, bInRange, bItemStillThere)) {
        if (HasPendingGift()) {
            if (!bAccept && Giver) {
                Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Declined", "Gift declined"), FLinearColor(0.7f, 0.7f, 0.7f));
            }
            else if (bAccept) {
                ClientNotifyGiftResult(NSLOCTEXT("Gift", "Unavailable", "Gift no longer available"), FLinearColor(0.7f, 0.7f, 0.7f));
            }
        }
        ClearPendingGift();
        return;
    }

    UMythicInventoryComponent *DestInv = nullptr;
    if (const UItemDefinition *Def = OfferedItem->GetItemDefinition()) {
        DestInv = GetInventoryForItemType(Def->ItemType);
    }
    EMythicGiftResult Result = EMythicGiftResult::NoRoom;
    int32 Moved = 0;
    if (DestInv) {
        const UItemDefinition *OfferedDef = OfferedItem->GetItemDefinition();
        const int32 StacksBefore = OfferedItem->GetStacks();
        const int32 GiftQty = MythicGift::ComputeGiftQuantity(PendingGiftQuantity, StacksBefore);

        if (GiftQty >= StacksBefore) {
            FMythicInventorySourceLocator GiftSource;
            GiftSource.Inventory = SourceInv;
            GiftSource.SlotIndex = SourceSlot;
            GiftSource.ExpectedItemGuid = OfferedItem->GetItemInstanceGuid();
            GiftSource.ExpectedQuantity = StacksBefore;
            SourceInv->TryTransferPlayerItemToAnySlot(GiftSource, DestInv, Moved);
            Result = MythicGift::ClassifyGiftMove(StacksBefore, Moved);
        }
        else {
            FMythicInventorySourceLocator GiftSource;
            GiftSource.Inventory = SourceInv;
            GiftSource.SlotIndex = SourceSlot;
            GiftSource.ExpectedItemGuid = OfferedItem->GetItemInstanceGuid();
            GiftSource.ExpectedQuantity = StacksBefore;
            int32 SplitSlot = INDEX_NONE;
            if (SourceInv->TrySplitStackToFreeSlot(GiftSource, GiftQty, SplitSlot)
                != EMythicInventoryActionResult::Succeeded) {
                Result = EMythicGiftResult::NoRoom;
                Moved = 0;
            }
            else {
                UMythicItemInstance *SplitItem = SourceInv->GetItem(SplitSlot);
                if (SplitItem && SplitItem->GetItemDefinition() == OfferedDef) {
                    FMythicInventorySourceLocator SplitSource;
                    SplitSource.Inventory = SourceInv;
                    SplitSource.SlotIndex = SplitSlot;
                    SplitSource.ExpectedItemGuid = SplitItem->GetItemInstanceGuid();
                    SplitSource.ExpectedQuantity = SplitItem->GetStacks();
                    SourceInv->TryTransferPlayerItemToAnySlot(SplitSource, DestInv, Moved);
                }
                Result = MythicGift::ClassifyGiftMove(GiftQty, Moved);
            }
        }
    }

    if (Result == EMythicGiftResult::Success || Result == EMythicGiftResult::Partial) {
        ClientNotifyGiftResult(FText::Format(NSLOCTEXT("Gift", "Received", "Received {0} x{1}"), ItemName, FText::AsNumber(Moved)),
                               FLinearColor(0.45f, 0.9f, 0.45f));
    }
    if (Giver) {
        switch (Result) {
        case EMythicGiftResult::Success:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Given", "Gift given"), FLinearColor(0.45f, 0.9f, 0.45f));
            break;
        case EMythicGiftResult::Partial:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Partial", "Gift partly given (no room for all)"), FLinearColor(0.95f, 0.8f, 0.3f));
            break;
        default:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "NoRoom", "Recipient has no room"), FLinearColor(1.0f, 0.5f, 0.3f));
            break;
        }
    }
    ClearPendingGift();
}

void AMythicPlayerController::ClientNotifyGiftResult_Implementation(const FText &Message, FLinearColor Color) {
}

bool AMythicPlayerController::ServerDeployPlaceable_Validate(UMythicInventoryComponent *Inventory, int32 SlotIndex,
                                                            FVector AimOrigin, FVector AimDirection) {
    return Inventory != nullptr && SlotIndex >= 0 && !AimDirection.IsNearlyZero();
}

void AMythicPlayerController::ServerDeployPlaceable_Implementation(UMythicInventoryComponent *Inventory, int32 SlotIndex,
                                                                  FVector AimOrigin, FVector AimDirection) {
    if (!HasAuthority() || !Inventory) {
        return;
    }
    UWorld *World = GetWorld();
    APawn *MyPawn = GetPawn();
    if (!World || !MyPawn) {
        return;
    }

    const bool bAuthorized = CanPlayerAccessInventory(Inventory);
    UMythicItemInstance *Item = Inventory->GetItem(SlotIndex);
    const UPlaceableFragment *Placeable = Item ? Item->GetFragment<UPlaceableFragment>() : nullptr;
    const bool bHasPlaceableItem = (Item != nullptr) && (Placeable != nullptr) && Inventory->CanPlayerTakeFromSlot(SlotIndex);
    const bool bHasDeployedClass = (Placeable != nullptr) && !Placeable->DeployedActorClass.IsNull();

    EPlaceablePlacementResult Placement = EPlaceablePlacementResult::NoSurface;
    FVector CandidatePoint = FVector::ZeroVector;
    if (Placeable) {
        const FVector Dir = AimDirection.GetSafeNormal();
        const FVector TraceEnd = AimOrigin + Dir * Placeable->MaxPlacementReach;

        FCollisionQueryParams TraceParams(FName(TEXT("MythicDeployPlaceable")), false, MyPawn);
        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, ECC_Visibility, TraceParams);
        CandidatePoint = bHit ? Hit.ImpactPoint : TraceEnd;

        const bool bBlocked = World->OverlapAnyTestByChannel(CandidatePoint, FQuat::Identity, ECC_Pawn,
                                                             FCollisionShape::MakeSphere(Placeable->RequiredClearanceRadius), TraceParams);

        const FPlaceablePlacementQuery Query = UPlaceableFragment::BuildPlacementQuery(
            bHit, Hit.ImpactPoint, Hit.ImpactNormal, TraceEnd, MyPawn->GetActorLocation(), bBlocked);
        Placement = Placeable->EvaluatePlacement(Query);
    }

    const EPlaceableDeployResult Decision = UPlaceableFragment::PlanDeploy(bAuthorized, bHasPlaceableItem, bHasDeployedClass, Placement);
    if (Decision != EPlaceableDeployResult::Deployed) {
        const FText Reason = UPlaceableFragment::DescribeDeployFailure(Decision, Placement);
        if (!Reason.IsEmpty()) {
            ClientNotifyDeployRejected(Reason);
        }
        return;
    }

    FMythicPendingDeploy Pending;
    Pending.Inventory = Inventory;
    Pending.Item = Item;
    Pending.SlotIndex = SlotIndex;
    Pending.SpawnTransform = FTransform(FRotator(0.0f, MyPawn->GetActorRotation().Yaw, 0.0f), CandidatePoint);

    if (UClass *Resident = Placeable->DeployedActorClass.Get()) {
        FinishDeployPlaceable(Resident, Pending);
        return;
    }
    const FSoftObjectPath ClassPath = Placeable->DeployedActorClass.ToSoftObjectPath();
    UAssetManager::GetStreamableManager().RequestAsyncLoad(
        ClassPath, FStreamableDelegate::CreateUObject(this, &AMythicPlayerController::HandleDeployClassLoaded, ClassPath, Pending));
}

void AMythicPlayerController::HandleDeployClassLoaded(FSoftObjectPath ClassPath, FMythicPendingDeploy Pending) {
    UClass *DeployedClass = Cast<UClass>(ClassPath.ResolveObject());
    FinishDeployPlaceable(DeployedClass, Pending);
}

void AMythicPlayerController::FinishDeployPlaceable(UClass *DeployedClass, const FMythicPendingDeploy &Pending) {
    if (!HasAuthority() || !DeployedClass) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicInventoryComponent *Inventory = Pending.Inventory.Get();
    UMythicItemInstance *Item = Pending.Item.Get();
    if (!World || !Inventory || !Item) {
        return;
    }

    if (Inventory->GetItem(Pending.SlotIndex) != Item || !Inventory->CanPlayerTakeFromSlot(Pending.SlotIndex) ||
        !CanPlayerAccessInventory(Inventory)) {
        return;
    }

    const bool bCapped = MaxDeployedPlaceables > 0;
    if (bCapped) {
        DeployedPlaceables.RemoveAll([](const TWeakObjectPtr<AActor> &P) { return !P.IsValid(); });
        if (!CanDeployMore(DeployedPlaceables.Num(), MaxDeployedPlaceables)) {
            ClientNotifyDeployRejected(NSLOCTEXT("Placeable", "DeployAtCap", "Build limit reached"));
            return;
        }
    }

    AActor *Deployed = World->SpawnActorDeferred<AActor>(DeployedClass, Pending.SpawnTransform, this, GetPawn(),
                                                         ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Deployed) {
        return;
    }
    Deployed->FinishSpawning(Pending.SpawnTransform);
    Item->ConsumeItem(1);

    if (bCapped) {
        DeployedPlaceables.Add(Deployed);
    }
}

bool AMythicPlayerController::CanDeployMore(int32 CurrentValidCount, int32 MaxAllowed) {
    return MaxAllowed <= 0 || CurrentValidCount < MaxAllowed;
}

void AMythicPlayerController::ClientNotifyDeployRejected_Implementation(const FText &Reason) {
    OnDeployRejected(Reason);
}

bool AMythicPlayerController::ServerInteractPrimary_Validate(AActor *Interactable) {
    return Interactable != nullptr;
}

void AMythicPlayerController::ServerInteractPrimary_Implementation(AActor *Interactable) {
    if (!HasAuthority() || !IsValid(Interactable)) {
        return;
    }
    if (!Interactable->GetClass()->ImplementsInterface(UMythicInteractable::StaticClass())) {
        return;
    }

    if (const APawn *MyPawn = GetPawn()) {
        float ReachSq = FMath::Square(400.0f);
        const UMythicInteractionComponent *Interaction = FindComponentByClass<UMythicInteractionComponent>();
        if (!Interaction) {
            Interaction = MyPawn->FindComponentByClass<UMythicInteractionComponent>();
        }
        if (Interaction) {
            ReachSq = FMath::Square(Interaction->InteractionRange * 1.5f);
        }
        if (FVector::DistSquared(MyPawn->GetActorLocation(), Interactable->GetActorLocation()) > ReachSq) {
            return;
        }
    }

    IMythicInteractable::Execute_OnPrimaryInteract(Interactable, this);
}

void AMythicPlayerController::ResetPendingContextActionHold() {
    PendingContextActionHold.Reset();
}

void AMythicPlayerController::EnterContextActionAuthorityBarrier() {
    if (!HasAuthority()) {
        return;
    }
    ResetPendingContextActionHold();
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(
            ContextActionOfferRefreshTimerHandle);
    }
    if (UMythicEntityActionGrantComponent *Grants =
            ResolveEntityActionGrantComponent()) {
        Grants->AuthorityRevokeAllActionGrants();
    }
    AuthorityRequestedContextActionSubject.Reset();
}

bool AMythicPlayerController::ResolveAndValidateContextActionRequest(
    const FMythicEntityPresentationInstance &Subject,
    const FGameplayTag ActionTag, const uint32 ObservedOfferRevision,
    UMythicEntityActionGrantComponent *&OutGrantComponent,
    UObject *&OutProvider,
    UMythicContextActionDefinition *&OutDefinition,
    AActor *&OutSubjectActor,
    uint32 &OutProviderSourceRevision,
    FGameplayTag &OutFailureReason) {
    OutGrantComponent = nullptr;
    OutProvider = nullptr;
    OutDefinition = nullptr;
    OutSubjectActor = nullptr;
    OutProviderSourceRevision = 0;
    OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;

    if (!HasAuthority() || !Subject.IsValid() || !ActionTag.IsValid()
        || !ActionTag.MatchesTag(CONTEXT_ACTION_ROOT)
        || ActionTag.MatchesTag(CONTEXT_ACTION_REASON_ROOT)
        || AuthorityRequestedContextActionSubject != Subject) {
        return false;
    }

    const FMythicContextActionProjectionRuntimePolicy Policy =
        FMythicContextActionProjectionRules::BuildRuntimePolicy(
            ContextActionProjectionPolicy);
    if (!Policy.bValid) {
        OutFailureReason = CONTEXT_ACTION_REASON_UNAVAILABLE;
        return false;
    }

    UWorld *World = GetWorld();
    UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
              : nullptr;
    UMythicEntityPresentationComponent *Presentation = Registry
        ? Registry->ResolvePresentationComponent(Subject) : nullptr;
    AActor *SubjectActor = Presentation ? Presentation->GetOwner() : nullptr;
    if (!Presentation || !IsValid(SubjectActor)
        || SubjectActor->GetWorld() != World
        || !FMythicContextActionProjectionRules::IsExactResolvedSubject(
            Subject, Presentation->GetPresentationInstance())) {
        return false;
    }

    const FVector SubjectLocation =
        Presentation->GetPresentationAnchorWorldLocation();
    FVector ViewLocation;
    FVector ViewForward;
    float SubjectDistanceSquared = 0.0f;
    if (!MythicValidateContextActionDiscovery(
            this, SubjectActor, SubjectLocation, Policy, ViewLocation,
            ViewForward, SubjectDistanceSquared, OutFailureReason)) {
        return false;
    }

    UMythicEntityActionGrantComponent *GrantComponent =
        ResolveEntityActionGrantComponent();
    FMythicReplicatedContextActionGrant CurrentGrant;
    if (!GrantComponent
        || !GrantComponent->FindActionGrant(Subject, ActionTag, CurrentGrant)
        || CurrentGrant.State != EMythicContextActionGrantState::Available
        || CurrentGrant.OfferRevision != ObservedOfferRevision) {
        OutFailureReason = CONTEXT_ACTION_REASON_STALE;
        return false;
    }

    UObject *BoundProvider = nullptr;
    UMythicContextActionDefinition *BoundDefinition = nullptr;
    uint32 ProviderSourceRevision = 0;
    if (!GrantComponent->AuthorityResolveActionGrantBinding(
            Subject, ActionTag, ObservedOfferRevision, BoundProvider,
            BoundDefinition, ProviderSourceRevision)
        || !IsValid(BoundProvider) || !IsValid(BoundDefinition)) {
        GrantComponent->AuthorityRevokeActionGrant(Subject, ActionTag);
        OutFailureReason = CONTEXT_ACTION_REASON_STALE;
        return false;
    }

    OutFailureReason = FGameplayTag();
    if (!MythicValidateContextActionSpatialRules(
            this, SubjectActor, SubjectLocation, *BoundDefinition,
            Policy, OutFailureReason)) {
        return false;
    }

    OutGrantComponent = GrantComponent;
    OutProvider = BoundProvider;
    OutDefinition = BoundDefinition;
    OutSubjectActor = SubjectActor;
    OutProviderSourceRevision = ProviderSourceRevision;
    return true;
}

void AMythicPlayerController::ServerBeginContextActionHold_Implementation(
    const FMythicEntityPresentationInstance Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision) {
    auto Reject = [this](const FGameplayTag Reason) {
        ClientNotifyContextActionRejected(
            MythicSanitizeContextActionFailure(Reason));
    };
    ResetPendingContextActionHold();
    if (ObservedOfferRevision <= 0
        || ObservedOfferRevision > static_cast<int64>(MAX_uint32)) {
        Reject(CONTEXT_ACTION_REASON_INVALID_TARGET);
        return;
    }

    UMythicEntityActionGrantComponent *GrantComponent = nullptr;
    UObject *Provider = nullptr;
    UMythicContextActionDefinition *Definition = nullptr;
    AActor *SubjectActor = nullptr;
    uint32 ProviderSourceRevision = 0;
    FGameplayTag FailureReason;
    const uint32 CompactRevision =
        static_cast<uint32>(ObservedOfferRevision);
    if (!ResolveAndValidateContextActionRequest(
            Subject, ActionTag, CompactRevision, GrantComponent, Provider,
            Definition, SubjectActor, ProviderSourceRevision, FailureReason)
        || !Definition
        || !FMythicContextActionProjectionRules::IsHoldDurationValid(
            Definition->HoldDurationSeconds)
        || Definition->HoldDurationSeconds <= 0.0f) {
        if (GrantComponent && Definition
            && !FMythicContextActionProjectionRules::IsHoldDurationValid(
                Definition->HoldDurationSeconds)) {
            GrantComponent->AuthorityRevokeActionGrant(Subject, ActionTag);
        }
        Reject(FailureReason.IsValid()
                   ? FailureReason : CONTEXT_ACTION_REASON_UNAVAILABLE);
        return;
    }

    FailureReason = FGameplayTag();
    if (!IMythicContextActionProvider::Execute_CanExecuteContextAction(
            Provider, this, SubjectActor, ActionTag,
            static_cast<int64>(ProviderSourceRevision), FailureReason)) {
        GrantComponent->AuthorityRevokeActionGrant(Subject, ActionTag);
        Reject(FailureReason);
        return;
    }
    UObject *RevalidatedProvider = nullptr;
    UMythicContextActionDefinition *RevalidatedDefinition = nullptr;
    uint32 RevalidatedSourceRevision = 0;
    if (!GrantComponent->AuthorityResolveActionGrantBinding(
            Subject, ActionTag, CompactRevision, RevalidatedProvider,
            RevalidatedDefinition, RevalidatedSourceRevision)
        || RevalidatedProvider != Provider
        || RevalidatedDefinition != Definition
        || RevalidatedSourceRevision != ProviderSourceRevision) {
        Reject(CONTEXT_ACTION_REASON_STALE);
        return;
    }

    PendingContextActionHold.Subject = Subject;
    PendingContextActionHold.ActionTag = ActionTag;
    PendingContextActionHold.OfferRevision = CompactRevision;
    PendingContextActionHold.AuthorityStartSeconds =
        GetContextActionRequestClockSeconds();
    PendingContextActionHold.RequiredDurationSeconds =
        Definition->HoldDurationSeconds;
}

void AMythicPlayerController::ServerCancelContextActionHold_Implementation(
    const FMythicEntityPresentationInstance Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision) {
    if (!HasAuthority() || ObservedOfferRevision <= 0
        || ObservedOfferRevision > static_cast<int64>(MAX_uint32)) {
        return;
    }
    if (PendingContextActionHold.Matches(
            Subject, ActionTag,
            static_cast<uint32>(ObservedOfferRevision))) {
        ResetPendingContextActionHold();
    }
}

void AMythicPlayerController::ServerExecuteContextAction_Implementation(
    const FMythicEntityPresentationInstance Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision) {
    auto Reject = [this](const FGameplayTag Reason) {
        ClientNotifyContextActionRejected(
            MythicSanitizeContextActionFailure(Reason));
    };
    if (ObservedOfferRevision <= 0
        || ObservedOfferRevision > static_cast<int64>(MAX_uint32)) {
        Reject(CONTEXT_ACTION_REASON_INVALID_TARGET);
        return;
    }

    const uint32 CompactRevision =
        static_cast<uint32>(ObservedOfferRevision);
    const bool bTargetsPendingHold = PendingContextActionHold.Matches(
        Subject, ActionTag, CompactRevision);
    UMythicEntityActionGrantComponent *GrantComponent = nullptr;
    UObject *Provider = nullptr;
    UMythicContextActionDefinition *Definition = nullptr;
    AActor *SubjectActor = nullptr;
    uint32 ResolvedProviderSourceRevision = 0;
    FGameplayTag FailureReason;
    if (!ResolveAndValidateContextActionRequest(
            Subject, ActionTag, CompactRevision, GrantComponent, Provider,
            Definition, SubjectActor, ResolvedProviderSourceRevision,
            FailureReason)
        || !GrantComponent || !IsValid(Provider) || !IsValid(Definition)
        || !IsValid(SubjectActor)) {
        if (bTargetsPendingHold) {
            ResetPendingContextActionHold();
        }
        Reject(FailureReason);
        return;
    }

    const FMythicAuthorityContextActionDefinitionSignature
        ValidatedDefinitionSignature =
            FMythicAuthorityContextActionDefinitionSignature::Capture(
                *Definition);
    const FMythicContextActionProjectionRuntimePolicy ValidatedPolicy =
        FMythicContextActionProjectionRules::BuildRuntimePolicy(
            ContextActionProjectionPolicy);
    if (!ValidatedPolicy.bValid) {
        GrantComponent->AuthorityRevokeActionGrant(Subject, ActionTag);
        ResetPendingContextActionHold();
        Reject(CONTEXT_ACTION_REASON_UNAVAILABLE);
        return;
    }

    if (!FMythicContextActionProjectionRules::IsHoldDurationValid(
            Definition->HoldDurationSeconds)) {
        GrantComponent->AuthorityRevokeActionGrant(Subject, ActionTag);
        ResetPendingContextActionHold();
        Reject(CONTEXT_ACTION_REASON_UNAVAILABLE);
        return;
    }

    if (Definition->HoldDurationSeconds > 0.0f) {
        const bool bMatchingHold = PendingContextActionHold.Matches(
            Subject, ActionTag, CompactRevision);
        const bool bUnchangedDuration = bMatchingHold
            && FMath::IsNearlyEqual(
                PendingContextActionHold.RequiredDurationSeconds,
                Definition->HoldDurationSeconds, UE_KINDA_SMALL_NUMBER);
        const bool bTimingValid = bUnchangedDuration
            && FMythicContextActionProjectionRules::
                IsHoldCompletionTimingValid(
                    PendingContextActionHold.AuthorityStartSeconds,
                    GetContextActionRequestClockSeconds(),
                    Definition->HoldDurationSeconds);
        // A completion attempt consumes the timed handshake whether it was early, stale, or valid.
        ResetPendingContextActionHold();
        if (!bTimingValid) {
            Reject(CONTEXT_ACTION_REASON_STALE);
            return;
        }
    } else {
        // Tap actions remain immediate and cannot leave an unrelated timed authorization resident.
        ResetPendingContextActionHold();
    }

    UObject *ConsumedProvider = nullptr;
    UMythicContextActionDefinition *ConsumedDefinition = nullptr;
    uint32 ConsumedProviderSourceRevision = 0;
    if (!GrantComponent->AuthorityConsumeActionGrantBinding(
            Subject, ActionTag, CompactRevision, ConsumedProvider,
            ConsumedDefinition, ConsumedProviderSourceRevision)
        || !IsValid(ConsumedProvider) || !IsValid(ConsumedDefinition)
        || !IsValid(SubjectActor)
        || ConsumedProvider != Provider
        || ConsumedDefinition != Definition
        || ConsumedProviderSourceRevision
               != ResolvedProviderSourceRevision) {
        ResetPendingContextActionHold();
        Reject(CONTEXT_ACTION_REASON_STALE);
        return;
    }

    // The exact lease is gone before provider code runs, so Blueprint/native re-entry cannot replay the same issuance.
    FailureReason = FGameplayTag();
    if (!IMythicContextActionProvider::Execute_CanExecuteContextAction(
            ConsumedProvider, this, SubjectActor, ActionTag,
            static_cast<int64>(ConsumedProviderSourceRevision),
            FailureReason)) {
        Reject(FailureReason);
        return;
    }

    // CanExecute is a Blueprint/native domain callback. Prove its retained issuer and exact embodiment are still live
    // before crossing the mutating callback, even though the one-shot lease has already been consumed.
    UWorld *World = GetWorld();
    UMythicEntityPresentationRegistry *Registry = World
        ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    UMythicEntityPresentationComponent *CurrentPresentation = Registry
        ? Registry->ResolvePresentationComponent(Subject) : nullptr;
    if (!IsValid(ConsumedProvider) || !IsValid(ConsumedDefinition)
        || !IsValid(SubjectActor) || !CurrentPresentation
        || CurrentPresentation->GetOwner() != SubjectActor
        || AuthorityRequestedContextActionSubject != Subject
        || !ValidatedDefinitionSignature.Matches(*ConsumedDefinition)
        || !ConsumedProvider->GetClass()->ImplementsInterface(
            UMythicContextActionProvider::StaticClass())
        || !MythicDoesContextActionProviderBelongToSubject(
            ConsumedProvider, SubjectActor)) {
        Reject(CONTEXT_ACTION_REASON_STALE);
        return;
    }

    const FVector CurrentSubjectLocation =
        CurrentPresentation->GetPresentationAnchorWorldLocation();
    FVector CurrentViewLocation;
    FVector CurrentViewForward;
    float CurrentSubjectDistanceSquared = 0.0f;
    FailureReason = FGameplayTag();
    if (!MythicValidateContextActionDiscovery(
            this, SubjectActor, CurrentSubjectLocation, ValidatedPolicy,
            CurrentViewLocation, CurrentViewForward,
            CurrentSubjectDistanceSquared, FailureReason)
        || !MythicValidateContextActionSpatialRules(
            this, SubjectActor, CurrentSubjectLocation,
            *ConsumedDefinition, ValidatedPolicy, FailureReason)) {
        Reject(FailureReason);
        return;
    }
    FailureReason = FGameplayTag();
    if (!IMythicContextActionProvider::Execute_ExecuteContextAction(
            ConsumedProvider, this, SubjectActor, ActionTag,
            static_cast<int64>(ConsumedProviderSourceRevision),
            FailureReason)) {
        Reject(FailureReason);
        return;
    }
}

void AMythicPlayerController::ClientNotifyContextActionRejected_Implementation(
    const FGameplayTag SafeReasonTag) {
    OnContextActionRejected(MythicSanitizeContextActionFailure(SafeReasonTag));
}

bool AMythicPlayerController::ServerRequestNpcDialogue_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::OfferNpcQuestIfAny(AMythicNPCCharacter *NPC) {
    if (!IsValid(NPC) || NPC->GetWorld() != GetWorld()) {
        return;
    }
    if (UObjectiveDefinition *Offer = NPC->GetQuestOffer()) {
        FObjectiveProgress Progress;
        EObjectiveOfferResult Result = EObjectiveOfferResult::Invalid;
        if (!NPC->IsActorInTradeRange(GetPawn())) {
            Result = EObjectiveOfferResult::OutOfRange;
        }
        else if (ObjectiveTracker) {
            Result = ObjectiveTracker->ServerTryAddObjective(Offer, Progress);
        }

        if (Result == EObjectiveOfferResult::Assigned ||
            Result == EObjectiveOfferResult::AlreadyActive ||
            Result == EObjectiveOfferResult::AlreadyCompleted ||
            Result == EObjectiveOfferResult::OutOfRange ||
            Result == EObjectiveOfferResult::PrerequisitesNotMet) {
            const EObjectiveNotifyCategory Category = (Result == EObjectiveOfferResult::Assigned
                                                       || Result == EObjectiveOfferResult::OutOfRange
                                                       || Result == EObjectiveOfferResult::PrerequisitesNotMet)
                ? EObjectiveNotifyCategory::Assignment
                : EObjectiveNotifyCategory::Duplicate;
            ClientNotifyObjectiveResult(Offer->GetCalloutText(Progress.bCompleted), Category, Result,
                                        Progress.CurrentCount, Offer->RequiredCount, true, false, 0);
        }
    }
}

void AMythicPlayerController::ServerRequestNpcDialogue_Implementation(AMythicNPCCharacter *NPC) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }

    OfferNpcQuestIfAny(NPC);
    if (NPC->IsActorInTradeRange(GetPawn())) {
        NotifyTalkedToNPC(NPC->GetQuestNpcTag());
        if (ObjectiveTracker && InventoryComponent) {
            ObjectiveTracker->ServerTurnInDeliveriesTo(NPC->GetQuestNpcTag(), InventoryComponent);
        }
    }

    const FText Line = NPC->SelectDialogueFor(this);
    ClientReceiveNpcDialogue(NPC, Line);
}

void AMythicPlayerController::ClientReceiveNpcDialogue_Implementation(AMythicNPCCharacter *NPC, const FText &Line) {
    if (IsValid(NPC)) {
        NPC->FireBark(Line, this);
    }
}

void AMythicPlayerController::ClientOpenNpcTrade_Implementation(
    AMythicNPCCharacter *NPC) {
    if (IsValid(NPC)) {
        NPC->OpenTradeForLocalController(this);
    }
}


bool AMythicPlayerController::ServerPerformSocialVerb_Validate(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb) {
    return NPC != nullptr;
}

void AMythicPlayerController::ServerPerformSocialVerb_Implementation(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }
    if (!NPC->IsActorInTradeRange(GetPawn())) {
        return;
    }
    if (Verb >= EMythicSocialVerb::COUNT) {
        return;
    }

    const FMythicSocialReactionResult Result = NPC->ResolveSocialVerb(Verb, this);
    NPC->ApplySocialReaction(Result, Verb, this);

    ClientReceiveSocialReaction(NPC, Verb, Result.Reaction, UMythicSocialVerbLibrary::DefaultBarkFor(Verb, Result.Reaction));
}

void AMythicPlayerController::ClientReceiveSocialReaction_Implementation(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line) {
    if (IsValid(NPC)) {
        NPC->FireReaction(Verb, Reaction, Line, this);
    }
}

bool AMythicPlayerController::ServerRecruitNpc_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::ServerRecruitNpc_Implementation(AMythicNPCCharacter *NPC) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }
    if (!NPC->IsActorInTradeRange(GetPawn()) || !NPC->IsRecruitable()) {
        return;
    }

    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    if (!Party) {
        return;
    }
    OfferNpcQuestIfAny(NPC);
    if (Party->IsInParty(NPC)) {
        ClientReceiveNpcDialogue(NPC, NPC->SelectDialogueFor(this));
        return;
    }
    // The gates UNPCDefinition documents. Checked here rather than on entry: this RPC is the ONLY interaction verb
    // a recruitable NPC has (MythicNPCCharacter routes every interact through it and returns), so refusing early
    // left them mute - no dialogue, no quest - instead of merely unrecruited.
    {
        FGameplayTagContainer PlayerTags;
        if (const UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(this)) {
            ASC->GetOwnedGameplayTags(PlayerTags);
        }

        float Standing = 0.0f;
        const UMythicLivingWorldSubsystem *LWS = GetGameInstance()
                                                     ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>()
                                                     : nullptr;
        if (const AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>()) {
            const UMythicFactionStandingComponent *Faction = PS->GetFactionStanding();
            const UMythicFactionDatabase *FactionDB = LWS ? LWS->GetFactionDatabase() : nullptr;
            if (Faction && FactionDB) {
                Standing = Faction->GetStanding(FactionDB->FindFactionId(NPC->GetNPCDataRef().Faction));
            }
        }

        const UMythicLivingWorldSettings *LivingWorld = LWS ? LWS->GetSettings() : nullptr;
        // The class default carries the authored fallback, so a world with no settings asset still gates.
        const float Threshold = (LivingWorld ? LivingWorld : GetDefault<UMythicLivingWorldSettings>())->RecruitStandingThreshold;

        if (!FMythicRecruitRules::MeetsTagGate(PlayerTags, NPC->GetNPCDataRef().TagsRequiredToRecruit)) {
            UE_LOG(Myth, Verbose, TEXT("ServerRecruitNpc: '%s' refused - the player lacks %s."),
                   *GetNameSafe(NPC), *NPC->GetNPCDataRef().TagsRequiredToRecruit.ToStringSimple());
            ClientReceiveRecruitResult(NPC, false);
            return;
        }
        if (!FMythicRecruitRules::MeetsStandingGate(Standing, Threshold)) {
            UE_LOG(Myth, Verbose, TEXT("ServerRecruitNpc: '%s' refused - standing %.1f is below %.1f."),
                   *GetNameSafe(NPC), Standing, Threshold);
            ClientReceiveRecruitResult(NPC, false);
            return;
        }
    }

    if (!NPC->CognitiveBrain) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' is recruitable but has no cognitive brain — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    const FMassEntityHandle Src = NPC->CognitiveBrain->GetSourceEntity();
    if (!Src.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' has no valid MASS source entity — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    FString RecruiterKey;
    if (const AMythicPlayerState *RecruiterPS = GetPlayerState<AMythicPlayerState>()) {
        RecruiterKey = RecruiterPS->GetCanonicalPlayerKey();
    }
    const bool bOk = Party->AddCompanion(RecruiterKey, NPC, Src);
    ClientReceiveRecruitResult(NPC, bOk);
}

void AMythicPlayerController::ClientReceiveRecruitResult_Implementation(AMythicNPCCharacter *NPC, bool bSucceeded) {
}

bool AMythicPlayerController::ServerIssueCompanionOrder_Validate(AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget) {
    return true;
}

void AMythicPlayerController::ServerIssueCompanionOrder_Implementation(AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget) {
    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    if (!Party) {
        return;
    }
    FString PlayerKey;
    if (const AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>()) {
        PlayerKey = PS->GetCanonicalPlayerKey();
    }
    if (PlayerKey.IsEmpty()) {
        return;
    }
    Party->IssueCompanionOrder(PlayerKey, Companion, Order, OrderTarget);
}

void AMythicPlayerController::ClientReceiveHarvestFeedback_Implementation(
    const FMythicHarvestClientFeedback &Feedback) {
    // Every authority response carries a positive sequence, allowing this unreliable
    // presentation stream to discard duplicates and reordered older feedback.
    if (Feedback.ServerSequence > 0) {
        if (Feedback.ServerSequence <= LastPresentedHarvestFeedbackSequence) {
            return;
        }
        LastPresentedHarvestFeedbackSequence = Feedback.ServerSequence;
    }
    OnHarvestFeedback(Feedback);
    if (Feedback.Outcome != EMythicHarvestOutcome::Rejected) {
        return;
    }

    const FText ToolName = Feedback.RequiredToolType
        ? Feedback.RequiredToolType->DisplayName
        : NSLOCTEXT("MythicHarvest", "GenericTool", "the proper tool");
    FText Message;
    switch (Feedback.RejectReason) {
        case EMythicHarvestRejectReason::NoTool:
        case EMythicHarvestRejectReason::WrongTool:
            Message = FText::Format(
                NSLOCTEXT("MythicHarvest", "RequiresTool", "Requires {0}"),
                ToolName);
            break;
        case EMythicHarvestRejectReason::ToolTierTooLow:
            Message = FText::Format(
                NSLOCTEXT("MythicHarvest", "RequiresToolTier",
                          "Requires {0} (Tier {1})"),
                ToolName, FText::AsNumber(Feedback.RequiredToolTier));
            break;
        case EMythicHarvestRejectReason::ToolBroken:
            Message = FText::Format(
                NSLOCTEXT("MythicHarvest", "ToolBroken", "{0} is broken"),
                ToolName);
            break;
        case EMythicHarvestRejectReason::NodeDepleted:
            Message = NSLOCTEXT("MythicHarvest", "NodeDepleted",
                                "This resource is regrowing");
            break;
        case EMythicHarvestRejectReason::ClaimedByOther:
            Message = NSLOCTEXT("MythicHarvest", "ClaimedByOther",
                                "Another player is harvesting this resource");
            break;
        case EMythicHarvestRejectReason::OutOfRange:
            Message = NSLOCTEXT("MythicHarvest", "OutOfRange",
                                "Move closer to harvest");
            break;
        case EMythicHarvestRejectReason::NoLineOfSight:
            Message = NSLOCTEXT("MythicHarvest", "NoLineOfSight",
                                "The resource is obstructed");
            break;
        default:
            return;
    }

    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = Message;
    Notice.Accent = FLinearColor(0.95f, 0.68f, 0.22f);
    Notice.StackKey = FName(TEXT("HarvestRequirement"));
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyProficiencyLevel_Implementation(const FText &ProfName, int32 NewLevel, const FText &MilestoneName) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Progression;
    Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ProfLevelUp", "{0}  Lv {1}"), ProfName, FText::AsNumber(NewLevel));
    Notice.Detail = MilestoneName.IsEmpty()
                        ? FText::GetEmpty()
                        : FText::Format(NSLOCTEXT("Mythic", "MilestoneUnlocked", "{0} unlocked"), MilestoneName);
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyCompanionDeparted_Implementation(const FText &Name, FVector Location) {
}

void AMythicPlayerController::ClientNotifyCompanionBetrayed_Implementation(const FText &Name, FVector Location) {
}

void AMythicPlayerController::ClientNotifyObjective_Implementation(const FText &DisplayText, int32 Current, int32 Required, bool bCompleted, int32 StackIndex,
                                                                   const FText &QuestTitle) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Objective;
    Notice.Text = DisplayText;
    Notice.Detail = QuestTitle;
    Notice.Accent = FLinearColor(0.86f, 0.81f, 0.70f);
    Notice.StackKey = FName(*DisplayText.ToString());
    Notice.Count = Current;
    Notice.Total = Required;
    Notice.bTerminal = bCompleted;
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyObjectiveResult_Implementation(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                                         EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                                         bool bRewardSucceeded, bool bRewardDroppedNearby, int32 StackIndex) {
}

void AMythicPlayerController::RaiseHudNotice(const FMythicHudNotice &Notice) {
    OnHudNotice.Broadcast(Notice);
}

void AMythicPlayerController::ClientNotifyLootPickup_Implementation(const FText &ItemName, int32 Quantity, FLinearColor RarityColor) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Loot;
    Notice.Text = FText::Format(NSLOCTEXT("Mythic", "LootPickup", "+{0}"), ItemName);
    Notice.Accent = RarityColor;
    Notice.Count = FMath::Max(1, Quantity);
    Notice.StackKey = FName(*ItemName.ToString());
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyRewardCelebration_Implementation(UItemDefinition *ItemDef, int32 Quantity) {
    OnRewardCelebration(ItemDef, Quantity);
}

void AMythicPlayerController::ClientNotifyTradeResult_Implementation(EMythicTradeResult Result) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = MythicTrade::DescribeResult(Result);
    Notice.Accent = FLinearColor(0.78f, 0.35f, 0.30f);
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyEnvironmentHazard_Implementation(const FText &HazardName, bool bOnset) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = bOnset ? HazardName : FText::Format(NSLOCTEXT("Mythic", "HazardEnded", "{0} passed"), HazardName);
    Notice.Accent = bOnset ? FLinearColor(0.85f, 0.55f, 0.22f) : FLinearColor(0.60f, 0.66f, 0.60f);
    Notice.StackKey = FName(*HazardName.ToString());
    Notice.bTerminal = !bOnset;
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyItemDurability_Implementation(const FText &ItemName, EMythicItemDurabilityBeat Beat) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    switch (Beat) {
        case EMythicItemDurabilityBeat::Broken:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemBroke", "{0} broke"), ItemName);
            Notice.Accent = FLinearColor(0.80f, 0.28f, 0.24f);
            break;
        case EMythicItemDurabilityBeat::Repaired:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemRepaired", "{0} repaired"), ItemName);
            Notice.Accent = FLinearColor(0.45f, 0.72f, 0.42f);
            break;
        default:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemWorn", "{0} is nearly broken"), ItemName);
            Notice.Accent = FLinearColor(0.85f, 0.70f, 0.30f);
            break;
    }
    Notice.StackKey = FName(*ItemName.ToString());
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::NotifyItemAcquired(const UItemDefinition *ItemDef, int32 Quantity) {
    if (!ItemDef || Quantity <= 0) {
        return;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC || !ASC->IsOwnerActorAuthoritative()) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_ACQUIRED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = static_cast<float>(Quantity);
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_ACQUIRED, &Payload);
}

void AMythicPlayerController::NotifyItemUsed(const UItemDefinition *ItemDef, int32 Quantity) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bValidPayload = ItemDef && ItemDef->ItemType.IsValid();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, bValidPayload) || Quantity <= 0) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_USED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = static_cast<float>(Quantity);
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_USED, &Payload);
}

void AMythicPlayerController::NotifyItemEquipped(const UItemDefinition *ItemDef) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bValidPayload = ItemDef && ItemDef->ItemType.IsValid();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, bValidPayload)) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_EQUIPPED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_EQUIPPED, &Payload);
}

void AMythicPlayerController::NotifyTalkedToNPC(const FGameplayTag &NpcTag) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, NpcTag.IsValid())) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_TALKED_TO_NPC;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.TargetTags.AddTag(NpcTag);
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_TALKED_TO_NPC, &Payload);
}

void AMythicPlayerController::ClientShowShieldBroken_Implementation() {
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
        DamageNumbers->AddCombatText(Location + FVector(0.0f, 0.0f, 40.0f), TEXT("Shield Broken!"),
                                     FLinearColor(0.6f, 0.9f, 1.0f), 1.5f);
    }
}

void AMythicPlayerController::ClientShowDodge_Implementation() {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        DamageNumbers->AddDodgeNumber(AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f));
    }
}

void AMythicPlayerController::QueueResolvedCombatText(const FMythicResolvedCombatTextEvent &Event) {
    if (!HasAuthority()) {
        return;
    }

    PendingResolvedCombatText.Add(Event);
    if (PendingResolvedCombatText.Num() >= MaxResolvedCombatTextBatchSize) {
        FlushResolvedCombatTextQueue();
        return;
    }

    if (!ResolvedCombatTextFlushTimer.IsValid()) {
        ResolvedCombatTextFlushTimer = GetWorldTimerManager().SetTimerForNextTick(
            this, &AMythicPlayerController::FlushResolvedCombatTextQueue);
    }
}

void AMythicPlayerController::FlushResolvedCombatTextQueue() {
    if (ResolvedCombatTextFlushTimer.IsValid()) {
        GetWorldTimerManager().ClearTimer(ResolvedCombatTextFlushTimer);
    }
    ResolvedCombatTextFlushTimer.Invalidate();
    if (PendingResolvedCombatText.IsEmpty()) {
        return;
    }

    TArray<FMythicResolvedCombatTextEvent> Batch = MoveTemp(PendingResolvedCombatText);
    PendingResolvedCombatText.Reset();
    ClientReceiveResolvedCombatTextBatch(Batch);
}

void AMythicPlayerController::ClientReceiveResolvedCombatTextBatch_Implementation(
    const TArray<FMythicResolvedCombatTextEvent> &Events) {
    if (UWorld *World = GetWorld()) {
        UMythicEntityAttentionSubsystem *Attention = GetLocalPlayer()
            ? GetLocalPlayer()->GetSubsystem<
                  UMythicEntityAttentionSubsystem>()
            : nullptr;
        UMythicDamageNumberSubsystem *DamageNumbers =
            World->GetSubsystem<UMythicDamageNumberSubsystem>();
        for (const FMythicResolvedCombatTextEvent &Event : Events) {
            if (DamageNumbers) {
                DamageNumbers->AddResolvedCombatText(Event);
            }
            AActor *RelevantSubject = Event.bOutgoingForViewer
                ? Event.TargetActor.Get() : Event.SourceActor.Get();
            const UMythicEntityPresentationComponent *Presentation =
                IsValid(RelevantSubject)
                ? RelevantSubject->FindComponentByClass<
                      UMythicEntityPresentationComponent>()
                : nullptr;
            if (Attention && Presentation) {
                Attention->NotifyAttentionSignal(
                    Presentation->GetPresentationInstance(),
                    Event.bOutgoingForViewer
                        ? EMythicEntityAttentionSignalKind::Combat
                        : EMythicEntityAttentionSignalKind::CombatFromSubjectToViewer,
                    3.0f, Event.bCritical ? 1.0f : 0.75f);
            }
        }
    }
}

void AMythicPlayerController::ClientNotifyExhausted_Implementation(bool bExhausted) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Loc = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
        if (bExhausted) {
            DamageNumbers->AddCombatText(Loc, TEXT("Winded!"), FLinearColor(1.0f, 0.55f, 0.1f), 1.2f);
        }
        else {
            DamageNumbers->AddCombatText(Loc, TEXT("Recovered"), FLinearColor(0.45f, 0.9f, 0.45f), 1.0f);
        }
    }
}

void AMythicPlayerController::CheckZoneEntry() {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicTerritoryGrid *Grid = LW ? LW->GetTerritoryGrid() : nullptr;
    if (!Grid) {
        return;
    }
    const FMythicCellCoord Cell = Grid->WorldToCell(AvatarPawn->GetActorLocation());

    FMythicSettlementData Data;
    const int32 NewSettlementId = LW->CopySettlementAtCell(Cell, Data) ? Data.SettlementId : INDEX_NONE;
    if (NewSettlementId != LastSettlementId) {
        LastSettlementId = NewSettlementId;
        if (NewSettlementId != INDEX_NONE) {
            DiscoveredSettlements.Add(NewSettlementId);
        }
    }

    const FMythicFactionId DomFaction = Grid->GetDominantFaction(Cell);
    if (DomFaction.Index != LastTerritoryFactionIndex) {
        LastTerritoryFactionIndex = DomFaction.Index;

        AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>();
        UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
        const bool bUnwelcome = DomFaction.IsValid() && Standing
            && Standing->TierForStanding(Standing->GetStanding(DomFaction)) == EMythicStandingTier::Hostile;
        if (bUnwelcome) {
            if (UMythicActionEventSubsystem *ActionSub = GetWorld() ? GetWorld()->GetSubsystem<UMythicActionEventSubsystem>() : nullptr) {
                FMythicActionEvent Trespass;
                Trespass.Perpetrator = AvatarPawn;
                Trespass.VictimFactionOverride = DomFaction;
                Trespass.OverrideCell = Cell;
                Trespass.ActionTag = TAG_LIVINGWORLD_ACTION_PROPERTY_TRESPASS;
                Trespass.CategoryFlags = EMythicEventCategory::Social;
                Trespass.Significance = 0.3f;
                Trespass.MoralVector = FMythicMoralSignature::MakeTrespassActionMoralVector();
                if (PS) {
                    Trespass.PerpPlayerKey = PS->GetCanonicalPlayerKey();
                }
                ActionSub->SubmitAction(Trespass);
            }
        }
    }
}

bool AMythicPlayerController::CanFastTravel(const TSet<int32> &Discovered, int32 SettlementId, bool bBlocked) {
    return SettlementId != INDEX_NONE && !bBlocked && Discovered.Contains(SettlementId);
}

bool AMythicPlayerController::ServerFastTravel_Validate(int32 SettlementId) {
    return true;
}

void AMythicPlayerController::ServerFastTravel_Implementation(int32 SettlementId) {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }

    bool bBlocked = false;
    if (const UAbilitySystemComponent *ASC = GetAbilitySystemComponent()) {
        bBlocked = ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
    }
    const bool bOverloaded = IsOverloadedForFastTravel();

    const bool bBetweenOk =
        MythicFastTravel::CanFastTravelBetween(DiscoveredSettlements, LastSettlementId, SettlementId, bBlocked);
    if (!MythicFastTravel::CanFastTravelWithCargo(bBetweenOk, bOverloaded)) {
        return;
    }

    UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    if (!LW) {
        return;
    }

    FVector Anchor = FVector::ZeroVector;
    bool bResolved = false;
    if (const AMythicSettlement *Settlement = LW->GetSettlementActorSafe(SettlementId)) {
        Anchor = Settlement->GetActorLocation();
        bResolved = true;
    }
    else {
        FMythicSettlementData Data;
        if (LW->CopySettlementById(SettlementId, Data)) {
            if (const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid()) {
                Anchor = Grid->CellToWorld(Data.CenterCell);
                Anchor.Z = AvatarPawn->GetActorLocation().Z;
                bResolved = true;
            }
        }
    }
    if (!bResolved) {
        return;
    }

    Anchor.Z += 100.0f;
    EnterContextActionAuthorityBarrier();
    AvatarPawn->TeleportTo(Anchor, AvatarPawn->GetActorRotation());
}

bool AMythicPlayerController::IsOverloadedForFastTravel() const {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bEncumbranceEnabled) {
        return false;
    }
    float TotalWeight = 0.0f;
    for (const UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (Inv) {
            TotalWeight += Inv->GetTotalCarriedWeight();
        }
    }
    return MythicEncumbrance::ComputeTier(TotalWeight, Settings->EncumbranceSoftCapacity, Settings->EncumbranceHardCapacity)
        == EMythicEncumbranceTier::Overloaded;
}

bool AMythicPlayerController::ServerFastTravelToPOI_Validate(int32 POIId) {
    return true;
}

void AMythicPlayerController::ServerFastTravelToPOI_Implementation(int32 POIId) {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UGameInstance *GI = GetGameInstance();
    UMythicPOIDiscoverySubsystem *POI = GI ? GI->GetSubsystem<UMythicPOIDiscoverySubsystem>() : nullptr;
    if (!POI) {
        return;
    }

    if (const UAbilitySystemComponent *ASC = GetAbilitySystemComponent()) {
        if (ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT)) {
            ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelInCombat", "Not while you are being hunted."));
            return;
        }
    }
    if (IsOverloadedForFastTravel()) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelOverloaded", "Too heavily laden to travel. Drop something, or walk."));
        return;
    }
    if (!POI->IsPOIUnlocked(POIId)) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelUnknownDest", "You have not found that place yet."));
        return;
    }
    if (POI->ResolveCurrentPOI(AvatarPawn->GetActorLocation()) == INDEX_NONE) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelNotAtNode", "You can only depart from a landmark you have found."));
        return;
    }

    EnterContextActionAuthorityBarrier();
    POI->ServerFastTravelToPOI(AvatarPawn, POIId);
}

void AMythicPlayerController::ClientNotifyFastTravelRefused_Implementation(const FText &Reason) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = Reason;
    Notice.Accent = FLinearColor(0.85f, 0.70f, 0.30f);
    Notice.StackKey = FName(TEXT("FastTravelRefused"));
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyStatusLearned_Implementation(const FText &StatusName, const FText &Description,
                                                                      FLinearColor Accent) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Status;
    Notice.Text = StatusName;
    Notice.Detail = Description;
    Notice.Accent = Accent;
    // Keyed so a status inflicted twice in the same breath cannot stack two banners; it can only ever teach once
    // anyway, but the key keeps that true if the rule is ever relaxed.
    Notice.StackKey = FName(*StatusName.ToString());
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    UnbindContextActionAttention();
    ResetInventoryActionSubmissionState();
    if (ULocalPlayer *LocalPlayer = GetLocalPlayer()) {
        if (UMythicInventoryInteractionCoordinator *Coordinator =
                LocalPlayer->GetSubsystem<
                    UMythicInventoryInteractionCoordinator>()) {
            Coordinator->DetachFromController(this);
        }
    }
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ZoneCheckTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(
            ContextActionOfferRefreshTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(
            CombatPresentationRefreshTimerHandle);
    }
    if (HasAuthority()) {
        EnterContextActionAuthorityBarrier();
        ClearAuthorityCombatPresentationFocus();
    }
    Super::EndPlay(EndPlayReason);
}

bool AMythicPlayerController::ServerExecuteBarterOffer_Validate(AMythicNPCCharacter *NPC, int32 OfferIndex) {
    return NPC != nullptr && OfferIndex >= 0;
}

void AMythicPlayerController::ServerExecuteBarterOffer_Implementation(AMythicNPCCharacter *NPC, int32 OfferIndex) {
    if (!HasAuthority() || !IsValid(NPC) || !NPC->IsMerchant() || !NPC->IsActorInTradeRange(GetPawn())) {
        return;
    }
    const TArray<FMythicMerchantOffer> &Offers = NPC->GetMerchantOffers();
    if (!Offers.IsValidIndex(OfferIndex)) {
        return;
    }
    const FMythicMerchantOffer &Offer = Offers[OfferIndex];
    UItemDefinition *CostDef = Offer.CostItem.LoadSynchronous();
    UItemDefinition *RewardDef = Offer.RewardItem.LoadSynchronous();
    if (!CostDef || !RewardDef || Offer.CostQty < 1 || Offer.RewardQty < 1) {
        return;
    }

    UMythicInventoryComponent *PlayerInv = GetInventoryComponent();
    if (!PlayerInv) {
        return;
    }
    if (PlayerInv->GetItemCount(CostDef) < Offer.CostQty) {
        return;
    }
    PlayerInv->ServerRemoveItemByDefinition(CostDef, Offer.CostQty);

    if (UMythicLootManagerSubsystem *Loot = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>()
        : nullptr) {
        Loot->CreateAndGive(RewardDef, Offer.RewardQty, this, this, 0);
    }
}

bool AMythicPlayerController::ServerRerollItemAffixes_Validate(UMythicItemInstance *Item) {
    return Item != nullptr;
}

void AMythicPlayerController::ServerRerollItemAffixes_Implementation(UMythicItemInstance *Item) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    const UAffixesFragment *Affixes = Item->GetFragment<UAffixesFragment>();
    if (!Affixes) {
        return;
    }

    {
        bool bNearForge = false;
        const APawn *MyPawn = GetPawn();
        UWorld *World = GetWorld();
        if (MyPawn && World) {
            const FVector MyLoc = MyPawn->GetActorLocation();
            for (TActorIterator<AMythicConversionStation> It(World); It && !bNearForge; ++It) {
                AMythicConversionStation *Station = *It;
                const UConversionStationComponent *Conv = Station ? Station->GetConversionComponent() : nullptr;
                if (!Conv || !Conv->GetStationTags().HasTag(ITEMIZATION_STATION_FORGE)) {
                    continue;
                }
                const float DistSq = FVector::DistSquared(MyLoc, Station->GetActorLocation());
                if (IsWithinStationRange(DistSq, Conv->GetServerUseRangeSq())) {
                    bNearForge = true;
                }
            }
        }
        if (!bNearForge) {
            ClientNotifyTradeResult(EMythicTradeResult::RequiresStation);
            return;
        }
    }

    int32 RerollCost = 0;
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        const int32 RarityIndex = Item->GetItemDefinition() ? static_cast<int32>(Item->GetItemDefinition()->Rarity.GetValue()) : 0;
        RerollCost = MythicCurrency::ComputeRerollCost(Item->GetItemLevel(), RarityIndex, Settings->RerollBaseCost,
                                                       Settings->RerollCostPerLevelFraction, Settings->RerollCostPerRarityFraction);
    }

    TArray<UMythicInventoryComponent *> CurrencyInventories;
    if (RerollCost > 0) {
        int32 Wallet = 0;
        CurrencyInventories = GetAllInventoryComponents();
        for (const UMythicInventoryComponent *Inv : CurrencyInventories) {
            if (Inv) {
                Wallet += Inv->GetTotalCurrency();
            }
        }
        if (!MythicCurrency::CanAfford(Wallet, RerollCost)) {
            ClientNotifyTradeResult(EMythicTradeResult::InsufficientFunds);
            return;
        }
    }

    // The affix component publishes snapshots only after its complete equipped-stat transaction succeeds. Charge
    // after that commit so invalid data, a stale closure, or a GAS reconciliation failure can never consume currency.
    if (!const_cast<UAffixesFragment *>(Affixes)->RerollUnlockedAffixes(Item->GetItemLevel())) {
        return;
    }

    if (RerollCost > 0) {
        int32 Remaining = RerollCost;
        for (UMythicInventoryComponent *Inv : CurrencyInventories) {
            if (Remaining <= 0) {
                break;
            }
            if (Inv) {
                Remaining -= Inv->SpendCurrency(Remaining);
            }
        }
        ensureAlwaysMsgf(Remaining == 0,
                         TEXT("Reroll committed but the prevalidated currency debit was short by %d."),
                         Remaining);
    }
}

bool AMythicPlayerController::IsWithinStationRange(float DistSq, float RangeSq) {
    return RangeSq > 0.0f && DistSq <= RangeSq;
}

bool AMythicPlayerController::ServerSetItemAffixLocked_Validate(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked) {
    return Item != nullptr && AffixIndex >= 0;
}

void AMythicPlayerController::ServerSetItemAffixLocked_Implementation(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    if (const UAffixesFragment *Affixes = Item->GetFragment<UAffixesFragment>()) {
        const_cast<UAffixesFragment *>(Affixes)->SetAffixLocked(AffixIndex, bLocked);
    }
}

void AMythicPlayerController::SetupInputComponent() {
    Super::SetupInputComponent();
    if (HarvestFocusComponent) {
        HarvestFocusComponent->InitializeLocalInput(InputComponent);
    }
}

void AMythicPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused) {
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        MythicASC->ProcessAbilityInput(DeltaTime, bGamePaused);
    }

    Super::PostProcessInput(DeltaTime, bGamePaused);
}
