#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Placement/MythicPlacedProxyTypes.h"
#include "MythicPlacedProxySubsystem.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FMythicProxyTypeDef {
    GENERATED_BODY()

    // Mesh drawn while dormant. Null = the record is invisible until promoted (fine for buried secrets).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proxy")
    TObjectPtr<UStaticMesh> DormantMesh = nullptr;

    // Actor spawned on promotion. Null = this type never promotes (pure scenery with a record).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proxy")
    TSubclassOf<AActor> ActorClass = nullptr;
};

UCLASS()
class MYTHIC_API UMythicPlacedProxySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    static UMythicPlacedProxySubsystem *Get(const UObject *WorldContextObject);

    // Register one placed thing. Returns its id. Server only — the registry is authoritative state.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Placement")
    FGuid RegisterProxy(FGameplayTag Type, const FTransform &Transform, int32 StateFlags = 0);

    // Adopt an ALREADY-PLACED actor: learn its class and mesh, record its transform, and register it. This is how a
    // level full of hand-placed chests becomes a level full of records without re-authoring anything — the actor adds
    // one component, and on load it hands itself to the registry and steps aside.
    //
    // The caller destroys the actor afterwards; the registry re-spawns the same class when a player approaches.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Placement")
    FGuid AdoptPlacedActor(AActor *Actor, FGameplayTag Type, int32 StateFlags = 0);

    // True while the registry is mid-promotion. A registration component uses this to tell "I was placed by a level
    // designer" from "I was just spawned BY the registry" — without it, every promoted actor would re-register itself
    // and immediately vanish again.
    UFUNCTION(BlueprintPure, Category = "Mythic|Placement")
    bool IsPromotionInProgress() const {
        return bPromotionInProgress;
    }

    // Forget a placed thing entirely (it was destroyed for good). Removes its instance and any live actor.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Placement")
    bool UnregisterProxy(const FGuid &Id);

    // Read/patch the durable state that must survive demotion (opened, looted, discovered).
    UFUNCTION(BlueprintPure, Category = "Mythic|Placement")
    int32 GetProxyState(const FGuid &Id) const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Placement")
    bool SetProxyState(const FGuid &Id, int32 StateFlags);

    UFUNCTION(BlueprintPure, Category = "Mythic|Placement")
    int32 GetProxyCount() const {
        return Proxies.Num();
    }

    // How many currently have a live actor. This is the number that actually costs anything.
    UFUNCTION(BlueprintPure, Category = "Mythic|Placement")
    int32 GetPromotedCount() const;

    // ── Config ──
    // Per-type mesh + actor class. Populated by whatever owns the placement data.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Placement")
    TMap<FGameplayTag, FMythicProxyTypeDef> TypeDefs;

    /** Spawn the real actor when a player gets this close (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Placement", meta = (ClampMin = "0.0"))
    float PromoteRadius = 2500.0f;

    /** Destroy it again once every player is beyond this (cm). MUST exceed PromoteRadius — see the rules header. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Placement", meta = (ClampMin = "0.0"))
    float DemoteRadius = 3500.0f;

    /** Seconds between promotion passes. Coarse: a chest appearing a third of a second early is invisible. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Placement", meta = (ClampMin = "0.05"))
    float PassInterval = 0.33f;

protected:
    virtual void Deinitialize() override;

private:
    UPROPERTY()
    TArray<FMythicPlacedProxy> Proxies;

    UPROPERTY()
    TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> InstanceGroups;

    UPROPERTY()
    TMap<FGuid, TObjectPtr<AActor>> LiveActors;

    UPROPERTY()
    TObjectPtr<AActor> InstanceHost = nullptr;

    FTimerHandle PassTimer;

    bool bPromotionInProgress = false;

    FMythicPlacedProxy *FindProxy(const FGuid &Id);
    const FMythicPlacedProxy *FindProxy(const FGuid &Id) const;

    UInstancedStaticMeshComponent *GetOrCreateGroup(UStaticMesh *Mesh);
    AActor *GetOrCreateInstanceHost();

    void AddInstanceFor(FMythicPlacedProxy &Proxy);
    void RemoveInstanceFor(FMythicPlacedProxy &Proxy);

    void Promote(FMythicPlacedProxy &Proxy);
    void Demote(FMythicPlacedProxy &Proxy);

    void RunPass();
    void UpdateTimer();
};
