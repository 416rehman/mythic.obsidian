#include "UI/Nameplate/MythicNameplateDirector.h"

#include "CommonUIExtensions.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformTime.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"
#include "Interaction/Attention/MythicEntityAttentionTypes.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "UI/Nameplate/MythicNameplateLayer.h"
#include "UI/Nameplate/MythicNameplatePolicy.h"
#include "UI/Nameplate/MythicNameplateRules.h"
#include "UI/Nameplate/MythicNameplateVisualStyle.h"
#include "UI/Nameplate/MythicEntityInspectPage.h"
#include "UI/Settings/MythicUserSettings.h"
#include "UI/MythicTags_UI.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/Entity/MythicEntityKnowledgeFactDefinition.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityPresentationTags.h"
#include "World/Entity/MythicEntityViewerKnowledgeComponent.h"
#include "World/Entity/MythicEntityViewerKnowledgeTypes.h"
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Roles/RoleTypes.h"

#define LOCTEXT_NAMESPACE "MythicNameplateDirector"

namespace {
int32 DirectorTierOrdinal(const EMythicNameplateDisclosureTier Tier) {
    return static_cast<int32>(Tier);
}

bool IsBossRank(const EMythicPresentedCombatRank Rank) {
    return Rank == EMythicPresentedCombatRank::Boss
        || Rank == EMythicPresentedCombatRank::WorldBoss;
}

EMythicNameplateAttentionState ResolveAttentionState(
    const FMythicEntityAttentionObservation &Observation) {
    if (Observation.bHardTarget) {
        return EMythicNameplateAttentionState::HardCombatTarget;
    }
    if (Observation.bInteractionTarget) {
        return EMythicNameplateAttentionState::InteractionTarget;
    }
    // Ordinary camera focus is admission evidence, not a player-facing target
    // semantic. This prevents Blueprint skins from drawing the old gaze
    // crosshair while retaining explicit interaction and hard-target states.
    return EMythicNameplateAttentionState::Observed;
}

int32 DirectorLaneRank(const EMythicNameplateLane Lane) {
    switch (Lane) {
    case EMythicNameplateLane::Focus:
        return 4;
    case EMythicNameplateLane::Safety:
        return 3;
    case EMythicNameplateLane::Opportunity:
        return 2;
    case EMythicNameplateLane::Awareness:
    default:
        return 1;
    }
}

bool IsCurrentFact(const FMythicObservableFactItem &Fact,
                   const FMythicEntityPresentationInstance &Instance) {
    return Instance.IsValid() && Fact.Subject == Instance.Handle
        && Fact.EmbodimentGeneration == Instance.EmbodimentGeneration
        && Fact.Revision != 0;
}

bool IsCurrentStatus(const FMythicPublicStatusPresentationItem &Status,
                     const FMythicEntityPresentationInstance &Instance) {
    return Instance.IsValid() && Status.Subject == Instance.Handle
        && Status.EmbodimentGeneration == Instance.EmbodimentGeneration
        && Status.Revision != 0;
}

EMythicNameplateStatusUrgency ToNameplateUrgency(
    const EMythicStatusPresentationCategory Category) {
    switch (Category) {
    case EMythicStatusPresentationCategory::HardControl:
        return EMythicNameplateStatusUrgency::HardCrowdControl;
    case EMythicStatusPresentationCategory::Damage:
        return EMythicNameplateStatusUrgency::Damaging;
    case EMythicStatusPresentationCategory::Control:
        return EMythicNameplateStatusUrgency::MovementControl;
    case EMythicStatusPresentationCategory::Debuff:
    case EMythicStatusPresentationCategory::Buff:
    case EMythicStatusPresentationCategory::Cosmetic:
    default:
        return EMythicNameplateStatusUrgency::Other;
    }
}
}

bool UMythicNameplateDirector::ShouldCreateSubsystem(UObject *Outer) const {
    const ULocalPlayer *LocalPlayer = Cast<ULocalPlayer>(Outer);
    return LocalPlayer && !LocalPlayer->IsTemplate();
}

void UMythicNameplateDirector::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    PresentationMode = EMythicNameplatePresentationMode::Contextual;
    ActiveEntries.Reserve(16);
    ArchetypeLabels.Reserve(64);
    HandleSemanticDatabasesLoaded();
    EnsureRuntimeBindings();
}

void UMythicNameplateDirector::Deinitialize() {
    if (PendingLocalContextActionHold.IsActive()) {
        CancelFocusedContextActionHold(
            PendingLocalContextActionHold.ActionTag);
    }
    CloseEntityInspectInternal(true);
    if (UMythicNameplateLayer *Layer = PresentationLayer.Get()) {
        Layer->ReleaseAllProjections();
    }
    PresentationLayer.Reset();
    UnbindUserSettings();
    UnbindPlayerState();
    UnbindAttention();

    if (SemanticDatabaseLoad.IsValid()) {
        SemanticDatabaseLoad->CancelHandle();
        SemanticDatabaseLoad.Reset();
    }
    for (TPair<FPrimaryAssetId, TSharedPtr<FStreamableHandle>> &Pair :
         PendingPrimaryAssetLoads) {
        if (Pair.Value.IsValid()) {
            Pair.Value->CancelHandle();
        }
    }
    for (TPair<FPrimaryAssetId, TSharedPtr<FStreamableHandle>> &Pair :
         PendingKnowledgeFactLoads) {
        if (Pair.Value.IsValid()) {
            Pair.Value->CancelHandle();
        }
    }
    PendingPrimaryAssetLoads.Reset();
    PendingKnowledgeFactLoads.Reset();
    RequestedPrimaryAssets.Reset();
    RequestedKnowledgeFactAssets.Reset();
    IdentityDefinitions.Reset();
    ActionDefinitions.Reset();
    KnowledgeFactDefinitions.Reset();
    RoleDatabase = nullptr;
    CreatureSpeciesTable = nullptr;
    ArchetypeLabels.Reset();
    ActiveEntries.Reset();
    PendingAmbientWhisperChallenger.Reset();
    PendingAmbientWhisperChallengerSinceSeconds = 0.0;
    OnNameplateProjectionsChanged.Clear();
    OnEntityInspectProjectionChanged.Clear();
    Super::Deinitialize();
}

void UMythicNameplateDirector::Tick(const float DeltaTime) {
    (void)DeltaTime;
    EnsureRuntimeBindings();
    EnsureSemanticDatabasesRequested();
    if (bProjectionRefreshRequested) {
        RebuildProjectionSet();
    }
    UpdateActivePlacements();
}

TStatId UMythicNameplateDirector::GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMythicNameplateDirector,
                                    STATGROUP_Tickables);
}

