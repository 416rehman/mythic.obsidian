#pragma once
#include "CoreMinimal.h"
#include "ConsumableEffectFragment.h"
#include "Iris/Serialization/NetSerializer.h"
#include "MythicConsumableEffectNetSerializer.generated.h"

/** Network-only projection. Authored and SaveGame data keep the existing TMap. */
USTRUCT()
struct FMythicConsumableMagnitudeWire {
    GENERATED_BODY()
    UPROPERTY() FGameplayTag Tag;
    UPROPERTY() float Magnitude = 0.f;
};

USTRUCT()
struct FMythicConsumableEffectWire {
    GENERATED_BODY()
    UPROPERTY() TSubclassOf<UGameplayEffect> GameplayEffectClass;
    UPROPERTY() float EffectLevel = 1.f;
    UPROPERTY() TArray<FMythicConsumableMagnitudeWire> Magnitudes;
    UPROPERTY() FText DisplayTextOverride;
};

namespace MythicConsumableWire {
    constexpr int32 MaxMagnitudes = FConsumableEffectEntry::MaxSetByCallerMagnitudes;
    MYTHIC_API bool ToWire(const FConsumableEffectEntry& Source, FMythicConsumableEffectWire& Out);
    MYTHIC_API bool FromWire(const FMythicConsumableEffectWire& Source, FConsumableEffectEntry& Out);
}

USTRUCT()
struct FMythicConsumableEffectEntryNetSerializerConfig : public FNetSerializerConfig {
    GENERATED_BODY()
};

#if UE_WITH_IRIS
namespace UE::Net {
    UE_NET_DECLARE_SERIALIZER(FMythicConsumableEffectEntryNetSerializer, MYTHIC_API);
}
#endif
