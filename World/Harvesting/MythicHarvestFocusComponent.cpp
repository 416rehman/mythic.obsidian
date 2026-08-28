#include "World/Harvesting/MythicHarvestFocusComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/HarvestToolFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicPlayerController.h"
#include "Resources/MythicResourceISM.h"
#include "TimerManager.h"
#include "UI/HUD/MythicHudNotice.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestFocusComponent)

namespace {
constexpr double MinimumFocusLossGraceSeconds = 0.15;

struct FMythicHarvestFocusCandidate {
    TWeakObjectPtr<UMythicResourceISM> Resource;
    FPrimitiveInstanceId PrimitiveInstanceId;
    FMythicHarvestNodeId NodeId;
    FVector AnchorLocation = FVector::ZeroVector;
    double Score = TNumericLimits<double>::Max();
};

struct FMythicHarvestRuntimeToolCandidate {
    FMythicHarvestToolEligibilityProbe Probe;
    TWeakObjectPtr<UMythicInventoryComponent> Inventory;
    int32 SlotIndex = INDEX_NONE;
    TWeakObjectPtr<UMythicItemInstance> Item;
    TWeakObjectPtr<const UHarvestToolFragment> HarvestFragment;
    TWeakObjectPtr<const UDurabilityFragment> DurabilityFragment;
};

bool IsFiniteVector(const FVector &Value) {
    return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y)
        && FMath::IsFinite(Value.Z);
}

FText GetToolDisplayName(
    const UMythicHarvestToolTypeDefinition *ToolType) {
    return ToolType && !ToolType->DisplayName.IsEmpty()
        ? ToolType->DisplayName
        : NSLOCTEXT("MythicHarvestFocus", "GenericTool", "Tool");
}

FText FormatTier(const int32 Tier) {
    static const TCHAR *RomanTiers[] = {
        TEXT("0"), TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"),
        TEXT("V"), TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"),
        TEXT("X")};
    if (Tier >= 1 && Tier < UE_ARRAY_COUNT(RomanTiers)) {
        return FText::FromString(RomanTiers[Tier]);
    }
    return FText::AsNumber(Tier);
}

bool PresentationsEqual(const FMythicHarvestFocusPresentation &Left,
                        const FMythicHarvestFocusPresentation &Right) {
    return Left.bHasFocus == Right.bHasFocus
        && Left.Availability == Right.Availability
        && Left.PromptText.EqualTo(Right.PromptText)
        && Left.ContextAction == Right.ContextAction
        && Left.HarvestableDefinition == Right.HarvestableDefinition
        && Left.RequiredToolType == Right.RequiredToolType
        && Left.AnchorLocation.Equals(Right.AnchorLocation, 0.1)
        && Left.RequiredToolTier == Right.RequiredToolTier
        && Left.ResolvedToolTier == Right.ResolvedToolTier
        && Left.bCanHarvest == Right.bCanHarvest;
}
}