UWorld *UMythicNameplateDirector::GetTickableGameObjectWorld() const {
    const ULocalPlayer *LocalPlayer = GetLocalPlayer();
    return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

bool UMythicNameplateDirector::IsTickable() const {
    const UWorld *World = GetTickableGameObjectWorld();
    return !IsTemplate() && World && World->IsGameWorld();
}

void UMythicNameplateDirector::ConfigureNameplates(
    UMythicNameplatePolicy *InPolicy) {
    if (!IsInGameThread()) {
        return;
    }
    Policy = InPolicy;
    EnsureRuntimeBindings();
    if (UMythicEntityAttentionSubsystem *AttentionSubsystem = Attention.Get()) {
        AttentionSubsystem->ConfigureAttention(
            Policy ? Policy->Attention : FMythicEntityAttentionConfig());
    }
    RequestProjectionRefresh();
}

TArray<FMythicNameplateProjection>
UMythicNameplateDirector::GetActiveProjections() const {
    TArray<FMythicNameplateProjection> Result;
    Result.Reserve(ActiveEntries.Num());
    for (const FProjectionEntry &Entry : ActiveEntries) {
        Result.Add(Entry.Projection);
    }
    return Result;
}

bool UMythicNameplateDirector::GetFocusedProjection(
    FMythicNameplateProjection &OutProjection) const {
    for (const FProjectionEntry &Entry : ActiveEntries) {
        if (Entry.Projection.DisclosureTier
            == EMythicNameplateDisclosureTier::Focus) {
            OutProjection = Entry.Projection;
            return true;
        }
    }
    OutProjection = FMythicNameplateProjection();
    return false;
}

bool UMythicNameplateDirector::GetFocusedActionRailProjection(
    FMythicNameplateActionRailProjection &OutProjection) const {
    for (const FProjectionEntry &Entry : ActiveEntries) {
        if (Entry.Projection.DisclosureTier
                == EMythicNameplateDisclosureTier::Focus
            && Entry.ActionRailProjection.IsPresentable()) {
            OutProjection = Entry.ActionRailProjection;
            return true;
        }
    }
    OutProjection = FMythicNameplateActionRailProjection();
    return false;
}

bool UMythicNameplateDirector::ExecuteFocusedContextAction(
    const FGameplayTag ActionTag) {
    if (!IsInGameThread() || !ActionTag.IsValid()) {
        return false;
    }

    FMythicNameplateActionRailProjection FocusedRail;
    if (!GetFocusedActionRailProjection(FocusedRail)
        || !FocusedRail.Instance.IsValid()) {
        return false;
    }
    const FMythicNameplateActionProjection *Action =
        FocusedRail.Actions.FindByPredicate(
            [ActionTag](const FMythicNameplateActionProjection &Candidate) {
                return Candidate.ActionTag == ActionTag;
            });
    if (!Action) {
        return false;
    }

    if (!FMythicContextActionProjectionRules::IsHoldDurationValid(
            Action->HoldDurationSeconds)) {
        return false;
    }
    const bool bRequiresHold = Action->HoldDurationSeconds > 0.0f;
    if (bRequiresHold
        && !PendingLocalContextActionHold.Matches(
            FocusedRail.Instance, Action->ActionTag,
            Action->OfferRevision)) {
        return false;
    }

    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = GetTickableGameObjectWorld();
    AMythicPlayerController *Controller = Cast<AMythicPlayerController>(
        LocalPlayer && World
            ? LocalPlayer->GetPlayerController(World) : nullptr);
    if (!Controller || !Controller->IsLocalController()) {
        return false;
    }
    if (!bRequiresHold && PendingLocalContextActionHold.IsActive()) {
        CancelFocusedContextActionHold(
            PendingLocalContextActionHold.ActionTag);
    }
    Controller->ServerExecuteContextAction(
        FocusedRail.Instance, Action->ActionTag,
        static_cast<int64>(Action->OfferRevision));
    if (bRequiresHold) {
        PendingLocalContextActionHold.Reset();
    }
    return true;
}

bool UMythicNameplateDirector::BeginFocusedContextActionHold(
    const FGameplayTag ActionTag) {
    if (!IsInGameThread() || !ActionTag.IsValid()) {
        return false;
    }

    FMythicNameplateActionRailProjection FocusedRail;
    if (!GetFocusedActionRailProjection(FocusedRail)
        || !FocusedRail.Instance.IsValid()) {
        return false;
    }
    const FMythicNameplateActionProjection *Action =
        FocusedRail.Actions.FindByPredicate(
            [ActionTag](const FMythicNameplateActionProjection &Candidate) {
                return Candidate.ActionTag == ActionTag;
            });
    if (!Action
        || !FMythicContextActionProjectionRules::IsHoldDurationValid(
            Action->HoldDurationSeconds)
        || Action->HoldDurationSeconds <= 0.0f) {
        return false;
    }
    if (PendingLocalContextActionHold.Matches(
            FocusedRail.Instance, Action->ActionTag,
            Action->OfferRevision)) {
        return true;
    }

    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = GetTickableGameObjectWorld();
    AMythicPlayerController *Controller = Cast<AMythicPlayerController>(
        LocalPlayer && World
            ? LocalPlayer->GetPlayerController(World) : nullptr);
    if (!Controller || !Controller->IsLocalController()) {
        return false;
    }
    if (PendingLocalContextActionHold.IsActive()) {
        Controller->ServerCancelContextActionHold(
            PendingLocalContextActionHold.Subject,
            PendingLocalContextActionHold.ActionTag,
            static_cast<int64>(
                PendingLocalContextActionHold.OfferRevision));
        PendingLocalContextActionHold.Reset();
    }

    PendingLocalContextActionHold.Subject = FocusedRail.Instance;
    PendingLocalContextActionHold.ActionTag = Action->ActionTag;
    PendingLocalContextActionHold.OfferRevision = Action->OfferRevision;
    Controller->ServerBeginContextActionHold(
        FocusedRail.Instance, Action->ActionTag,
        static_cast<int64>(Action->OfferRevision));
    return true;
}

void UMythicNameplateDirector::CancelFocusedContextActionHold(
    const FGameplayTag ActionTag) {
    if (!IsInGameThread() || !PendingLocalContextActionHold.IsActive()
        || PendingLocalContextActionHold.ActionTag != ActionTag) {
        return;
    }

    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = GetTickableGameObjectWorld();
    AMythicPlayerController *Controller = Cast<AMythicPlayerController>(
        LocalPlayer && World
            ? LocalPlayer->GetPlayerController(World) : nullptr);
    if (Controller && Controller->IsLocalController()) {
        Controller->ServerCancelContextActionHold(
            PendingLocalContextActionHold.Subject,
            PendingLocalContextActionHold.ActionTag,
            static_cast<int64>(
                PendingLocalContextActionHold.OfferRevision));
    }
    PendingLocalContextActionHold.Reset();
}

void UMythicNameplateDirector::RequestProjectionRefresh() {
    bProjectionRefreshRequested = true;
}

bool UMythicNameplateDirector::OpenFocusedEntityInspect() {
    if (!IsInGameThread()) {
        return false;
    }

    EnsureRuntimeBindings();
    UMythicEntityAttentionSubsystem *AttentionSubsystem = Attention.Get();
    UMythicNameplatePolicy *CurrentPolicy = Policy;
    if (!AttentionSubsystem || !CurrentPolicy
        || !CurrentPolicy->Inspect.InspectPageClass) {
        return false;
    }

    const FMythicEntityPresentationInstance FocusedInstance =
        AttentionSubsystem->GetFocusedInstance();
    if (!FocusedInstance.IsValid()) {
        return false;
    }

    if (InspectedInstance == FocusedInstance
        && ActiveInspectPage.IsValid()
        && CurrentInspectProjection.IsValid()) {
        return true;
    }

    CloseEntityInspectInternal(true);
    InspectedInstance = FocusedInstance;
    RebuildInspectProjection();
    if (!CurrentInspectProjection.IsValid()) {
        InspectedInstance.Reset();
        return false;
    }

    UCommonActivatableWidget *Pushed =
        UCommonUIExtensions::PushContentToLayer_ForPlayer(
            GetLocalPlayer(), UI_LAYER_GAME,
            CurrentPolicy->Inspect.InspectPageClass);
    UMythicEntityInspectPage *InspectPage =
        Cast<UMythicEntityInspectPage>(Pushed);
    if (!InspectPage) {
        InspectedInstance.Reset();
        CurrentInspectProjection = FMythicEntityInspectProjection();
        PublishInspectRevision();
        return false;
    }

    ActiveInspectPage = InspectPage;
    InspectPageDeactivatedHandle = InspectPage->OnDeactivated().AddUObject(
        this, &ThisClass::HandleInspectPageDeactivated);
    InspectPage->ApplyInspectProjection(CurrentInspectProjection);
    AttentionSubsystem->SetInspectTarget(InspectedInstance);
    RequestProjectionRefresh();
    return true;
}

void UMythicNameplateDirector::CloseEntityInspect() {
    CloseEntityInspectInternal(true);
}

bool UMythicNameplateDirector::GetCurrentInspectProjection(
    FMythicEntityInspectProjection &OutProjection) const {
    if (!InspectedInstance.IsValid()
        || !CurrentInspectProjection.IsValid()
        || CurrentInspectProjection.Instance != InspectedInstance) {
        OutProjection = FMythicEntityInspectProjection();
        return false;
    }
    OutProjection = CurrentInspectProjection;
    return true;
}

void UMythicNameplateDirector::AttachPresentationLayer(
    UMythicNameplateLayer *InLayer, UMythicNameplatePolicy *InPolicy) {
    if (!IsInGameThread() || !InLayer) {
        return;
    }
    if (UMythicNameplateLayer *Previous = PresentationLayer.Get();
        Previous && Previous != InLayer) {
        Previous->ReleaseAllProjections();
    }
    PresentationLayer = InLayer;
    bPresentationLayerNeedsFullSync = true;
    InLayer->SetRenderPreferences(RenderPreferences);
    ConfigureNameplates(InPolicy);
}

void UMythicNameplateDirector::DetachPresentationLayer(
    UMythicNameplateLayer *InLayer) {
    if (!InLayer || PresentationLayer.Get() != InLayer) {
        return;
    }
    InLayer->ReleaseAllProjections();
    PresentationLayer.Reset();
    bPresentationLayerNeedsFullSync = false;
}

void UMythicNameplateDirector::EnsureRuntimeBindings() {
    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = GetTickableGameObjectWorld();
    if (!LocalPlayer || !World) {
        return;
    }

    UMythicUserSettings *CurrentSettings = UMythicUserSettings::Get();
    if (UserSettings.Get() != CurrentSettings) {
        BindUserSettings(CurrentSettings);
    }

    UMythicEntityAttentionSubsystem *CurrentAttention =
        LocalPlayer->GetSubsystem<UMythicEntityAttentionSubsystem>();
    if (Attention.Get() != CurrentAttention) {
        BindAttention(CurrentAttention);
    }

    APlayerController *Controller = LocalPlayer->GetPlayerController(World);
    AMythicPlayerState *PlayerState = Controller
        ? Cast<AMythicPlayerState>(Controller->PlayerState)
        : nullptr;
    if (BoundPlayerState.Get() != PlayerState) {
        BindPlayerState(PlayerState);
    }
}

void UMythicNameplateDirector::BindUserSettings(
    UMythicUserSettings *InSettings) {
    UnbindUserSettings();
    UserSettings = InSettings;
    if (InSettings) {
        InterfaceSettingsChangedHandle =
            InSettings->OnInterfaceChanged.AddUObject(
                this, &ThisClass::HandleInterfaceSettingsChanged);
    }
    HandleInterfaceSettingsChanged();
}

void UMythicNameplateDirector::UnbindUserSettings() {
    if (UMythicUserSettings *Current = UserSettings.Get();
        Current && InterfaceSettingsChangedHandle.IsValid()) {
        Current->OnInterfaceChanged.Remove(InterfaceSettingsChangedHandle);
    }
    InterfaceSettingsChangedHandle.Reset();
    UserSettings.Reset();
}

void UMythicNameplateDirector::HandleInterfaceSettingsChanged() {
    EMythicNameplatePresentationMode NewMode =
        EMythicNameplatePresentationMode::Contextual;
    FMythicNameplateRenderPreferences NewPreferences;
    if (const UMythicUserSettings *Settings = UserSettings.Get()) {
        NewMode = Settings->GetNameplatePresentationMode();
        NewPreferences.Scale = Settings->GetNameplateScale();
        NewPreferences.bShowHealthPercent =
            Settings->GetShowNameplateHealthPercent();
        NewPreferences.bShowStatusText =
            Settings->GetShowNameplateStatusText();
        NewPreferences.bHighContrast =
            Settings->GetHighContrastNameplates();
        NewPreferences.bReducedMotion =
            Settings->GetReducedNameplateMotion();
    }

    const bool bDisclosureChanged = PresentationMode != NewMode;
    const bool bRenderChanged = RenderPreferences != NewPreferences;
    PresentationMode = NewMode;
    RenderPreferences = NewPreferences;
    if (bRenderChanged) {
        if (UMythicNameplateLayer *Layer = PresentationLayer.Get()) {
            Layer->SetRenderPreferences(RenderPreferences);
        }
    }
    if (bDisclosureChanged || bRenderChanged) {
        RequestProjectionRefresh();
    }
}

void UMythicNameplateDirector::BindAttention(
    UMythicEntityAttentionSubsystem *InAttention) {
    UnbindAttention();
    Attention = InAttention;
    if (!InAttention) {
        return;
    }
    AttentionUpdatedHandle = InAttention->OnAttentionUpdated.AddUObject(
        this, &ThisClass::HandleAttentionUpdated);
    InAttention->ConfigureAttention(
        Policy ? Policy->Attention : FMythicEntityAttentionConfig());
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::UnbindAttention() {
    if (UMythicEntityAttentionSubsystem *Current = Attention.Get();
        Current && AttentionUpdatedHandle.IsValid()) {
        Current->OnAttentionUpdated.Remove(AttentionUpdatedHandle);
    }
    AttentionUpdatedHandle.Reset();
    Attention.Reset();
}

void UMythicNameplateDirector::BindPlayerState(
    AMythicPlayerState *InPlayerState) {
    if (BoundPlayerState.IsValid() && BoundPlayerState.Get() != InPlayerState) {
        CloseEntityInspectInternal(true);
    }
    UnbindPlayerState();
    BoundPlayerState = InPlayerState;
    if (!InPlayerState) {
        RequestProjectionRefresh();
        return;
    }

    ViewerKnowledge = InPlayerState->GetEntityViewerKnowledgeComponent();
    if (UMythicEntityViewerKnowledgeComponent *Knowledge =
            ViewerKnowledge.Get()) {
        KnowledgeUpdatedHandle = Knowledge->OnNativeKnowledgeRevision().AddUObject(
            this, &ThisClass::HandleKnowledgeUpdated);
    }

    ActionGrants = InPlayerState->GetEntityActionGrantComponent();
    if (UMythicEntityActionGrantComponent *Grants = ActionGrants.Get()) {
        ActionGrantsUpdatedHandle = Grants->OnNativeGrantRevision().AddUObject(
            this, &ThisClass::HandleActionGrantsUpdated);
    }
    CombatPresentation =
        InPlayerState->GetEntityCombatPresentationComponent();
    if (UMythicEntityCombatPresentationComponent *Combat =
            CombatPresentation.Get()) {
        CombatPresentationUpdatedHandle =
            Combat->OnNativeCombatPresentationRevision().AddUObject(
                this, &ThisClass::HandleCombatPresentationUpdated);
    }
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::UnbindPlayerState() {
    if (UMythicEntityViewerKnowledgeComponent *Knowledge = ViewerKnowledge.Get();
        Knowledge && KnowledgeUpdatedHandle.IsValid()) {
        Knowledge->OnNativeKnowledgeRevision().Remove(KnowledgeUpdatedHandle);
    }
    if (UMythicEntityActionGrantComponent *Grants = ActionGrants.Get();
        Grants && ActionGrantsUpdatedHandle.IsValid()) {
        Grants->OnNativeGrantRevision().Remove(ActionGrantsUpdatedHandle);
    }
    if (UMythicEntityCombatPresentationComponent *Combat =
            CombatPresentation.Get();
        Combat && CombatPresentationUpdatedHandle.IsValid()) {
        Combat->OnNativeCombatPresentationRevision().Remove(
            CombatPresentationUpdatedHandle);
    }
    KnowledgeUpdatedHandle.Reset();
    ActionGrantsUpdatedHandle.Reset();
    CombatPresentationUpdatedHandle.Reset();
    ViewerKnowledge.Reset();
    ActionGrants.Reset();
    CombatPresentation.Reset();
    BoundPlayerState.Reset();
}

void UMythicNameplateDirector::HandleAttentionUpdated(
    const uint64 AttentionRevision) {
    (void)AttentionRevision;
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::HandleKnowledgeUpdated(
    const uint32 KnowledgeRevision) {
    (void)KnowledgeRevision;
    RequestProjectionRefresh();
    if (InspectedInstance.IsValid()) {
        RebuildInspectProjection();
    }
}

void UMythicNameplateDirector::HandleActionGrantsUpdated(
    const uint32 GrantRevision) {
    (void)GrantRevision;
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::HandleCombatPresentationUpdated(
    const uint32 CombatRevision) {
    (void)CombatRevision;
    RequestProjectionRefresh();
    if (InspectedInstance.IsValid()) {
        RebuildInspectProjection();
    }
}

void UMythicNameplateDirector::HandleInspectPageDeactivated() {
    CloseEntityInspectInternal(false);
}

void UMythicNameplateDirector::RebuildProjectionSet() {
    bProjectionRefreshRequested = false;
    const double NowSeconds = FPlatformTime::Seconds();
    UMythicEntityAttentionSubsystem *AttentionSubsystem = Attention.Get();
    if (!AttentionSubsystem) {
        TArray<FProjectionEntry> Empty;
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        CommitBoundedEntries(Empty, NowSeconds);
        return;
    }

    TArray<FProjectionEntry> Candidates;
    Candidates.Reserve(16);
    TSet<FMythicEntityPresentationInstance> ImmediateCollapseInstances;
    ImmediateCollapseInstances.Reserve(16);
    TSet<FMythicEntityPresentationInstance> ImmediateFadeInstances;
    ImmediateFadeInstances.Reserve(16);
    for (const FMythicEntityAttentionObservation &Observation :
         AttentionSubsystem->GetObservationsView()) {
        // Offscreen/Inspect suppression cannot leave a world-overlay renderer
        // at an invalid screen position. A stable LOS loss still receives the
        // standard whole-surface release, but no additional semantic grace.
        if (!Observation.bOnScreen || Observation.bInspectTarget) {
            ImmediateCollapseInstances.Add(Observation.Instance);
        } else if (!Observation.bHasLineOfSight) {
            ImmediateFadeInstances.Add(Observation.Instance);
        }
        FProjectionEntry Entry;
        if (BuildProjectionEntry(Observation, Entry)) {
            Entry.LastEligibleSeconds = NowSeconds;
            Entry.bFreshEvidence = true;
            Candidates.Add(MoveTemp(Entry));
        } else if (Entry.bRejectedByPassiveRange || Entry.bObservedDead) {
            ImmediateFadeInstances.Add(Observation.Instance);
        }
    }
    AppendReleaseGraceLeases(Candidates, ImmediateCollapseInstances,
                             ImmediateFadeInstances, NowSeconds);
    CommitBoundedEntries(Candidates, NowSeconds);
}

bool UMythicNameplateDirector::BuildProjectionEntry(
    const FMythicEntityAttentionObservation &Observation,
    FProjectionEntry &OutEntry) {
    UMythicEntityPresentationComponent *Component = Observation.Component.Get();
    if (!Component || !Component->RepresentsInstance(Observation.Instance)
        || !Observation.bOnScreen || Observation.bInspectTarget) {
        return false;
    }

    const FMythicPublicIdentitySnapshot &Identity =
        Component->GetPublicIdentitySnapshot();
    if (!Identity.IsActive() || Identity.Instance != Observation.Instance) {
        return false;
    }

    FMythicEntityKnowledgeView Knowledge;
    const bool bHasKnowledge = ViewerKnowledge.IsValid()
        && ViewerKnowledge->GetKnowledgeForSubject(Observation.Instance,
                                                   Knowledge)
        && Knowledge.bRecognitionGranted;
    const FMythicEntityKnowledgeView *KnowledgePtr =
        bHasKnowledge ? &Knowledge : nullptr;

    TArray<FActionCandidate> Actions;
    Actions.Reserve(8);
    bool bActionPromotesContext = false;
    bool bCanAssist = false;
    GatherActions(Observation.Instance, Actions, bActionPromotesContext,
                  bCanAssist);

    TArray<EMythicNameplatePrimaryCue> CueCandidates;
    CueCandidates.Reserve(8);
    FText UnusedCueText;
    bool bDownedOrDying = false;
    bool bCombatRelevant = false;
    GatherCueCandidates(Observation, *Component, Actions, CueCandidates,
                        UnusedCueText, bDownedOrDying, bCombatRelevant);
    EMythicNameplatePrimaryCue PrimaryCue =
        FMythicNameplateRules::SelectPrimaryCue(CueCandidates);
    const FActionCandidate *AvailableCorpseAction = Actions.FindByPredicate(
        [](const FActionCandidate &Action) {
            return Action.Definition && Action.bAvailable
                && Action.Projection.OfferRevision != 0
                && Action.Projection.ActionTag.IsValid()
                && Action.Projection.InputActionTag.IsValid()
                && !Action.Projection.ResolvedLabel.IsEmpty();
        });
    const bool bObservedDead =
        PrimaryCue == EMythicNameplatePrimaryCue::Dead;
    OutEntry.bObservedDead = bObservedDead;
    const bool bDeliberateCorpseAttention = Observation.bFocused
        || Observation.bInteractionTarget;
    if (bObservedDead) {
        const FProjectionEntry *Existing = ActiveEntries.FindByPredicate(
            [&Observation](const FProjectionEntry &Entry) {
                return Entry.Projection.Instance == Observation.Instance;
            });
        if (!bDeliberateCorpseAttention || !AvailableCorpseAction
            || (Existing && !Existing->bActionContextOnly)) {
            // The previous live plate must leave as one retained 0.14 second
            // unit. A corpse action can acquire only after that live surface
            // is gone; no dead/skull variant is constructed in between.
            return false;
        }
        PrimaryCue = CueFromActionSemantic(*AvailableCorpseAction->Definition);
        OutEntry.bActionContextOnly = true;
    }

    // The shared attention pass already classifies this from its canonical status-definition cache so crowd
    // prioritization and tier promotion consume one semantic evidence seam rather than performing duplicate scans.
    bool bSafetyCriticalStatus = Observation.bSafetyCriticalStatus;

    const FMythicReplicatedEntityCombatPresentation *CombatRead =
        CombatPresentation.IsValid()
        ? CombatPresentation->FindCurrentCombatPresentation(
              Observation.Instance)
        : nullptr;
    const bool bIncomingCombatException =
        Observation.bRecentCombatSignal
        && Observation.bRecentIncomingCombatSignal;
    const bool bSafetyException = bDownedOrDying
        || Observation.bSafetySignal
        || (Observation.bPublicSafetyEvidence && !bObservedDead)
        || bSafetyCriticalStatus;
    const bool bValidatedActionContext = AvailableCorpseAction
        && (Observation.bInteractionTarget
            || Observation.bRecentActionSignal
            || Observation.bOpportunitySignal);
    const bool bBossException = CombatRead
        && IsBossRank(CombatRead->PresentedCombatRank);
    const bool bSemanticDistanceException = Observation.bHardTarget
        || Observation.bInteractionTarget || bIncomingCombatException
        || bSafetyException || bValidatedActionContext || bBossException;

    const bool bFactPromotesContext =
        PrimaryCue == EMythicNameplatePrimaryCue::Downed
        || PrimaryCue == EMythicNameplatePrimaryCue::Dying
        || PrimaryCue == EMythicNameplatePrimaryCue::AttackingViewer
        || PrimaryCue == EMythicNameplatePrimaryCue::Fleeing
        || PrimaryCue == EMythicNameplatePrimaryCue::Surrendering;

    const FMythicEntityAttentionConfig AttentionConfig = Policy
        ? Policy->Attention
        : FMythicEntityAttentionConfig();
    FMythicNameplateDisclosureEvidence Evidence;
    const bool bSupportsInspection = ResolveInspectionSupport(Identity);
    Evidence.bPresentationPermitted = true;
    Evidence.bHasLineOfSight = Observation.bHasLineOfSight;
    Evidence.bHardTarget = Observation.bHardTarget;
    Evidence.bInteractionTarget = Observation.bInteractionTarget;
    Evidence.bFocusAttention = Observation.bFocused;
    Evidence.bContextSignal = Observation.bSafetySignal
        || Observation.bOpportunitySignal || Observation.bAwarenessSignal
        || Observation.bRecentCombatSignal
        || Observation.bRecentActionSignal || bActionPromotesContext
        || Observation.bPublicSafetyEvidence
        || Observation.bPublicAwarenessEvidence
        || bFactPromotesContext || bSafetyCriticalStatus;
    Evidence.bGazeAttention = Observation.bGazeCandidate
        && Observation.StableGazeSeconds
               >= AttentionConfig.WhisperAcquireDwellSeconds;
    Evidence.bPersonalSpaceAttention = Observation.bWithinAmbientRange
        && Observation.ViewAlignment > 0.0f;

    FMythicNameplateProjection Projection;
    Projection.Instance = Observation.Instance;
    Projection.DisclosureTier =
        FMythicNameplateRules::ResolveDesiredDisclosure(Evidence);
    const bool bMandatoryPrimaryCue =
        PrimaryCue == EMythicNameplatePrimaryCue::Downed
        || PrimaryCue == EMythicNameplatePrimaryCue::Dying
        || PrimaryCue == EMythicNameplatePrimaryCue::AttackingViewer;
    const bool bMandatoryMinimalSurface =
        DirectorTierOrdinal(Projection.DisclosureTier)
            >= DirectorTierOrdinal(
                EMythicNameplateDisclosureTier::Focus)
        || Observation.bHardTarget || Observation.bSafetySignal
        || Observation.bRecentCombatSignal
        || Observation.bPublicSafetyEvidence || bMandatoryPrimaryCue
        || bSafetyCriticalStatus;
    Projection.DisclosureTier = FMythicNameplateRules::ApplyPresentationMode(
        Projection.DisclosureTier, PresentationMode,
        bMandatoryMinimalSurface);
    if (Projection.DisclosureTier
        == EMythicNameplateDisclosureTier::Silent) {
        return false;
    }

    const bool bPassiveWhisper = !bSemanticDistanceException
        && Projection.DisclosureTier
            == EMythicNameplateDisclosureTier::Whisper;
    const bool bPassiveFocus = !bSemanticDistanceException
        && Projection.DisclosureTier
            == EMythicNameplateDisclosureTier::Focus
        && Observation.bFocused;
    OutEntry.bPassiveNeutral = bPassiveWhisper || bPassiveFocus;
    OutEntry.LastDistanceCentimeters = Observation.DistanceCentimeters;
    if (OutEntry.bPassiveNeutral) {
        const FMythicNameplatePassiveIdentityPolicy PassiveIdentity = Policy
            ? Policy->PassiveIdentity
            : FMythicNameplatePassiveIdentityPolicy();
        const bool bPassiveIncumbent = ActiveEntries.ContainsByPredicate(
            [&Observation](const FProjectionEntry &Entry) {
                return Entry.Projection.Instance == Observation.Instance
                    && Entry.bPassiveNeutral;
            });
        const float AcquireDistance = bPassiveWhisper
            ? PassiveIdentity.WhisperAcquireDistanceCentimeters
            : PassiveIdentity.FocusAcquireDistanceCentimeters;
        const float ReleaseDistance = bPassiveWhisper
            ? PassiveIdentity.WhisperReleaseDistanceCentimeters
            : PassiveIdentity.FocusReleaseDistanceCentimeters;
        if (!FMythicNameplateRules::ShouldAdmitPassiveSurface(
                Observation.DistanceCentimeters, AcquireDistance,
                ReleaseDistance, bPassiveIncumbent)) {
            OutEntry.bRejectedByPassiveRange = true;
            return false;
        }
    }
    Projection.PrimaryCue = PrimaryCue;
    Projection.AttentionState = ResolveAttentionState(Observation);
    Projection.Lane = FMythicNameplateRules::ResolveLane(
        PrimaryCue,
        Projection.DisclosureTier
            == EMythicNameplateDisclosureTier::Focus);
    Projection.ResolvedName = ResolveIdentity(Identity, KnowledgePtr,
                                              Component);

    const bool bDeliberateSurface =
        DirectorTierOrdinal(Projection.DisclosureTier)
        >= DirectorTierOrdinal(
            EMythicNameplateDisclosureTier::Focus);
    FText ResolvedRole;
    FText ResolvedFaction;
    if (bDeliberateSurface) {
        UMythicEntityIdentityDefinition *PublicDefinition =
            ResolveIdentityDefinition(Identity.PublicIdentityDefinitionId);
        const FGameplayTag PublicRoleTag = PublicDefinition
            ? PublicDefinition->PublicArchetypeTag : FGameplayTag();
        const FGameplayTag RoleTag = KnowledgePtr && KnowledgePtr->bRoleKnown
            ? KnowledgePtr->KnownRoleTag
            : PublicRoleTag;
        const FGameplayTag FactionTag =
            KnowledgePtr && KnowledgePtr->bFactionKnown
            ? KnowledgePtr->KnownFactionTag
            : PublicDefinition
                ? PublicDefinition->PresentedFactionTag : FGameplayTag();
        ResolvedRole = ResolveArchetypeLabel(RoleTag);
        ResolvedFaction = ResolveFactionLabel(FactionTag);
        if (!ResolvedRole.IsEmpty() && !ResolvedFaction.IsEmpty()) {
            Projection.ResolvedSubtitle = FText::Format(
                LOCTEXT("IdentitySubtitleFormat", "{0} \u2022 {1}"),
                ResolvedRole, ResolvedFaction);
        } else {
            Projection.ResolvedSubtitle = !ResolvedRole.IsEmpty()
                ? ResolvedRole : ResolvedFaction;
        }
    }

    if (CombatRead
        && Projection.DisclosureTier
            != EMythicNameplateDisclosureTier::Whisper) {
        Projection.bCombatCapable = CombatRead->bCombatCapable;
        Projection.PresentedCombatRank =
            CombatRead->PresentedCombatRank;
        Projection.ThreatBand = (bDeliberateSurface || Observation.bHardTarget)
            ? CombatRead->ThreatBand : EMythicThreatBand::Unknown;

        FMythicNameplateLevelContext LevelContext;
        LevelContext.bExactLevelPermitted =
            CombatRead->bHasExactCombatLevel;
        LevelContext.bCombatCapable = CombatRead->bCombatCapable;
        LevelContext.bCurrentCombatTarget = Observation.bHardTarget;
        Projection.bShowExactLevel =
            FMythicNameplateRules::ShouldShowExactLevel(
                Projection.DisclosureTier, LevelContext);
        Projection.CombatLevel = Projection.bShowExactLevel
            ? CombatRead->ExactCombatLevel : 0;
        Projection.ResolvedLevelText = Projection.bShowExactLevel
            ? FText::AsNumber(Projection.CombatLevel)
            : FText::GetEmpty();
    }

    GatherStatuses(*Component, Projection.DisclosureTier,
                   Projection.Statuses,
                   Projection.StatusOverflowCount, bSafetyCriticalStatus);
    ResolveHealth(*Component, Projection.DisclosureTier, Observation,
                  bDownedOrDying, bCombatRelevant, bCanAssist,
                  Projection.PresentedCombatRank, Projection);
    Projection.bHealthPercentEligible = Projection.bShowHealth
        && (bDeliberateSurface || Observation.bHardTarget);

    const bool bUrgentProtectedAlly = bCanAssist && bDownedOrDying;
    const bool bCombatPresentationEarned =
        Projection.bCombatCapable || Projection.bShowHealth
        || bCombatRelevant
        || Projection.PresentedCombatRank
            != EMythicPresentedCombatRank::Unknown;
    Projection.VisualFamily = FMythicNameplateRules::ResolveVisualFamily(
        Projection.PresentedCombatRank, bCombatPresentationEarned,
        bUrgentProtectedAlly);
    if (Projection.VisualFamily != EMythicNameplateVisualFamily::Identity) {
        Projection.ResolvedSubtitle = FText::GetEmpty();
    }

    if (OutEntry.bActionContextOnly) {
        // This is a new viewer-specific corpse action surface, not a dead
        // variant of the live plate. Retain only action meaning and execution.
        Projection.ResolvedName = FText::GetEmpty();
        Projection.ResolvedSubtitle = FText::GetEmpty();
        Projection.VisualFamily = EMythicNameplateVisualFamily::Identity;
        Projection.bShowHealth = false;
        Projection.HealthFraction = 0.0f;
        Projection.bHealthPercentEligible = false;
        Projection.bCombatCapable = false;
        Projection.PresentedCombatRank =
            EMythicPresentedCombatRank::Unknown;
        Projection.ThreatBand = EMythicThreatBand::Unknown;
        Projection.bShowExactLevel = false;
        Projection.CombatLevel = 0;
        Projection.ResolvedLevelText = FText::GetEmpty();
        Projection.Statuses.Reset();
        Projection.StatusOverflowCount = 0;
    }

    if (bDeliberateSurface) {
        FMythicNameplateActionRailProjection &Rail =
            OutEntry.ActionRailProjection;
        Rail.Instance = Observation.Instance;
        Rail.bInspectAvailable = !OutEntry.bActionContextOnly
            && bSupportsInspection
            && KnowledgePtr != nullptr;
        if (Rail.bInspectAvailable) {
            Rail.InspectInputActionTag = UI_ACTION_INSPECT_ENTITY;
            Rail.ResolvedInspectLabel = LOCTEXT("InspectAction", "Inspect");
        }
        const int32 AuthoredCap = FMath::Clamp(
            Policy ? Policy->Actions.FocusActionCap : 2, 1, 2);
        const int32 ActionCap = Rail.bInspectAvailable
            ? FMath::Max(0, AuthoredCap - 1) : AuthoredCap;
        Rail.Actions.Reserve(FMath::Min(ActionCap, Actions.Num()));
        TArray<FGameplayTag, TInlineAllocator<6>> ProjectedInputTags;
        for (const FActionCandidate &Action : Actions) {
            if (Rail.Actions.Num() >= ActionCap) {
                break;
            }
            if (!Action.bAvailable || Action.Projection.OfferRevision == 0
                || !Action.Projection.ActionTag.IsValid()
                || !Action.Projection.InputActionTag.IsValid()
                || (Rail.bInspectAvailable
                    && Action.Projection.InputActionTag
                        == Rail.InspectInputActionTag)
                || ProjectedInputTags.Contains(
                    Action.Projection.InputActionTag)) {
                continue;
            }
            ProjectedInputTags.Add(Action.Projection.InputActionTag);
            Rail.Actions.Add(Action.Projection);
        }
        if (!Rail.IsPresentable()) {
            Rail = FMythicNameplateActionRailProjection();
        }
    }

    FMythicNameplateRules::SanitizeProjectionForPresentation(Projection);

    OutEntry.Projection = MoveTemp(Projection);
    OutEntry.Component = Component;
    OutEntry.LastScreenPosition = Observation.ScreenPosition;
    OutEntry.Score = Observation.Score;
    OutEntry.bPureAmbientWhisper =
        OutEntry.Projection.DisclosureTier
            == EMythicNameplateDisclosureTier::Whisper
        && Observation.PriorityClass
               == EMythicEntityAttentionPriorityClass::Ambient;
    return true;
}

void UMythicNameplateDirector::AppendReleaseGraceLeases(
    TArray<FProjectionEntry> &Candidates,
    const TSet<FMythicEntityPresentationInstance> &ImmediateCollapseInstances,
    const TSet<FMythicEntityPresentationInstance> &ImmediateFadeInstances,
    const double NowSeconds) const {
    const FMythicEntityAttentionConfig AttentionConfig = Policy
        ? Policy->Attention
        : FMythicEntityAttentionConfig();

    for (const FProjectionEntry &ActiveEntry : ActiveEntries) {
        const FMythicEntityPresentationInstance Instance =
            ActiveEntry.Projection.Instance;
        const bool bAlreadyFresh = Candidates.ContainsByPredicate(
            [&Instance](const FProjectionEntry &Candidate) {
                return Candidate.Projection.Instance == Instance;
            });
        if (bAlreadyFresh || ImmediateCollapseInstances.Contains(Instance)) {
            continue;
        }

        FProjectionEntry Lease = ActiveEntry;
        Lease.bFreshEvidence = false;
        // Executable grants never survive without fresh viewer-specific
        // evidence, even while harmless outgoing visuals finish releasing.
        Lease.ActionRailProjection = FMythicNameplateActionRailProjection();
        if (!IsLeaseSubjectStillValid(ActiveEntry)) {
            // Death/deactivation can revoke the registry/component before the
            // local outgoing plate sees its last frame. Freeze only sanitized
            // render state; never query or rebind the retiring component.
            Lease.Component.Reset();
            Lease.bFrozenRetiring = true;
        }

        const float ReleaseSeconds = FMath::Max(0.0f,
            Policy ? Policy->PassiveIdentity.ReleaseTransitionSeconds
                   : FMythicNameplatePassiveIdentityPolicy()
                         .ReleaseTransitionSeconds);
        if (Lease.bReleasing) {
            const double ReleaseEndSeconds = Lease.TransitionStartSeconds
                + static_cast<double>(ReleaseSeconds);
            if (RenderPreferences.bReducedMotion
                || NowSeconds >= ReleaseEndSeconds) {
                continue;
            }
            Candidates.Add(MoveTemp(Lease));
            continue;
        }

        if (Lease.bFrozenRetiring || Lease.bActionContextOnly
            || ImmediateFadeInstances.Contains(Instance)) {
            BeginReleaseTransition(Lease, NowSeconds);
            Candidates.Add(MoveTemp(Lease));
            continue;
        }

        const float EvidenceLostSeconds = static_cast<float>(
            FMath::Max(0.0, NowSeconds - ActiveEntry.LastEligibleSeconds));
        if (FMythicNameplateRules::ShouldDemote(
                ActiveEntry.Projection.DisclosureTier,
                EMythicNameplateDisclosureTier::Silent,
                EvidenceLostSeconds, AttentionConfig)) {
            BeginReleaseTransition(Lease, NowSeconds);
            Candidates.Add(MoveTemp(Lease));
            continue;
        }

        Candidates.Add(MoveTemp(Lease));
    }
}

void UMythicNameplateDirector::BeginReleaseTransition(
    FProjectionEntry &Entry, const double NowSeconds) const {
    if (Entry.bReleasing) {
        return;
    }
    const FMythicNameplatePassiveIdentityPolicy PassiveIdentity = Policy
        ? Policy->PassiveIdentity
        : FMythicNameplatePassiveIdentityPolicy();
    Entry.TemporalAlpha = FMythicNameplateRules::ResolveTemporalAlpha(
        Entry.TransitionStartAlpha, false, Entry.TransitionStartSeconds,
        NowSeconds, PassiveIdentity.AcquireTransitionSeconds,
        PassiveIdentity.ReleaseTransitionSeconds,
        RenderPreferences.bReducedMotion);
    Entry.TransitionStartAlpha = Entry.TemporalAlpha;
    Entry.TransitionStartSeconds = NowSeconds;
    Entry.bReleasing = true;
}

bool UMythicNameplateDirector::IsLeaseSubjectStillValid(
    const FProjectionEntry &Entry) const {
    UMythicEntityPresentationComponent *Component = Entry.Component.Get();
    if (!Component
        || !Component->RepresentsInstance(Entry.Projection.Instance)) {
        return false;
    }
    const AActor *Owner = Component->GetOwner();
    if (!IsValid(Owner) || Owner->IsHidden()) {
        return false;
    }
    const FMythicPublicIdentitySnapshot &Identity =
        Component->GetPublicIdentitySnapshot();
    return Identity.IsActive()
        && Identity.Instance == Entry.Projection.Instance;
}

void UMythicNameplateDirector::StabilizeAmbientWhisperIncumbent(
    TArray<FProjectionEntry> &Candidates,
    const int32 AmbientWhisperCapacity,
    const double NowSeconds) {
    if (AmbientWhisperCapacity != 1) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        return;
    }

    const FProjectionEntry *ActiveIncumbent = ActiveEntries.FindByPredicate(
        [](const FProjectionEntry &Entry) {
            return Entry.bPureAmbientWhisper;
        });
    if (!ActiveIncumbent) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        return;
    }

    const FMythicEntityPresentationInstance IncumbentInstance =
        ActiveIncumbent->Projection.Instance;
    FProjectionEntry *Incumbent = Candidates.FindByPredicate(
        [&IncumbentInstance](const FProjectionEntry &Entry) {
            return Entry.bPureAmbientWhisper
                && Entry.Projection.Instance == IncumbentInstance;
        });
    if (!Incumbent) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        return;
    }

    FProjectionEntry *Challenger = nullptr;
    for (FProjectionEntry &Candidate : Candidates) {
        if (!Candidate.bPureAmbientWhisper
            || Candidate.Projection.Instance == IncumbentInstance) {
            continue;
        }
        if (!Challenger || Candidate.Score > Challenger->Score
            || (FMath::IsNearlyEqual(Candidate.Score, Challenger->Score)
                && GetTypeHash(Candidate.Projection.Instance)
                    < GetTypeHash(Challenger->Projection.Instance))) {
            Challenger = &Candidate;
        }
    }
    if (!Challenger) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        return;
    }

    const FMythicEntityAttentionConfig AttentionConfig = Policy
        ? Policy->Attention
        : FMythicEntityAttentionConfig();
    const bool bHasContinuousLead =
        FMythicNameplateRules::HasReplacementScoreLead(
            Incumbent->Score, Challenger->Score, AttentionConfig);
    if (!bHasContinuousLead) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
        Candidates.RemoveAllSwap(
            [&IncumbentInstance](const FProjectionEntry &Candidate) {
                return Candidate.bPureAmbientWhisper
                    && Candidate.Projection.Instance != IncumbentInstance;
            }, EAllowShrinking::No);
        return;
    }
    if (PendingAmbientWhisperChallenger
        != Challenger->Projection.Instance) {
        PendingAmbientWhisperChallenger = Challenger->Projection.Instance;
        PendingAmbientWhisperChallengerSinceSeconds = NowSeconds;
    }
    const float ChallengerLeadSeconds = static_cast<float>(
        FMath::Max(0.0,
                   NowSeconds
                       - PendingAmbientWhisperChallengerSinceSeconds));
    const bool bReplace = FMythicNameplateRules::ShouldReplaceIncumbent(
        Incumbent->Score, Challenger->Score, ChallengerLeadSeconds,
        false, AttentionConfig);
    const FMythicEntityPresentationInstance AllowedInstance = bReplace
        ? Challenger->Projection.Instance
        : IncumbentInstance;
    Candidates.RemoveAllSwap(
        [&AllowedInstance](const FProjectionEntry &Candidate) {
            return Candidate.bPureAmbientWhisper
                && Candidate.Projection.Instance != AllowedInstance;
        }, EAllowShrinking::No);
    if (bReplace) {
        PendingAmbientWhisperChallenger.Reset();
        PendingAmbientWhisperChallengerSinceSeconds = 0.0;
    }
}

