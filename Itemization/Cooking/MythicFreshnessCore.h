
#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace MythicFreshness {
constexpr uint8 PerishablePayloadVersion = 1;

constexpr double DefaultBucketSeconds = 300.0;

inline int64 QuantizeToBucket(double Seconds, double BucketSeconds) {
    const double Width = BucketSeconds > 0.0 ? BucketSeconds : 1.0;
    return static_cast<int64>(FMath::FloorToDouble(Seconds / Width));
}

inline int64 MergedStampBucket(int64 BucketA, int64 BucketB) {
    return FMath::Min(BucketA, BucketB);
}

inline double AccrueAgedSeconds(double BankedSeconds, double AnchorUtcSeconds, double NowUtcSeconds, float PreservationMultiplier) {
    const double Elapsed = FMath::Max(0.0, NowUtcSeconds - AnchorUtcSeconds);
    const double Rate = FMath::Max(0.0f, PreservationMultiplier);
    return FMath::Max(0.0, BankedSeconds) + Elapsed * Rate;
}

inline float FreshnessFraction(double AgedSeconds, double ShelfLifeSeconds) {
    if (ShelfLifeSeconds <= 0.0) {
        return 1.0f;
    }
    const double Fraction = 1.0 - FMath::Max(0.0, AgedSeconds) / ShelfLifeSeconds;
    return static_cast<float>(FMath::Clamp(Fraction, 0.0, 1.0));
}

enum class EWindow : uint8 {
    Fresh,
    Stale,
    Spoiled
};

inline EWindow ClassifyWindow(float FreshFraction, float StaleBelowFraction = 0.5f) {
    if (FreshFraction <= 0.0f) {
        return EWindow::Spoiled;
    }
    const float Threshold = FMath::Clamp(StaleBelowFraction, 0.0f, 1.0f);
    return FreshFraction < Threshold ? EWindow::Stale : EWindow::Fresh;
}

inline bool CanStackPerishables(double BankedA, double AnchorA, float MultA,
                                double BankedB, double AnchorB, float MultB,
                                double BucketSeconds, double NowUtcSeconds) {
    const int64 AgeBucketA = QuantizeToBucket(AccrueAgedSeconds(BankedA, AnchorA, NowUtcSeconds, MultA), BucketSeconds);
    const int64 AgeBucketB = QuantizeToBucket(AccrueAgedSeconds(BankedB, AnchorB, NowUtcSeconds, MultB), BucketSeconds);
    if (AgeBucketA != AgeBucketB) {
        return false;
    }
    return FMath::IsNearlyEqual(FMath::Max(0.0f, MultA), FMath::Max(0.0f, MultB), 0.001f);
}

inline void SerializeState(TArray<uint8> &OutData, double BankedSeconds, double AnchorUtcSeconds, float PreservationMultiplier) {
    FMemoryWriter Writer(OutData);
    uint8 Version = PerishablePayloadVersion;
    double Banked = FMath::Max(0.0, BankedSeconds);
    double Anchor = AnchorUtcSeconds;
    float Mult = FMath::Max(0.0f, PreservationMultiplier);
    Writer << Version;
    Writer << Banked;
    Writer << Anchor;
    Writer << Mult;
}

inline bool DeserializeState(const TArray<uint8> &InData, double &OutBankedSeconds, double &OutAnchorUtcSeconds, float &OutPreservationMultiplier) {
    if (InData.Num() == 0) {
        return false;
    }
    FMemoryReader Reader(InData);
    uint8 Version = 0;
    Reader << Version;
    constexpr int64 PayloadBytes = static_cast<int64>(sizeof(double) * 2 + sizeof(float));
    if (Version != PerishablePayloadVersion || Reader.TotalSize() - Reader.Tell() < PayloadBytes) {
        return false;
    }
    double Banked = 0.0, Anchor = 0.0;
    float Mult = 1.0f;
    Reader << Banked;
    Reader << Anchor;
    Reader << Mult;
    OutBankedSeconds = FMath::Max(0.0, Banked);
    OutAnchorUtcSeconds = Anchor;
    OutPreservationMultiplier = FMath::Max(0.0f, Mult);
    return true;
}
}
