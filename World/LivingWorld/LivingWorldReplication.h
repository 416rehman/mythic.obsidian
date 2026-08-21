
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LivingWorldTypes.h"
#include "Factions/FactionDatabase.h"
#include "GameFramework/Info.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "LivingWorldReplication.generated.h"


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    FMythicFactionId FactionId;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    EMythicFactionStatus Status = EMythicFactionStatus::Dormant;

    // Approximated population for UI / client logic
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    int32 Population = 0;

    // Total controlled cells
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    int32 ControlledCellCount = 0;

    // Relative wealth level [0-255 mapped to 0-max] for UI
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    uint8 WealthLevel = 0;

    // Relative military strength [0-255 mapped to 0-1] for UI
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    uint8 MilitaryStrength = 0;

    void PreReplicatedRemove(const struct FMythicFactionProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicFactionProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicFactionProxyArray &InArraySerializer);
};

USTRUCT()
struct MYTHIC_API FMythicFactionProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicFactionProxyItem> Items;

    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicFactionProxyItem, FMythicFactionProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicFactionProxyArray> : public TStructOpsTypeTraitsBase2<FMythicFactionProxyArray> {
    enum { WithNetDeltaSerializer = true };
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTerritoryProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    FMythicCellCoord Cell;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    FMythicFactionId ControllingFaction;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    uint8 ContestedLevel = 0;

    void PreReplicatedRemove(const struct FMythicTerritoryProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicTerritoryProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicTerritoryProxyArray &InArraySerializer);
};

USTRUCT()
struct MYTHIC_API FMythicTerritoryProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicTerritoryProxyItem> Items;

    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicTerritoryProxyItem, FMythicTerritoryProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicTerritoryProxyArray> : public TStructOpsTypeTraitsBase2<FMythicTerritoryProxyArray> {
    enum { WithNetDeltaSerializer = true };
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEncounterProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    uint32 EncounterId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FGameplayTag TemplateTag;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    EMythicEncounterState State = EMythicEncounterState::Pending;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FMythicCellCoord Cell;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FMythicFactionId OriginFaction;

    void PreReplicatedRemove(const struct FMythicEncounterProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicEncounterProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicEncounterProxyArray &InArraySerializer);
};

USTRUCT()
struct MYTHIC_API FMythicEncounterProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicEncounterProxyItem> Items;

    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicEncounterProxyItem, FMythicEncounterProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicEncounterProxyArray> : public TStructOpsTypeTraitsBase2<FMythicEncounterProxyArray> {
    enum { WithNetDeltaSerializer = true };
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSettlementProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    int32 SettlementId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Settlement")
    FMythicCellCoord CenterCell;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Settlement")
    FMythicFactionId GoverningFaction;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Settlement")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Settlement")
    bool bIsCapital = false;

    void PreReplicatedRemove(const struct FMythicSettlementProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicSettlementProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicSettlementProxyArray &InArraySerializer);
};

USTRUCT()
struct MYTHIC_API FMythicSettlementProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicSettlementProxyItem> Items;

    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicSettlementProxyItem, FMythicSettlementProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicSettlementProxyArray> : public TStructOpsTypeTraitsBase2<FMythicSettlementProxyArray> {
    enum { WithNetDeltaSerializer = true };
};


UCLASS(NotBlueprintable, Transient)
class MYTHIC_API AMythicLivingWorldReplicator : public AInfo {
    GENERATED_BODY()

public:
    AMythicLivingWorldReplicator();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(Replicated)
    FMythicFactionProxyArray FactionProxies;

    UPROPERTY(Replicated)
    FMythicTerritoryProxyArray TerritoryProxies;

    UPROPERTY(Replicated)
    FMythicEncounterProxyArray EncounterProxies;

    UPROPERTY(Replicated)
    FMythicSettlementProxyArray SettlementProxies;

    TMap<FMythicFactionId, int32> FactionProxyIndex;

    TMap<FMythicCellCoord, int32> TerritoryProxyIndex;

    void SyncProxies(class UMythicLivingWorldSubsystem *Subsystem);

    static bool TerritoryProxyNeedsUpdate(const FMythicTerritoryProxyItem &Existing, FMythicFactionId NewFaction, uint8 NewContestedLevel);


    const FMythicFactionProxyItem *GetFactionProxy(FMythicFactionId FactionId) const;

    const TArray<FMythicFactionProxyItem> &GetAllFactionProxies() const { return FactionProxies.Items; }

    bool GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const;

    const TArray<FMythicTerritoryProxyItem> &GetAllTerritoryProxies() const { return TerritoryProxies.Items; }

    const TArray<FMythicEncounterProxyItem> &GetAllEncounterProxies() const { return EncounterProxies.Items; }

    const TArray<FMythicSettlementProxyItem> &GetAllSettlementProxies() const { return SettlementProxies.Items; }

    void NotifyClientProxiesChanged();

private:
    TWeakObjectPtr<class UMythicLivingWorldSubsystem> ClientSubsystem;
};
