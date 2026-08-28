#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/MythicResourceISM.h"
#include "UObject/Package.h"
#include "World/Harvesting/MythicHarvestAuthoredIdentity.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"

namespace {

const FGuid AuthoredNodeSetGuid(0x10203040, 0x50607080, 0x90a0b0c0,
                                0xd0e0f001);
const FGuid FirstInstanceGuid(0x11112222, 0x33334444, 0x55556666,
                              0x77778888);
const FGuid SecondInstanceGuid(0x9999aaaa, 0xbbbbcccc, 0xddddeeee,
                               0xffff0001);

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestAuthoredIdentityDeterminismTest,
    "Mythic.Harvesting.AuthoredIdentity.DeterminismAndScope",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestAuthoredIdentityDeterminismTest::RunTest(
    const FString &Parameters) {
    const FMythicHarvestAuthoredIdentityInput Input{
        AuthoredNodeSetGuid, FirstInstanceGuid};
    TArray<uint8> CanonicalBytes;
    TestTrue(TEXT("canonical authored bytes build"),
             MythicHarvestAuthoredIdentity::BuildCanonicalBytes(
                 Input, CanonicalBytes));
    constexpr int32 FixedPayloadByteCount = sizeof(uint32)
        + sizeof(uint32) * 4 * 2;
    const int32 PayloadOffset =
        CanonicalBytes.Num() - FixedPayloadByteCount;
    TestTrue(TEXT("canonical protocol domain precedes typed payload"),
             PayloadOffset > 0);
    if (PayloadOffset > 0) {
        TestEqual(TEXT("canonical domain is NUL terminated"),
                  CanonicalBytes[PayloadOffset - 1], static_cast<uint8>(0));
        TestEqual(TEXT("canonical version is frozen at one"),
                  CanonicalBytes[PayloadOffset + 3], static_cast<uint8>(1));
        TestEqual(TEXT("node-set word uses big-endian ordering"),
                  CanonicalBytes[PayloadOffset + 4],
                  static_cast<uint8>(0x10));
        TestEqual(TEXT("instance guid follows node-set guid"),
                  CanonicalBytes[PayloadOffset + 20],
                  static_cast<uint8>(0x11));
    }

    FMythicHarvestNodeId First;
    FMythicHarvestNodeId Repeated;
    TestTrue(TEXT("first authored node builds"),
             MythicHarvestAuthoredIdentity::TryBuildNodeId(Input, First));
    TestTrue(TEXT("same authored node builds again"),
             MythicHarvestAuthoredIdentity::TryBuildNodeId(Input, Repeated));
    TestTrue(TEXT("same typed input is deterministic"), First == Repeated);

    FMythicHarvestNodeId ChangedSet;
    TestTrue(TEXT("different node set builds"),
             MythicHarvestAuthoredIdentity::TryBuildNodeId(
                 {FGuid(0x10203041, 0x50607080, 0x90a0b0c0, 0xd0e0f001),
                  FirstInstanceGuid},
                 ChangedSet));
    TestTrue(TEXT("node-set namespace participates"), ChangedSet != First);

    FMythicHarvestNodeId ChangedInstance;
    TestTrue(TEXT("different instance builds"),
             MythicHarvestAuthoredIdentity::TryBuildNodeId(
                 {AuthoredNodeSetGuid, SecondInstanceGuid}, ChangedInstance));
    TestTrue(TEXT("per-instance guid participates"),
             ChangedInstance != First);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestAuthoredIdentityBatchTest,
    "Mythic.Harvesting.AuthoredIdentity.OrderAndDuplicateGate",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestAuthoredIdentityBatchTest::RunTest(
    const FString &Parameters) {
    const TArray<FGuid> OriginalGuids = {FirstInstanceGuid,
                                         SecondInstanceGuid};
    TArray<FMythicHarvestNodeId> OriginalIds;
    const FMythicHarvestAuthoredIdentityValidationResult OriginalResult =
        MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
            AuthoredNodeSetGuid, OriginalGuids, OriginalIds);
    TestTrue(TEXT("original authored batch validates"),
             OriginalResult.IsValid());
    TestEqual(TEXT("original batch produces every id"), OriginalIds.Num(), 2);

    const TArray<FGuid> ReorderedGuids = {SecondInstanceGuid,
                                          FirstInstanceGuid};
    TArray<FMythicHarvestNodeId> ReorderedIds;
    const FMythicHarvestAuthoredIdentityValidationResult ReorderedResult =
        MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
            AuthoredNodeSetGuid, ReorderedGuids, ReorderedIds);
    TestTrue(TEXT("reordered authored batch validates"),
             ReorderedResult.IsValid());
    TestTrue(TEXT("second instance identity travels with its GUID"),
             ReorderedIds[0] == OriginalIds[1]);
    TestTrue(TEXT("first instance identity travels with its GUID"),
             ReorderedIds[1] == OriginalIds[0]);