void UMythicNameplateDirector::CommitBoundedEntries(
    TArray<FProjectionEntry> &Candidates,
    const double NowSeconds) {
    const FMythicNameplateCapacityPolicy Capacity = Policy
        ? Policy->Capacity
        : FMythicNameplateCapacityPolicy();
    const bool bHasFocusedSurface = Candidates.ContainsByPredicate(
        [](const FProjectionEntry &Candidate) {
            return Candidate.Projection.Lane
                == EMythicNameplateLane::Focus;
        });
    const int32 AmbientWhisperCap =
        FMythicNameplateRules::GetAmbientWhisperCapacity(
            bHasFocusedSurface, PresentationMode, Capacity);
    StabilizeAmbientWhisperIncumbent(Candidates, AmbientWhisperCap,
                                     NowSeconds);
    Candidates.Sort([](const FProjectionEntry &Left,
                       const FProjectionEntry &Right) {
        const int32 LeftLane = DirectorLaneRank(Left.Projection.Lane);
        const int32 RightLane = DirectorLaneRank(Right.Projection.Lane);
        if (LeftLane != RightLane) {
            return LeftLane > RightLane;
        }
        if (!FMath::IsNearlyEqual(Left.Score, Right.Score)) {
            return Left.Score > Right.Score;
        }
        return GetTypeHash(Left.Projection.Instance)
            < GetTypeHash(Right.Projection.Instance);
    });

    int32 FocusCount = 0;
    int32 SafetyCount = 0;
    int32 OpportunityCount = 0;
    int32 AwarenessCount = 0;
    int32 PureAmbientWhisperCount = 0;
    TArray<FProjectionEntry> Selected;
    Selected.Reserve(FMath::Min(Capacity.MaxDrawnPlates, Candidates.Num()));

    auto LaneCounter = [&](const EMythicNameplateLane Lane) -> int32 & {
        switch (Lane) {
        case EMythicNameplateLane::Focus:
            return FocusCount;
        case EMythicNameplateLane::Safety:
            return SafetyCount;
        case EMythicNameplateLane::Opportunity:
            return OpportunityCount;
        case EMythicNameplateLane::Awareness:
        default:
            return AwarenessCount;
        }
    };

    for (FProjectionEntry &Candidate : Candidates) {
        if (Selected.Num() >= Capacity.MaxDrawnPlates) {
            break;
        }
        if (Candidate.bPureAmbientWhisper
            && PureAmbientWhisperCount >= AmbientWhisperCap) {
            continue;
        }
        int32 &Count = LaneCounter(Candidate.Projection.Lane);
        if (Count >= FMythicNameplateRules::GetLaneCapacity(
                         Candidate.Projection.Lane, Capacity)) {
            continue;
        }
        ++Count;
        if (Candidate.bPureAmbientWhisper) {
            ++PureAmbientWhisperCount;
        }
        Selected.Add(MoveTemp(Candidate));
    }

    TSet<FMythicEntityPresentationInstance> NewInstances;
    NewInstances.Reserve(Selected.Num());
    for (const FProjectionEntry &Entry : Selected) {
        NewInstances.Add(Entry.Projection.Instance);
    }

    auto FindOldEntry = [this](
        const FMythicEntityPresentationInstance &Instance)
        -> const FProjectionEntry * {
        return ActiveEntries.FindByPredicate(
            [&Instance](const FProjectionEntry &Entry) {
                return Entry.Projection.Instance == Instance;
            });
    };
    bool bProjectionSetChanged =
        ActiveEntries.Num() != Selected.Num();
    if (!bProjectionSetChanged) {
        for (const FProjectionEntry &Entry : Selected) {
            const FProjectionEntry *OldEntry =
                FindOldEntry(Entry.Projection.Instance);
            if (!OldEntry
                || !FMythicNameplateRules::AreProjectionsEquivalent(
                    OldEntry->Projection, Entry.Projection)
                || !FMythicNameplateRules::AreActionRailProjectionsEquivalent(
                    OldEntry->ActionRailProjection,
                    Entry.ActionRailProjection)) {
                bProjectionSetChanged = true;
                break;
            }
        }
    }

    if (UMythicNameplateLayer *Layer = PresentationLayer.Get()) {
        for (const FProjectionEntry &OldEntry : ActiveEntries) {
            if (!NewInstances.Contains(OldEntry.Projection.Instance)) {
                Layer->ReleaseProjection(OldEntry.Projection.Instance);
                Layer->ReleaseActionRailProjection(
                    OldEntry.Projection.Instance);
            }
        }
        for (FProjectionEntry &Entry : Selected) {
            const FProjectionEntry *OldEntry =
                FindOldEntry(Entry.Projection.Instance);
            if (OldEntry) {
                Entry.LastResolvedScreenPosition =
                    OldEntry->LastResolvedScreenPosition;
                Entry.bCollisionSuppressed =
                    OldEntry->bCollisionSuppressed;
                Entry.DistanceAlpha = OldEntry->DistanceAlpha;
                Entry.PresentationScale = OldEntry->PresentationScale;
                Entry.TemporalAlpha = OldEntry->TemporalAlpha;
                Entry.TransitionStartAlpha =
                    OldEntry->TransitionStartAlpha;
                Entry.TransitionStartSeconds =
                    OldEntry->TransitionStartSeconds;
                if (Entry.bFreshEvidence && OldEntry->bReleasing) {
                    const FMythicNameplatePassiveIdentityPolicy
                        PassiveIdentity = Policy
                            ? Policy->PassiveIdentity
                            : FMythicNameplatePassiveIdentityPolicy();
                    Entry.TemporalAlpha =
                        FMythicNameplateRules::ResolveTemporalAlpha(
                            OldEntry->TransitionStartAlpha, true,
                            OldEntry->TransitionStartSeconds, NowSeconds,
                            PassiveIdentity.AcquireTransitionSeconds,
                            PassiveIdentity.ReleaseTransitionSeconds,
                            RenderPreferences.bReducedMotion);
                    Entry.TransitionStartAlpha = Entry.TemporalAlpha;
                    Entry.TransitionStartSeconds = NowSeconds;
                    Entry.bReleasing = false;
                }
            } else {
                Entry.TransitionStartAlpha = 0.0f;
                Entry.TransitionStartSeconds = NowSeconds;
                Entry.TemporalAlpha = RenderPreferences.bReducedMotion
                    ? 1.0f : 0.0f;
            }

            if (!Entry.bReleasing && Entry.bPassiveNeutral) {
                const FMythicNameplatePassiveIdentityPolicy
                    PassiveIdentity = Policy
                        ? Policy->PassiveIdentity
                        : FMythicNameplatePassiveIdentityPolicy();
                const bool bWhisper = Entry.Projection.DisclosureTier
                    == EMythicNameplateDisclosureTier::Whisper;
                const FMythicNameplateDistancePresentation Distance =
                    FMythicNameplateRules::ResolvePassiveDistancePresentation(
                        Entry.LastDistanceCentimeters,
                        bWhisper
                            ? PassiveIdentity
                                  .WhisperFullDistanceCentimeters
                            : PassiveIdentity.FocusFullDistanceCentimeters,
                        bWhisper
                            ? PassiveIdentity
                                  .WhisperReleaseDistanceCentimeters
                            : PassiveIdentity.FocusReleaseDistanceCentimeters,
                        bWhisper ? PassiveIdentity.WhisperFullAlpha
                                 : PassiveIdentity.FocusFullAlpha,
                        bWhisper ? PassiveIdentity.WhisperReleaseScale
                                 : PassiveIdentity.FocusReleaseScale,
                        RenderPreferences.bReducedMotion
                            || RenderPreferences.bHighContrast);
                Entry.DistanceAlpha = Distance.Alpha;
                Entry.PresentationScale = Distance.Scale;
            } else if (!Entry.bReleasing) {
                Entry.DistanceAlpha = 1.0f;
                Entry.PresentationScale = 1.0f;
            }
            const float PresentationAlpha = Entry.bCollisionSuppressed
                ? 0.0f
                : Entry.DistanceAlpha * Entry.TemporalAlpha;
            const FVector2D RenderPosition = Entry.bFrozenRetiring
                ? Entry.LastResolvedScreenPosition
                : Entry.LastScreenPosition;
            const bool bPlateChanged = bPresentationLayerNeedsFullSync
                || !OldEntry
                || !FMythicNameplateRules::AreProjectionsEquivalent(
                    OldEntry->Projection, Entry.Projection);
            const bool bActionRailChanged = bPresentationLayerNeedsFullSync
                || !OldEntry
                || !FMythicNameplateRules::AreActionRailProjectionsEquivalent(
                    OldEntry->ActionRailProjection,
                    Entry.ActionRailProjection);
            bool bPlateClaimed = bPlateChanged
                ? Layer->ApplyProjection(Entry.Projection,
                                         RenderPosition,
                                         PresentationAlpha,
                                         Entry.PresentationScale)
                : Layer->UpdateProjectionPlacement(
                      Entry.Projection.Instance,
                      RenderPosition, PresentationAlpha,
                      Entry.PresentationScale);
            if (!bPlateClaimed && !bPlateChanged) {
                // A prior pool-pressure miss must remain retryable even when
                // its semantic projection is unchanged.
                bPlateClaimed = Layer->ApplyProjection(
                    Entry.Projection, RenderPosition,
                    PresentationAlpha, Entry.PresentationScale);
            }
            if (!bPlateClaimed) {
                // The separate rail may never survive without its exact plate.
                Layer->ReleaseActionRailProjection(
                    Entry.Projection.Instance);
                continue;
            }
            if (Entry.ActionRailProjection.IsPresentable()) {
                bool bRailClaimed = bActionRailChanged
                    ? Layer->ApplyActionRailProjection(
                          Entry.ActionRailProjection,
                          RenderPosition, PresentationAlpha,
                          Entry.PresentationScale)
                    : Layer->UpdateActionRailPlacement(
                          Entry.Projection.Instance,
                          RenderPosition, PresentationAlpha,
                          Entry.PresentationScale);
                if (!bRailClaimed && !bActionRailChanged) {
                    bRailClaimed = Layer->ApplyActionRailProjection(
                        Entry.ActionRailProjection,
                        RenderPosition, PresentationAlpha,
                        Entry.PresentationScale);
                }
                if (!bRailClaimed) {
                    Layer->ReleaseActionRailProjection(
                        Entry.Projection.Instance);
                }
            } else if (bActionRailChanged) {
                Layer->ReleaseActionRailProjection(
                    Entry.Projection.Instance);
            }
        }
    }
    bPresentationLayerNeedsFullSync = false;

    ActiveEntries = MoveTemp(Selected);
    if (bProjectionSetChanged) {
        PublishProjectionRevision();
    }
}

