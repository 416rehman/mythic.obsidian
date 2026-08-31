#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

#include "MythicContextActionProjectionPolicy.generated.h"

/**
 * Server-authored limits for converting one viewer's focused entity into owner-only contextual-action leases.
 *
 * This policy is referenced directly by the PlayerController class used by a game mode. It is deliberately separate
 * from individual action definitions: these values are security, query-budget, and refresh-envelope limits shared by
 * every action, while each action definition still owns its own tighter focus, range, and execution requirements.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicContextActionProjectionPolicy : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Primary Asset type used to audit and cook contextual-action projection policies. */
    static const FPrimaryAssetType PrimaryAssetType;

    /**
     * Absolute server-side discovery radius in centimeters measured from the viewer pawn to the presentation anchor.
     * Providers are never queried outside this radius, even when an action definition has no range requirement.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Discovery",
              meta = (ClampMin = "1.0", ClampMax = "10000.0", Units = "cm"))
    float MaximumDiscoveryRangeCentimeters = 1500.0f;

    /** Collision channel used by the mandatory server discovery LOS trace and action execution LOS revalidation. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Discovery")
    TEnumAsByte<ECollisionChannel> DiscoveryTraceChannel = ECC_Visibility;

    /** Whether mandatory discovery and action LOS checks trace complex collision on the server. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Discovery")
    bool bTraceComplex = false;

    /**
     * Minimum real-time seconds between accepted client refresh requests. Focus loss still revokes immediately while
     * an early replacement request is deferred, preventing request spam without preserving a stale previous grant.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Cadence",
              meta = (ClampMin = "0.05", ClampMax = "2.0", Units = "s"))
    float MinimumClientRequestIntervalSeconds = 0.10f;

    /**
     * Server refresh cadence in seconds while one exact subject remains nominated. This must be no faster than the
     * request interval and is independent of render or nameplate decision frequency.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Cadence",
              meta = (ClampMin = "0.05", ClampMax = "5.0", Units = "s"))
    float AuthorityRefreshIntervalSeconds = 0.40f;

    /**
     * Duration in synchronized server-world seconds for each projected offer lease. It must exceed the authority
     * refresh interval so one delayed refresh does not flicker otherwise-current UI state.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Cadence",
              meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "s"))
    float OfferLeaseDurationSeconds = 1.0f;

    /** Maximum provider components inspected on the resolved subject, excluding the actor provider itself. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Budgets",
              meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaximumProviderComponents = 24;

    /** Maximum rows consumed from any one authority provider response before excess rows are ignored. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Budgets",
              meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaximumOffersPerProvider = 16;

    /**
     * Maximum unique actions retained for the focused subject after deterministic priority selection. This cannot
     * exceed the owner-only grant component's fixed per-subject transport bound.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action Projection|Budgets",
              meta = (ClampMin = "1", ClampMax = "16"))
    int32 MaximumProjectedOffers = 12;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};

/** Validated, allocation-free authority settings copied from a projection policy before processing a request. */
struct MYTHIC_API FMythicContextActionProjectionRuntimePolicy {
    float MaximumDiscoveryRangeCentimeters = 0.0f;
    float MinimumClientRequestIntervalSeconds = 0.0f;
    float AuthorityRefreshIntervalSeconds = 0.0f;
    float OfferLeaseDurationSeconds = 0.0f;
    int32 MaximumProviderComponents = 0;
    int32 MaximumOffersPerProvider = 0;
    int32 MaximumProjectedOffers = 0;
    ECollisionChannel DiscoveryTraceChannel = ECC_Visibility;
    bool bTraceComplex = false;
    bool bValid = false;
};

/** Pure security and bounded-selection rules shared by runtime coordination and automation tests. */
struct MYTHIC_API FMythicContextActionProjectionRules {
    static constexpr float HardMaximumDiscoveryRangeCentimeters = 10000.0f;
    static constexpr float HardMinimumRequestIntervalSeconds = 0.05f;
    static constexpr float HardMaximumRequestIntervalSeconds = 2.0f;
    static constexpr float HardMaximumRefreshIntervalSeconds = 5.0f;
    static constexpr float HardMaximumLeaseDurationSeconds = 30.0f;
    static constexpr float HardMinimumHoldDurationSeconds = 0.10f;
    static constexpr float HardMaximumHoldDurationSeconds = 10.0f;
    static constexpr double HoldCompletionEarlyToleranceSeconds = 0.025;
    static constexpr double HoldCompletionGraceSeconds = 2.0;
    static constexpr int32 HardMaximumProviderComponents = 32;
    static constexpr int32 HardMaximumOffersPerProvider = 32;
    static constexpr int32 HardMaximumProjectedOffers =
        UMythicEntityActionGrantComponent::MaximumReplicatedGrantsPerSubject;

    /** Builds a fail-closed runtime snapshot; null, nonfinite, contradictory, or out-of-envelope assets are invalid. */
    static FMythicContextActionProjectionRuntimePolicy BuildRuntimePolicy(
        const UMythicContextActionProjectionPolicy *Policy);

    /** True only when an exact requested handle-generation pair still equals the resolved active embodiment. */
    static bool IsExactResolvedSubject(
        const FMythicEntityPresentationInstance &Requested,
        const FMythicEntityPresentationInstance &Resolved);

    /** Returns seconds remaining in the request throttle, or zero when the next bounded provider pass may begin. */
    static double GetRequestThrottleDelaySeconds(
        double NowSeconds, double LastAcceptedSeconds,
        float MinimumIntervalSeconds);

    /** True for a tap duration of zero or a finite authored hold inside the supported security and UX envelope. */
    static bool IsHoldDurationValid(float HoldDurationSeconds);

    /**
     * Validates the authority clock interval for one completed hold. A small early tolerance absorbs frame/network
     * quantization, while the bounded late grace prevents an abandoned hold from becoming a reusable authorization.
     */
    static bool IsHoldCompletionTimingValid(double AuthorityStartSeconds,
                                            double AuthorityCompletionSeconds,
                                            float RequiredHoldDurationSeconds);

    /**
     * Sanitizes and inserts one authority offer into a deterministic fixed-size top-K set. Duplicate action tags are
     * merged fail-closed; Hidden, malformed, unexplainable unavailable, and lower-ranked overflow rows are discarded.
     */
    static bool TryInsertBoundedOffer(
        const FMythicContextActionOffer &Candidate,
        int32 MaximumOffers,
        TArray<FMythicContextActionOffer> &InOutOffers);
};
