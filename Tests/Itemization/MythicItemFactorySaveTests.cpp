#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemFactorySubsystem.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerController.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace {
const FGuid TestItemFragmentFrameMagic(0x4D594954, 0x454D4652, 0x41474D31, 0xB729D06C);

int32 FindBytes(const TArray<uint8> &Haystack, const TArray<uint8> &Needle) {
    if (Needle.IsEmpty() || Needle.Num() > Haystack.Num()) return INDEX_NONE;
    for (int32 Start = 0; Start <= Haystack.Num() - Needle.Num(); ++Start) {
        if (FMemory::Memcmp(Haystack.GetData() + Start, Needle.GetData(), Needle.Num()) == 0) return Start;
    }
    return INDEX_NONE;
}

TArray<uint8> SerializeEmptyCompleteItem(AActor *Owner) {
    UMythicItemInstance *Item = NewObject<UMythicItemInstance>(Owner);
    Item->SetOwner(Owner);
    UItemDefinition *Definition = NewObject<UItemDefinition>(Owner);
    Definition->StackSizeMax = 1;
    Item->InitializeFixtureForTests(Definition, 1, 1);
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes, true);
    FObjectAndNameAsStringProxyArchive Archive(Writer, false);
    Archive.ArIsSaveGame = true;
    Item->Serialize(Archive);
    return Bytes;
}

int32 FindFrameOffset(const TArray<uint8> &ItemBytes) {
    TArray<uint8> MarkerBytes;
    FMemoryWriter Writer(MarkerBytes, true);
    FGuid Marker = TestItemFragmentFrameMagic;
    Writer << Marker;
    return FindBytes(ItemBytes, MarkerBytes);
}

bool CompleteItemLoadHasError(const TArray<uint8> &Bytes, AActor *Owner) {
    UMythicItemInstance *Restored = NewObject<UMythicItemInstance>(Owner);
    Restored->SetOwner(Owner);
    FMemoryReader Reader(Bytes, true);
    FObjectAndNameAsStringProxyArchive Archive(Reader, true);
    Archive.ArIsSaveGame = true;
    Restored->Serialize(Archive);
    return Archive.IsError();
}

