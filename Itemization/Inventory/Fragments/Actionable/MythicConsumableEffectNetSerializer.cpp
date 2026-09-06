#include "MythicConsumableEffectNetSerializer.h"
#include "UObject/CoreNet.h"

namespace MythicConsumableWire {
bool ToWire(const FConsumableEffectEntry& Source, FMythicConsumableEffectWire& Out) {
    if (Source.SetByCallerMagnitudes.Num() > MaxMagnitudes || !FMath::IsFinite(Source.EffectLevel) || Source.EffectLevel <= 0.f) return false;
    FMythicConsumableEffectWire Staged;
    Staged.GameplayEffectClass = Source.GameplayEffectClass;
    Staged.EffectLevel = Source.EffectLevel;
    Staged.DisplayTextOverride = Source.DisplayTextOverride;
    for (const auto& Pair : Source.SetByCallerMagnitudes) {
        if (!Pair.Key.IsValid() || !FMath::IsFinite(Pair.Value)) return false;
        auto& Row = Staged.Magnitudes.AddDefaulted_GetRef(); Row.Tag = Pair.Key; Row.Magnitude = Pair.Value;
    }
    Staged.Magnitudes.Sort([](const auto& A, const auto& B) { return A.Tag.ToString().Compare(B.Tag.ToString(), ESearchCase::CaseSensitive) < 0; });
    Out = MoveTemp(Staged);
    return true;
}

bool FromWire(const FMythicConsumableEffectWire& Source, FConsumableEffectEntry& Out) {
    if (Source.Magnitudes.Num() > MaxMagnitudes || !FMath::IsFinite(Source.EffectLevel) || Source.EffectLevel <= 0.f) return false;
    // Null is a valid default/unmapped reference. Canonical use admission requires a resolved GE class.
    if (Source.GameplayEffectClass && !Source.GameplayEffectClass->IsChildOf(UGameplayEffect::StaticClass())) return false;
    FConsumableEffectEntry Staged;
    Staged.GameplayEffectClass = Source.GameplayEffectClass;
    Staged.EffectLevel = Source.EffectLevel;
    Staged.DisplayTextOverride = Source.DisplayTextOverride;
    FString Previous;
    for (const auto& Row : Source.Magnitudes) {
        const FString Name = Row.Tag.ToString();
        if (!Row.Tag.IsValid() || !FMath::IsFinite(Row.Magnitude) || (!Previous.IsEmpty() && Previous.Compare(Name, ESearchCase::CaseSensitive) >= 0)) return false;
        Staged.SetByCallerMagnitudes.Add(Row.Tag, Row.Magnitude);
        Previous = Name;
    }
    Out = MoveTemp(Staged);
    return true;
}
}

bool FConsumableEffectEntry::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) {
    bOutSuccess = false;
    FMythicConsumableEffectWire Wire;
    if (!Map || (Ar.IsSaving() && !MythicConsumableWire::ToWire(*this, Wire))) { Ar.SetError(); return true; }
    UObject* ClassObject = Wire.GameplayEffectClass.Get();
    const bool bClassMapped = Map->SerializeObject(Ar, UClass::StaticClass(), ClassObject);
    if (ClassObject && (!ClassObject->IsA<UClass>() || !CastChecked<UClass>(ClassObject)->IsChildOf(UGameplayEffect::StaticClass()))) { Ar.SetError(); return bClassMapped; }
    if (Ar.IsLoading()) Wire.GameplayEffectClass = Cast<UClass>(ClassObject);
    Ar << Wire.EffectLevel;
    uint8 Count = static_cast<uint8>(Wire.Magnitudes.Num());
    Ar.SerializeBits(&Count, 7);
    if (Count > MythicConsumableWire::MaxMagnitudes) { Ar.SetError(); return bClassMapped; }
    if (Ar.IsLoading()) Wire.Magnitudes.SetNum(Count);
    bool bTagsMapped = true;
    for (auto& Row : Wire.Magnitudes) {
        bool bTagSuccess = false;
        bTagsMapped = Row.Tag.NetSerialize(Ar, Map, bTagSuccess) && bTagsMapped;
        if (!bTagSuccess) { Ar.SetError(); return bClassMapped && bTagsMapped; }
        Ar << Row.Magnitude;
    }
    Ar << Wire.DisplayTextOverride;
    if (Ar.IsError()) return bClassMapped && bTagsMapped;
    if (Ar.IsLoading() && !MythicConsumableWire::FromWire(Wire, *this)) { Ar.SetError(); return bClassMapped && bTagsMapped; }
    bOutSuccess = true;
    // false asks legacy replication to retry unresolved class/tag references; it is not malformed data.
    return bClassMapped && bTagsMapped;
}

