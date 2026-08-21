#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Info.h"
#include "MythicPOIReplicator.generated.h"


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPOIProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    // Stable POI id (the dedup key). Set once when unlocked; a POI is never removed (permanent discovery).
    UPROPERTY(BlueprintReadOnly, Category = "POI")
    int32 POIId = INDEX_NONE;

    // World anchor the fast-travel path teleports to / the compass points at.
    UPROPERTY(BlueprintReadOnly, Category = "POI")
    FVector Anchor = FVector::ZeroVector;

    // Category/identity tag (POI.Landmark by default; a designer may set a more specific POI.* tag).
    UPROPERTY(BlueprintReadOnly, Category = "POI")
    FGameplayTag POITag;

    // Player-facing name ("Sunken Temple") for the discovery toast / map label.
    UPROPERTY(BlueprintReadOnly, Category = "POI")
    FText DisplayName;

    void PostReplicatedAdd(const struct FMythicPOIProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicPOIProxyArray &InArraySerializer);
    void PreReplicatedRemove(const struct FMythicPOIProxyArray &InArraySerializer);
};

USTRUCT()
struct MYTHIC_API FMythicPOIProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicPOIProxyItem> Items;

    class AMythicPOIReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicPOIProxyItem, FMythicPOIProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicPOIProxyArray> : public TStructOpsTypeTraitsBase2<FMythicPOIProxyArray> {
    enum { WithNetDeltaSerializer = true };
};


UCLASS(NotBlueprintable, Transient)
class MYTHIC_API AMythicPOIReplicator : public AInfo {
    GENERATED_BODY()

public:
    AMythicPOIReplicator();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(Replicated)
    FMythicPOIProxyArray POIProxies;

    void ServerAddPOI(int32 POIId, const FVector &Anchor, const FGameplayTag &POITag, const FText &DisplayName);

    const TArray<FMythicPOIProxyItem> &GetAllPOIs() const { return POIProxies.Items; }

    void NotifyClientPOIsChanged();

private:
    TWeakObjectPtr<class UMythicPOIDiscoverySubsystem> ClientSubsystem;
};