void UMythicNameplateDirector::UpdateActivePlacements() {
    UMythicNameplateLayer *Layer = PresentationLayer.Get();
    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = GetTickableGameObjectWorld();
    APlayerController *Controller = LocalPlayer && World
        ? LocalPlayer->GetPlayerController(World)
        : nullptr;
    if (!Layer || !Controller) {
        return;
    }

    const FMythicNameplateDeclutterPolicy Declutter = Policy
        ? Policy->Declutter : FMythicNameplateDeclutterPolicy();
    const UMythicNameplateVisualStyle *VisualStyle =
        Layer->GetVisualStyle();
    if (!VisualStyle) {
        return;
    }
    const float ScreenPixelsPerLogicalPixel =
        Layer->GetScreenPixelsPerLogicalPixel();
    const FMythicNameplatePassiveIdentityPolicy PassiveIdentity = Policy
        ? Policy->PassiveIdentity
        : FMythicNameplatePassiveIdentityPolicy();
    const double NowSeconds = FPlatformTime::Seconds();
    FVector CameraLocation;
    FRotator CameraRotation;
    Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);
    TArray<FBox2D, TInlineAllocator<16>> HigherPriorityBounds;
    HigherPriorityBounds.Reserve(ActiveEntries.Num());

    bool bFoundStaleEntry = false;
    for (FProjectionEntry &Entry : ActiveEntries) {
        if (Entry.bFrozenRetiring) {
            Entry.TemporalAlpha =
                FMythicNameplateRules::ResolveTemporalAlpha(
                    Entry.TransitionStartAlpha, true,
                    Entry.TransitionStartSeconds, NowSeconds,
                    PassiveIdentity.AcquireTransitionSeconds,
                    PassiveIdentity.ReleaseTransitionSeconds,
                    RenderPreferences.bReducedMotion);
            const float PresentationAlpha = Entry.bCollisionSuppressed
                ? 0.0f
                : FMath::Clamp(Entry.DistanceAlpha
                        * Entry.TemporalAlpha,
                    0.0f, 1.0f);
            if (!Layer->UpdateProjectionPlacement(
                    Entry.Projection.Instance,
                    Entry.LastResolvedScreenPosition,
                    PresentationAlpha, Entry.PresentationScale)) {
                bFoundStaleEntry = true;
            }
            if (Entry.TemporalAlpha <= UE_KINDA_SMALL_NUMBER) {
                bFoundStaleEntry = true;
            }
            continue;
        }

        UMythicEntityPresentationComponent *Component = Entry.Component.Get();
        FVector2D ScreenPosition;
        FVector AnchorWorldLocation = FVector::ZeroVector;
        if (!Component
            || !Component->RepresentsInstance(Entry.Projection.Instance)
            || (AnchorWorldLocation =
                    Component->GetPresentationAnchorWorldLocation())
                    .ContainsNaN()
            || !Controller->ProjectWorldLocationToScreen(
                AnchorWorldLocation, ScreenPosition, true)) {
            Layer->ReleaseProjection(Entry.Projection.Instance);
            Layer->ReleaseActionRailProjection(
                Entry.Projection.Instance);
            bFoundStaleEntry = true;
            continue;
        }

        Entry.LastDistanceCentimeters = FVector::Distance(
            CameraLocation, AnchorWorldLocation);
        if (!FMath::IsFinite(Entry.LastDistanceCentimeters)) {
            Layer->ReleaseProjection(Entry.Projection.Instance);
            Layer->ReleaseActionRailProjection(
                Entry.Projection.Instance);
            bFoundStaleEntry = true;
            continue;
        }

        if (!Entry.bReleasing && Entry.bPassiveNeutral) {
            const bool bWhisper = Entry.Projection.DisclosureTier
                == EMythicNameplateDisclosureTier::Whisper;
            const FMythicNameplateDistancePresentation Distance =
                FMythicNameplateRules::ResolvePassiveDistancePresentation(
                    Entry.LastDistanceCentimeters,
                    bWhisper
                        ? PassiveIdentity.WhisperFullDistanceCentimeters
                        : PassiveIdentity.FocusFullDistanceCentimeters,
                    bWhisper
                        ? PassiveIdentity.WhisperReleaseDistanceCentimeters
                        : PassiveIdentity.FocusReleaseDistanceCentimeters,
                    bWhisper ? PassiveIdentity.WhisperFullAlpha
                             : PassiveIdentity.FocusFullAlpha,
                    bWhisper ? PassiveIdentity.WhisperReleaseScale
                             : PassiveIdentity.FocusReleaseScale,
                    RenderPreferences.bReducedMotion
                        || RenderPreferences.bHighContrast);
            if (Distance.bBeyondRelease) {
                // Retain the last admitted distance treatment and release the
                // whole surface. This is essential for High Contrast, where
                // the surface remains opaque until the exact release edge.
                BeginReleaseTransition(Entry, NowSeconds);
            } else {
                Entry.DistanceAlpha = Distance.Alpha;
                Entry.PresentationScale = Distance.Scale;
            }
        } else if (!Entry.bReleasing) {
            Entry.DistanceAlpha = 1.0f;
            Entry.PresentationScale = 1.0f;
        }

        Entry.TemporalAlpha = FMythicNameplateRules::ResolveTemporalAlpha(
            Entry.TransitionStartAlpha, Entry.bReleasing,
            Entry.TransitionStartSeconds, NowSeconds,
            PassiveIdentity.AcquireTransitionSeconds,
            PassiveIdentity.ReleaseTransitionSeconds,
            RenderPreferences.bReducedMotion);
        const float UnoccludedAlpha = FMath::Clamp(
            Entry.DistanceAlpha * Entry.TemporalAlpha, 0.0f, 1.0f);

        const FMythicNameplateProfileGeometry &Geometry =
            VisualStyle->ResolveGeometry(
                Entry.Projection.DisclosureTier,
                Entry.Projection.VisualFamily,
                Entry.Projection.PresentedCombatRank);
        const float PlateHeight = FMath::Max(
            1.0f, Geometry.MaximumSize.Y);
        const bool bHasActionRail =
            Entry.ActionRailProjection.IsPresentable();
        const float ActionRailHeight = bHasActionRail
            ? FMath::Max(0.0f,
                         VisualStyle->ActionRailMaximumSize.Y)
            : 0.0f;
        const FVector2D LogicalFootprint(
            FMath::Max(1.0f,
                bHasActionRail
                    ? FMath::Max(Geometry.MaximumSize.X,
                                 VisualStyle->ActionRailMaximumSize.X)
                    : Geometry.MaximumSize.X),
            PlateHeight + ActionRailHeight);
        // Plate content grows above the world anchor while the optional
        // action rail grows below it. Resolve their compound bounds as one
        // semantic surface so neither can collide with a lower-priority read.
        const float LogicalCenterOffsetY =
            (ActionRailHeight - PlateHeight) * 0.5f;
        const float EffectiveScreenPixelsPerLogicalPixel =
            ScreenPixelsPerLogicalPixel
            * FMath::Clamp(Entry.PresentationScale, 0.75f, 1.0f);
        const FVector2D DesiredCenter = ScreenPosition + FVector2D(
            0.0f, LogicalCenterOffsetY
                * EffectiveScreenPixelsPerLogicalPixel);
        FVector2D ResolvedCenter = DesiredCenter;
        FBox2D ResolvedBounds(DesiredCenter, DesiredCenter);
        const bool bParticipatesInDeclutter =
            UnoccludedAlpha > UE_KINDA_SMALL_NUMBER;
        const bool bVisible = bParticipatesInDeclutter
            && FMythicNameplateRules::ResolveDeclutteredPlacement(
                DesiredCenter, LogicalFootprint,
                Entry.Projection.Lane,
                Entry.bCollisionSuppressed,
                EffectiveScreenPixelsPerLogicalPixel,
                HigherPriorityBounds, Declutter,
                ResolvedCenter, ResolvedBounds);
        const FVector2D ResolvedScreenPosition = ResolvedCenter
            - FVector2D(0.0f, LogicalCenterOffsetY
                * EffectiveScreenPixelsPerLogicalPixel);

        Entry.LastScreenPosition = ScreenPosition;
        Entry.LastResolvedScreenPosition = ResolvedScreenPosition;
        if (bParticipatesInDeclutter) {
            Entry.bCollisionSuppressed = !bVisible;
        }
        if (bVisible) {
            HigherPriorityBounds.Add(ResolvedBounds);
        }
        const float PresentationAlpha = bVisible
            ? UnoccludedAlpha : 0.0f;
        if (!Layer->UpdateProjectionPlacement(
                Entry.Projection.Instance, ResolvedScreenPosition,
                PresentationAlpha, Entry.PresentationScale)) {
            bFoundStaleEntry = true;
        }
        if (bHasActionRail
            && !Layer->UpdateActionRailPlacement(
                Entry.Projection.Instance, ResolvedScreenPosition,
                PresentationAlpha, Entry.PresentationScale)) {
            bFoundStaleEntry = true;
        }
        if (Entry.bReleasing
            && Entry.TemporalAlpha <= UE_KINDA_SMALL_NUMBER) {
            bFoundStaleEntry = true;
        }
    }
    if (bFoundStaleEntry) {
        RequestProjectionRefresh();
    }
}

