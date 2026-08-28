#pragma once

#include "CoreMinimal.h"

#include "MythicHarvestReplicationTypes.generated.h"

/**
 * Ordering result for opaque harvest-presentation streams.
 * Invalid and Conflict are deliberately distinct from Older so corrupt wire data can be diagnosed instead of hidden.
 */
enum class EMythicHarvestPresentationStreamOrder : uint8 {
    Invalid,
    Older,
    Same,
    Newer,
    Conflict,
};

/**
 * Native-only incarnation of the replicated harvest presentation stream.
 *
 * This is intentionally unrelated to the authority-only durable harvest WorldEpoch. The random nonce prevents a
 * newly created world from aliasing an old stream, while the nonzero serial supplies RFC-1982 ordering for multiple
 * in-place restores on one live connection. It is transient, never saved, and never exposed to Blueprint.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestPresentationStreamToken {
    GENERATED_BODY()

public:
    FMythicHarvestPresentationStreamToken() = default;
    FMythicHarvestPresentationStreamToken(const FGuid &InNonce,
                                          uint32 InSerial)
        : Nonce(InNonce), Serial(InSerial) {}

    /** Returns true only when both independent identity fields are usable on the wire. */
    bool IsValid() const { return Nonce.IsValid() && Serial != 0; }

    /** Returns the opaque random stream nonce; it is never the durable authority WorldEpoch. */
    const FGuid &GetNonce() const { return Nonce; }

    /** Returns the nonzero live-session serial used for wrap-safe stream ordering. */
    uint32 GetSerial() const { return Serial; }

    bool operator==(const FMythicHarvestPresentationStreamToken &Other) const {
        return Nonce == Other.Nonce && Serial == Other.Serial;
    }

    bool operator!=(const FMythicHarvestPresentationStreamToken &Other) const {
        return !(*this == Other);
    }

    /**
     * Compares Left to Right using RFC-1982 serial arithmetic within one nonce-scoped authority lifetime.
     * Different nonces or an exact half-range serial separation are protocol conflicts, never orderable streams.
     */
    static EMythicHarvestPresentationStreamOrder Compare(
        const FMythicHarvestPresentationStreamToken &Left,
        const FMythicHarvestPresentationStreamToken &Right);

    /** Builds the first valid stream for one newly initialized authority world. */
    static bool TryMakeInitial(
        const FGuid &Nonce,
        FMythicHarvestPresentationStreamToken &OutToken);

    /** Advances a valid stream within its nonce-scoped authority lifetime, skipping zero when the serial wraps. */
    static bool TryAdvance(
        const FMythicHarvestPresentationStreamToken &Current,
        FMythicHarvestPresentationStreamToken &OutToken);

private:
    /** Independent random presentation identity; never derived from or copied from the save/reward WorldEpoch. */
    UPROPERTY()
    FGuid Nonce;

    /** Nonzero live-session incarnation ordered with RFC-1982 arithmetic. */
    UPROPERTY()
    uint32 Serial = 0;
};