EMythicHarvestFocusAvailability FMythicHarvestFocusRules::EvaluateToolSelection(
    const UMythicHarvestToolTypeDefinition *RequiredToolType,
    const int32 RequiredToolTier,
    const TConstArrayView<FMythicHarvestToolEligibilityProbe> Candidates,
    int32 &OutSlottedToolIndex,
    int32 &OutEquipCandidateIndex) {
    OutSlottedToolIndex = INDEX_NONE;
    OutEquipCandidateIndex = INDEX_NONE;
    if (!RequiredToolType) {
        // Authority resolves the wear target by exact required family. A nullable
        // definition can never name one, so local prediction must not advertise a
        // harvestable node that authority will always reject.
        return EMythicHarvestFocusAvailability::InvalidSource;
    }

    auto GuidLess = [](const FGuid &Left, const FGuid &Right) {
        if (Left.A != Right.A) return Left.A < Right.A;
        if (Left.B != Right.B) return Left.B < Right.B;
        if (Left.C != Right.C) return Left.C < Right.C;
        return Left.D < Right.D;
    };
    auto PreferCandidate = [&Candidates, &GuidLess](
        const int32 CandidateIndex, const int32 ExistingIndex) {
        if (ExistingIndex == INDEX_NONE) {
            return true;
        }
        const FMythicHarvestToolEligibilityProbe &Candidate =
            Candidates[CandidateIndex];
        const FMythicHarvestToolEligibilityProbe &Existing =
            Candidates[ExistingIndex];
        if (Candidate.ToolTier != Existing.ToolTier) {
            return Candidate.ToolTier > Existing.ToolTier;
        }
        if (Candidate.ItemGuid.IsValid() != Existing.ItemGuid.IsValid()) {
            return Candidate.ItemGuid.IsValid();
        }
        return Candidate.ItemGuid.IsValid()
            ? GuidLess(Candidate.ItemGuid, Existing.ItemGuid)
            : CandidateIndex < ExistingIndex;
    };

    int32 SlottedMatch = INDEX_NONE;
    int32 SlottedMatchCount = 0;
    for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num();
         ++CandidateIndex) {
        const FMythicHarvestToolEligibilityProbe &Candidate =
            Candidates[CandidateIndex];
        if (Candidate.ToolType == RequiredToolType && Candidate.bInGearSlot) {
            ++SlottedMatchCount;
            if (PreferCandidate(CandidateIndex, SlottedMatch)) {
                SlottedMatch = CandidateIndex;
            }
        }
    }
    // Authority rejects two gear-slotted tools of one family as ambiguous, so
    // the prompt cannot promise a harvest the server would refuse.
    if (SlottedMatchCount > 1) {
        return EMythicHarvestFocusAvailability::InvalidSource;
    }
    if (SlottedMatch != INDEX_NONE) {
        OutSlottedToolIndex = SlottedMatch;
        const FMythicHarvestToolEligibilityProbe &Selected =
            Candidates[SlottedMatch];
        if (!Selected.bHasDurabilityFragment) {
            return EMythicHarvestFocusAvailability::InvalidSource;
        }
        if (Selected.ToolTier < RequiredToolTier) {
            return EMythicHarvestFocusAvailability::ToolTierTooLow;
        }
        if (Selected.bBroken) {
            return EMythicHarvestFocusAvailability::ToolBroken;
        }
        return EMythicHarvestFocusAvailability::Ready;
    }

    int32 BestCarried = INDEX_NONE;
    int32 BestBroken = INDEX_NONE;
    int32 BestLowTier = INDEX_NONE;
    int32 BestInvalid = INDEX_NONE;
    for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num();
         ++CandidateIndex) {
        const FMythicHarvestToolEligibilityProbe &Candidate =
            Candidates[CandidateIndex];
        if (Candidate.ToolType != RequiredToolType || Candidate.bInGearSlot) {
            continue;
        }
        int32 *Bucket = nullptr;
        if (!Candidate.bHasDurabilityFragment) {
            Bucket = &BestInvalid;
        }
        else if (Candidate.ToolTier < RequiredToolTier) {
            Bucket = &BestLowTier;
        }
        else if (Candidate.bBroken) {
            Bucket = &BestBroken;
        }
        else {
            Bucket = &BestCarried;
        }
        if (PreferCandidate(CandidateIndex, *Bucket)) {
            *Bucket = CandidateIndex;
        }
    }

    if (BestCarried != INDEX_NONE) {
        OutEquipCandidateIndex = BestCarried;
        return EMythicHarvestFocusAvailability::EquipRequired;
    }
    if (BestBroken != INDEX_NONE) {
        OutEquipCandidateIndex = BestBroken;
        return EMythicHarvestFocusAvailability::ToolBroken;
    }
    if (BestLowTier != INDEX_NONE) {
        OutEquipCandidateIndex = BestLowTier;
        return EMythicHarvestFocusAvailability::ToolTierTooLow;
    }
    if (BestInvalid != INDEX_NONE) {
        OutEquipCandidateIndex = BestInvalid;
        return EMythicHarvestFocusAvailability::InvalidSource;
    }
    return EMythicHarvestFocusAvailability::RequiresTool;
}