void UMythicNameplateDirector::PublishProjectionRevision() {
    ++ProjectionRevision;
    if (ProjectionRevision == 0
        || ProjectionRevision > static_cast<uint32>(MAX_int32)) {
        ProjectionRevision = 1;
    }
    OnNameplateProjectionsChanged.Broadcast(
        static_cast<int32>(ProjectionRevision));
}

void UMythicNameplateDirector::RebuildInspectProjection() {
    if (!InspectedInstance.IsValid()) {
        return;
    }

    FMythicEntityInspectProjection NextProjection;
    if (!BuildInspectProjection(NextProjection)) {
        CloseEntityInspectInternal(true);
        return;
    }

    CurrentInspectProjection = MoveTemp(NextProjection);
    if (UMythicEntityInspectPage *InspectPage = ActiveInspectPage.Get()) {
        InspectPage->ApplyInspectProjection(CurrentInspectProjection);
    }
    PublishInspectRevision();
}

bool UMythicNameplateDirector::BuildInspectProjection(
    FMythicEntityInspectProjection &OutProjection) {
    OutProjection = FMythicEntityInspectProjection();
    UWorld *World = GetTickableGameObjectWorld();
    UMythicEntityPresentationRegistry *Registry = World
        ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
        : nullptr;
    UMythicEntityPresentationComponent *Component = Registry
        ? Registry->ResolvePresentationComponent(InspectedInstance)
        : nullptr;
    if (!Component || !Component->RepresentsInstance(InspectedInstance)) {
        return false;
    }

    const FMythicPublicIdentitySnapshot &Identity =
        Component->GetPublicIdentitySnapshot();
    FMythicEntityKnowledgeView Knowledge;
    if (!Identity.IsActive() || Identity.Instance != InspectedInstance
        || !ResolveInspectionSupport(Identity) || !ViewerKnowledge.IsValid()
        || !ViewerKnowledge->GetKnowledgeForSubject(InspectedInstance,
                                                    Knowledge)
        || !Knowledge.bRecognitionGranted) {
        return false;
    }

    OutProjection.Instance = InspectedInstance;
    OutProjection.ResolvedName = ResolveIdentity(Identity, &Knowledge,
                                                 Component);
    UMythicEntityIdentityDefinition *PublicDefinition =
        ResolveIdentityDefinition(Identity.PublicIdentityDefinitionId);
    const FGameplayTag PublicRoleTag = PublicDefinition
        ? PublicDefinition->PublicArchetypeTag : FGameplayTag();
    OutProjection.ResolvedRole = ResolveArchetypeLabel(
        Knowledge.bRoleKnown ? Knowledge.KnownRoleTag
                             : PublicRoleTag);
    OutProjection.ResolvedFaction = ResolveFactionLabel(
        Knowledge.bFactionKnown ? Knowledge.KnownFactionTag
                                : PublicDefinition
                                    ? PublicDefinition->PresentedFactionTag
                                    : FGameplayTag());
    OutProjection.ResolvedRelationship =
        ResolveRelationshipBand(Knowledge.RelationshipBand);
    OutProjection.ResolvedStanding =
        ResolveStandingBand(Knowledge.StandingBand);

    if (const FMythicReplicatedEntityCombatPresentation *CombatRead =
            CombatPresentation.IsValid()
            ? CombatPresentation->FindCurrentCombatPresentation(
                  InspectedInstance)
            : nullptr) {
        OutProjection.ThreatBand = CombatRead->ThreatBand;
        OutProjection.bCombatCapable = CombatRead->bCombatCapable;
        OutProjection.bBoss = IsBossRank(
            CombatRead->PresentedCombatRank);
        OutProjection.bShowExactCombatLevel =
            CombatRead->bCombatCapable
            && CombatRead->bHasExactCombatLevel;
        OutProjection.ExactCombatLevel =
            OutProjection.bShowExactCombatLevel
            ? CombatRead->ExactCombatLevel : 0;
    }

    AppendResolvedFacts(Knowledge.DiscoveredTraits,
                        EMythicEntityKnowledgeFactSection::Trait,
                        OutProjection.Traits);
    AppendResolvedFacts(Knowledge.DiscoveredHistory,
                        EMythicEntityKnowledgeFactSection::History,
                        OutProjection.History);
    AppendResolvedFacts(Knowledge.KnownLikes,
                        EMythicEntityKnowledgeFactSection::Like,
                        OutProjection.Likes);
    AppendResolvedFacts(Knowledge.KnownDislikes,
                        EMythicEntityKnowledgeFactSection::Dislike,
                        OutProjection.Dislikes);
    AppendResolvedFacts(Knowledge.DiscoveredConnections,
                        EMythicEntityKnowledgeFactSection::Connection,
                        OutProjection.Connections);

    const int32 TotalCap = FMath::Clamp(
        Policy ? Policy->Inspect.MaxTotalFacts : 24, 1, 40);
    TArray<FMythicEntityInspectFactProjection> *Sections[] = {
        &OutProjection.Traits,
        &OutProjection.History,
        &OutProjection.Likes,
        &OutProjection.Dislikes,
        &OutProjection.Connections,
    };
    int32 TotalFacts = 0;
    for (const TArray<FMythicEntityInspectFactProjection> *Section : Sections) {
        TotalFacts += Section->Num();
    }
    while (TotalFacts > TotalCap) {
        TArray<FMythicEntityInspectFactProjection> *WorstSection = nullptr;
        int32 WorstIndex = INDEX_NONE;
        for (TArray<FMythicEntityInspectFactProjection> *Section : Sections) {
            for (int32 Index = 0; Index < Section->Num(); ++Index) {
                const FMythicEntityInspectFactProjection &Candidate =
                    (*Section)[Index];
                if (!WorstSection) {
                    WorstSection = Section;
                    WorstIndex = Index;
                    continue;
                }
                const FMythicEntityInspectFactProjection &Worst =
                    (*WorstSection)[WorstIndex];
                const bool bLowerPriority =
                    Candidate.PresentationPriority < Worst.PresentationPriority;
                const bool bLaterStableTag =
                    Candidate.PresentationPriority == Worst.PresentationPriority
                    && Worst.FactTag.GetTagName().LexicalLess(
                        Candidate.FactTag.GetTagName());
                if (bLowerPriority || bLaterStableTag) {
                    WorstSection = Section;
                    WorstIndex = Index;
                }
            }
        }
        if (!WorstSection || WorstIndex == INDEX_NONE) {
            break;
        }
        WorstSection->RemoveAt(WorstIndex, 1, EAllowShrinking::No);
        --TotalFacts;
    }

    return OutProjection.IsValid();
}