    const TArray<FGuid> DuplicateGuids = {FirstInstanceGuid,
                                          FirstInstanceGuid};
    const FMythicHarvestAuthoredIdentityValidationResult DuplicateResult =
        MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
            AuthoredNodeSetGuid, DuplicateGuids, ReorderedIds);
    TestEqual(TEXT("duplicate raw GUID rejects complete batch"),
              DuplicateResult.Error,
              EMythicHarvestAuthoredIdentityError::DuplicateInstanceGuid);
    TestEqual(TEXT("duplicate reports original row"),
              DuplicateResult.ConflictingIndex, 0);
    TestEqual(TEXT("duplicate reports repeated row"),
              DuplicateResult.FailureIndex, 1);
    TestTrue(TEXT("duplicate returns no partial node ids"),
             ReorderedIds.IsEmpty());

    const FGuid InvalidGuid;
    const FMythicHarvestAuthoredIdentityValidationResult InvalidResult =
        MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
            AuthoredNodeSetGuid, MakeArrayView(&InvalidGuid, 1),
            ReorderedIds);
    TestEqual(TEXT("missing per-instance GUID fails closed"),
              InvalidResult.Error,
              EMythicHarvestAuthoredIdentityError::InvalidInstanceGuid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestAuthoredIdentityPackingTest,
    "Mythic.Harvesting.AuthoredIdentity.PackedPersistence",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestAuthoredIdentityPackingTest::RunTest(
    const FString &Parameters) {
    TArray<float> Packed;
    TestTrue(TEXT("persistent per-instance GUID packs"),
             MythicHarvestAuthoredIdentity::AppendPackedInstanceGuid(
                 FirstInstanceGuid, Packed));
    TestEqual(TEXT("authored GUID uses frozen eight-float encoding"),
              Packed.Num(), MythicHarvestPCGIdentity::PackedFloatCount);
    FGuid Decoded;
    TestTrue(TEXT("persistent per-instance GUID decodes"),
             MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
                 Packed, Decoded));
    TestEqual(TEXT("packing preserves every authored GUID bit"), Decoded,
              FirstInstanceGuid);
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestAuthoredIdentityBakeTest,
    "Mythic.Harvesting.AuthoredIdentity.EditorBakePreservesData",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestAuthoredIdentityBakeTest::RunTest(
    const FString &Parameters) {
    UMythicResourceISM *Component =
        NewObject<UMythicResourceISM>(GetTransientPackage());
    Component->IdentitySource = EMythicHarvestIdentitySource::EditorAuthored;
    Component->AuthoredNodeSetGuid = AuthoredNodeSetGuid;
    Component->IdentityCustomDataStartIndex = 2;
    Component->SetNumCustomDataFloats(10);
    Component->AddInstance(FTransform::Identity);
    Component->AddInstance(FTransform(FVector(100.0, 0.0, 0.0)));

    TArray<float> FirstRow;
    FirstRow.Add(7.0f);
    FirstRow.Add(9.0f);
    TestTrue(TEXT("first valid GUID packs into reserved offset"),
             MythicHarvestAuthoredIdentity::AppendPackedInstanceGuid(
                 FirstInstanceGuid, FirstRow));
    TArray<float> MissingRow;
    MissingRow.SetNumZeroed(10);
    MissingRow[0] = 11.0f;
    MissingRow[1] = 13.0f;
    TestTrue(TEXT("first row custom data sets"),
             Component->SetCustomData(0, FirstRow, false));
    TestTrue(TEXT("second row custom data sets"),
             Component->SetCustomData(1, MissingRow, false));

    TestTrue(TEXT("editor bake succeeds"),
             Component->TryBakeMissingAuthoredIdentities());
    TestEqual(TEXT("material custom data survives first row"),
              Component->PerInstanceSMCustomData[0], 7.0f);
    TestEqual(TEXT("material custom data survives second row"),
              Component->PerInstanceSMCustomData[10], 11.0f);

    FGuid BakedFirst;
    FGuid BakedSecond;
    TestTrue(TEXT("first baked row decodes"),
             MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
                 MakeArrayView(Component->PerInstanceSMCustomData.GetData() + 2,
                               8),
                 BakedFirst));
    TestTrue(TEXT("second baked row decodes"),
             MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
                 MakeArrayView(Component->PerInstanceSMCustomData.GetData() + 12,
                               8),
                 BakedSecond));
    TestEqual(TEXT("valid authored identity is preserved"), BakedFirst,
              FirstInstanceGuid);
    TestTrue(TEXT("missing authored identity is created in editor"),
             BakedSecond.IsValid());
    TestNotEqual(TEXT("new authored identity is unique"), BakedSecond,
                 BakedFirst);

    const TArray<float> FirstBakeSnapshot =
        Component->PerInstanceSMCustomData;
    TestTrue(TEXT("idempotent second bake succeeds"),
             Component->TryBakeMissingAuthoredIdentities());
    TestTrue(TEXT("idempotent second bake does not drift data"),
             FirstBakeSnapshot == Component->PerInstanceSMCustomData);

    UMythicResourceISM *DuplicateComponent =
        NewObject<UMythicResourceISM>(GetTransientPackage());
    DuplicateComponent->IdentitySource =
        EMythicHarvestIdentitySource::EditorAuthored;
    DuplicateComponent->IdentityCustomDataStartIndex = 0;
    DuplicateComponent->SetNumCustomDataFloats(8);
    DuplicateComponent->AddInstance(FTransform::Identity);
    DuplicateComponent->AddInstance(FTransform(FVector(200.0, 0.0, 0.0)));
    TArray<float> DuplicateRow;
    TestTrue(TEXT("duplicate test GUID packs"),
             MythicHarvestAuthoredIdentity::AppendPackedInstanceGuid(
                 FirstInstanceGuid, DuplicateRow));
    DuplicateComponent->SetCustomData(0, DuplicateRow, false);
    DuplicateComponent->SetCustomData(1, DuplicateRow, false);
    const TArray<float> BeforeRejectedBake =
        DuplicateComponent->PerInstanceSMCustomData;
    AddExpectedError(TEXT("valid authored GUID is duplicated"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("valid duplicate rejects complete bake"),
              DuplicateComponent->TryBakeMissingAuthoredIdentities());
    TestFalse(TEXT("rejected bake does not create node-set identity"),
              DuplicateComponent->AuthoredNodeSetGuid.IsValid());
    TestTrue(TEXT("rejected bake does not mutate custom data"),
             BeforeRejectedBake
                 == DuplicateComponent->PerInstanceSMCustomData);
    return true;
}
#endif // WITH_EDITOR

#endif // WITH_DEV_AUTOMATION_TESTS
