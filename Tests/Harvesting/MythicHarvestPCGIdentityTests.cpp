#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/Harvesting/MythicHarvestPCGIdentity.h"

namespace {

FMythicHarvestPCGIdentityInput MakeInput(const int64 PointIdentity) {
    FMythicHarvestPCGIdentityInput Input;
    Input.ProviderGuid = FGuid(0x11223344, 0x55667788, 0x99aabbcc,
                               0xddeeff01);
    Input.DomainGuid = FGuid(0x13579bdf, 0x2468ace0, 0x0badf00d,
                             0xc001d00d);
    Input.PointIdentity = PointIdentity;
    return Input;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPCGPointIdentityTest,
    "Mythic.Harvesting.PCGIdentity.TypedPointInput",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPCGPointIdentityTest::RunTest(
    const FString &Parameters) {
    int64 First = INDEX_NONE;
    int64 Second = INDEX_NONE;
    TestTrue(TEXT("typed native seed/position identity builds"),
             MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                 1234, FVector(100.2, -42.6, 9000.49), First));
    TestTrue(TEXT("same typed input builds again"),
             MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                 1234, FVector(100.2, -42.6, 9000.49), Second));
    TestEqual(TEXT("typed identity is deterministic"), First, Second);
    TestTrue(TEXT("typed identity is non-negative"), First >= 0);