void UMythicNameplateDirector::CloseEntityInspectInternal(
    const bool bDeactivatePage) {
    UMythicEntityInspectPage *InspectPage = ActiveInspectPage.Get();
    if (InspectPage && InspectPageDeactivatedHandle.IsValid()) {
        InspectPage->OnDeactivated().Remove(InspectPageDeactivatedHandle);
    }
    InspectPageDeactivatedHandle.Reset();
    ActiveInspectPage.Reset();
    if (bDeactivatePage && InspectPage && InspectPage->IsActivated()) {
        InspectPage->DeactivateWidget();
    }

    if (UMythicEntityAttentionSubsystem *AttentionSubsystem = Attention.Get()) {
        AttentionSubsystem->SetInspectTarget(
            FMythicEntityPresentationInstance());
    }

    const bool bHadInspect = InspectedInstance.IsValid()
        || CurrentInspectProjection.IsValid();
    InspectedInstance.Reset();
    CurrentInspectProjection = FMythicEntityInspectProjection();
    if (bHadInspect) {
        PublishInspectRevision();
        RequestProjectionRefresh();
    }
}

void UMythicNameplateDirector::PublishInspectRevision() {
    ++InspectRevision;
    if (InspectRevision == 0
        || InspectRevision > static_cast<uint32>(MAX_int32)) {
        InspectRevision = 1;
    }
    OnEntityInspectProjectionChanged.Broadcast(
        static_cast<int32>(InspectRevision));
}

void UMythicNameplateDirector::AppendResolvedFacts(
    const FGameplayTagContainer &FactTags,
    const EMythicEntityKnowledgeFactSection ExpectedSection,
    TArray<FMythicEntityInspectFactProjection> &OutFacts) {
    OutFacts.Reset();
    const int32 SectionCap = FMath::Clamp(
        Policy ? Policy->Inspect.MaxFactsPerSection : 8, 1, 16);
    TArray<FMythicEntityInspectFactProjection> Candidates;
    Candidates.Reserve(FMath::Min(FactTags.Num(), SectionCap));
    for (const FGameplayTag FactTag : FactTags) {
        UMythicEntityKnowledgeFactDefinition *Definition =
            ResolveKnowledgeFactDefinition(FactTag);
        if (!Definition || Definition->FactTag != FactTag
            || Definition->Section != ExpectedSection
            || Definition->DisplayName.IsEmpty()) {
            continue;
        }

        FMythicEntityInspectFactProjection &Candidate =
            Candidates.AddDefaulted_GetRef();
        Candidate.FactTag = FactTag;
        Candidate.ResolvedLabel = Definition->DisplayName;
        Candidate.ResolvedDescription = Definition->Description;
        Candidate.Icon = Definition->Icon.Get();
        Candidate.PresentationPriority = Definition->PresentationPriority;
    }
    Candidates.Sort([](const FMythicEntityInspectFactProjection &Left,
                       const FMythicEntityInspectFactProjection &Right) {
        if (Left.PresentationPriority != Right.PresentationPriority) {
            return Left.PresentationPriority > Right.PresentationPriority;
        }
        return Left.FactTag.GetTagName().LexicalLess(
            Right.FactTag.GetTagName());
    });
    if (Candidates.Num() > SectionCap) {
        Candidates.SetNum(SectionCap, EAllowShrinking::No);
    }
    OutFacts = MoveTemp(Candidates);
}

