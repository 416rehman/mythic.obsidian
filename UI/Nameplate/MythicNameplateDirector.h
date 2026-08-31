#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "UI/Nameplate/MythicEntityInspectTypes.h"
#include "UI/Nameplate/MythicNameplateTypes.h"

#include "MythicNameplateDirector.generated.h"

class AMythicPlayerState;
class UMythicContextActionDefinition;
class UMythicEntityActionGrantComponent;
class UMythicEntityAttentionSubsystem;
class UMythicEntityIdentityDefinition;
class UMythicEntityInspectPage;
class UMythicEntityKnowledgeFactDefinition;
class UMythicEntityPresentationComponent;
class UMythicEntityViewerKnowledgeComponent;
class UMythicNameplateLayer;
class UMythicNameplatePolicy;
class UMythicRoleDatabase;
class UMythicUserSettings;
class UDataTable;
class UMythicEntityCombatPresentationComponent;
struct FStreamableHandle;
enum class EMythicEntityKnowledgeFactSection : uint8;
enum class EMythicKnownRelationshipBand : uint8;
enum class EMythicKnownStandingBand : uint8;
enum class EMythicNameplatePresentationMode : uint8;

/** Blueprint invalidation edge fired after the bounded viewer-safe projection set changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicNameplateProjectionsChanged,
                                            int32, LocalRevision);

/** Blueprint invalidation edge fired after the current learned Inspect projection changes or closes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicEntityInspectProjectionChanged,
                                            int32, LocalRevision);

/**
 * Viewer-relative projection coordinator for contextual entity presentation.
 *
 * One instance exists per LocalPlayer. It combines only public observation with owner-only grants and learned
 * knowledge, applies density/tier policy at attention cadence, and drives a prewarmed HUD layer. Widgets receive
 * immutable sanitized DTOs and never query actors, GAS, quests, factions, dialogue, or LivingWorld simulation.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicNameplateDirector : public ULocalPlayerSubsystem,
                                           public FTickableGameObject {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual UWorld *GetTickableGameObjectWorld() const override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual bool IsTickableInEditor() const override { return false; }

    /**
     * Replaces the canonical local policy and configures the shared attention service from the same authored source.
     * Passing null restores allocation-safe native defaults until a policy asset becomes available.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Nameplate")
    void ConfigureNameplates(UMythicNameplatePolicy *InPolicy);

    /** Returns the policy currently driving density and disclosure, or null while native defaults are in use. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    UMythicNameplatePolicy *GetNameplatePolicy() const { return Policy; }

    /** Copies the current bounded, viewer-safe projection set for diagnostics or infrequent Blueprint consumers. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    TArray<FMythicNameplateProjection> GetActiveProjections() const;

    /** Copies the single current overhead Focus projection; false resets OutProjection when none is active. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    bool GetFocusedProjection(FMythicNameplateProjection &OutProjection) const;

    /** Copies the available-only action rail for the current exact Focus subject; false resets OutProjection. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate|Context Actions")
    bool GetFocusedActionRailProjection(
        FMythicNameplateActionRailProjection &OutProjection) const;

    /**
     * Executes one available action from the current exact Focus projection. The local DTO supplies only the opaque
     * instance, action tag, and offer revision; authority resolves and revalidates every domain and spatial rule.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Nameplate|Context Actions")
    bool ExecuteFocusedContextAction(FGameplayTag ActionTag);

    /** Starts the local and authority hold handshake for the exact current Focus action; tap actions return false. */
    bool BeginFocusedContextActionHold(FGameplayTag ActionTag);

    /** Cancels the matching local and authority hold handshake; unrelated action holds remain untouched. */
    void CancelFocusedContextActionHold(FGameplayTag ActionTag);

    /** Returns the monotonic local invalidation counter in 1..2147483647. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    int32 GetLocalProjectionRevision() const {
        return static_cast<int32>(ProjectionRevision);
    }

    /** Requests a bounded rebuild on the next local tick; it does not force a world scan or synchronous asset load. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Nameplate")
    void RequestProjectionRefresh();

    /**
     * Opens learned-knowledge Inspect for the one stable focused entity. Bind this to a CommonUI hold action so a
     * quick target acquisition never steals controls or opens a screen; false means focus/knowledge/page was absent.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Inspect")
    bool OpenFocusedEntityInspect();

    /** Closes the local Inspect page and releases its attention override without changing learned knowledge. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Inspect")
    void CloseEntityInspect();

    /** Copies the current viewer-safe learned dossier; false resets OutProjection when Inspect is closed. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Inspect")
    bool GetCurrentInspectProjection(
        FMythicEntityInspectProjection &OutProjection) const;

    /** Returns true only while a valid exact embodiment owns the local Inspect surface. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Inspect")
    bool IsEntityInspectOpen() const { return InspectedInstance.IsValid(); }

    /** Fired after a coherent projection-set commit; all Blueprint getters are safe to query from the callback. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Nameplate")
    FMythicNameplateProjectionsChanged OnNameplateProjectionsChanged;

    /** Fired after a coherent learned-dossier commit or close; query GetCurrentInspectProjection in the callback. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Entity Inspect")
    FMythicEntityInspectProjectionChanged OnEntityInspectProjectionChanged;

    /** Attaches the HUD-owned fixed pool for this LocalPlayer; a previous layer is detached without destroying it. */
    void AttachPresentationLayer(UMythicNameplateLayer *InLayer,
                                 UMythicNameplatePolicy *InPolicy);

    /** Detaches only when InLayer is the current HUD owner, preventing stale teardown from clearing a replacement. */
    void DetachPresentationLayer(UMythicNameplateLayer *InLayer);

