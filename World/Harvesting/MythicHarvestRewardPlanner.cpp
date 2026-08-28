#include "World/Harvesting/MythicHarvestRewardPlanner.h"

#include "Hash/Blake3.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Math/BigInt.h"

namespace MythicHarvestRewardPlannerPrivate {

constexpr uint8 CanonicalDomain[] = {
    'M', 'Y', 'T', 'H', 'I', 'C', '_', 'H', 'A', 'R', 'V', 'E', 'S', 'T', '_',
    'R', 'E', 'W', 'A', 'R', 'D', '_', 'V', '2', 0,
};

enum class ESeedPurpose : uint8 {
    Eligibility = 1,
    ChannelSelection = 2,
    Quantity = 3,
    ItemFactory = 4,
    Quality = 5,
    QuantityMultiplierRounding = 6,
};

using FRewardBigUInt = TBigInt<256, false>;

struct FCanonicalParticipant {
    const FMythicHarvestRewardParticipant *Participant = nullptr;
    int32 SourceIndex = INDEX_NONE;
};

struct FWeightedCandidate {
    int32 RowIndex = INDEX_NONE;
    uint64 Weight = 0;
};

FRewardBigUInt MakeBigUInt(const uint64 Value) {
    FRewardBigUInt Result;
    Result.GetBits()[0] = static_cast<uint32>(Value);
    Result.GetBits()[1] = static_cast<uint32>(Value >> 32);
    return Result;
}

bool TryBigUIntToInt32(const FRewardBigUInt &Value, int32 &OutValue) {
    const uint32 *Bits = Value.GetBits();
    for (int32 WordIndex = 1; WordIndex < 8; ++WordIndex) {
        if (Bits[WordIndex] != 0) {
            OutValue = 0;
            return false;
        }
    }
    if (Bits[0] > static_cast<uint32>(MAX_int32)) {
        OutValue = 0;
        return false;
    }
    OutValue = static_cast<int32>(Bits[0]);
    return true;
}

bool IsValidQuality(const EMythicYieldQuality Quality) {
    return Quality == EMythicYieldQuality::Ragged
        || Quality == EMythicYieldQuality::Common
        || Quality == EMythicYieldQuality::Fine
        || Quality == EMythicYieldQuality::Pristine;
}

bool IsValidQualityPolicy(const EMythicHarvestRewardQualityPolicy Policy) {
    return Policy == EMythicHarvestRewardQualityPolicy::DefinitionDefault
        || Policy == EMythicHarvestRewardQualityPolicy::Fixed
        || Policy == EMythicHarvestRewardQualityPolicy::ContributorProficiency;
}

const UYieldQualityFragment *FindYieldQualityFragment(
    const UItemDefinition &Definition, int32 &OutCount) {
    OutCount = 0;
    const UYieldQualityFragment *Found = nullptr;
    for (const UItemFragment *Fragment : Definition.Fragments) {
        if (const UYieldQualityFragment *Quality =
                Cast<UYieldQualityFragment>(Fragment)) {
            ++OutCount;
            Found = Quality;
        }
    }
    return OutCount == 1 ? Found : nullptr;
}

bool AreQualityRollRulesValid(const FMythicYieldQualityRules &Rules) {
    const auto IsProbability = [](const float Value) {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    };
    const auto IsNonNegative = [](const float Value) {
        return FMath::IsFinite(Value) && Value >= 0.0f;
    };
    return IsProbability(Rules.BaseFineChance)
        && IsProbability(Rules.BasePristineChance)
        && IsNonNegative(Rules.FineChancePerMasteryLevel)
        && IsNonNegative(Rules.PristineChancePerMasteryLevel)
        && IsProbability(Rules.MaxFineChance)
        && IsProbability(Rules.MaxPristineChance)
        && Rules.FineFloorAtMasteryLevel >= 0
        && Rules.PristineFloorAtMasteryLevel >= 0
        && static_cast<double>(Rules.BaseFineChance)
                + static_cast<double>(Rules.BasePristineChance) <= 1.0
        && static_cast<double>(Rules.MaxFineChance)
                + static_cast<double>(Rules.MaxPristineChance) <= 1.0;
}

void AppendUInt32BigEndian(const uint32 Value, TArray<uint8> &OutBytes) {
    OutBytes.Add(static_cast<uint8>(Value >> 24));
    OutBytes.Add(static_cast<uint8>(Value >> 16));
    OutBytes.Add(static_cast<uint8>(Value >> 8));
    OutBytes.Add(static_cast<uint8>(Value));
}

void AppendGuidBigEndian(const FGuid &Guid, TArray<uint8> &OutBytes) {
    AppendUInt32BigEndian(Guid.A, OutBytes);
    AppendUInt32BigEndian(Guid.B, OutBytes);
    AppendUInt32BigEndian(Guid.C, OutBytes);
    AppendUInt32BigEndian(Guid.D, OutBytes);
}

uint64 ReadUInt64BigEndian(const uint8 *Bytes) {
    uint64 Value = 0;
    for (int32 Index = 0; Index < 8; ++Index) {
        Value = (Value << 8) | static_cast<uint64>(Bytes[Index]);
    }
    return Value;
}

uint64 DeriveSeed(const FGuid &WorldEpoch,
                  const FMythicHarvestNodeId &NodeId,
                  const uint32 Generation,
                  const EMythicHarvestRewardChannel Channel,
                  const int32 RewardRowIndex,
                  const ESeedPurpose Purpose,
                  const FString &ContributorKey) {
    if (!WorldEpoch.IsValid() || !NodeId.IsValid() || Generation == 0
        || RewardRowIndex < INDEX_NONE) {
        return 0;
    }

    FTCHARToUTF8 ContributorUtf8(*ContributorKey);
    const int32 ContributorByteCount = ContributorUtf8.Length();

    TArray<uint8> Bytes;
    Bytes.Reserve(UE_ARRAY_COUNT(CanonicalDomain) + 4 + 16 + 16 + 4 + 1 + 4 + 1 + 4
                  + ContributorByteCount);
    Bytes.Append(CanonicalDomain, UE_ARRAY_COUNT(CanonicalDomain));
    AppendUInt32BigEndian(FMythicHarvestRewardPlanner::CanonicalVersion, Bytes);
    AppendGuidBigEndian(WorldEpoch, Bytes);
    AppendGuidBigEndian(NodeId.GetGuid(), Bytes);
    AppendUInt32BigEndian(Generation, Bytes);
    Bytes.Add(static_cast<uint8>(Channel));
    AppendUInt32BigEndian(static_cast<uint32>(RewardRowIndex), Bytes);
    Bytes.Add(static_cast<uint8>(Purpose));
    AppendUInt32BigEndian(static_cast<uint32>(ContributorByteCount), Bytes);
    if (ContributorByteCount > 0) {
        Bytes.Append(reinterpret_cast<const uint8 *>(ContributorUtf8.Get()),
                     ContributorByteCount);
    }

    const FBlake3Hash Digest = FBlake3::HashBuffer(
        Bytes.GetData(), static_cast<uint64>(Bytes.Num()));
    const uint64 Seed = ReadUInt64BigEndian(Digest.GetBytes());
    // The item factory reserves zero to mean "derive from a random instance GUID".
    return Seed == 0 ? 0x9e3779b97f4a7c15ull : Seed;
}

uint64 NextSplitMix64(uint64 &State) {
    State += 0x9e3779b97f4a7c15ull;
    uint64 Value = State;
    Value = (Value ^ (Value >> 30)) * 0xbf58476d1ce4e5b9ull;
    Value = (Value ^ (Value >> 27)) * 0x94d049bb133111ebull;
    return Value ^ (Value >> 31);
}

uint64 UniformBelow(const uint64 Seed, const uint64 Bound) {
    check(Bound > 0);
    uint64 State = Seed;
    const uint64 RejectionThreshold = ((MAX_uint64 % Bound) + 1) % Bound;
    for (;;) {
        const uint64 Value = NextSplitMix64(State);
        if (Value >= RejectionThreshold) {
            return Value % Bound;
        }
    }
}

bool PassesProbability(const float Probability, const uint64 Seed) {
    if (!FMath::IsFinite(Probability) || Probability <= 0.0f) {
        return false;
    }
    if (Probability >= 1.0f) {
        return true;
    }

    constexpr double ProbabilityScale = 4294967296.0;
    const uint64 Threshold = static_cast<uint64>(
        static_cast<double>(Probability) * ProbabilityScale);
    uint64 State = Seed;
    const uint32 Draw = static_cast<uint32>(NextSplitMix64(State) >> 32);
    return static_cast<uint64>(Draw) < Threshold;
}

bool TryQuantizeWeight(const float Weight, uint64 &OutWeight) {
    constexpr double WeightScale = 1000000.0;
    constexpr double MaximumWeight =
        static_cast<double>(MAX_uint64 / 4) / WeightScale;
    if (!FMath::IsFinite(Weight) || Weight <= 0.0f
        || static_cast<double>(Weight) > MaximumWeight) {
        OutWeight = 0;
        return false;
    }
    OutWeight = FMath::Max<uint64>(
        1, static_cast<uint64>(FMath::RoundToDouble(
               static_cast<double>(Weight) * WeightScale)));
    return true;
}

uint64 GreatestCommonDivisor(uint64 A, uint64 B) {
    while (B != 0) {
        const uint64 Remainder = A % B;
        A = B;
        B = Remainder;
    }
    return A;
}

bool BuildCanonicalParticipants(
    const TConstArrayView<FMythicHarvestRewardParticipant> Participants,
    TArray<FCanonicalParticipant> &OutParticipants,
    EMythicHarvestRewardPlanStatus &OutStatus) {
    OutParticipants.Reset();
    OutParticipants.Reserve(Participants.Num());
    for (int32 Index = 0; Index < Participants.Num(); ++Index) {
        const FMythicHarvestRewardParticipant &Participant = Participants[Index];
        if (Participant.ContributorKey.IsEmpty()
            || Participant.ContributionQuanta <= 0 || Participant.ItemLevel < 1
            || Participant.QuantityMultiplierQuanta < 0
            || Participant.QuantityMultiplierQuanta
                > FMythicHarvestRewardPlanner::QuantityMultiplierScale
                    * FMythicHarvestRewardPlanner::MaximumQuantityMultiplier
            || Participant.ProficiencyLevel < 0) {
            OutStatus = EMythicHarvestRewardPlanStatus::InvalidContributor;
            return false;
        }
        OutParticipants.Add({&Participant, Index});
    }

    OutParticipants.Sort([](const FCanonicalParticipant &Left,
                            const FCanonicalParticipant &Right) {
        return Left.Participant->ContributorKey.Compare(
                   Right.Participant->ContributorKey,
                   ESearchCase::CaseSensitive) < 0;
    });

    for (int32 Index = 1; Index < OutParticipants.Num(); ++Index) {
        if (OutParticipants[Index - 1].Participant->ContributorKey
            == OutParticipants[Index].Participant->ContributorKey) {
            OutStatus = EMythicHarvestRewardPlanStatus::DuplicateContributor;
            return false;
        }
    }
    return !OutParticipants.IsEmpty();
}

// Computes floor(Multiplier * Value / Divisor) and its remainder without a
// wider-than-64-bit intermediate. Multiplier is an int32 reward quantity.
void MultiplyDivideWithRemainder(const uint32 Multiplier,
                                 const uint64 Value,
                                 const uint64 Divisor,
                                 uint64 &OutQuotient,
                                 uint64 &OutRemainder) {
    check(Divisor > 0 && Value <= Divisor);
    OutQuotient = 0;
    OutRemainder = 0;

    for (int32 BitIndex = 31; BitIndex >= 0; --BitIndex) {
        OutQuotient *= 2;
        if (OutRemainder >= Divisor - OutRemainder) {
            OutRemainder -= Divisor - OutRemainder;
            ++OutQuotient;
        }
        else {
            OutRemainder += OutRemainder;
        }

        if ((Multiplier & (1u << BitIndex)) != 0) {
            if (OutRemainder >= Divisor - Value) {
                OutRemainder -= Divisor - Value;
                ++OutQuotient;
            }
            else {
                OutRemainder += Value;
            }
        }
    }
}

bool ValidateRewardEntry(const FMythicHarvestRewardEntry &Entry) {
    if (!IsValid(Entry.ItemDefinition)
        || !IsValidQualityPolicy(Entry.QualityPolicy)
        || !IsValidQuality(Entry.FixedQuality)) {
        return false;
    }
    int32 QualityFragmentCount = 0;
    const UYieldQualityFragment *DefinitionQuality =
        FindYieldQualityFragment(*Entry.ItemDefinition, QualityFragmentCount);
    if (QualityFragmentCount > 1
        || (DefinitionQuality
            && (!IsValidQuality(DefinitionQuality->QualityTier)
                || DefinitionQuality->QualityTier
                    == EMythicYieldQuality::Ragged))) {
        return false;
    }
    if (Entry.QualityPolicy != EMythicHarvestRewardQualityPolicy::DefinitionDefault
        && (!DefinitionQuality
            || (Entry.QualityPolicy == EMythicHarvestRewardQualityPolicy::Fixed
                && Entry.FixedQuality == EMythicYieldQuality::Ragged))) {
        return false;
    }
    return Entry.MinQuantity > 0
        && Entry.MaxQuantity >= Entry.MinQuantity
        && FMath::IsFinite(Entry.Probability)
        && Entry.Probability >= 0.0f
        && Entry.Probability <= 1.0f
        && FMath::IsFinite(Entry.SelectionWeight)
        && Entry.SelectionWeight > 0.0f;
}

bool ResolveQuality(
    const FMythicHarvestRewardEntry &Entry,
    const FMythicYieldQualityRules &Rules,
    const FMythicHarvestRewardCompletionKey &CompletionKey,
    const EMythicHarvestRewardChannel Channel,
    const int32 RewardRowIndex,
    const FMythicHarvestRewardParticipant &Participant,
    bool &bOutHasResolvedQuality,
    EMythicYieldQuality &OutResolvedQuality) {
    bOutHasResolvedQuality = false;
    OutResolvedQuality = EMythicYieldQuality::Common;
    if (Entry.QualityPolicy
        == EMythicHarvestRewardQualityPolicy::DefinitionDefault) {
        int32 QualityFragmentCount = 0;
        const UYieldQualityFragment *DefinitionQuality =
            Entry.ItemDefinition
            ? FindYieldQualityFragment(
                  *Entry.ItemDefinition, QualityFragmentCount)
            : nullptr;
        if (QualityFragmentCount > 1) {
            return false;
        }
        if (!DefinitionQuality) {
            return true;
        }
        bOutHasResolvedQuality = true;
        OutResolvedQuality = DefinitionQuality->QualityTier;
        return IsValidQuality(OutResolvedQuality)
            && OutResolvedQuality != EMythicYieldQuality::Ragged;
    }
    if (Entry.QualityPolicy == EMythicHarvestRewardQualityPolicy::Fixed) {
        if (!IsValidQuality(Entry.FixedQuality)
            || Entry.FixedQuality == EMythicYieldQuality::Ragged) {
            return false;
        }
        bOutHasResolvedQuality = true;
        OutResolvedQuality = Entry.FixedQuality;
        return true;
    }
    if (Entry.QualityPolicy
            != EMythicHarvestRewardQualityPolicy::ContributorProficiency
        || Participant.ProficiencyLevel < 0 || !AreQualityRollRulesValid(Rules)) {
        return false;
    }

    const uint64 QualitySeed = DeriveSeed(
        CompletionKey.WorldEpoch, CompletionKey.NodeId,
        CompletionKey.Generation, Channel, RewardRowIndex,
        ESeedPurpose::Quality, Participant.ContributorKey);
    uint64 QualityState = QualitySeed;
    // The top 24 bits map exactly into a binary32 value in [0,1), avoiding
    // platform-dependent distribution changes from a full-width integer cast.
    const uint32 Draw24 = static_cast<uint32>(
        NextSplitMix64(QualityState) >> 40);
    const float Roll = static_cast<float>(Draw24) / 16777216.0f;
    bOutHasResolvedQuality = true;
    OutResolvedQuality = FMythicYieldQuality::RollQuality(
        Rules, Roll, Participant.ProficiencyLevel,
        EMythicYieldQuality::Common, EMythicYieldQuality::Common);
    return IsValidQuality(OutResolvedQuality)
        && OutResolvedQuality != EMythicYieldQuality::Ragged;
}

bool PlanChannel(
    const TArray<FMythicHarvestRewardEntry> &Entries,
    const EMythicHarvestRewardChannel Channel,
    const FMythicHarvestRewardCompletionKey &CompletionKey,
    const FMythicYieldQualityRules &QualityRules,
    const TArray<FCanonicalParticipant> &Participants,
    TArray<FMythicHarvestPlannedRewardGrant> &OutGrants,
    EMythicHarvestRewardPlanStatus &OutStatus,
    FName &OutDiagnosticCode) {
    TArray<FWeightedCandidate> Candidates;
    Candidates.Reserve(Entries.Num());

    for (int32 RowIndex = 0; RowIndex < Entries.Num(); ++RowIndex) {
        const FMythicHarvestRewardEntry &Entry = Entries[RowIndex];
        if (!ValidateRewardEntry(Entry)) {
            OutStatus = EMythicHarvestRewardPlanStatus::InvalidDefinition;
            OutDiagnosticCode = TEXT("InvalidRewardEntry");
            return false;
        }

        const uint64 EligibilitySeed = DeriveSeed(
            CompletionKey.WorldEpoch, CompletionKey.NodeId,
            CompletionKey.Generation, Channel, RowIndex,
            ESeedPurpose::Eligibility, FString());
        if (!PassesProbability(Entry.Probability, EligibilitySeed)) {
            continue;
        }

        uint64 QuantizedWeight = 0;
        if (!TryQuantizeWeight(Entry.SelectionWeight, QuantizedWeight)) {
            OutStatus = EMythicHarvestRewardPlanStatus::InvalidDefinition;
            OutDiagnosticCode = TEXT("UnquantizableRewardWeight");
            return false;
        }
        Candidates.Add({RowIndex, QuantizedWeight});
    }

    if (Candidates.IsEmpty()) {
        return true;
    }

    uint64 WeightGcd = Candidates[0].Weight;
    for (int32 Index = 1; Index < Candidates.Num(); ++Index) {
        WeightGcd = GreatestCommonDivisor(WeightGcd, Candidates[Index].Weight);
    }

    uint64 TotalWeight = 0;
    for (FWeightedCandidate &Candidate : Candidates) {
        Candidate.Weight /= FMath::Max<uint64>(1, WeightGcd);
        if (Candidate.Weight > MAX_uint64 - TotalWeight) {
            OutStatus = EMythicHarvestRewardPlanStatus::ArithmeticOverflow;
            OutDiagnosticCode = TEXT("RewardWeightOverflow");
            return false;
        }
        TotalWeight += Candidate.Weight;
    }

    const uint64 SelectionSeed = DeriveSeed(
        CompletionKey.WorldEpoch, CompletionKey.NodeId,
        CompletionKey.Generation, Channel, INDEX_NONE,
        ESeedPurpose::ChannelSelection, FString());
    uint64 Selection = UniformBelow(SelectionSeed, TotalWeight);
    int32 SelectedRowIndex = Candidates.Last().RowIndex;
    for (const FWeightedCandidate &Candidate : Candidates) {
        if (Selection < Candidate.Weight) {
            SelectedRowIndex = Candidate.RowIndex;
            break;
        }
        Selection -= Candidate.Weight;
    }

    const FMythicHarvestRewardEntry &SelectedEntry = Entries[SelectedRowIndex];
    const uint64 QuantitySeed = DeriveSeed(
        CompletionKey.WorldEpoch, CompletionKey.NodeId,
        CompletionKey.Generation, Channel,
        SelectedRowIndex, ESeedPurpose::Quantity, FString());
    const uint64 QuantityRange =
        static_cast<uint64>(SelectedEntry.MaxQuantity)
        - static_cast<uint64>(SelectedEntry.MinQuantity) + 1;
    const int32 Quantity = SelectedEntry.MinQuantity
        + static_cast<int32>(UniformBelow(QuantitySeed, QuantityRange));

    TArray<FMythicHarvestRewardParticipant> CanonicalParticipantValues;
    CanonicalParticipantValues.Reserve(Participants.Num());
    for (const FCanonicalParticipant &Canonical : Participants) {
        CanonicalParticipantValues.Add(*Canonical.Participant);
    }

    TArray<int32> Shares;
    const uint64 MultiplierRoundingSeed = DeriveSeed(
        CompletionKey.WorldEpoch, CompletionKey.NodeId,
        CompletionKey.Generation, Channel, SelectedRowIndex,
        ESeedPurpose::QuantityMultiplierRounding, FString());
    const bool bSplitSucceeded = Channel
            == EMythicHarvestRewardChannel::PrimaryMaterial
        ? FMythicHarvestRewardPlanner::SplitPrimaryMaterialQuantity(
              Quantity, MultiplierRoundingSeed,
              CanonicalParticipantValues, Shares)
        : FMythicHarvestRewardPlanner::SplitQuantityLargestRemainder(
              Quantity, CanonicalParticipantValues, Shares);
    if (!bSplitSucceeded) {
        OutStatus = EMythicHarvestRewardPlanStatus::ArithmeticOverflow;
        OutDiagnosticCode = TEXT("ContributionSplitFailed");
        return false;
    }

    for (int32 ParticipantIndex = 0;
         ParticipantIndex < CanonicalParticipantValues.Num();
         ++ParticipantIndex) {
        if (Shares[ParticipantIndex] <= 0) {
            continue;
        }

        const FMythicHarvestRewardParticipant &Participant =
            CanonicalParticipantValues[ParticipantIndex];
        FMythicHarvestPlannedRewardGrant &Grant = OutGrants.AddDefaulted_GetRef();
        Grant.CompletionKey = CompletionKey;
        Grant.Channel = Channel;
        Grant.RewardRowIndex = SelectedRowIndex;
        Grant.ContributorKey = Participant.ContributorKey;
        Grant.ItemDefinition = SelectedEntry.ItemDefinition;
        Grant.ItemDefinitionId = SelectedEntry.ItemDefinition->GetPrimaryAssetId();
        Grant.Quantity = Shares[ParticipantIndex];
        Grant.ItemLevel = Participant.ItemLevel;
        if (!ResolveQuality(
                SelectedEntry, QualityRules, CompletionKey, Channel,
                SelectedRowIndex, Participant, Grant.bHasResolvedQuality,
                Grant.ResolvedQuality)) {
            OutStatus = EMythicHarvestRewardPlanStatus::InvalidDefinition;
            OutDiagnosticCode = TEXT("InvalidRewardQualityPolicy");
            return false;
        }
        Grant.ItemSeed = FMythicHarvestRewardPlanner::DeriveItemSeed(
            CompletionKey.WorldEpoch, CompletionKey.NodeId,
            CompletionKey.Generation, Channel, SelectedRowIndex,
            Participant.ContributorKey);
        Grant.InitialController = Participant.InitialController;
    }

    return true;
}

} // namespace MythicHarvestRewardPlannerPrivate