FText UMythicNameplateDirector::ResolveRelationshipBand(
    const EMythicKnownRelationshipBand Band) {
    switch (Band) {
    case EMythicKnownRelationshipBand::Hostile:
        return LOCTEXT("RelationshipHostile", "Hostile");
    case EMythicKnownRelationshipBand::Wary:
        return LOCTEXT("RelationshipWary", "Wary");
    case EMythicKnownRelationshipBand::Neutral:
        return LOCTEXT("RelationshipNeutral", "Neutral");
    case EMythicKnownRelationshipBand::Familiar:
        return LOCTEXT("RelationshipFamiliar", "Familiar");
    case EMythicKnownRelationshipBand::Friendly:
        return LOCTEXT("RelationshipFriendly", "Friendly");
    case EMythicKnownRelationshipBand::Trusted:
        return LOCTEXT("RelationshipTrusted", "Trusted");
    case EMythicKnownRelationshipBand::Unknown:
    default:
        return FText::GetEmpty();
    }
}

FText UMythicNameplateDirector::ResolveStandingBand(
    const EMythicKnownStandingBand Band) {
    switch (Band) {
    case EMythicKnownStandingBand::Hostile:
        return LOCTEXT("StandingHostile", "Hostile");
    case EMythicKnownStandingBand::Unfriendly:
        return LOCTEXT("StandingUnfriendly", "Unfriendly");
    case EMythicKnownStandingBand::Neutral:
        return LOCTEXT("StandingNeutral", "Neutral");
    case EMythicKnownStandingBand::Friendly:
        return LOCTEXT("StandingFriendly", "Friendly");
    case EMythicKnownStandingBand::Honored:
        return LOCTEXT("StandingHonored", "Honored");
    case EMythicKnownStandingBand::Unknown:
    default:
        return FText::GetEmpty();
    }
}

FText UMythicNameplateDirector::ResolveIdentity(
    const FMythicPublicIdentitySnapshot &Identity,
    const FMythicEntityKnowledgeView *Knowledge,
    const UMythicEntityPresentationComponent *Component) {
    using namespace MythicEntityPresentationTags;
    UMythicEntityIdentityDefinition *Definition =
        ResolveIdentityDefinition(Identity.PublicIdentityDefinitionId);
    const FGameplayTag PublicKind = Definition
        && Definition->PublicKindTag.IsValid()
        ? Definition->PublicKindTag : Identity.PublicKindTag;
    if (PublicKind == EntityKindPlayer && Component) {
        const APawn *PlayerPawn = Cast<APawn>(Component->GetOwner());
        const APlayerState *PlayerState = PlayerPawn
            ? PlayerPawn->GetPlayerState() : nullptr;
        if (PlayerState && !PlayerState->GetPlayerName().IsEmpty()) {
            return FText::FromString(PlayerState->GetPlayerName());
        }
    }
    if (Knowledge && Knowledge->bNameKnown
        && !Knowledge->RecognizedName.IsEmpty()) {
        return Knowledge->RecognizedName;
    }

    if (Definition) {
        if (Definition->bNameVisibleOnSight
            && !Definition->PublicDisplayName.IsEmpty()) {
            return Definition->PublicDisplayName;
        }
    }

    if (PublicKind == EntityKindCreature) {
        return LOCTEXT("UnknownCreature", "Creature");
    }
    if (PublicKind == EntityKindPlayer) {
        return LOCTEXT("UnknownPlayer", "Adventurer");
    }
    if (PublicKind == EntityKindHumanoid) {
        return LOCTEXT("UnknownHumanoid", "Stranger");
    }
    if (PublicKind == EntityKindConstruct) {
        return LOCTEXT("UnknownConstruct", "Construct");
    }
    if (PublicKind == EntityKindWorldObject) {
        return LOCTEXT("UnknownWorldObject", "Object");
    }
    return LOCTEXT("UnknownEntity", "Unknown");
}

FText UMythicNameplateDirector::ResolveArchetypeLabel(
    const FGameplayTag ArchetypeTag) const {
    if (!ArchetypeTag.IsValid()) {
        return FText::GetEmpty();
    }
    const FText *Found = ArchetypeLabels.Find(ArchetypeTag);
    return Found ? *Found : FText::GetEmpty();
}

FText UMythicNameplateDirector::ResolveFactionLabel(
    const FGameplayTag FactionTag) const {
    if (!FactionTag.IsValid()) {
        return FText::GetEmpty();
    }
    UWorld *World = GetTickableGameObjectWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LivingWorld = GameInstance
        ? GameInstance->GetSubsystem<UMythicLivingWorldSubsystem>()
        : nullptr;
    UMythicFactionDatabase *FactionDatabase = LivingWorld
        ? LivingWorld->GetFactionDatabase()
        : nullptr;
    FMythicFactionData Faction;
    return FactionDatabase
        && FactionDatabase->FindFactionByTag(FactionTag, Faction)
        ? Faction.DisplayName
        : FText::GetEmpty();
}

UMythicEntityIdentityDefinition *
UMythicNameplateDirector::ResolveIdentityDefinition(
    const FPrimaryAssetId &AssetId) {
    if (!AssetId.IsValid()) {
        return nullptr;
    }
    if (TObjectPtr<UMythicEntityIdentityDefinition> *Found =
            IdentityDefinitions.Find(AssetId)) {
        return *Found;
    }
    if (UMythicEntityIdentityDefinition *Loaded =
            Cast<UMythicEntityIdentityDefinition>(
                UAssetManager::Get().GetPrimaryAssetObject(AssetId))) {
        IdentityDefinitions.Add(AssetId, Loaded);
        return Loaded;
    }
    RequestPrimaryAsset(AssetId, true);
    return nullptr;
}

bool UMythicNameplateDirector::ResolveInspectionSupport(
    const FMythicPublicIdentitySnapshot &Identity) {
    if (!Identity.PublicIdentityDefinitionId.IsValid()) {
        return Identity.PublicKindTag.IsValid();
    }
    const UMythicEntityIdentityDefinition *Definition =
        ResolveIdentityDefinition(Identity.PublicIdentityDefinitionId);
    return Definition && Definition->bSupportsInspection;
}

UMythicContextActionDefinition *
UMythicNameplateDirector::ResolveActionDefinition(
    const FGameplayTag ActionTag) {
    if (!ActionTag.IsValid()) {
        return nullptr;
    }
    if (TObjectPtr<UMythicContextActionDefinition> *Found =
            ActionDefinitions.Find(ActionTag)) {
        return *Found;
    }
    const FPrimaryAssetId AssetId(
        UMythicContextActionDefinition::PrimaryAssetType,
        ActionTag.GetTagName());
    if (UMythicContextActionDefinition *Loaded =
            Cast<UMythicContextActionDefinition>(
                UAssetManager::Get().GetPrimaryAssetObject(AssetId))) {
        if (Loaded->ActionTag == ActionTag) {
            ActionDefinitions.Add(ActionTag, Loaded);
            return Loaded;
        }
        return nullptr;
    }
    RequestPrimaryAsset(AssetId, false);
    return nullptr;
}

UMythicEntityKnowledgeFactDefinition *
UMythicNameplateDirector::ResolveKnowledgeFactDefinition(
    const FGameplayTag FactTag) {
    if (!FactTag.IsValid()) {
        return nullptr;
    }
    if (TObjectPtr<UMythicEntityKnowledgeFactDefinition> *Found =
            KnowledgeFactDefinitions.Find(FactTag)) {
        return *Found;
    }

    const FPrimaryAssetId AssetId(
        UMythicEntityKnowledgeFactDefinition::PrimaryAssetType,
        FactTag.GetTagName());
    if (UMythicEntityKnowledgeFactDefinition *Loaded =
            Cast<UMythicEntityKnowledgeFactDefinition>(
                UAssetManager::Get().GetPrimaryAssetObject(AssetId))) {
        if (Loaded->FactTag == FactTag) {
            KnowledgeFactDefinitions.Add(FactTag, Loaded);
            return Loaded;
        }
        return nullptr;
    }
    RequestKnowledgeFactDefinition(FactTag);
    return nullptr;
}

void UMythicNameplateDirector::RequestKnowledgeFactDefinition(
    const FGameplayTag FactTag) {
    if (!FactTag.IsValid()) {
        return;
    }
    const FPrimaryAssetId AssetId(
        UMythicEntityKnowledgeFactDefinition::PrimaryAssetType,
        FactTag.GetTagName());
    if (!AssetId.IsValid()
        || RequestedKnowledgeFactAssets.Contains(AssetId)) {
        return;
    }

    const FSoftObjectPath AssetPath =
        UAssetManager::Get().GetPrimaryAssetPath(AssetId);
    RequestedKnowledgeFactAssets.Add(AssetId);
    if (!AssetPath.IsValid()) {
        return;
    }

    TSharedPtr<FStreamableHandle> Handle =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            AssetPath,
            FStreamableDelegate::CreateUObject(
                this, &ThisClass::HandleKnowledgeFactDefinitionLoaded,
                FactTag, AssetId));
    if (Handle.IsValid()) {
        PendingKnowledgeFactLoads.Add(AssetId, Handle);
    }
}

void UMythicNameplateDirector::HandleKnowledgeFactDefinitionLoaded(
    const FGameplayTag FactTag, const FPrimaryAssetId AssetId) {
    PendingKnowledgeFactLoads.Remove(AssetId);
    UObject *Object = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
    if (!Object) {
        Object = UAssetManager::Get().GetPrimaryAssetPath(AssetId)
            .ResolveObject();
    }
    if (UMythicEntityKnowledgeFactDefinition *Definition =
            Cast<UMythicEntityKnowledgeFactDefinition>(Object);
        Definition && Definition->FactTag == FactTag) {
        KnowledgeFactDefinitions.Add(FactTag, Definition);
    }
    if (InspectedInstance.IsValid()) {
        RebuildInspectProjection();
    }
}

void UMythicNameplateDirector::RequestPrimaryAsset(
    const FPrimaryAssetId &AssetId, const bool bIdentityDefinition) {
    if (!AssetId.IsValid() || RequestedPrimaryAssets.Contains(AssetId)) {
        return;
    }
    const FSoftObjectPath AssetPath =
        UAssetManager::Get().GetPrimaryAssetPath(AssetId);
    if (!AssetPath.IsValid()) {
        RequestedPrimaryAssets.Add(AssetId);
        return;
    }

    RequestedPrimaryAssets.Add(AssetId);
    TSharedPtr<FStreamableHandle> Handle =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            AssetPath,
            FStreamableDelegate::CreateUObject(
                this, &ThisClass::HandlePrimaryAssetLoaded,
                AssetId, bIdentityDefinition));
    if (Handle.IsValid()) {
        PendingPrimaryAssetLoads.Add(AssetId, Handle);
        if (Handle->HasLoadCompleted()) {
            HandlePrimaryAssetLoaded(AssetId, bIdentityDefinition);
        }
    }
}

void UMythicNameplateDirector::HandlePrimaryAssetLoaded(
    const FPrimaryAssetId AssetId, const bool bIdentityDefinition) {
    PendingPrimaryAssetLoads.Remove(AssetId);
    UObject *Object = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
    if (!Object) {
        Object = UAssetManager::Get().GetPrimaryAssetPath(AssetId)
            .ResolveObject();
    }
    if (bIdentityDefinition) {
        if (UMythicEntityIdentityDefinition *Definition =
                Cast<UMythicEntityIdentityDefinition>(Object)) {
            IdentityDefinitions.Add(AssetId, Definition);
        }
    } else if (UMythicContextActionDefinition *Definition =
                   Cast<UMythicContextActionDefinition>(Object)) {
        if (Definition->ActionTag.IsValid()) {
            ActionDefinitions.Add(Definition->ActionTag, Definition);
        }
    }
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::EnsureSemanticDatabasesRequested() {
    if (bSemanticDatabasesRequested) {
        return;
    }
    UWorld *World = GetTickableGameObjectWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LivingWorld = GameInstance
        ? GameInstance->GetSubsystem<UMythicLivingWorldSubsystem>()
        : nullptr;
    const UMythicLivingWorldSettings *Settings = LivingWorld
        ? LivingWorld->GetSettings()
        : nullptr;
    if (!Settings) {
        return;
    }

    bSemanticDatabasesRequested = true;
    TArray<FSoftObjectPath> Paths;
    if (!Settings->RoleDatabase.IsNull()) {
        Paths.Add(Settings->RoleDatabase.ToSoftObjectPath());
    }
    if (!Settings->CreatureSpeciesTable.IsNull()) {
        Paths.Add(Settings->CreatureSpeciesTable.ToSoftObjectPath());
    }
    if (Paths.IsEmpty()) {
        HandleSemanticDatabasesLoaded();
        return;
    }
    SemanticDatabaseLoad =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            Paths, FStreamableDelegate::CreateUObject(
                       this, &ThisClass::HandleSemanticDatabasesLoaded));
}