UMythicHarvestFocusComponent::UMythicHarvestFocusComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void UMythicHarvestFocusComponent::BeginPlay() {
    Super::BeginPlay();

    OwnerController = Cast<AMythicPlayerController>(GetOwner());
    if (!OwnerController.IsValid()
        || !OwnerController->IsLocalPlayerController() || !GetWorld()) {
        return;
    }

    ResolveInputAssets();
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!Settings || !FMath::IsFinite(Settings->FocusScanIntervalSeconds)
        || Settings->FocusScanIntervalSeconds <= 0.0f) {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        FocusScanTimerHandle, this,
        &UMythicHarvestFocusComponent::RefreshLocalFocus,
        Settings->FocusScanIntervalSeconds, true);
    RefreshLocalFocus();
}

void UMythicHarvestFocusComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(FocusScanTimerHandle);
    }
    ClearFocus();
    if (bInputBindingInstalled && BoundInputComponent.IsValid()) {
        BoundInputComponent->RemoveBindingByHandle(InputBindingHandle);
    }
    bInputBindingInstalled = false;
    InputBindingHandle = 0;
    BoundInputComponent.Reset();
    InputSubsystem.Reset();
    bEquipRequestSent = false;

    Super::EndPlay(EndPlayReason);
}

void UMythicHarvestFocusComponent::InitializeLocalInput(
    UInputComponent *InputComponent) {
    AMythicPlayerController *Controller =
        Cast<AMythicPlayerController>(GetOwner());
    UEnhancedInputComponent *EnhancedInput =
        Cast<UEnhancedInputComponent>(InputComponent);
    if (!Controller || !Controller->IsLocalPlayerController() || !EnhancedInput) {
        return;
    }

    OwnerController = Controller;
    ResolveInputAssets();
    if (!ResolvedContextInteractAction || !ResolvedContextMappingContext) {
        return;
    }
    const bool bMappingContainsAction =
        ResolvedContextMappingContext->GetMappings().ContainsByPredicate(
            [this](const FEnhancedActionKeyMapping &Mapping) {
                return Mapping.Action == ResolvedContextInteractAction;
            });
    if (!bMappingContainsAction) {
        return;
    }

    if (bInputBindingInstalled && BoundInputComponent == EnhancedInput) {
        return;
    }
    if (bInputBindingInstalled && BoundInputComponent.IsValid()) {
        BoundInputComponent->RemoveBindingByHandle(InputBindingHandle);
    }

    FEnhancedInputActionEventBinding &Binding = EnhancedInput->BindAction(
        ResolvedContextInteractAction, ETriggerEvent::Started, this,
        &UMythicHarvestFocusComponent::HandleContextInteractStarted);
    InputBindingHandle = Binding.GetHandle();
    bInputBindingInstalled = true;
    BoundInputComponent = EnhancedInput;
    SetContextMappingActive(CurrentFocus.bHasFocus);
}

