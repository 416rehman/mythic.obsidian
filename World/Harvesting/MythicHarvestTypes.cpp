#include "World/Harvesting/MythicHarvestTypes.h"

bool FMythicHarvestWork::TryFromWorkUnits(const double WorkUnits, FMythicHarvestWork &OutWork) {
    OutWork = FMythicHarvestWork();

    // Keep a full work unit of headroom. A double cannot distinguish MAX_int64 from MAX_int64 - 1,
    // so comparing against the nominal int64 limit could admit a value that overflows RoundToInt64.
    constexpr int64 MaxWholeWorkUnits = (MAX_int64 / QuantaPerWorkUnit) - 1;
    constexpr double MaxRepresentableWork = static_cast<double>(MaxWholeWorkUnits);
    if (!FMath::IsFinite(WorkUnits) || WorkUnits < 0.0 || WorkUnits > MaxRepresentableWork) {
        return false;
    }

    const double Scaled = WorkUnits * static_cast<double>(QuantaPerWorkUnit);
    if (!FMath::IsFinite(Scaled) || Scaled > static_cast<double>(MAX_int64 - 1)) {
        return false;
    }

    OutWork.Quanta = FMath::RoundToInt64(Scaled);
    return true;
}

FMythicHarvestWork FMythicHarvestWork::FromQuanta(const int64 InQuanta) {
    FMythicHarvestWork Result;
    Result.Quanta = FMath::Max<int64>(0, InQuanta);
    return Result;
}

double FMythicHarvestWork::ToWorkUnits() const { return static_cast<double>(Quanta) / static_cast<double>(QuantaPerWorkUnit); }

FMythicHarvestWork FMythicHarvestWork::SubtractClamped(const FMythicHarvestWork Amount) const {
    return FromQuanta(Amount.Quanta >= Quanta ? 0 : Quanta - Amount.Quanta);
}

FMythicHarvestWork FMythicHarvestWork::Min(const FMythicHarvestWork A, const FMythicHarvestWork B) { return FromQuanta(FMath::Min(A.Quanta, B.Quanta)); }

double FMythicHarvestContributionMath::CalculateProportionalShare(
    const int64 ContributorQuanta, const int64 TotalEligibleQuanta,
    const double TotalPool) {
    if (ContributorQuanta <= 0 || TotalEligibleQuanta <= 0
        || ContributorQuanta > TotalEligibleQuanta
        || !FMath::IsFinite(TotalPool) || TotalPool <= 0.0) {
        return 0.0;
    }
    const double Share = TotalPool
        * (static_cast<double>(ContributorQuanta)
           / static_cast<double>(TotalEligibleQuanta));
    return FMath::IsFinite(Share) && Share > 0.0 ? Share : 0.0;
}

bool FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
    const EMythicHarvestNodeState State, const uint32 Generation,
    const bool bHasPartialWork) {
    (void)Generation;
    return State != EMythicHarvestNodeState::Available || bHasPartialWork;
}

bool FMythicHarvestCadencePolicy::TryCalculateExpiry(
    const double IssuedServerTime, const double MaximumMontageSeconds,
    const double CapturedPlayRate, const double ToleranceSeconds,
    double &OutExpiresServerTime) {
    OutExpiresServerTime = 0.0;
    if (!FMath::IsFinite(IssuedServerTime) || IssuedServerTime < 0.0
        || !FMath::IsFinite(MaximumMontageSeconds)
        || MaximumMontageSeconds <= 0.0
        || !FMath::IsFinite(CapturedPlayRate) || CapturedPlayRate <= 0.0
        || !FMath::IsFinite(ToleranceSeconds) || ToleranceSeconds < 0.0) {
        return false;
    }
    const double Duration = MaximumMontageSeconds / CapturedPlayRate
        + ToleranceSeconds;
    const double Expiry = IssuedServerTime + Duration;
    if (!FMath::IsFinite(Duration) || Duration <= 0.0
        || !FMath::IsFinite(Expiry) || Expiry <= IssuedServerTime) {
        return false;
    }
    OutExpiresServerTime = Expiry;
    return true;
}

bool FMythicHarvestCadencePolicy::IsExpired(
    const double ServerNow, const double ExpiresServerTime) {
    return !FMath::IsFinite(ServerNow) || ServerNow < 0.0
        || !FMath::IsFinite(ExpiresServerTime) || ExpiresServerTime <= 0.0
        || ServerNow > ExpiresServerTime;
}