namespace MHRewardPrivate = MythicHarvestRewardPlannerPrivate;

bool FMythicHarvestPlannedRewardGrant::IsValid() const {
    return CompletionKey.IsValid() && RewardRowIndex >= 0
        && !ContributorKey.IsEmpty() && ::IsValid(ItemDefinition.Get())
        && ItemDefinitionId.IsValid() && Quantity > 0 && ItemLevel > 0
        && ItemSeed != 0
        && MHRewardPrivate::IsValidQuality(ResolvedQuality)
        && (bHasResolvedQuality
            ? ResolvedQuality != EMythicYieldQuality::Ragged
            : ResolvedQuality == EMythicYieldQuality::Common);
}

FMythicHarvestRewardPlanResult FMythicHarvestRewardPlanner::PlanCompletion(
    const UMythicHarvestableDefinition &Definition,
    const FGuid &WorldEpoch,
    const FMythicHarvestNodeId &NodeId,
    const uint32 Generation,
    const FMythicYieldQualityRules &QualityRules,
    const TConstArrayView<FMythicHarvestRewardParticipant> Participants) {
    FMythicHarvestRewardPlanResult Result;
    const FMythicHarvestRewardCompletionKey CompletionKey{
        WorldEpoch, NodeId, Generation};
    if (!CompletionKey.IsValid()) {
        Result.Status = EMythicHarvestRewardPlanStatus::InvalidCompletion;
        Result.DiagnosticCode = TEXT("InvalidCompletionKey");
        return Result;
    }

    TArray<MHRewardPrivate::FCanonicalParticipant> CanonicalParticipants;
    if (!MHRewardPrivate::BuildCanonicalParticipants(
            Participants, CanonicalParticipants, Result.Status)) {
        Result.DiagnosticCode = Result.Status
                == EMythicHarvestRewardPlanStatus::DuplicateContributor
            ? FName(TEXT("DuplicateContributor"))
            : FName(TEXT("InvalidContributor"));
        return Result;
    }

    if (Definition.PrimaryMaterials.IsEmpty()) {
        Result.Status = EMythicHarvestRewardPlanStatus::InvalidDefinition;
        Result.DiagnosticCode = TEXT("MissingPrimaryRewards");
        return Result;
    }

    if (!MHRewardPrivate::PlanChannel(
            Definition.PrimaryMaterials,
            EMythicHarvestRewardChannel::PrimaryMaterial,
            CompletionKey, QualityRules, CanonicalParticipants, Result.Grants,
            Result.Status, Result.DiagnosticCode)
        || !MHRewardPrivate::PlanChannel(
            Definition.BonusLoot,
            EMythicHarvestRewardChannel::BonusLoot,
            CompletionKey, QualityRules, CanonicalParticipants, Result.Grants,
            Result.Status, Result.DiagnosticCode)) {
        Result.Grants.Reset();
        return Result;
    }

    Result.Status = EMythicHarvestRewardPlanStatus::Success;
    Result.DiagnosticCode = NAME_None;
    return Result;
}