void UMythicHarvestFocusComponent::ResolveInputAssets() {
    if (ResolvedContextInteractAction && ResolvedContextMappingContext
        && InputSubsystem.IsValid()) {
        return;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    AMythicPlayerController *Controller = OwnerController.Get();
    ULocalPlayer *LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
    if (!Settings || !LocalPlayer
        || Settings->ContextInteractAction.IsNull()
        || Settings->ContextMappingContext.IsNull()) {
        return;
    }

    ResolvedContextInteractAction =
        Settings->ContextInteractAction.LoadSynchronous();
    ResolvedContextMappingContext =
        Settings->ContextMappingContext.LoadSynchronous();
    InputSubsystem =
        LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

void UMythicHarvestFocusComponent::SetContextMappingActive(
    const bool bActive) {
    ResolveInputAssets();
    UEnhancedInputLocalPlayerSubsystem *Subsystem = InputSubsystem.Get();
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!Subsystem || !Settings || !ResolvedContextMappingContext) {
        bMappingContextInstalled = false;
        return;
    }

    if (bActive && !bMappingContextInstalled) {
        Subsystem->AddMappingContext(ResolvedContextMappingContext,
                                     Settings->ContextMappingPriority);
        bMappingContextInstalled = true;
    }
    else if (!bActive && bMappingContextInstalled) {
        Subsystem->RemoveMappingContext(ResolvedContextMappingContext);
        bMappingContextInstalled = false;
    }
}

void UMythicHarvestFocusComponent::RefreshLocalFocus() {
    AMythicPlayerController *Controller = OwnerController.Get();
    UWorld *World = GetWorld();
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!Controller || !Controller->IsLocalPlayerController() || !World
        || !Controller->GetPawn() || !Settings
        || !FMath::IsFinite(Settings->FocusRangeCentimeters)
        || Settings->FocusRangeCentimeters <= 0.0f
        || !FMath::IsFinite(Settings->FocusRadiusCentimeters)
        || Settings->FocusRadiusCentimeters < 0.0f) {
        ClearFocus();
        return;
    }

    FVector ViewOrigin;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewOrigin, ViewRotation);
    const FVector ViewDirection = ViewRotation.Vector();
    if (!IsFiniteVector(ViewOrigin) || !IsFiniteVector(ViewDirection)
        || ViewDirection.IsNearlyZero()) {
        ClearFocus();
        return;
    }

    const FVector TraceEnd = ViewOrigin
        + ViewDirection * Settings->FocusRangeCentimeters;
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_Destructible);
    APawn *ControlledPawn = Controller->GetPawn().Get();
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MythicHarvestFocus),
                                      false, ControlledPawn);
    QueryParams.bTraceComplex = false;

    TArray<FHitResult> Hits;
    const bool bHitAny = Settings->FocusRadiusCentimeters > KINDA_SMALL_NUMBER
        ? World->SweepMultiByObjectType(
              Hits, ViewOrigin, TraceEnd, FQuat::Identity, ObjectQuery,
              FCollisionShape::MakeSphere(Settings->FocusRadiusCentimeters),
              QueryParams)
        : World->LineTraceMultiByObjectType(
              Hits, ViewOrigin, TraceEnd, ObjectQuery, QueryParams);

    FMythicHarvestFocusCandidate BestCandidate;
    bool bHasBestCandidate = false;
    bool bSawCurrentFocus = false;
    TSet<uint64> SeenRuntimeInstances;
    if (bHitAny) {
        for (const FHitResult &Hit : Hits) {
            UMythicResourceISM *Resource =
                Cast<UMythicResourceISM>(Hit.GetComponent());
            if (!Resource || !Resource->HarvestableDefinition
                || Hit.Item < 0) {
                continue;
            }

            FPrimitiveInstanceId PrimitiveInstanceId;
            FMythicHarvestNodeId NodeId;
            if (!Resource->ResolveAuthoritativeHitInstance(
                    Hit.Item, PrimitiveInstanceId, NodeId)
                || !PrimitiveInstanceId.IsValid() || !NodeId.IsValid()) {
                continue;
            }

            const uint64 RuntimeInstanceKey =
                (static_cast<uint64>(Resource->GetUniqueID()) << 32)
                | static_cast<uint32>(PrimitiveInstanceId.GetAsIndex());
            if (SeenRuntimeInstances.Contains(RuntimeInstanceKey)) {
                continue;
            }
            SeenRuntimeInstances.Add(RuntimeInstanceKey);

            FTransform InstanceTransform;
            if (!Resource->GetInstanceTransform(Hit.Item, InstanceTransform, true)
                || !IsFiniteVector(InstanceTransform.GetLocation())) {
                continue;
            }
            const FVector Anchor = InstanceTransform.GetLocation();
            const FVector ToAnchor = Anchor - ViewOrigin;
            const double AlongRay = FVector::DotProduct(ToAnchor, ViewDirection);
            if (!FMath::IsFinite(AlongRay) || AlongRay < 0.0
                || AlongRay > Settings->FocusRangeCentimeters
                    + Settings->FocusRadiusCentimeters) {
                continue;
            }
            const double LateralDistance =
                FMath::Sqrt(FMath::Max(0.0,
                    ToAnchor.SquaredLength() - AlongRay * AlongRay));
            double Score = AlongRay + LateralDistance * 0.25;
            const bool bIsCurrent = FocusedResource == Resource
                && FocusedNodeId == NodeId
                && FocusedPrimitiveInstanceId == PrimitiveInstanceId;
            if (bIsCurrent) {
                bSawCurrentFocus = true;
                Score -= FMath::Max(10.0f,
                                    Settings->FocusRadiusCentimeters * 0.5f);
            }

            if (!bHasBestCandidate || Score < BestCandidate.Score) {
                BestCandidate.Resource = Resource;
                BestCandidate.PrimitiveInstanceId = PrimitiveInstanceId;
                BestCandidate.NodeId = NodeId;
                BestCandidate.AnchorLocation = Anchor;
                BestCandidate.Score = Score;
                bHasBestCandidate = true;
            }
        }
    }

    const double NowSeconds = World->GetTimeSeconds();
    if (bSawCurrentFocus) {
        LastFocusedSeenTimeSeconds = NowSeconds;
    }
    if (bHasBestCandidate && BestCandidate.Resource.IsValid()) {
        ApplyFocusedInstance(*BestCandidate.Resource.Get(),
                             BestCandidate.PrimitiveInstanceId,
                             BestCandidate.NodeId,
                             BestCandidate.AnchorLocation);
        LastFocusedSeenTimeSeconds = NowSeconds;
        return;
    }

    const double GraceSeconds = FMath::Max(
        MinimumFocusLossGraceSeconds,
        static_cast<double>(Settings->FocusScanIntervalSeconds) * 2.0);
    FVector RetainedAnchor;
    if (LastFocusedSeenTimeSeconds >= 0.0
        && NowSeconds - LastFocusedSeenTimeSeconds <= GraceSeconds
        && IsFocusedInstanceStillValid(&RetainedAnchor)) {
        FocusedAnchorLocation = RetainedAnchor;
        RefreshFocusedPresentation();
        return;
    }
    ClearFocus();
}