#if UE_WITH_IRIS
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Iris/ReplicationState/ReplicationStateDescriptorBuilder.h"
#include "Iris/Serialization/NetSerializerDelegates.h"
#include "Iris/Serialization/NetSerializers.h"
#include "Iris/Serialization/NetSerializationContext.h"

namespace UE::Net {
struct FMythicConsumableEffectEntryNetSerializer {
    static const uint32 Version = 0;
    static constexpr bool bIsForwardingSerializer = true;
    static constexpr bool bHasCustomNetReference = true;
    static constexpr bool bHasDynamicState = true;
    using SourceType = FConsumableEffectEntry;
    // UE 5.8's complete wire descriptor (including text history) requires 352 bytes.
    // Registration verifies size/alignment before any quantize operation can write here.
    struct QuantizedType { alignas(16) uint8 Data[352]; };
    using ConfigType = FMythicConsumableEffectEntryNetSerializerConfig;
    static const ConfigType DefaultConfig;

    static void Serialize(FNetSerializationContext&, const FNetSerializeArgs&);
    static void Deserialize(FNetSerializationContext&, const FNetDeserializeArgs&);
    static void SerializeDelta(FNetSerializationContext&, const FNetSerializeDeltaArgs&);
    static void DeserializeDelta(FNetSerializationContext&, const FNetDeserializeDeltaArgs&);
    static void Quantize(FNetSerializationContext&, const FNetQuantizeArgs&);
    static void Dequantize(FNetSerializationContext&, const FNetDequantizeArgs&);
    static bool IsEqual(FNetSerializationContext&, const FNetIsEqualArgs&);
    static bool Validate(FNetSerializationContext&, const FNetValidateArgs&);
    static void CloneDynamicState(FNetSerializationContext&, const FNetCloneDynamicStateArgs&);
    static void FreeDynamicState(FNetSerializationContext&, const FNetFreeDynamicStateArgs&);
    static void CollectNetReferences(FNetSerializationContext&, const FNetCollectReferencesArgs&);

private:
    class FRegistry final : private FNetSerializerRegistryDelegates {
    public: virtual ~FRegistry();
    private:
        virtual void OnPreFreezeNetSerializerRegistry() override;
        virtual void OnPostFreezeNetSerializerRegistry() override;
    };
    static FRegistry Registry;
    static FStructNetSerializerConfig WireConfig;
    static const FNetSerializer* Forward;
};
UE_NET_IMPLEMENT_SERIALIZER(FMythicConsumableEffectEntryNetSerializer);
const FMythicConsumableEffectEntryNetSerializer::ConfigType FMythicConsumableEffectEntryNetSerializer::DefaultConfig;
FMythicConsumableEffectEntryNetSerializer::FRegistry FMythicConsumableEffectEntryNetSerializer::Registry;
FStructNetSerializerConfig FMythicConsumableEffectEntryNetSerializer::WireConfig;
const FNetSerializer* FMythicConsumableEffectEntryNetSerializer::Forward = &UE_NET_GET_SERIALIZER(FStructNetSerializer);

#define MYTHIC_CONSUMABLE_FORWARD(Method, ArgsType) \
void FMythicConsumableEffectEntryNetSerializer::Method(FNetSerializationContext& Context, const ArgsType& Args) { \
    auto Internal = Args; Internal.NetSerializerConfig = &WireConfig; Forward->Method(Context, Internal); \
}
MYTHIC_CONSUMABLE_FORWARD(Serialize, FNetSerializeArgs)
MYTHIC_CONSUMABLE_FORWARD(Deserialize, FNetDeserializeArgs)
MYTHIC_CONSUMABLE_FORWARD(SerializeDelta, FNetSerializeDeltaArgs)
MYTHIC_CONSUMABLE_FORWARD(DeserializeDelta, FNetDeserializeDeltaArgs)
MYTHIC_CONSUMABLE_FORWARD(CloneDynamicState, FNetCloneDynamicStateArgs)
MYTHIC_CONSUMABLE_FORWARD(FreeDynamicState, FNetFreeDynamicStateArgs)
MYTHIC_CONSUMABLE_FORWARD(CollectNetReferences, FNetCollectReferencesArgs)
#undef MYTHIC_CONSUMABLE_FORWARD

void FMythicConsumableEffectEntryNetSerializer::Quantize(FNetSerializationContext& Context, const FNetQuantizeArgs& Args) {
    FMythicConsumableEffectWire Wire;
    if (!MythicConsumableWire::ToWire(*reinterpret_cast<const SourceType*>(Args.Source), Wire)) { Context.SetError(GNetError_InvalidValue); return; }
    auto Internal = Args; Internal.NetSerializerConfig = &WireConfig; Internal.Source = NetSerializerValuePointer(&Wire);
    Forward->Quantize(Context, Internal);
}
void FMythicConsumableEffectEntryNetSerializer::Dequantize(FNetSerializationContext& Context, const FNetDequantizeArgs& Args) {
    FMythicConsumableEffectWire Wire;
    auto Internal = Args; Internal.NetSerializerConfig = &WireConfig; Internal.Target = NetSerializerValuePointer(&Wire);
    Forward->Dequantize(Context, Internal);
    if (!Context.HasError() && !MythicConsumableWire::FromWire(Wire, *reinterpret_cast<SourceType*>(Args.Target))) Context.SetError(GNetError_InvalidValue);
}
bool FMythicConsumableEffectEntryNetSerializer::IsEqual(FNetSerializationContext& Context, const FNetIsEqualArgs& Args) {
    auto Internal = Args; Internal.NetSerializerConfig = &WireConfig;
    if (Args.bStateIsQuantized) return Forward->IsEqual(Context, Internal);
    FMythicConsumableEffectWire A, B;
    if (!MythicConsumableWire::ToWire(*reinterpret_cast<const SourceType*>(Args.Source0), A) || !MythicConsumableWire::ToWire(*reinterpret_cast<const SourceType*>(Args.Source1), B)) return false;
    Internal.Source0 = NetSerializerValuePointer(&A); Internal.Source1 = NetSerializerValuePointer(&B);
    return Forward->IsEqual(Context, Internal);
}
bool FMythicConsumableEffectEntryNetSerializer::Validate(FNetSerializationContext& Context, const FNetValidateArgs& Args) {
    FMythicConsumableEffectWire Wire;
    if (!MythicConsumableWire::ToWire(*reinterpret_cast<const SourceType*>(Args.Source), Wire)) return false;
    auto Internal = Args; Internal.NetSerializerConfig = &WireConfig; Internal.Source = NetSerializerValuePointer(&Wire);
    return Forward->Validate(Context, Internal);
}

static const FName NAME_ConsumableEffectEntry("ConsumableEffectEntry");
UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(NAME_ConsumableEffectEntry, FMythicConsumableEffectEntryNetSerializer);
FMythicConsumableEffectEntryNetSerializer::FRegistry::~FRegistry() { UE_NET_UNREGISTER_NETSERIALIZER_INFO(NAME_ConsumableEffectEntry); }
void FMythicConsumableEffectEntryNetSerializer::FRegistry::OnPreFreezeNetSerializerRegistry() { UE_NET_REGISTER_NETSERIALIZER_INFO(NAME_ConsumableEffectEntry); }
void FMythicConsumableEffectEntryNetSerializer::FRegistry::OnPostFreezeNetSerializerRegistry() {
    WireConfig.StateDescriptor = FReplicationStateDescriptorBuilder::CreateDescriptorForStruct(FMythicConsumableEffectWire::StaticStruct());
    const auto* Descriptor = WireConfig.StateDescriptor.GetReference();
    check(Descriptor);
    ValidateForwardingNetSerializerTraits(&UE_NET_GET_SERIALIZER(FMythicConsumableEffectEntryNetSerializer), Descriptor->Traits);
    if (sizeof(QuantizedType) < Descriptor->InternalSize || alignof(QuantizedType) < Descriptor->InternalAlignment) {
        LowLevelFatalError(TEXT("Consumable wire storage %u/%u requires %u/%u"), uint32(sizeof(QuantizedType)), uint32(alignof(QuantizedType)), uint32(Descriptor->InternalSize), uint32(Descriptor->InternalAlignment));
    }
}
}
#endif