TArray<uint8> BuildOneFragmentFrame(const TArray<uint8> &CompleteItemPrefix,
                                    const int32 FrameOffset,
                                    const FString &ClassPath,
                                    TConstArrayView<uint8> Payload) {
    TArray<uint8> Result;
    Result.Append(CompleteItemPrefix.GetData(), FrameOffset);
    FMemoryWriter Writer(Result, true);
    Writer.Seek(FrameOffset);
    FGuid Marker = TestItemFragmentFrameMagic;
    int32 Version = 1;
    int32 Count = 1;
    Writer << Marker;
    Writer << Version;
    Writer << Count;
    const FTCHARToUTF8 Utf8(*ClassPath);
    int32 ClassPathBytes = Utf8.Length();
    Writer << ClassPathBytes;
    Writer.Serialize(const_cast<ANSICHAR *>(Utf8.Get()), ClassPathBytes);
    int32 PayloadBytes = Payload.Num();
    Writer << PayloadBytes;
    if (PayloadBytes > 0) Writer.Serialize(const_cast<uint8 *>(Payload.GetData()), PayloadBytes);
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemFactoryReadinessTest,
    "Mythic.Itemization.Factory.ReadinessIsExactAndPrewarmedOnly",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemFactoryReadinessTest::RunTest(const FString &Parameters) {
    TSharedPtr<const FCompiledAffixProfile> Compiled;
    FName Diagnostic;

    TestEqual(TEXT("missing definition is invalid data"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  nullptr, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::InvalidData);

    UItemDefinition *PlainDefinition = NewObject<UItemDefinition>();
    TestEqual(TEXT("an item without affixes needs no itemization closure"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  PlainDefinition, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::Success);

    UItemDefinition *InvalidAffixDefinition = NewObject<UItemDefinition>();
    UAffixesFragment *InvalidAffixes = NewObject<UAffixesFragment>(InvalidAffixDefinition);
    InvalidAffixDefinition->Fragments.Add(InvalidAffixes);
    TestEqual(TEXT("an affix fragment without its one profile is invalid data"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  InvalidAffixDefinition, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::InvalidData);

    UMythicAffixProfile *Profile =
        NewObject<UMythicAffixProfile>(GetTransientPackage());
    Profile->ProfileTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.AffixProfile.GlobalWeapon.S0")), true);
    InvalidAffixes->AffixesConfig.AffixProfile.SetAsset(Profile);
    TestEqual(TEXT("a concrete profile is NotReady until the active ruleset is prewarmed"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  InvalidAffixDefinition, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::NotReady);
    TestEqual(TEXT("NotReady has a stable diagnostic code"), Diagnostic,
              FName(TEXT("ActiveRulesetNotReady")));

    UItemDefinition *GemDefinition = NewObject<UItemDefinition>();
    UMythicGemFragment *Gem = NewObject<UMythicGemFragment>(GemDefinition);
    GemDefinition->Fragments.Add(Gem);
    FMythicAffixGrantSpec &Grant = Gem->GrantSpecs.AddDefaulted_GetRef();
    Grant.GrantGuid = FGuid(1, 2, 3, 4);
    UMythicAffixDefinition *GrantDefinition =
        NewObject<UMythicAffixDefinition>(GetTransientPackage());
    GrantDefinition->AffixTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.Affix.Power")), true);
    Grant.AffixDefinition.SetAsset(GrantDefinition);
    Grant.TierMode = EMythicAffixGrantTierMode::ExactTier;
    Grant.ExactTierRank = 2;
    TestEqual(TEXT("a structurally valid gem is NotReady until its exact grant closure is loaded"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  GemDefinition, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::NotReady);
    TestEqual(TEXT("gem NotReady has a stable diagnostic code"), Diagnostic,
              FName(TEXT("GemGrantClosureNotReady")));

    Grant.ExactTierRank = 0;
    TestEqual(TEXT("a gem without a positive exact tier rank is invalid"),
              UMythicItemFactorySubsystem::EvaluateDefinitionReadyState(
                  GemDefinition, nullptr, Compiled, Diagnostic),
              EMythicCreateItemStatus::InvalidData);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemFragmentFrameFailureTest,
    "Mythic.Itemization.Save.CompleteItemFragmentFramesFailClosed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemFragmentFrameFailureTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) return false;
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };
    AActor *Owner = GameInstance->GetWorld()->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("authority item owner exists"), Owner)) return false;

    const TArray<uint8> Valid = SerializeEmptyCompleteItem(Owner);
    const int32 FrameOffset = FindFrameOffset(Valid);
    TestTrue(TEXT("complete item save contains the versioned fragment marker"), FrameOffset != INDEX_NONE);
    if (FrameOffset == INDEX_NONE) return false;
    TestFalse(TEXT("the complete, empty-fragment item roundtrips"),
              CompleteItemLoadHasError(Valid, Owner));

    TArray<uint8> Truncated = Valid;
    Truncated.SetNum(Truncated.Num() - 1);
    TestTrue(TEXT("truncated complete item payload fails"),
             CompleteItemLoadHasError(Truncated, Owner));

    TArray<uint8> UnknownVersion = Valid;
    const int32 UnsupportedVersion = 999;
    FMemory::Memcpy(UnknownVersion.GetData() + FrameOffset + 16,
                    &UnsupportedVersion, sizeof(UnsupportedVersion));
    AddExpectedError(TEXT("Unknown item fragment frame version 999"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestTrue(TEXT("unknown framed version fails closed"),
             CompleteItemLoadHasError(UnknownVersion, Owner));

    TArray<uint8> InvalidCount = Valid;
    const int32 ExcessiveCount = 65;
    FMemory::Memcpy(InvalidCount.GetData() + FrameOffset + 20,
                    &ExcessiveCount, sizeof(ExcessiveCount));
    TestTrue(TEXT("over-limit fragment count fails before allocation"),
             CompleteItemLoadHasError(InvalidCount, Owner));

    const TArray<uint8> MissingClass = BuildOneFragmentFrame(
        Valid, FrameOffset, TEXT("/Script/Mythic.DefinitelyMissingFragment"), {});
    AddExpectedError(TEXT("Missing framed item fragment class /Script/Mythic.DefinitelyMissingFragment"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestTrue(TEXT("missing fragment class rejects the whole item"),
             CompleteItemLoadHasError(MissingClass, Owner));

    const uint8 CorruptPayload = 0x7f;
    const TArray<uint8> NestedCorruption = BuildOneFragmentFrame(
        Valid, FrameOffset, FSoftClassPath(UAffixesFragment::StaticClass()).ToString(),
        MakeArrayView(&CorruptPayload, 1));
    AddExpectedError(TEXT("Corrupt payload for item fragment class /Script/Mythic.AffixesFragment"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestTrue(TEXT("nested fragment archive errors propagate to the complete item"),
             CompleteItemLoadHasError(NestedCorruption, Owner));
    return true;
}

#endif