private:
    struct FPendingLocalContextActionHold {
        FMythicEntityPresentationInstance Subject;
        FGameplayTag ActionTag;
        uint32 OfferRevision = 0;

        bool IsActive() const {
            return Subject.IsValid() && ActionTag.IsValid();
        }

        bool Matches(const FMythicEntityPresentationInstance &InSubject,
                     FGameplayTag InActionTag,
                     uint32 InOfferRevision) const {
            return IsActive() && Subject == InSubject
                && ActionTag == InActionTag
                && OfferRevision == InOfferRevision;
        }

        void Reset() { *this = FPendingLocalContextActionHold(); }
    };

    struct FProjectionEntry {
        FMythicNameplateProjection Projection;
        FMythicNameplateActionRailProjection ActionRailProjection;
        TWeakObjectPtr<UMythicEntityPresentationComponent> Component;
        FVector2D LastScreenPosition = FVector2D::ZeroVector;
        FVector2D LastResolvedScreenPosition = FVector2D::ZeroVector;
        float Score = 0.0f;
        /** Latest camera-to-anchor distance in centimetres, owned only by this LocalPlayer. */
        float LastDistanceCentimeters = 0.0f;
        /** Distance alpha retained when a release begins so High Contrast can fade as one coherent unit. */
        float DistanceAlpha = 1.0f;
        /** Local passive distance scale before the accessibility multiplier. */
        float PresentationScale = 1.0f;
        /** Temporal alpha at the latest placement update, retained across decision-pass state changes. */
        float TemporalAlpha = 0.0f;
        /** Temporal alpha captured when the current acquire or release transition began. */
        float TransitionStartAlpha = 0.0f;
        /** Monotonic local time at which the current acquire or release transition began. */
        double TransitionStartSeconds = 0.0;
        /** Last decision time at which current disclosure evidence was freshly rebuilt for this exact embodiment. */
        double LastEligibleSeconds = 0.0;
        bool bPureAmbientWhisper = false;
        /** True only for ordinary neutral Whisper/Focus surfaces governed by local passive distance policy. */
        bool bPassiveNeutral = false;
        /** True while the retained outgoing projection completes its local whole-surface release transition. */
        bool bReleasing = false;
        /**
         * True after the exact presentation component deactivates; placement is frozen and the retained projection
         * may only fade out without consulting an actor, registry, or replacement generation.
         */
        bool bFrozenRetiring = false;
        /** Build diagnostic used to route a passive boundary rejection directly into release instead of semantic grace. */
        bool bRejectedByPassiveRange = false;
        /** True for a corpse's new viewer-specific action surface; it contains no retained live identity or combat data. */
        bool bActionContextOnly = false;
        /** True when the current public facts report authoritative death, even if no projection may be built. */
        bool bObservedDead = false;
        /** True only for entries rebuilt from the current attention observation set, never for a release-grace lease. */
        bool bFreshEvidence = false;
        bool bCollisionSuppressed = false;
    };

    struct FActionCandidate {
        FMythicNameplateActionProjection Projection;
        TObjectPtr<UMythicContextActionDefinition> Definition = nullptr;
        bool bAvailable = false;
    };

    void EnsureRuntimeBindings();
    void BindUserSettings(UMythicUserSettings *InSettings);
    void UnbindUserSettings();
    void HandleInterfaceSettingsChanged();
    void BindAttention(UMythicEntityAttentionSubsystem *InAttention);
    void UnbindAttention();
    void BindPlayerState(AMythicPlayerState *InPlayerState);
    void UnbindPlayerState();
    void HandleAttentionUpdated(uint64 AttentionRevision);
    void HandleKnowledgeUpdated(uint32 KnowledgeRevision);
    void HandleActionGrantsUpdated(uint32 GrantRevision);
    void HandleCombatPresentationUpdated(uint32 CombatRevision);
    void HandleInspectPageDeactivated();

    void RebuildProjectionSet();
    bool BuildProjectionEntry(const struct FMythicEntityAttentionObservation &Observation,
                              FProjectionEntry &OutEntry);
    void AppendReleaseGraceLeases(
        TArray<FProjectionEntry> &Candidates,
        const TSet<FMythicEntityPresentationInstance> &ImmediateCollapseInstances,
        const TSet<FMythicEntityPresentationInstance> &ImmediateFadeInstances,
        double NowSeconds) const;
    void BeginReleaseTransition(FProjectionEntry &Entry,
                                double NowSeconds) const;
    void StabilizeAmbientWhisperIncumbent(
        TArray<FProjectionEntry> &Candidates,
        int32 AmbientWhisperCapacity,
        double NowSeconds);
    bool IsLeaseSubjectStillValid(const FProjectionEntry &Entry) const;
    void CommitBoundedEntries(TArray<FProjectionEntry> &Candidates,
                              double NowSeconds);
    void UpdateActivePlacements();
    void PublishProjectionRevision();
    void RebuildInspectProjection();
    bool BuildInspectProjection(FMythicEntityInspectProjection &OutProjection);
    void CloseEntityInspectInternal(bool bDeactivatePage);
    void PublishInspectRevision();

    FText ResolveIdentity(const struct FMythicPublicIdentitySnapshot &Identity,
                          const struct FMythicEntityKnowledgeView *Knowledge,
                          const UMythicEntityPresentationComponent *Component);
    FText ResolveArchetypeLabel(FGameplayTag ArchetypeTag) const;
    FText ResolveFactionLabel(FGameplayTag FactionTag) const;
    UMythicEntityIdentityDefinition *ResolveIdentityDefinition(
        const FPrimaryAssetId &AssetId);
    bool ResolveInspectionSupport(
        const FMythicPublicIdentitySnapshot &Identity);
    UMythicContextActionDefinition *ResolveActionDefinition(FGameplayTag ActionTag);
    void RequestPrimaryAsset(const FPrimaryAssetId &AssetId, bool bIdentityDefinition);
    void HandlePrimaryAssetLoaded(FPrimaryAssetId AssetId, bool bIdentityDefinition);
    void EnsureSemanticDatabasesRequested();
    void HandleSemanticDatabasesLoaded();
    UMythicEntityKnowledgeFactDefinition *ResolveKnowledgeFactDefinition(
        FGameplayTag FactTag);
    void RequestKnowledgeFactDefinition(FGameplayTag FactTag);
    void HandleKnowledgeFactDefinitionLoaded(FGameplayTag FactTag,
                                             FPrimaryAssetId AssetId);
    void AppendResolvedFacts(
        const FGameplayTagContainer &FactTags,
        EMythicEntityKnowledgeFactSection ExpectedSection,
        TArray<FMythicEntityInspectFactProjection> &OutFacts);
    static FText ResolveRelationshipBand(
        EMythicKnownRelationshipBand Band);
    static FText ResolveStandingBand(EMythicKnownStandingBand Band);

    void GatherActions(const FMythicEntityPresentationInstance &Subject,
                       TArray<FActionCandidate> &OutActions,
                       bool &OutPromotesContext,
                       bool &OutCanAssist);
    void GatherCueCandidates(
        const struct FMythicEntityAttentionObservation &Observation,
        UMythicEntityPresentationComponent &Component,
        TConstArrayView<FActionCandidate> Actions,
        TArray<EMythicNameplatePrimaryCue> &OutCues,
        FText &OutCueText,
        bool &OutDownedOrDying,
        bool &OutCombatRelevant) const;
    void GatherStatuses(UMythicEntityPresentationComponent &Component,
                        EMythicNameplateDisclosureTier Tier,
                        TArray<FMythicNameplateStatusCandidate> &OutStatuses,
                        int32 &OutOverflowCount,
                        bool &OutSafetyCritical) const;
    void ResolveHealth(UMythicEntityPresentationComponent &Component,
                       EMythicNameplateDisclosureTier Tier,
                       const struct FMythicEntityAttentionObservation &Observation,
                       bool bDownedOrDying,
                       bool bCombatRelevant,
                       bool bCanAssist,
                       EMythicPresentedCombatRank PresentedRank,
                       FMythicNameplateProjection &InOutProjection) const;
    static EMythicNameplatePrimaryCue CueFromActionSemantic(
        const UMythicContextActionDefinition &Definition);
    /** Authored policy retained by the LocalPlayer subsystem; null activates the validated native default structs. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicNameplatePolicy> Policy;

    UPROPERTY(Transient)
    TMap<FPrimaryAssetId, TObjectPtr<UMythicEntityIdentityDefinition>> IdentityDefinitions;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UMythicContextActionDefinition>> ActionDefinitions;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UMythicEntityKnowledgeFactDefinition>>
        KnowledgeFactDefinitions;

    UPROPERTY(Transient)
    TObjectPtr<UMythicRoleDatabase> RoleDatabase;

    UPROPERTY(Transient)
    TObjectPtr<UDataTable> CreatureSpeciesTable;

    /** Localized semantic labels indexed once from canonical role/species data; tags are identity, never UI strings. */
    UPROPERTY(Transient)
    TMap<FGameplayTag, FText> ArchetypeLabels;

    TWeakObjectPtr<UMythicNameplateLayer> PresentationLayer;
    TWeakObjectPtr<UMythicUserSettings> UserSettings;
    TWeakObjectPtr<UMythicEntityAttentionSubsystem> Attention;
    TWeakObjectPtr<AMythicPlayerState> BoundPlayerState;
    TWeakObjectPtr<UMythicEntityViewerKnowledgeComponent> ViewerKnowledge;
    TWeakObjectPtr<UMythicEntityActionGrantComponent> ActionGrants;
    TWeakObjectPtr<UMythicEntityCombatPresentationComponent>
        CombatPresentation;
    TWeakObjectPtr<UMythicEntityInspectPage> ActiveInspectPage;

    FDelegateHandle AttentionUpdatedHandle;
    FDelegateHandle InterfaceSettingsChangedHandle;
    FDelegateHandle KnowledgeUpdatedHandle;
    FDelegateHandle ActionGrantsUpdatedHandle;
    FDelegateHandle CombatPresentationUpdatedHandle;
    FDelegateHandle InspectPageDeactivatedHandle;

    TArray<FProjectionEntry> ActiveEntries;
    TSet<FPrimaryAssetId> RequestedPrimaryAssets;
    TMap<FPrimaryAssetId, TSharedPtr<FStreamableHandle>> PendingPrimaryAssetLoads;
    TSharedPtr<FStreamableHandle> SemanticDatabaseLoad;
    TSet<FPrimaryAssetId> RequestedKnowledgeFactAssets;
    TMap<FPrimaryAssetId, TSharedPtr<FStreamableHandle>>
        PendingKnowledgeFactLoads;

    FMythicEntityPresentationInstance InspectedInstance;
    FMythicEntityPresentationInstance PendingAmbientWhisperChallenger;
    FMythicEntityInspectProjection CurrentInspectProjection;
    FMythicNameplateRenderPreferences RenderPreferences;
    FPendingLocalContextActionHold PendingLocalContextActionHold;

    uint32 ProjectionRevision = 0;
    uint32 InspectRevision = 0;
    double PendingAmbientWhisperChallengerSinceSeconds = 0.0;
    bool bProjectionRefreshRequested = true;
    bool bPresentationLayerNeedsFullSync = false;
    bool bSemanticDatabasesRequested = false;
    EMythicNameplatePresentationMode PresentationMode;
};
