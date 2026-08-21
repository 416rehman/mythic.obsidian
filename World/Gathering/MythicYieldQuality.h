
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicYieldQuality.generated.h"

UENUM(BlueprintType)
enum class EMythicYieldQuality : uint8 {
    Ragged = 0 UMETA(ToolTip = "Botched-kill tier - hunting only. Crops/produce never mint Ragged (they bottom out at Common)."),
    Common = 1 UMETA(ToolTip = "The neutral baseline tier: 1.0x potency, 1.0x price. Every source rolls Common until its wave lands (M6)."),
    Fine = 2,
    Pristine = 3,
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicYieldQualityRules {
    GENERATED_BODY()

    // ── Tier → POTENCY multiplier (consumed by cooking potency; Common MUST stay 1.0 for the inert default) ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potency", meta = (ClampMin = "0.0"))
    float RaggedPotencyMultiplier = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potency", meta = (ClampMin = "0.0"))
    float CommonPotencyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potency", meta = (ClampMin = "0.0"))
    float FinePotencyMultiplier = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potency", meta = (ClampMin = "0.0"))
    float PristinePotencyMultiplier = 1.35f;

    // ── Tier → PRICE multiplier (consumed by economy pricing for the quality-tier item defs; Common = 1.0) ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Price", meta = (ClampMin = "0.0"))
    float RaggedPriceMultiplier = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Price", meta = (ClampMin = "0.0"))
    float CommonPriceMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Price", meta = (ClampMin = "0.0"))
    float FinePriceMultiplier = 1.75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Price", meta = (ClampMin = "0.0"))
    float PristinePriceMultiplier = 3.0f;

    // ── Roll chances (injected roll; chance to UPGRADE above the source's base tier). Gentle defaults. ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseFineChance = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BasePristineChance = 0.02f;

    // Per-mastery-level additive chance growth (a level-50 farmer sees noticeably better crops, never guaranteed ones).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0"))
    float FineChancePerMasteryLevel = 0.004f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0"))
    float PristineChancePerMasteryLevel = 0.002f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFineChance = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxPristineChance = 0.25f;

    // ── Mastery floors: at/above these mastery levels the roll can never come out BELOW the tier. 0 = disabled. ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mastery Floors", meta = (ClampMin = "0"))
    int32 FineFloorAtMasteryLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mastery Floors", meta = (ClampMin = "0"))
    int32 PristineFloorAtMasteryLevel = 0;
};

struct FMythicYieldQuality {
    static constexpr int32 NumTiers = 4;

    static int32 TierIndex(EMythicYieldQuality Tier) { return static_cast<int32>(Tier); }

    static EMythicYieldQuality TierFromIndex(int32 Index) {
        return static_cast<EMythicYieldQuality>(FMath::Clamp(Index, 0, NumTiers - 1));
    }

    static float TierValue(EMythicYieldQuality Tier) { return static_cast<float>(TierIndex(Tier)); }

    static float PotencyMultiplier(const FMythicYieldQualityRules &Rules, EMythicYieldQuality Tier) {
        const float Raw[NumTiers] = {
            Rules.RaggedPotencyMultiplier, Rules.CommonPotencyMultiplier,
            Rules.FinePotencyMultiplier, Rules.PristinePotencyMultiplier
        };
        return SanitizedAt(Raw, TierIndex(Tier));
    }

    static float PriceMultiplier(const FMythicYieldQualityRules &Rules, EMythicYieldQuality Tier) {
        const float Raw[NumTiers] = {
            Rules.RaggedPriceMultiplier, Rules.CommonPriceMultiplier,
            Rules.FinePriceMultiplier, Rules.PristinePriceMultiplier
        };
        return SanitizedAt(Raw, TierIndex(Tier));
    }

    static float PotencyMultiplierForTierValue(const FMythicYieldQualityRules &Rules, float InTierValue) {
        const float Clamped = FMath::Clamp(InTierValue, 0.0f, static_cast<float>(NumTiers - 1));
        const int32 Lo = FMath::FloorToInt32(Clamped);
        const int32 Hi = FMath::Min(Lo + 1, NumTiers - 1);
        const float Alpha = Clamped - static_cast<float>(Lo);
        const float MultLo = PotencyMultiplier(Rules, TierFromIndex(Lo));
        const float MultHi = PotencyMultiplier(Rules, TierFromIndex(Hi));
        return FMath::Lerp(MultLo, MultHi, Alpha);
    }

