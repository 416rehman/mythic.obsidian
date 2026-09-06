#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicConsumableWireTestTypes.h"
#include "Itemization/Inventory/Fragments/Actionable/MythicConsumableEffectNetSerializer.h"
#include "GAS/MythicTags_GAS.h"
#include "Tests/GAS/MythicGA_RuneTestTypes.h"
#include "Misc/EngineNetworkCustomVersion.h"
#include "Serialization/CustomVersion.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>
#if UE_WITH_IRIS
#include "Modules/ModuleManager.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Iris/ReplicationState/ReplicationStateDescriptorBuilder.h"
#include "Iris/ReplicationState/ReplicationStateDescriptor.h"
#include "Iris/Serialization/NetSerializer.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicConsumableEffectWireTest,"Mythic.Controls.ConsumableEffectWire",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicConsumableEffectWireTest::RunTest(const FString&) {
    TStrongObjectPtr<UMythicConsumableWireTestPackageMap> Map(NewObject<UMythicConsumableWireTestPackageMap>());
    Map->KnownClass=UMythicRuneTestHealMetaEffect::StaticClass();
    FConsumableEffectEntry Source;
    Source.GameplayEffectClass=Map->KnownClass.Get(); Source.EffectLevel=2.5f;
    Source.SetByCallerMagnitudes.Add(GAS_SETBYCALLER_HEAL,60.f);
    Source.SetByCallerMagnitudes.Add(GAS_SETBYCALLER_GENERIC,12.25f);
    Source.DisplayTextOverride=NSLOCTEXT("MythicWireTests","ConsumableEffectDisplay","Restores health");
    // Exercise the actual legacy archive path without requiring a live connection's dynamic token store.
    // This selects the engine's legacy tag-index/name format through archive protocol version only.
    const auto Versions=FCurrentCustomVersions::GetAll();
    const auto SetProtocol=[&Versions](FArchive& Archive) {
        Archive.SetCustomVersions(Versions);
        Archive.SetEngineNetVer(FEngineNetworkCustomVersion::CustomExports-1);
        // UsingCustomVersion must not silently upgrade only the writer's negotiated protocol.
        Archive.SetShouldSkipUpdateCustomVersion(true);
    };
    FNetBitWriter Writer(Map.Get(),65536); SetProtocol(Writer); bool bSaved=false;
    const bool bMapped=Source.NetSerialize(Writer,Map.Get(),bSaved);
    if (!TestTrue(TEXT("actual entry serializer writes a valid class and nonzero SBC map"),bSaved && bMapped && !Writer.IsError())) return false;
    FConsumableEffectEntry Loaded;
    FNetBitReader Reader(Map.Get(),Writer.GetData(),Writer.GetNumBits()); SetProtocol(Reader); bool bRead=false;
    TestTrue(TEXT("network reader maps every class reference"),Loaded.NetSerialize(Reader,Map.Get(),bRead));
    TestTrue(TEXT("entry reader succeeds and consumes the complete frame"),bRead && !Reader.IsError() && Reader.GetPosBits()==Writer.GetNumBits());
    TestEqual(TEXT("real GE class survives wire"),Loaded.GameplayEffectClass.Get(),Source.GameplayEffectClass.Get());
    TestEqual(TEXT("effect level survives wire"),Loaded.EffectLevel,2.5f);
    TestEqual(TEXT("both SBC pairs survive wire"),Loaded.SetByCallerMagnitudes.Num(),2);
    TestEqual(TEXT("nonzero heal survives wire"),Loaded.SetByCallerMagnitudes.FindRef(GAS_SETBYCALLER_HEAL),60.f);
    TestEqual(TEXT("fractional generic magnitude survives wire"),Loaded.SetByCallerMagnitudes.FindRef(GAS_SETBYCALLER_GENERIC),12.25f);
    TestTrue(TEXT("display override survives wire"),Loaded.DisplayTextOverride.EqualTo(Source.DisplayTextOverride));

    FConsumableEffectEntry Reordered=Source;
    Reordered.SetByCallerMagnitudes.Reset();
    Reordered.SetByCallerMagnitudes.Add(GAS_SETBYCALLER_GENERIC,12.25f);
    Reordered.SetByCallerMagnitudes.Add(GAS_SETBYCALLER_HEAL,60.f);
    FNetBitWriter ReorderedWriter(Map.Get(),65536); SetProtocol(ReorderedWriter); bool bReordered=false;
    Reordered.NetSerialize(ReorderedWriter,Map.Get(),bReordered);
    TestTrue(TEXT("map insertion order produces identical bounded network bytes"),bReordered && ReorderedWriter.GetNumBits()==Writer.GetNumBits()
        && FMemory::Memcmp(ReorderedWriter.GetData(),Writer.GetData(),Writer.GetNumBytes())==0);

    Map->bResolveClass=false;
    FNetBitReader UnmappedReader(Map.Get(),Writer.GetData(),Writer.GetNumBits()); SetProtocol(UnmappedReader);
    FConsumableEffectEntry Unmapped; bool bUnmappedSuccess=false;
    TestFalse(TEXT("unmapped class requests UE reference retry"),Unmapped.NetSerialize(UnmappedReader,Map.Get(),bUnmappedSuccess));
    TestTrue(TEXT("unmapped class is not falsely treated as malformed"),bUnmappedSuccess && !UnmappedReader.IsError());
    TestNull(TEXT("unmapped class cannot authorize effect use"),Unmapped.GameplayEffectClass.Get());
    TestEqual(TEXT("mapped SBC remains intact during reference retry"),Unmapped.SetByCallerMagnitudes.FindRef(GAS_SETBYCALLER_HEAL),60.f);
    Map->bResolveClass=true;
    FNetBitReader RetryReader(Map.Get(),Writer.GetData(),Writer.GetNumBits()); SetProtocol(RetryReader); bool bRetry=false;
    TestTrue(TEXT("replaying reference-bearing payload completes mapping"),Unmapped.NetSerialize(RetryReader,Map.Get(),bRetry) && bRetry && Unmapped.GameplayEffectClass==Source.GameplayEffectClass);

    FMythicConsumableEffectWire Wire;
    TestTrue(TEXT("map converts to shared Iris transfer schema"),MythicConsumableWire::ToWire(Source,Wire));
    const auto ValidWire=Wire;
    FConsumableEffectEntry Unchanged=Source;
    const auto Duplicate=Wire.Magnitudes[0]; Wire.Magnitudes.Add(Duplicate);
    TestFalse(TEXT("duplicate or out-of-order received SBC keys fail closed"),MythicConsumableWire::FromWire(Wire,Unchanged));
    TestEqual(TEXT("rejected staged decode does not overwrite prior valid payload"),Unchanged.SetByCallerMagnitudes.Num(),2);
    Wire=ValidWire; Wire.Magnitudes[0].Magnitude=std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("non-finite receive magnitude is rejected"),MythicConsumableWire::FromWire(Wire,Unchanged));
    Wire=ValidWire; Wire.Magnitudes.SetNum(MythicConsumableWire::MaxMagnitudes+1);
    TestFalse(TEXT("oversize Iris transfer is not admitted"),MythicConsumableWire::FromWire(Wire,Unchanged));
    FConsumableEffectEntry BadLevel=Source; BadLevel.EffectLevel=0.f;
    FNetBitWriter BadWriter(Map.Get(),4096); SetProtocol(BadWriter); bool bBad=true;
    BadLevel.NetSerialize(BadWriter,Map.Get(),bBad);
    TestTrue(TEXT("invalid source fails before any network field is emitted"),!bBad && BadWriter.IsError() && BadWriter.GetNumBits()==0);

    // Valid envelope carrying count65: bounds reject before allocation/read, with no deliberate reader overflow.
    FNetBitWriter CountWriter(Map.Get(),4096); SetProtocol(CountWriter);
    UObject* Class=Source.GameplayEffectClass.Get(); Map->SerializeObject(CountWriter,UClass::StaticClass(),Class);
    float Level=1.f; CountWriter << Level; uint8 Count=MythicConsumableWire::MaxMagnitudes+1; CountWriter.SerializeBits(&Count,7);
    FNetBitReader CountReader(Map.Get(),CountWriter.GetData(),CountWriter.GetNumBits()); SetProtocol(CountReader); bool bCount=true;
    FConsumableEffectEntry BadCount=Source; BadCount.NetSerialize(CountReader,Map.Get(),bCount);
    TestTrue(TEXT("legacy oversize count is rejected before array allocation"),!bCount && CountReader.IsError());
    TestEqual(TEXT("malformed count retains prior valid data"),BadCount.SetByCallerMagnitudes.Num(),2);

