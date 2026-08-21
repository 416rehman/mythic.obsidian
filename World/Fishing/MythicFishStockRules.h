
#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "MythicFishStockRules.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFishStockConfig {
    GENERATED_BODY()

    /** Default stock units a spot holds when full (a spot's own MaxStockOverride > 0 wins). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stocks", meta = (ClampMin = "1"))
    int32 DefaultMaxStock = 10;

    /** Seconds for ONE stock unit to regenerate (lazy timestamp math — no timer). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stocks", meta = (ClampMin = "1.0"))
    float RegenSecondsPerUnit = 300.0f;

    /** Pressure.Fish pushed at the spot per successful catch (the slow over-fishing accrual). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stocks", meta = (ClampMin = "0.0"))
    float PressurePerCatch = 0.25f;

    /** Pressure.Fish pushed ONCE when a catch exhausts the spot (the fished-out spike the sim/pricing can read). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stocks", meta = (ClampMin = "0.0"))
    float PressureOnExhausted = 5.0f;
};

struct FMythicFishStockRules {
    static constexpr uint8 SaveVersion = 1;

    static int32 Resolve(int32 &StockUnits, double &AnchorTime, double Now, float RegenSecondsPerUnit, int32 MaxStock) {
        MaxStock = FMath::Max(1, MaxStock);
        StockUnits = FMath::Clamp(StockUnits, 0, MaxStock);
        const double PerUnit = FMath::Max(1.0f, RegenSecondsPerUnit);
        if (StockUnits >= MaxStock) {
            AnchorTime = Now;
            return StockUnits;
        }
        const double Gap = FMath::Max(0.0, Now - AnchorTime);
        const int32 Regened = static_cast<int32>(Gap / PerUnit);
        if (Regened > 0) {
            const int32 Banked = FMath::Min(Regened, MaxStock - StockUnits);
            StockUnits += Banked;
            AnchorTime = (StockUnits >= MaxStock) ? Now : AnchorTime + static_cast<double>(Banked) * PerUnit;
        }
        return StockUnits;
    }

    static bool ConsumeOne(int32 &StockUnits, double &AnchorTime, double Now, float RegenSecondsPerUnit, int32 MaxStock,
                           bool &bOutExhaustedNow) {
        bOutExhaustedNow = false;
        const int32 Resolved = Resolve(StockUnits, AnchorTime, Now, RegenSecondsPerUnit, MaxStock);
        if (Resolved <= 0) {
            return false;
        }
        if (Resolved >= FMath::Max(1, MaxStock)) {
            AnchorTime = Now;
        }
        StockUnits = Resolved - 1;
        bOutExhaustedNow = (StockUnits == 0);
        return true;
    }

    static float SecondsTowardNextUnit(int32 StockUnits, double AnchorTime, double Now, int32 MaxStock) {
        if (StockUnits >= FMath::Max(1, MaxStock)) {
            return 0.0f;
        }
        return static_cast<float>(FMath::Max(0.0, Now - AnchorTime));
    }

    static void EncodeStockSave(TArray<uint8> &Out, int32 StockUnits, float SecondsTowardNext) {
        FMemoryWriter Writer(Out);
        uint8 Version = SaveVersion;
        Writer << Version;
        Writer << StockUnits;
        Writer << SecondsTowardNext;
    }

    static bool DecodeStockSave(const TArray<uint8> &In, int32 &OutStockUnits, float &OutSecondsTowardNext) {
        OutStockUnits = 0;
        OutSecondsTowardNext = 0.0f;
        if (In.Num() == 0) {
            return false;
        }
        FMemoryReader Reader(In);
        uint8 Version = 0;
        Reader << Version;
        if (Version < 1 || Reader.IsError()) {
            return false;
        }
        Reader << OutStockUnits;
        Reader << OutSecondsTowardNext;
        if (Reader.IsError()) {
            OutStockUnits = 0;
            OutSecondsTowardNext = 0.0f;
            return false;
        }
        OutStockUnits = FMath::Max(0, OutStockUnits);
        OutSecondsTowardNext = FMath::Max(0.0f, OutSecondsTowardNext);
        return true;
    }
};