    static EMythicYieldQuality MasteryFloor(const FMythicYieldQualityRules &Rules, int32 MasteryLevel, EMythicYieldQuality SourceFloor) {
        int32 Floor = TierIndex(SourceFloor);
        if (Rules.FineFloorAtMasteryLevel > 0 && MasteryLevel >= Rules.FineFloorAtMasteryLevel) {
            Floor = FMath::Max(Floor, TierIndex(EMythicYieldQuality::Fine));
        }
        if (Rules.PristineFloorAtMasteryLevel > 0 && MasteryLevel >= Rules.PristineFloorAtMasteryLevel) {
            Floor = FMath::Max(Floor, TierIndex(EMythicYieldQuality::Pristine));
        }
        return TierFromIndex(Floor);
    }

    static float FineChanceAtLevel(const FMythicYieldQualityRules &Rules, int32 MasteryLevel) {
        const float Chance = Rules.BaseFineChance + FMath::Max(0, MasteryLevel) * FMath::Max(0.0f, Rules.FineChancePerMasteryLevel);
        return FMath::Clamp(Chance, 0.0f, FMath::Clamp(Rules.MaxFineChance, 0.0f, 1.0f));
    }

    static float PristineChanceAtLevel(const FMythicYieldQualityRules &Rules, int32 MasteryLevel) {
        const float Chance = Rules.BasePristineChance + FMath::Max(0, MasteryLevel) * FMath::Max(0.0f, Rules.PristineChancePerMasteryLevel);
        return FMath::Clamp(Chance, 0.0f, FMath::Clamp(Rules.MaxPristineChance, 0.0f, 1.0f));
    }

    static EMythicYieldQuality RollQuality(const FMythicYieldQualityRules &Rules, float Rand01, int32 MasteryLevel,
                                           EMythicYieldQuality SourceFloor = EMythicYieldQuality::Common,
                                           EMythicYieldQuality BaseTier = EMythicYieldQuality::Common) {
        const float PristineChance = PristineChanceAtLevel(Rules, MasteryLevel);
        const float FineChance = FineChanceAtLevel(Rules, MasteryLevel);
        const float Roll = FMath::Clamp(Rand01, 0.0f, 1.0f);

        EMythicYieldQuality Result = BaseTier;
        if (Roll < PristineChance) {
            Result = EMythicYieldQuality::Pristine;
        }
        else if (Roll < PristineChance + FineChance) {
            Result = EMythicYieldQuality::Fine;
        }

        const EMythicYieldQuality Floor = MasteryFloor(Rules, MasteryLevel, SourceFloor);
        return TierFromIndex(FMath::Max(TierIndex(Result), TierIndex(Floor)));
    }

    static EMythicYieldQuality DepleteTier(EMythicYieldQuality Tier, int32 DropSteps, EMythicYieldQuality Floor = EMythicYieldQuality::Common) {
        const int32 Dropped = TierIndex(Tier) - FMath::Max(0, DropSteps);
        return TierFromIndex(FMath::Max(Dropped, TierIndex(Floor)));
    }

    static FName QualityTagName(EMythicYieldQuality Tier) {
        switch (Tier) {
        case EMythicYieldQuality::Ragged: return FName(TEXT("Itemization.Quality.Ragged"));
        case EMythicYieldQuality::Fine: return FName(TEXT("Itemization.Quality.Fine"));
        case EMythicYieldQuality::Pristine: return FName(TEXT("Itemization.Quality.Pristine"));
        case EMythicYieldQuality::Common:
        default: return FName(TEXT("Itemization.Quality.Common"));
        }
    }

    static bool TierFromTagName(const FName &TagName, EMythicYieldQuality &OutTier) {
        for (int32 i = 0; i < NumTiers; i++) {
            const EMythicYieldQuality Tier = TierFromIndex(i);
            if (TagName == QualityTagName(Tier)) {
                OutTier = Tier;
                return true;
            }
        }
        return false;
    }

    static EMythicYieldQuality TierFromTags(const FGameplayTagContainer &Tags) {
        for (const FGameplayTag &Tag : Tags) {
            EMythicYieldQuality Tier;
            if (TierFromTagName(Tag.GetTagName(), Tier)) {
                return Tier;
            }
        }
        return EMythicYieldQuality::Common;
    }

private:
    static float SanitizedAt(const float (&Raw)[NumTiers], int32 Index) {
        float Value = 0.0f;
        const int32 Clamped = FMath::Clamp(Index, 0, NumTiers - 1);
        for (int32 i = 0; i <= Clamped; i++) {
            Value = FMath::Max(Value, FMath::Max(0.0f, Raw[i]));
        }
        return Value;
    }
};