bool FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
    const double Multiplier, int32 &OutMultiplierQuanta) {
    OutMultiplierQuanta = 0;
    if (!FMath::IsFinite(Multiplier) || Multiplier < 0.0
        || Multiplier > MaximumQuantityMultiplier) {
        return false;
    }
    const double Scaled = Multiplier
        * static_cast<double>(QuantityMultiplierScale);
    if (!FMath::IsFinite(Scaled)
        || Scaled > static_cast<double>(MAX_int32)) {
        return false;
    }
    OutMultiplierQuanta = static_cast<int32>(FMath::RoundToDouble(Scaled));
    return OutMultiplierQuanta >= 0;
}

bool FMythicHarvestRewardPlanner::SplitPrimaryMaterialQuantity(
    const int32 BaseQuantity,
    const uint64 RoundingSeed,
    const TConstArrayView<FMythicHarvestRewardParticipant> Participants,
    TArray<int32> &OutQuantities,
    TArray<int32> *OutCanonicalSourceIndices) {
    OutQuantities.Reset();
    if (OutCanonicalSourceIndices) {
        OutCanonicalSourceIndices->Reset();
    }
    if (BaseQuantity < 0 || RoundingSeed == 0 || Participants.IsEmpty()) {
        return false;
    }

    EMythicHarvestRewardPlanStatus UnusedStatus =
        EMythicHarvestRewardPlanStatus::Success;
    TArray<MHRewardPrivate::FCanonicalParticipant> Canonical;
    if (!MHRewardPrivate::BuildCanonicalParticipants(
            Participants, Canonical, UnusedStatus)) {
        return false;
    }

    uint64 TotalContribution = 0;
    for (const MHRewardPrivate::FCanonicalParticipant &Entry : Canonical) {
        const uint64 Contribution = static_cast<uint64>(
            Entry.Participant->ContributionQuanta);
        if (Contribution > MAX_uint64 - TotalContribution) {
            return false;
        }
        TotalContribution += Contribution;
    }
    if (TotalContribution == 0) {
        return false;
    }

    const MHRewardPrivate::FRewardBigUInt Denominator =
        MHRewardPrivate::MakeBigUInt(TotalContribution)
        * MHRewardPrivate::MakeBigUInt(
            static_cast<uint64>(QuantityMultiplierScale));
    if (Denominator.IsZero()) {
        return false;
    }

    struct FRemainderEntry {
        int32 CanonicalIndex = INDEX_NONE;
        MHRewardPrivate::FRewardBigUInt Remainder;
        FString ContributorKey;
    };

    OutQuantities.SetNumZeroed(Canonical.Num());
    TArray<FRemainderEntry> Remainders;
    Remainders.Reserve(Canonical.Num());
    MHRewardPrivate::FRewardBigUInt AggregateRemainder;
    int64 AssignedQuantity = 0;
    for (int32 Index = 0; Index < Canonical.Num(); ++Index) {
        const FMythicHarvestRewardParticipant &Participant =
            *Canonical[Index].Participant;
        MHRewardPrivate::FRewardBigUInt Numerator =
            MHRewardPrivate::MakeBigUInt(static_cast<uint64>(BaseQuantity))
            * MHRewardPrivate::MakeBigUInt(
                static_cast<uint64>(Participant.ContributionQuanta));
        Numerator *= MHRewardPrivate::MakeBigUInt(
            static_cast<uint64>(Participant.QuantityMultiplierQuanta));

        const MHRewardPrivate::FRewardBigUInt Quotient =
            Numerator / Denominator;
        const MHRewardPrivate::FRewardBigUInt Remainder =
            Numerator % Denominator;
        int32 WholeUnits = 0;
        if (!MHRewardPrivate::TryBigUIntToInt32(Quotient, WholeUnits)
            || AssignedQuantity > MAX_int64 - WholeUnits) {
            OutQuantities.Reset();
            if (OutCanonicalSourceIndices) {
                OutCanonicalSourceIndices->Reset();
            }
            return false;
        }
        OutQuantities[Index] = WholeUnits;
        AssignedQuantity += WholeUnits;
        AggregateRemainder += Remainder;
        Remainders.Add({Index, Remainder, Participant.ContributorKey});
        if (OutCanonicalSourceIndices) {
            OutCanonicalSourceIndices->Add(Canonical[Index].SourceIndex);
        }
    }

    const MHRewardPrivate::FRewardBigUInt WholeExtraBig =
        AggregateRemainder / Denominator;
    const MHRewardPrivate::FRewardBigUInt Residual =
        AggregateRemainder % Denominator;
    int32 WholeExtra = 0;
    if (!MHRewardPrivate::TryBigUIntToInt32(WholeExtraBig, WholeExtra)) {
        OutQuantities.Reset();
        if (OutCanonicalSourceIndices) {
            OutCanonicalSourceIndices->Reset();
        }
        return false;
    }

    bool bRoundResidualUp = false;
    if (!Residual.IsZero()) {
        uint64 State = RoundingSeed;
        const uint64 Draw = MHRewardPrivate::NextSplitMix64(State);
        const MHRewardPrivate::FRewardBigUInt DrawTimesDenominator =
            MHRewardPrivate::MakeBigUInt(Draw) * Denominator;
        const MHRewardPrivate::FRewardBigUInt ResidualTimesTwoTo64 =
            Residual << 64;
        bRoundResidualUp = DrawTimesDenominator < ResidualTimesTwoTo64;
    }

    const int32 UnitsToAllocate = WholeExtra + (bRoundResidualUp ? 1 : 0);
    if (UnitsToAllocate < 0 || UnitsToAllocate > Remainders.Num()) {
        OutQuantities.Reset();
        if (OutCanonicalSourceIndices) {
            OutCanonicalSourceIndices->Reset();
        }
        return false;
    }
    Remainders.Sort([](const FRemainderEntry &Left,
                       const FRemainderEntry &Right) {
        if (Left.Remainder != Right.Remainder) {
            return Left.Remainder > Right.Remainder;
        }
        return Left.ContributorKey.Compare(
                   Right.ContributorKey, ESearchCase::CaseSensitive) < 0;
    });
    for (int32 Index = 0; Index < UnitsToAllocate; ++Index) {
        int32 &Share = OutQuantities[Remainders[Index].CanonicalIndex];
        if (Share == MAX_int32) {
            OutQuantities.Reset();
            if (OutCanonicalSourceIndices) {
                OutCanonicalSourceIndices->Reset();
            }
            return false;
        }
        ++Share;
    }
    return true;
}