bool UMythicHarvestFocusComponent::IsFocusedInstanceStillValid(
    FVector *OutAnchorLocation) const {
    UMythicResourceISM *Resource = FocusedResource.Get();
    if (!Resource || !Resource->HarvestableDefinition
        || !FocusedNodeId.IsValid() || !FocusedPrimitiveInstanceId.IsValid()) {
        return false;
    }

    FPrimitiveInstanceId ResolvedPrimitiveId;
    if (!Resource->ResolvePrimitiveInstanceId(FocusedNodeId,
                                               ResolvedPrimitiveId)
        || !(ResolvedPrimitiveId == FocusedPrimitiveInstanceId)) {
        return false;
    }
    const int32 CurrentIndex =
        Resource->GetInstanceIndexForId(ResolvedPrimitiveId);
    FTransform InstanceTransform;
    if (CurrentIndex == INDEX_NONE
        || !Resource->GetInstanceTransform(CurrentIndex, InstanceTransform, true)
        || !IsFiniteVector(InstanceTransform.GetLocation())) {
        return false;
    }
    if (OutAnchorLocation) {
        *OutAnchorLocation = InstanceTransform.GetLocation();
    }
    return true;
}

void UMythicHarvestFocusComponent::ApplyFocusedInstance(
    UMythicResourceISM &Resource,
    const FPrimitiveInstanceId PrimitiveInstanceId,
    const FMythicHarvestNodeId &NodeId,
    const FVector &AnchorLocation) {
    FocusedResource = &Resource;
    FocusedPrimitiveInstanceId = PrimitiveInstanceId;
    FocusedNodeId = NodeId;
    FocusedAnchorLocation = AnchorLocation;
    RefreshFocusedPresentation();
}