    int64 SubCentimeter = INDEX_NONE;
    TestTrue(TEXT("sub-centimeter input builds"),
             MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                 1234, FVector(100.3, -42.51, 9000.4), SubCentimeter));
    TestEqual(TEXT("whole-centimeter quantization resists float noise"),
              First, SubCentimeter);

    int64 Moved = INDEX_NONE;
    TestTrue(TEXT("moved point builds"),
             MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                 1234, FVector(101.2, -42.6, 9000.49), Moved));
    TestNotEqual(TEXT("deliberate point move creates a new identity"),
                 First, Moved);

    int64 Reseeded = INDEX_NONE;
    TestTrue(TEXT("reseeded point builds"),
             MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                 1235, FVector(100.2, -42.6, 9000.49), Reseeded));
    TestNotEqual(TEXT("native point seed participates"), First, Reseeded);

    int64 Invalid = 7;
    TestFalse(TEXT("nonfinite position fails closed"),
              MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                  1234, FVector(NAN, 0.0, 0.0), Invalid));
    TestEqual(TEXT("failed typed identity clears output"), Invalid,
              static_cast<int64>(INDEX_NONE));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPCGIdentityRoundTripTest,
    "Mythic.Harvesting.PCGIdentity.RoundTrip",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPCGIdentityRoundTripTest::RunTest(
    const FString &Parameters) {
    const FMythicHarvestNodeId Original(
        FGuid(0x0000ffff, 0x00010000, 0x7fffffff, 0xffffffff));

    TArray<float> Packed;
    TestTrue(TEXT("valid id packs"),
             MythicHarvestPCGIdentity::AppendPackedNodeId(Original, Packed));
    TestEqual(TEXT("one id uses exactly eight floats"), Packed.Num(),
              MythicHarvestPCGIdentity::PackedFloatCount);
    const TArray<float> ExpectedPacked = {
        0.0f, 65535.0f, 1.0f, 0.0f,
        32767.0f, 65535.0f, 65535.0f, 65535.0f};
    TestTrue(TEXT("GUID words use stable high-half/low-half order"),
             Packed == ExpectedPacked);
    for (const float Value : Packed) {
        TestTrue(TEXT("every packed half is an exact uint16"),
                 FMath::IsFinite(Value) && Value >= 0.0f && Value <= 65535.0f
                     && FMath::FloorToFloat(Value) == Value);
    }

    FMythicHarvestNodeId Decoded;
    TestTrue(TEXT("packed id decodes"),
             MythicHarvestPCGIdentity::TryDecodePackedNodeId(Packed, Decoded));
    TestTrue(TEXT("round trip preserves all guid bits"), Original == Decoded);

    TArray<float> Fractional = Packed;
    Fractional[3] += 0.5f;
    TestFalse(TEXT("fractional custom data is rejected"),
              MythicHarvestPCGIdentity::TryDecodePackedNodeId(Fractional,
                                                              Decoded));
    TestFalse(TEXT("decode failure clears output"), Decoded.IsValid());

    TArray<float> Zeroed;
    Zeroed.SetNumZeroed(MythicHarvestPCGIdentity::PackedFloatCount);
    TestFalse(TEXT("zero guid is rejected"),
              MythicHarvestPCGIdentity::TryDecodePackedNodeId(Zeroed,
                                                              Decoded));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPCGIdentityOrderTest,
    "Mythic.Harvesting.PCGIdentity.OrderIndependent",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPCGIdentityOrderTest::RunTest(const FString &Parameters) {
    const TArray<FMythicHarvestPCGIdentityInput> OriginalInputs = {
        MakeInput(17), MakeInput(42), MakeInput(9001)};
    TArray<FMythicHarvestNodeId> OriginalIds;
    const FMythicHarvestPCGIdentityValidationResult OriginalValidation =
        MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(OriginalInputs,
                                                          OriginalIds);
    TestTrue(TEXT("original inputs validate"), OriginalValidation.IsValid());
    TestEqual(TEXT("original ids preserve input count"), OriginalIds.Num(), 3);

    const TArray<FMythicHarvestPCGIdentityInput> ReorderedInputs = {
        OriginalInputs[2], OriginalInputs[0], OriginalInputs[1]};
    TArray<FMythicHarvestNodeId> ReorderedIds;
    const FMythicHarvestPCGIdentityValidationResult ReorderedValidation =
        MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(ReorderedInputs,
                                                          ReorderedIds);
    TestTrue(TEXT("reordered inputs validate"), ReorderedValidation.IsValid());
    TestTrue(TEXT("point 9001 id ignores packed order"),
             ReorderedIds[0] == OriginalIds[2]);
    TestTrue(TEXT("point 17 id ignores packed order"),
             ReorderedIds[1] == OriginalIds[0]);
    TestTrue(TEXT("point 42 id ignores packed order"),
             ReorderedIds[2] == OriginalIds[1]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPCGIdentityDeterminismTest,
    "Mythic.Harvesting.PCGIdentity.DeterminismAndValidation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPCGIdentityDeterminismTest::RunTest(
    const FString &Parameters) {
    const FMythicHarvestPCGIdentityInput Input = MakeInput(123456789);
    TArray<uint8> CanonicalBytes;
    TestTrue(TEXT("canonical bytes build"),
             MythicHarvestPCGIdentity::BuildCanonicalBytes(Input,
                                                           CanonicalBytes));
    constexpr int32 FixedPayloadByteCount = sizeof(uint32)
        + sizeof(uint32) * 4 * 2 + sizeof(uint64);
    const int32 PayloadOffset = CanonicalBytes.Num() - FixedPayloadByteCount;
    TestTrue(TEXT("canonical protocol domain precedes the fixed payload"),
             PayloadOffset > 0);
    if (PayloadOffset > 0) {
        TestEqual(TEXT("canonical domain is NUL terminated"),
                  CanonicalBytes[PayloadOffset - 1], static_cast<uint8>(0));
        TestEqual(TEXT("version byte 0"), CanonicalBytes[PayloadOffset],
                  static_cast<uint8>(0));
        TestEqual(TEXT("version byte 1"), CanonicalBytes[PayloadOffset + 1],
                  static_cast<uint8>(0));
        TestEqual(TEXT("version byte 2"), CanonicalBytes[PayloadOffset + 2],
                  static_cast<uint8>(0));
        TestEqual(TEXT("version byte 3"), CanonicalBytes[PayloadOffset + 3],
                  static_cast<uint8>(1));
        TestEqual(TEXT("provider word is big endian"),
                  CanonicalBytes[PayloadOffset + 4], static_cast<uint8>(0x11));
        TestEqual(TEXT("domain follows all provider words"),
                  CanonicalBytes[PayloadOffset + 20], static_cast<uint8>(0x13));
        TestEqual(TEXT("point identity high byte is stable"),
                  CanonicalBytes[CanonicalBytes.Num() - 8],
                  static_cast<uint8>(0));
        TestEqual(TEXT("point identity low byte is stable"),
                  CanonicalBytes.Last(), static_cast<uint8>(0x15));
    }

    FMythicHarvestNodeId First;
    FMythicHarvestNodeId Second;
    TestTrue(TEXT("first deterministic build succeeds"),
             MythicHarvestPCGIdentity::TryBuildNodeId(Input, First));
    TestTrue(TEXT("second deterministic build succeeds"),
             MythicHarvestPCGIdentity::TryBuildNodeId(Input, Second));
    TestTrue(TEXT("same canonical input produces same id"), First == Second);

    FMythicHarvestPCGIdentityInput ChangedDomain = Input;
    ChangedDomain.DomainGuid.D ^= 1u;
    FMythicHarvestNodeId DomainId;
    TestTrue(TEXT("changed domain builds"),
             MythicHarvestPCGIdentity::TryBuildNodeId(ChangedDomain,
                                                      DomainId));
    TestTrue(TEXT("domain participates in identity"), DomainId != First);

    FMythicHarvestPCGIdentityInput ChangedProvider = Input;
    ChangedProvider.ProviderGuid.A ^= 1u;
    FMythicHarvestNodeId ProviderId;
    TestTrue(TEXT("changed provider builds"),
             MythicHarvestPCGIdentity::TryBuildNodeId(ChangedProvider,
                                                      ProviderId));
    TestTrue(TEXT("provider participates in identity"), ProviderId != First);

    TArray<FMythicHarvestNodeId> BuiltIds;
    const TArray<FMythicHarvestPCGIdentityInput> DuplicateInputs = {Input,
                                                                    Input};
    const FMythicHarvestPCGIdentityValidationResult DuplicateValidation =
        MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(DuplicateInputs,
                                                          BuiltIds);
    TestEqual(TEXT("duplicates fail closed"), DuplicateValidation.Error,
              EMythicHarvestPCGIdentityError::DuplicateStableIdentity);
    TestEqual(TEXT("duplicate reports first index"),
              DuplicateValidation.ConflictingIndex, 0);
    TestEqual(TEXT("duplicate reports repeated index"),
              DuplicateValidation.FailureIndex, 1);
    TestTrue(TEXT("duplicate batch returns no partial ids"),
             BuiltIds.IsEmpty());

    FMythicHarvestPCGIdentityInput Invalid = Input;
    Invalid.PointIdentity = INDEX_NONE;
    const FMythicHarvestPCGIdentityValidationResult InvalidValidation =
        MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(
            MakeArrayView(&Invalid, 1), BuiltIds);
    TestEqual(TEXT("negative point identity is invalid"),
              InvalidValidation.Error,
              EMythicHarvestPCGIdentityError::InvalidPointIdentity);
    TestTrue(TEXT("invalid batch returns no ids"), BuiltIds.IsEmpty());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