void UMythicNameplateDirector::HandleSemanticDatabasesLoaded() {
    ArchetypeLabels.Reset();
    for (const FMythicCreatureSpeciesRow &Species :
         MythicCreatureDefaults::GetCodeDefaultSpecies()) {
        if (Species.SpeciesTag.IsValid() && !Species.DisplayName.IsEmpty()) {
            ArchetypeLabels.Add(Species.SpeciesTag, Species.DisplayName);
        }
    }

    UWorld *World = GetTickableGameObjectWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LivingWorld = GameInstance
        ? GameInstance->GetSubsystem<UMythicLivingWorldSubsystem>()
        : nullptr;
    const UMythicLivingWorldSettings *Settings = LivingWorld
        ? LivingWorld->GetSettings()
        : nullptr;
    RoleDatabase = Settings ? Settings->RoleDatabase.Get() : nullptr;
    CreatureSpeciesTable = Settings
        ? Settings->CreatureSpeciesTable.Get()
        : nullptr;

    if (RoleDatabase) {
        for (const FMythicRoleDefinition &Role : RoleDatabase->Roles) {
            if (Role.RoleTag.IsValid() && !Role.DisplayName.IsEmpty()) {
                ArchetypeLabels.Add(Role.RoleTag, Role.DisplayName);
            }
        }
    }
    if (CreatureSpeciesTable
        && CreatureSpeciesTable->GetRowStruct()
               == FMythicCreatureSpeciesRow::StaticStruct()) {
        TArray<FMythicCreatureSpeciesRow *> Rows;
        CreatureSpeciesTable->GetAllRows<FMythicCreatureSpeciesRow>(
            TEXT("MythicNameplateDirector"), Rows);
        for (const FMythicCreatureSpeciesRow *Species : Rows) {
            if (Species && Species->SpeciesTag.IsValid()
                && !Species->DisplayName.IsEmpty()) {
                ArchetypeLabels.Add(Species->SpeciesTag,
                                    Species->DisplayName);
            }
        }
    }
    SemanticDatabaseLoad.Reset();
    RequestProjectionRefresh();
}

void UMythicNameplateDirector::GatherActions(
    const FMythicEntityPresentationInstance &Subject,
    TArray<FActionCandidate> &OutActions,
    bool &OutPromotesContext, bool &OutCanAssist) {
    OutActions.Reset();
    OutPromotesContext = false;
    OutCanAssist = false;
    UMythicEntityActionGrantComponent *Grants = ActionGrants.Get();
    if (!Grants || !Subject.IsValid()) {
        return;
    }

    TArray<FMythicReplicatedContextActionGrant> CurrentGrants;
    CurrentGrants.Reserve(8);
    Grants->GatherCurrentActionGrantsForSubject(Subject, CurrentGrants);
    for (const FMythicReplicatedContextActionGrant &Grant : CurrentGrants) {
        UMythicContextActionDefinition *Definition =
            ResolveActionDefinition(Grant.ActionTag);
        if (!Definition || Definition->ActionTag != Grant.ActionTag
            || Definition->DisplayName.IsEmpty()) {
            continue;
        }

        FActionCandidate &Candidate = OutActions.AddDefaulted_GetRef();
        Candidate.Definition = Definition;
        Candidate.Projection.ActionTag = Grant.ActionTag;
        Candidate.Projection.ResolvedLabel = Definition->DisplayName;
        Candidate.Projection.Icon = Definition->Icon.Get();
        Candidate.Projection.InputActionTag =
            Definition->CommonUIInputActionTag;
        Candidate.bAvailable =
            Grant.State == EMythicContextActionGrantState::Available;
        Candidate.Projection.OfferRevision = Grant.OfferRevision;
        Candidate.Projection.HoldDurationSeconds =
            Definition->HoldDurationSeconds;

        if (Candidate.bAvailable
            && Definition->WorldPresentationPolicy
                   == EMythicContextActionWorldPresentationPolicy::ContextWhenAvailable) {
            OutPromotesContext = true;
        }
        if (Candidate.bAvailable
            && Definition->PresentationSemantic
                   == EMythicContextActionPresentationSemantic::Assist) {
            OutCanAssist = true;
        }
    }

    OutActions.Sort([](const FActionCandidate &Left,
                       const FActionCandidate &Right) {
        if (Left.bAvailable != Right.bAvailable) {
            return Left.bAvailable;
        }
        const int32 LeftPriority = Left.Definition
            ? Left.Definition->PresentationPriority : 0;
        const int32 RightPriority = Right.Definition
            ? Right.Definition->PresentationPriority : 0;
        if (LeftPriority != RightPriority) {
            return LeftPriority > RightPriority;
        }
        return Left.Projection.ActionTag.GetTagName().LexicalLess(
            Right.Projection.ActionTag.GetTagName());
    });
}

void UMythicNameplateDirector::GatherCueCandidates(
    const FMythicEntityAttentionObservation &Observation,
    UMythicEntityPresentationComponent &Component,
    const TConstArrayView<FActionCandidate> Actions,
    TArray<EMythicNameplatePrimaryCue> &OutCues,
    FText &OutCueText, bool &OutDownedOrDying,
    bool &OutCombatRelevant) const {
    OutCues.Reset();
    OutCueText = FText::GetEmpty();
    OutDownedOrDying = false;
    OutCombatRelevant = Observation.bRecentCombatSignal
        || Observation.bHardTarget;
    EMythicNameplatePrimaryCue TextCue =
        EMythicNameplatePrimaryCue::None;

    auto AddCue = [&](const EMythicNameplatePrimaryCue Cue,
                      const FText &Text) {
        OutCues.Add(Cue);
        if (!Text.IsEmpty()
            && FMythicNameplateRules::GetCuePrecedence(Cue)
                   > FMythicNameplateRules::GetCuePrecedence(TextCue)) {
            TextCue = Cue;
            OutCueText = Text;
        }
    };

    const FMythicEntityPresentationInstance Instance =
        Component.GetPresentationInstance();
    for (const FMythicObservableFactItem &Fact :
         Component.GetObservableFactsView()) {
        if (!IsCurrentFact(Fact, Instance)) {
            continue;
        }
        using namespace MythicEntityPresentationTags;
        if (Fact.FactSlotTag == ObservableSlotLifeState) {
            if (Fact.ValueTag == ObservableLifeDead) {
                AddCue(EMythicNameplatePrimaryCue::Dead,
                       LOCTEXT("DeadCue", "Dead"));
            } else if (Fact.ValueTag == ObservableLifeDying) {
                OutDownedOrDying = true;
                AddCue(EMythicNameplatePrimaryCue::Dying,
                       LOCTEXT("DyingCue", "Dying"));
            } else if (Fact.ValueTag == ObservableLifeDowned) {
                OutDownedOrDying = true;
                AddCue(EMythicNameplatePrimaryCue::Downed,
                       LOCTEXT("DownedCue", "Downed"));
            }
        } else if (Fact.FactSlotTag == ObservableSlotBehavior) {
            if (Fact.ValueTag == ObservableBehaviorSurrendering) {
                AddCue(EMythicNameplatePrimaryCue::Surrendering,
                       LOCTEXT("SurrenderingCue", "Surrendering"));
            } else if (Fact.ValueTag == ObservableBehaviorFleeing) {
                AddCue(EMythicNameplatePrimaryCue::Fleeing,
                       LOCTEXT("FleeingCue", "Fleeing"));
            } else if (Fact.ValueTag == ObservableBehaviorFighting) {
                OutCombatRelevant = true;
                const EMythicNameplatePrimaryCue FightingCue =
                    FMythicNameplateRules::ResolveObservedFightingCue(
                        Observation.bRecentCombatSignal,
                        Observation.bRecentIncomingCombatSignal);
                AddCue(FightingCue,
                       FightingCue
                               == EMythicNameplatePrimaryCue::AttackingViewer
                           ? LOCTEXT("AttackingCue", "Attacking")
                           : LOCTEXT("FightingCue", "Fighting"));
            }
        } else if (Fact.FactSlotTag == ObservableSlotActivity) {
            AddCue(EMythicNameplatePrimaryCue::ObservableActivity,
                   FText::GetEmpty());
        }
    }

    const bool bDeliberateAttention = Observation.bFocused
        || Observation.bInteractionTarget || Observation.bHardTarget
        || Observation.bInspectTarget;
    for (const FActionCandidate &Action : Actions) {
        if (!Action.Definition || !Action.bAvailable) {
            continue;
        }
        const bool bMayHeadlineContext =
            Action.Definition->WorldPresentationPolicy
                   == EMythicContextActionWorldPresentationPolicy::ContextWhenAvailable;
        if (!bDeliberateAttention && !bMayHeadlineContext) {
            continue;
        }
        AddCue(CueFromActionSemantic(*Action.Definition),
               Action.Projection.ResolvedLabel);
    }
}

void UMythicNameplateDirector::GatherStatuses(
    UMythicEntityPresentationComponent &Component,
    const EMythicNameplateDisclosureTier Tier,
    TArray<FMythicNameplateStatusCandidate> &OutStatuses,
    int32 &OutOverflowCount, bool &OutSafetyCritical) const {
    OutStatuses.Reset();
    OutOverflowCount = 0;
    OutSafetyCritical = false;
    UWorld *World = GetTickableGameObjectWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UMythicStatusRegistry *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicStatusRegistry>()
        : nullptr;
    if (!Registry) {
        return;
    }

    const FMythicEntityPresentationInstance Instance =
        Component.GetPresentationInstance();
    TArray<FMythicNameplateStatusCandidate> Candidates;
    Candidates.Reserve(Component.GetPublicStatusesView().Num());
    int32 StableOrder = 0;
    for (const FMythicPublicStatusPresentationItem &Status :
         Component.GetPublicStatusesView()) {
        if (!IsCurrentStatus(Status, Instance)) {
            continue;
        }
        const UMythicStatusEffectDefinition *Definition =
            Registry->FindStatus(Status.StatusType);
        if (!Definition
            || Definition->WorldVisibility
                   == EMythicStatusWorldVisibility::Hidden) {
            continue;
        }
        const bool bTierPermits =
            Definition->WorldVisibility
                    != EMythicStatusWorldVisibility::FocusOnly
            || DirectorTierOrdinal(Tier)
                   >= DirectorTierOrdinal(
                       EMythicNameplateDisclosureTier::Focus);
        if (Definition->WorldVisibility
                == EMythicStatusWorldVisibility::SafetyCritical
            && Definition->bPromotesContextWhenObserved) {
            OutSafetyCritical = true;
        }

        FMythicNameplateStatusCandidate &Candidate =
            Candidates.AddDefaulted_GetRef();
        Candidate.StatusType = Status.StatusType;
        Candidate.ResolvedLabel = Definition->DisplayName;
        Candidate.Icon = Definition->Icon.Get();
        Candidate.DisplayColor = Definition->DisplayColor;
        Candidate.Urgency = ToNameplateUrgency(
            Definition->PresentationCategory);
        Candidate.bPresentationPermitted = bTierPermits;
        Candidate.bAppliedByViewerOrParty = false;
        Candidate.AuthoredPriority = FMath::Clamp(
            Definition->WorldPresentationPriority, -1000, 1000);
        Candidate.StableTieBreak = StableOrder++;
        Candidate.StackCount = Definition->bShowStackCount
            ? static_cast<int32>(Status.StackCount)
            : 0;
        Candidate.ServerEndTimeSeconds =
            Definition->bShowRemainingDuration
            ? Status.ServerEndTimeSeconds : 0.0;
    }

    const FMythicNameplateStatusPolicy StatusPolicy = Policy
        ? Policy->Statuses : FMythicNameplateStatusPolicy();
    FMythicNameplateRules::SelectStatusCandidates(
        Candidates,
        FMythicNameplateRules::GetStatusIconCap(Tier, StatusPolicy),
        OutStatuses, OutOverflowCount);
}

void UMythicNameplateDirector::ResolveHealth(
    UMythicEntityPresentationComponent &Component,
    const EMythicNameplateDisclosureTier Tier,
    const FMythicEntityAttentionObservation &Observation,
    const bool bDownedOrDying, const bool bCombatRelevant,
    const bool bCanAssist,
    const EMythicPresentedCombatRank PresentedRank,
    FMythicNameplateProjection &InOutProjection) const {
    const FMythicPublicVitalitySnapshot &Vitality =
        Component.GetPublicVitalitySnapshot();
    const FMythicEntityPresentationInstance Instance =
        Component.GetPresentationInstance();
    if (!Vitality.IsCurrentFor(Instance)) {
        return;
    }
    const float Fraction =
        UMythicEntityPresentationComponent::DequantizePublicHealthFraction(
            Vitality.HealthFractionQuantized);

    FMythicNameplateHealthContext Health;
    Health.bHealthPresentationPermitted = true;
    Health.bCurrentCombatTarget = Observation.bHardTarget;
    Health.bBoss = IsBossRank(PresentedRank);
    Health.bDownedOrDying = bDownedOrDying;
    Health.bCombatRelevant = bCombatRelevant
        || Observation.bSafetySignal;
    Health.bInjured = Fraction < 0.999f;
    Health.bPartyOrCompanion = false;
    Health.bCanAssist = bCanAssist;

    InOutProjection.bShowHealth =
        FMythicNameplateRules::ShouldShowHealth(Tier, Health);
    InOutProjection.HealthFraction = InOutProjection.bShowHealth
        ? Fraction : 0.0f;
}

EMythicNameplatePrimaryCue
UMythicNameplateDirector::CueFromActionSemantic(
    const UMythicContextActionDefinition &Definition) {
    switch (Definition.PresentationSemantic) {
    case EMythicContextActionPresentationSemantic::DirectedTalk:
        return EMythicNameplatePrimaryCue::DirectedTalk;
    case EMythicContextActionPresentationSemantic::QuestOffer:
        return EMythicNameplatePrimaryCue::QuestOffer;
    case EMythicContextActionPresentationSemantic::QuestTurnIn:
        return EMythicNameplatePrimaryCue::QuestTurnIn;
    case EMythicContextActionPresentationSemantic::Service:
        return EMythicNameplatePrimaryCue::Service;
    case EMythicContextActionPresentationSemantic::Talk:
    case EMythicContextActionPresentationSemantic::Assist:
    case EMythicContextActionPresentationSemantic::Other:
    default:
        return EMythicNameplatePrimaryCue::OtherAction;
    }
}

#undef LOCTEXT_NAMESPACE