void UMythicHarvestFocusComponent::RefreshFocusedPresentation() {
    UMythicResourceISM *Resource = FocusedResource.Get();
    const UMythicHarvestableDefinition *Definition =
        Resource ? Resource->HarvestableDefinition.Get() : nullptr;
    AMythicPlayerController *Controller = OwnerController.Get();
    if (!Resource || !Definition || !Controller) {
        ClearFocus();
        return;
    }

    TArray<FMythicHarvestRuntimeToolCandidate> RuntimeCandidates;
    TArray<FMythicHarvestToolEligibilityProbe> EligibilityProbes;
    for (UMythicInventoryComponent *Inventory :
         Controller->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        const TArray<FMythicInventorySlotEntry> &Slots =
            Inventory->GetAllSlots();
        for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex) {
            const FMythicInventorySlotEntry &Slot = Slots[SlotIndex];
            UMythicItemInstance *Item = Slot.SlottedItemInstance.Get();
            const UHarvestToolFragment *HarvestFragment =
                UHarvestToolFragment::FindOnItem(Item);
            if (!Item || !HarvestFragment) {
                continue;
            }

            FMythicHarvestRuntimeToolCandidate &Candidate =
                RuntimeCandidates.AddDefaulted_GetRef();
            Candidate.Inventory = Inventory;
            Candidate.SlotIndex = SlotIndex;
            Candidate.Item = Item;
            Candidate.HarvestFragment = HarvestFragment;
            Candidate.DurabilityFragment = Item->GetFragment<UDurabilityFragment>();
            Candidate.Probe.ToolType = HarvestFragment->ToolType;
            Candidate.Probe.ItemGuid = Item->GetItemInstanceGuid();
            Candidate.Probe.ToolTier = HarvestFragment->ToolTier;
            Candidate.Probe.bInGearSlot = Slot.IsGearSlot();
            Candidate.Probe.bHasDurabilityFragment =
                Candidate.DurabilityFragment.IsValid();
            Candidate.Probe.bBroken = Candidate.DurabilityFragment.IsValid()
                && Candidate.DurabilityFragment->IsBroken();
            EligibilityProbes.Add(Candidate.Probe);
        }
    }

    int32 SelectedCandidateIndex = INDEX_NONE;
    int32 EquipCandidateIndex = INDEX_NONE;
    const EMythicHarvestFocusAvailability Availability =
        FMythicHarvestFocusRules::EvaluateToolSelection(
            Definition->RequiredToolType, Definition->MinimumToolTier,
            EligibilityProbes, SelectedCandidateIndex, EquipCandidateIndex);

    FMythicHarvestFocusPresentation NewPresentation;
    NewPresentation.bHasFocus = true;
    NewPresentation.Availability = Availability;
    NewPresentation.ContextAction = ResolvedContextInteractAction;
    NewPresentation.HarvestableDefinition =
        const_cast<UMythicHarvestableDefinition *>(Definition);
    NewPresentation.RequiredToolType = Definition->RequiredToolType;
    NewPresentation.AnchorLocation = FocusedAnchorLocation;
    NewPresentation.RequiredToolTier = Definition->MinimumToolTier;
    SelectedToolInventory.Reset();
    SelectedToolSlotIndex = INDEX_NONE;
    SelectedToolItemGuid = FGuid();
    const int32 PresentedCandidateIndex =
        SelectedCandidateIndex != INDEX_NONE ? SelectedCandidateIndex : EquipCandidateIndex;
    if (RuntimeCandidates.IsValidIndex(PresentedCandidateIndex)) {
        NewPresentation.ResolvedToolTier =
            RuntimeCandidates[PresentedCandidateIndex].Probe.ToolTier;
        SelectedToolInventory =
            RuntimeCandidates[PresentedCandidateIndex].Inventory;
        SelectedToolSlotIndex =
            RuntimeCandidates[PresentedCandidateIndex].SlotIndex;
        SelectedToolItemGuid =
            RuntimeCandidates[PresentedCandidateIndex].Probe.ItemGuid;
    }
    NewPresentation.bCanHarvest =
        Availability == EMythicHarvestFocusAvailability::Ready;
    if (Availability != EMythicHarvestFocusAvailability::EquipRequired) {
        bEquipRequestSent = false;
    }

    const FText ToolName = GetToolDisplayName(Definition->RequiredToolType);
    switch (Availability) {
        case EMythicHarvestFocusAvailability::Ready:
            NewPresentation.PromptText = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "ReadyPrompt", "{0} {1}"),
                Definition->HarvestVerb, Definition->DisplayName);
            break;
        case EMythicHarvestFocusAvailability::EquipRequired:
            NewPresentation.PromptText = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "EquipPrompt", "Equip {0}"),
                ToolName);
            break;
        case EMythicHarvestFocusAvailability::ToolTierTooLow:
            NewPresentation.PromptText = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "TierPrompt",
                          "Requires Tier {0} {1}"),
                FormatTier(Definition->MinimumToolTier), ToolName);
            break;
        case EMythicHarvestFocusAvailability::ToolBroken:
            NewPresentation.PromptText = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "BrokenPrompt", "Repair {0}"),
                ToolName);
            break;
        case EMythicHarvestFocusAvailability::RequiresTool:
            NewPresentation.PromptText = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "RequiresPrompt", "Requires {0}"),
                ToolName);
            break;
        case EMythicHarvestFocusAvailability::InvalidSource:
        default:
            NewPresentation.PromptText = NSLOCTEXT(
                "MythicHarvestFocus", "UnavailablePrompt",
                "Harvest action unavailable");
            break;
    }

    const bool bHadFocus = CurrentFocus.bHasFocus;
    if (!PresentationsEqual(CurrentFocus, NewPresentation)) {
        CurrentFocus = MoveTemp(NewPresentation);
        OnFocusChanged.Broadcast(CurrentFocus);
    }
    if (!bHadFocus) {
        SetContextMappingActive(true);
    }
}

