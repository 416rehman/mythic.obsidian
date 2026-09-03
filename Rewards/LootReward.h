
#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "Rewards/LootScaling.h"
#include "UObject/ScriptInterface.h"
#include "LootReward.generated.h"

class IInventoryProviderInterface;
class UMythicLootManagerSubsystem;
class UMythicInventoryComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct FLootTableEntry {
    GENERATED_BODY()

    /** Item definition eligible to be generated when this loot-table row succeeds. */
    UPROPERTY(EditAnywhere)
    UItemDefinition *Item = nullptr;

    /** Inclusive stack-size roll for stackable items; ignored by non-stackable item definitions. */
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    FInt32Interval StackRange = FInt32Interval(1, 1);

    /** Absolute row drop chance in the range 0..1; zero delegates to the rarity-weighted global chance. */
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
    float OverrideDropChance = 0.0f;
};

UCLASS()
class MYTHIC_API UMythicLootTable : public UDataAsset {
    GENERATED_BODY()

public:
    /** Candidate item rows evaluated when this table is drawn. */
    UPROPERTY(EditAnywhere)
    TArray<FLootTableEntry> Entries;

    /** Maximum successful item rows emitted by one table draw; chance rolls may produce fewer. */
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    int32 MaxItems = 1;

    /** Probability that this table is drawn, from zero to one. */
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
    float DropChance = 0.3f;
};

USTRUCT(BlueprintType, Blueprintable)
struct FLootTableOverride {
    GENERATED_BODY()

    /** Loot tables independently rolled for this source before eligible item rows are selected. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<UMythicLootTable *> LootTables;

    /** Whether each eligible player receives a private, independently interactable copy of this source's loot. */
    UPROPERTY(EditAnywhere)
    bool IsPrivate = false;

    /** Whether this source omits the global loot table and rolls only the tables authored above. */
    UPROPERTY(EditAnywhere)
    bool bSkipGlobal = false;
};


USTRUCT(BlueprintType, Blueprintable)
struct FLootRewardContext : public FRewardContext {
    GENERATED_BODY()

    /** Optional destination inventory; absent or full inventories cause generated items to become world drops. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    TObjectPtr<UMythicInventoryComponent> PutInInventory = nullptr;

    /** World-drop location used when no inventory accepts the item; zero resolves to the player's location. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    FVector SpawnLocation = FVector::ZeroVector;

    /** Item level supplied to item generation for every result from this reward. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 ItemLevel = 0;

    /** Source enemy tier from 1 (Normal) to 5 (Boss); zero is inert, while higher tiers improve count and rarity. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 EnemyTierInt = 0;
};


UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API ULootReward : public URewardBase {
    GENERATED_BODY()

    static void RequestLootFromSource(float CommonRate, float RareRate, float EpicRate, float LegendaryRate, float MythicRate,
                                      float GoldMultiplier,
                                      APlayerController *PlayerController,
                                      int32 DropLevel, UMythicLootTable *LootTable, TScriptInterface<IInventoryProviderInterface> InventoryProvider,
                                      bool isPrivate, FVector SpawnLocation,
                                      UMythicLootManagerSubsystem *
                                      MythicLootManager,
                                      float RarityFind, const FLootTierBonus &TierBonus);

public:
    virtual bool Give(FRewardContext &Context) const override;

    static bool ResolveEntryDropChance(float OverrideDropChance, int32 RarityIndex, TConstArrayView<float> RarityWeights, float &OutChance);

    /**
     * The bonus every table of one credit shares: enemy tier and quantity find, then each listener's say through the
     * loot manager's OnPreLootRoll. Raised once per crediting controller and only for a slain enemy (EnemyTierInt above
     * zero); quest, chest and bounty loot roll the plain bonus.
     */
    static FLootTierBonus PrepareLootRoll(UMythicLootManagerSubsystem *LootManager, APlayerController *PlayerController, int32 EnemyTierInt,
                                          float QuantityFind);

    /** Source-specific loot tables and private/global-table policy used by this reward. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Reward Context")
    FLootTableOverride OverridenLootSource;

    /** Rolls Reward's configured tables and grants results to Context's inventory or world-drop destination. */
    UFUNCTION(BlueprintCallable, Category = "Loot Reward")
    static bool GiveLootReward(ULootReward *Reward, FLootRewardContext Context);
};
