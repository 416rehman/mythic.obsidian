
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceTypes.h"
#include "MythicAcquaintanceComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicOnNpcRelationChangedNative, const FMythicNpcRelation &, EMythicNpcInteraction);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnAcquaintanceChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicAcquaintanceComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicAcquaintanceComponent();


    float ServerRecordInteraction(uint32 NpcNameHash, FGameplayTag Faction, EMythicNpcInteraction Interaction);

    void RestoreRelations(const TArray<FMythicNpcRelation> &InRelations);


    const TArray<FMythicNpcRelation> &GetRelations() const { return Relations; }

    bool GetRelation(uint32 NpcNameHash, FMythicNpcRelation &Out) const;

    float GetCurrentWarmth(uint32 NpcNameHash) const;

    EMythicWarmthTier GetWarmthTier(uint32 NpcNameHash) const;

    /** Blueprint convenience: the warmth tier toward a live NPC actor (hashes its FName — the shared identity rule). */
    UFUNCTION(BlueprintPure, Category = "Acquaintance")
    EMythicWarmthTier GetWarmthTierForActor(const AActor *Npc) const;

    /** Number of remembered NPCs (test/diagnostic read). */
    UFUNCTION(BlueprintPure, Category = "Acquaintance")
    int32 GetRelationCount() const { return Relations.Num(); }

    FMythicOnNpcRelationChangedNative OnRelationChangedNative;

    UPROPERTY(BlueprintAssignable, Category = "Acquaintance")
    FMythicOnAcquaintanceChanged OnAcquaintanceChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Relations, SaveGame)
    TArray<FMythicNpcRelation> Relations;

    UFUNCTION()
    void OnRep_Relations();

    // Live tuning (code defaults — runs fully unauthored; a designer surface is a noted follow-up).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Acquaintance")
    FMythicAcquaintanceConfig Config;

private:
    double NowSeconds() const;
};