bool FMythicHarvestRewardPlanner::SplitQuantityLargestRemainder(
    const int32 Quantity,
    const TConstArrayView<FMythicHarvestRewardParticipant> Participants,
    TArray<int32> &OutQuantities,
    TArray<int32> *OutCanonicalSourceIndices) {
    OutQuantities.Reset();
    if (OutCanonicalSourceIndices) {
        OutCanonicalSourceIndices->Reset();
    }
    if (Quantity < 0 || Participants.IsEmpty()) {
        return false;
    }

    EMythicHarvestRewardPlanStatus UnusedStatus =
        EMythicHarvestRewardPlanStatus::Success;
    TArray<MHRewardPrivate::FCanonicalParticipant> Canonical;
    if (!MHRewardPrivate::BuildCanonicalParticipants(
            Participants, Canonical, UnusedStatus)) {
        return false;
    }

    uint64 TotalWeight = 0;
    for (const MHRewardPrivate::FCanonicalParticipant &Entry : Canonical) {
        const uint64 Weight = static_cast<uint64>(
            Entry.Participant->ContributionQuanta);
        if (Weight > MAX_uint64 - TotalWeight) {
            return false;
        }
        TotalWeight += Weight;
    }
    if (TotalWeight == 0) {
        return false;
    }

    struct FRemainderEntry {
        int32 CanonicalIndex = INDEX_NONE;
        uint64 Remainder = 0;
        FString ContributorKey;
    };

    OutQuantities.SetNumZeroed(Canonical.Num());
    TArray<FRemainderEntry> Remainders;
    Remainders.Reserve(Canonical.Num());
    int64 AssignedQuantity = 0;
    for (int32 Index = 0; Index < Canonical.Num(); ++Index) {
        const uint64 Weight = static_cast<uint64>(
            Canonical[Index].Participant->ContributionQuanta);
        uint64 Share = 0;
        uint64 Remainder = 0;
        MHRewardPrivate::MultiplyDivideWithRemainder(
            static_cast<uint32>(Quantity), Weight, TotalWeight, Share,
            Remainder);
        if (Share > static_cast<uint64>(MAX_int32)
            || AssignedQuantity > MAX_int64 - static_cast<int64>(Share)) {
            OutQuantities.Reset();
            return false;
        }
        OutQuantities[Index] = static_cast<int32>(Share);
        AssignedQuantity += static_cast<int64>(Share);
        Remainders.Add({Index, Remainder,
                        Canonical[Index].Participant->ContributorKey});
        if (OutCanonicalSourceIndices) {
            OutCanonicalSourceIndices->Add(Canonical[Index].SourceIndex);
        }
    }

    Remainders.Sort([](const FRemainderEntry &Left,
                       const FRemainderEntry &Right) {
        if (Left.Remainder != Right.Remainder) {
            return Left.Remainder > Right.Remainder;
        }
        return Left.ContributorKey.Compare(Right.ContributorKey,
                                           ESearchCase::CaseSensitive) < 0;
    });

    const int64 UnitsRemaining = static_cast<int64>(Quantity) - AssignedQuantity;
    if (UnitsRemaining < 0 || UnitsRemaining > Remainders.Num()) {
        OutQuantities.Reset();
        if (OutCanonicalSourceIndices) {
            OutCanonicalSourceIndices->Reset();
        }
        return false;
    }
    for (int64 Index = 0; Index < UnitsRemaining; ++Index) {
        ++OutQuantities[Remainders[static_cast<int32>(Index)].CanonicalIndex];
    }
    return true;
}

uint64 FMythicHarvestRewardPlanner::DeriveItemSeed(
    const FGuid &WorldEpoch,
    const FMythicHarvestNodeId &NodeId,
    const uint32 Generation,
    const EMythicHarvestRewardChannel Channel,
    const int32 RewardRowIndex,
    const FString &ContributorKey) {
    if (ContributorKey.IsEmpty() || RewardRowIndex < 0) {
        return 0;
    }
    return MHRewardPrivate::DeriveSeed(
        WorldEpoch, NodeId, Generation, Channel, RewardRowIndex,
        MHRewardPrivate::ESeedPurpose::ItemFactory, ContributorKey);
}