void UMythicHarvestFocusComponent::HandleContextInteractStarted() {
    AMythicPlayerController *Controller = OwnerController.Get();
    if (!Controller || !Controller->IsLocalPlayerController()
        || !CurrentFocus.bHasFocus) {
        return;
    }

    RefreshFocusedPresentation();
    // Harvesting is the weapon swing plus a passive tool-slot check, so the only
    // thing this contextual key can still do is put a carried tool in its slot.
    if (CurrentFocus.Availability
        == EMythicHarvestFocusAvailability::EquipRequired) {
        if (!RequestEquipFocusedTool()) {
            RaiseUnavailableNotice(
                EMythicHarvestFocusAvailability::InvalidSource);
        }
        return;
    }
    if (!CurrentFocus.bCanHarvest) {
        RaiseUnavailableNotice(CurrentFocus.Availability);
    }
}

bool UMythicHarvestFocusComponent::RequestEquipFocusedTool() {
    AMythicPlayerController *Controller = OwnerController.Get();
    UMythicInventoryComponent *SourceInventory = SelectedToolInventory.Get();
    if (!Controller || !Controller->IsLocalPlayerController()
        || !SourceInventory || !SelectedToolItemGuid.IsValid()
        || bEquipRequestSent) {
        return false;
    }

    const TArray<FMythicInventorySlotEntry> &SourceSlots =
        SourceInventory->GetAllSlots();
    if (!SourceSlots.IsValidIndex(SelectedToolSlotIndex)
        || !SourceInventory->CanPlayerTakeFromSlot(SelectedToolSlotIndex)) {
        return false;
    }
    UMythicItemInstance *Item =
        SourceSlots[SelectedToolSlotIndex].SlottedItemInstance.Get();
    if (!Item || Item->GetItemInstanceGuid() != SelectedToolItemGuid) {
        return false;
    }

    UMythicInventoryComponent *TargetInventory = nullptr;
    int32 TargetSlotIndex = INDEX_NONE;
    for (UMythicInventoryComponent *Inventory :
         Controller->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        const TArray<FMythicInventorySlotEntry> &Slots =
            Inventory->GetAllSlots();
        for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex) {
            const FMythicInventorySlotEntry &Slot = Slots[SlotIndex];
            if (Slot.SlottedItemInstance || !Slot.IsGearSlot()
                || !Inventory->CanSlotAcceptItem(SlotIndex, Item, true)) {
                continue;
            }
            TargetInventory = Inventory;
            TargetSlotIndex = SlotIndex;
            break;
        }
        if (TargetInventory) {
            break;
        }
    }
    if (!TargetInventory) {
        return false;
    }

    bEquipRequestSent = true;
    Controller->ServerMoveItemBetweenInventories(
        SourceInventory, SelectedToolSlotIndex, TargetInventory,
        TargetSlotIndex);
    return true;
}