#if UE_WITH_IRIS
    using namespace UE::Net;
    // A standalone automation world need not have created an Iris NetDriver. Linking IrisCore does not start
    // its registry. Initialize the real module so its normal pending serializer callbacks run, as in Iris play.
    const bool bIrisModuleWasStarted=FModuleManager::Get().IsModuleLoaded(TEXT("IrisCore"));
    FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("IrisCore"));
    AddInfo(FString::Printf(TEXT("IrisCore started before descriptor fixture=%d, after=%d"),int32(bIrisModuleWasStarted),int32(FModuleManager::Get().IsModuleLoaded(TEXT("IrisCore")))));
    const auto* RegistryInfo=FPropertyNetSerializerInfoRegistry::FindStructSerializerInfo(FConsumableEffectEntry::StaticStruct()->GetFName());
    if (!TestNotNull(TEXT("real Iris module registers the exact consumable source struct"),RegistryInfo)) return false;
    TestTrue(TEXT("registry dispatch is the production consumable adapter"),RegistryInfo->GetNetSerializer(nullptr)==&UE_NET_GET_SERIALIZER(FMythicConsumableEffectEntryNetSerializer));
    auto Descriptor=FReplicationStateDescriptorBuilder::CreateDescriptorForStruct(FConsumableEffectEntry::StaticStruct());
    if (TestNotNull(TEXT("Iris builds consumable entry descriptor"),Descriptor.GetReference())) {
        TestTrue(TEXT("Iris selects registered custom entry adapter instead of skipping TMap"),EnumHasAnyFlags(Descriptor->Traits,EReplicationStateTraits::IsStructWithCustomSerializer));
        TestEqual(TEXT("one serializer owns the whole entry"),Descriptor->MemberCount,uint16(1));
        if (Descriptor->MemberCount==1) TestTrue(TEXT("descriptor points at the actual forwarding adapter"),Descriptor->MemberSerializerDescriptors[0].Serializer==&UE_NET_GET_SERIALIZER(FMythicConsumableEffectEntryNetSerializer));
        TestTrue(TEXT("class references remain visible to Iris mapping/retry"),Descriptor->HasObjectReference());
    }
    // Iris byte roundtrip deliberately belongs to actual PIE: public FNetSerializationContext cannot initialize
    // the private replication system/cache/token-store required by nonempty arrays, GameplayTags and classes.
#endif
    return true;
}
#endif
