#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicTradeContractTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicTradeContractState : uint8 {
    Active,
    Completed,
    Abandoned
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTradeContractOffer {
    GENERATED_BODY()

    /** Board-unique id (monotonic; what ServerAcceptContract references). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    int32 OfferId = INDEX_NONE;

    /** Dedup key (mirrors the emergent-quest QuestKind: one open offer per kind per faction). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FGameplayTag QuestKind;

    /** The faction whose deficit this contract relieves (payer of standing; P9 injection target). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FMythicFactionId FactionId;

    /** Destination settlement (flavor/marker; delivery is accepted by ANY bAcceptsDeliveries vendor of the faction). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    int32 SettlementId = INDEX_NONE;

    /** Hierarchical item-type tag the contract accepts (e.g. Itemization.Type.Consumable.Food). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FGameplayTag DeliveryItemTag;

    /** Units required (danger-scaled at post time). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    int32 Units = 0;

    /** Which Reserve axis delivered units inject into through P9 (famine → Food, war-demand → Arms, …). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    EMythicResourceType ReserveAxis = EMythicResourceType::Food;

    /** Faction standing granted on completion (already tier-scaled at post time). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    float StandingReward = 0.0f;

    /** When true the accept gate requires FRIENDLY standing toward FactionId (war-demand arms runs — arming a
     *  faction is taking a side); default false = Neutral-or-better (not Hostile) suffices. */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    bool bRequiresFriendlyStanding = false;

    /** Player-facing headline (formatted at post time: faction/count baked in). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FText Headline;

    /** World-time after which the offer is retired from the board (accepted contracts live on). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    double ExpireTimeSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTradeContract {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    FGuid ContractId;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    FGameplayTag QuestKind;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    FMythicFactionId FactionId;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    int32 SettlementId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    FGameplayTag DeliveryItemTag;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    int32 UnitsRequired = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    int32 UnitsDelivered = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    EMythicTradeContractState State = EMythicTradeContractState::Active;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    EMythicResourceType ReserveAxis = EMythicResourceType::Food;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    float StandingReward = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Trading")
    FText Headline;

    bool IsActive() const { return State == EMythicTradeContractState::Active; }
    int32 UnitsRemaining() const { return FMath::Max(UnitsRequired - UnitsDelivered, 0); }
};

namespace MythicTradeContracts {
    struct FDeliveryApplication {
        int32 AcceptedUnits = 0;
        bool bCompleted = false;
    };

    inline FDeliveryApplication ApplyDelivery(int32 UnitsRequired, int32 UnitsDelivered, int32 OfferedUnits) {
        FDeliveryApplication Out;
        const int32 Remaining = FMath::Max(UnitsRequired - UnitsDelivered, 0);
        Out.AcceptedUnits = FMath::Clamp(OfferedUnits, 0, Remaining);
        Out.bCompleted = Remaining > 0 && Out.AcceptedUnits == Remaining;
        return Out;
    }

    inline int32 ComputeDeliveryPayout(int32 UnitValue, int32 Units, float ScarcityMultiplier) {
        if (UnitValue <= 0 || Units <= 0) {
            return 0;
        }
        const float Scarcity = FMath::Max(ScarcityMultiplier, 0.0f);
        return FMath::Max(FMath::RoundToInt(static_cast<float>(UnitValue) * static_cast<float>(Units) * Scarcity), 0);
    }

    inline bool ShouldFireDeficitBeat(float AxisReserves, float DeficitThreshold, float RearmThreshold, bool bLatched,
                                      bool &bOutLatched) {
        if (bLatched) {
            bOutLatched = !(AxisReserves > FMath::Max(RearmThreshold, DeficitThreshold));
            return false;
        }
        if (AxisReserves <= DeficitThreshold) {
            bOutLatched = true;
            return true;
        }
        bOutLatched = false;
        return false;
    }
}