void UMythicHarvestFocusComponent::RaiseUnavailableNotice(
    const EMythicHarvestFocusAvailability Availability) const {
    AMythicPlayerController *Controller = OwnerController.Get();
    const UMythicHarvestableDefinition *Definition =
        CurrentFocus.HarvestableDefinition;
    if (!Controller || !Controller->IsLocalPlayerController() || !Definition) {
        return;
    }

    const FText ToolName = GetToolDisplayName(Definition->RequiredToolType);
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Accent = FLinearColor(0.95f, 0.55f, 0.18f);
    switch (Availability) {
        case EMythicHarvestFocusAvailability::EquipRequired:
            Notice.Text = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "EquipNotice", "Equip {0}"),
                ToolName);
            Notice.StackKey = FName(TEXT("Harvest.EquipTool"));
            break;
        case EMythicHarvestFocusAvailability::ToolTierTooLow:
            Notice.Text = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "TierNotice",
                          "Requires Tier {0} {1}"),
                FormatTier(Definition->MinimumToolTier), ToolName);
            Notice.StackKey = FName(TEXT("Harvest.ToolTierTooLow"));
            break;
        case EMythicHarvestFocusAvailability::ToolBroken:
            Notice.Text = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "BrokenNotice", "Repair {0}"),
                ToolName);
            Notice.StackKey = FName(TEXT("Harvest.ToolBroken"));
            break;
        case EMythicHarvestFocusAvailability::RequiresTool:
            Notice.Text = FText::Format(
                NSLOCTEXT("MythicHarvestFocus", "RequiresNotice", "Requires {0}"),
                ToolName);
            Notice.StackKey = FName(TEXT("Harvest.RequiresTool"));
            break;
        case EMythicHarvestFocusAvailability::InvalidSource:
        default:
            Notice.Text = NSLOCTEXT(
                "MythicHarvestFocus", "InvalidSourceNotice",
                "Harvest action is not ready");
            Notice.StackKey = FName(TEXT("Harvest.InvalidSource"));
            break;
    }
    Controller->RaiseHudNotice(Notice);
}

void UMythicHarvestFocusComponent::ClearFocus() {
    FocusedResource.Reset();
    FocusedPrimitiveInstanceId = FPrimitiveInstanceId();
    FocusedNodeId = FMythicHarvestNodeId();
    FocusedAnchorLocation = FVector::ZeroVector;
    LastFocusedSeenTimeSeconds = -1.0;
    SelectedToolInventory.Reset();
    SelectedToolSlotIndex = INDEX_NONE;
    SelectedToolItemGuid = FGuid();
    bEquipRequestSent = false;
    SetContextMappingActive(false);

    const FMythicHarvestFocusPresentation ClearedPresentation;
    if (!PresentationsEqual(CurrentFocus, ClearedPresentation)) {
        CurrentFocus = ClearedPresentation;
        OnFocusChanged.Broadcast(CurrentFocus);
    }
}
