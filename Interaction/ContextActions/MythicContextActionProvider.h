#pragma once

#include "CoreMinimal.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "UObject/Interface.h"
#include "MythicContextActionProvider.generated.h"

class AController;

/** Server-side provider decision; Hidden is deliberately absent from replicated grant state. */
UENUM(BlueprintType)
enum class EMythicContextActionAvailability : uint8 {
    Hidden UMETA(DisplayName = "Hidden"),
    Available UMETA(DisplayName = "Available"),
    UnavailableWithReason UMETA(DisplayName = "Unavailable with Reason")
};

/**
 * One ephemeral, viewer-specific action offer gathered on authority.
 * The direct definition is resolved to ActionTag before replication; object pointers never cross the wire.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicContextActionOffer {
    GENERATED_BODY()

    /** Canonical action definition selected by the provider; null offers are ignored and never replicated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Action")
    TObjectPtr<UMythicContextActionDefinition> Definition = nullptr;

    /** Current server decision for this viewer; Hidden revokes any matching grant instead of replicating that state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Action")
    EMythicContextActionAvailability Availability = EMythicContextActionAvailability::Hidden;

    /** Optional safe Context.Action.Reason.* tag for an unavailable offer; it must not encode secret simulation truth. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Action",
              meta = (Categories = "Context.Action.Reason"))
    FGameplayTag UnavailableReasonTag;

    /**
     * Nonnegative provider revision checked by authority to reject stale UI offers; the valid range is 0..4294967295.
     * This is int64 only because Blueprint has no uint32 pin; authority range-checks it before compact uint32 transport.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context Action",
              meta = (ClampMin = "0", ClampMax = "4294967295"))
    int64 SourceRevision = 0;

    FGameplayTag GetActionTag() const {
        return Definition ? Definition->ActionTag : FGameplayTag();
    }
};

UINTERFACE(MinimalAPI, Blueprintable)
class UMythicContextActionProvider : public UInterface {
    GENERATED_BODY()
};

/** Authority-side contract implemented by actors or components that own contextual action domain rules. */
class MYTHIC_API IMythicContextActionProvider {
    GENERATED_BODY()

public:
    /**
     * Gathers current offers for one requesting controller and subject on the server; invalid inputs must yield no offers.
     * This is not an RPC, and callers must discard Hidden offers rather than serializing them.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Context Actions")
    void GatherContextActions(AController *RequestingController, AActor *Subject,
                              TArray<FMythicContextActionOffer> &OutOffers) const;

    /**
     * Revalidates a requested action on the server immediately before execution; failure returns a safe optional reason tag.
     * ObservedOfferRevision is the owning client's 0..4294967295 revision; stale or out-of-range requests return false.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Context Actions")
    bool CanExecuteContextAction(AController *RequestingController, AActor *Subject, FGameplayTag ActionTag,
                                 int64 ObservedOfferRevision, FGameplayTag &OutFailureReason) const;

    /**
     * Atomically executes one already revalidated action on the server; this is not an RPC and clients must use the owned
     * PlayerController request seam. ObservedOfferRevision must be 0..4294967295; false means no domain mutation and may
     * return a safe optional failure reason.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Context Actions")
    bool ExecuteContextAction(AController *RequestingController, AActor *Subject, FGameplayTag ActionTag,
                              int64 ObservedOfferRevision, FGameplayTag &OutFailureReason);
};
