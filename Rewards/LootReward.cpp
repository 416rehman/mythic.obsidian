// 


#include "LootReward.h"

#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "MythicDevSettings.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"

bool ULootReward::Give(FRewardContext &Context) const {
    // Cast the context to the correct type
    FLootRewardContext *LootContext = static_cast<FLootRewardContext *>(&Context);
    checkf(LootContext, TEXT("LootReward::Give - LootRewardContext is null"));

    // Get the player controller
    auto PlayerController = LootContext->PlayerController;
    checkf(PlayerController, TEXT("LootReward::Give - PlayerController is null"));

    // Check if the context is an ability system component
    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    // Get the MythicLootManager Subsystem
    auto MythicLootManager = PlayerController->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(MythicLootManager, TEXT("LootReward::Give - MythicLootManager not found"));

    // Get the game state
    auto GameState = PlayerController->GetWorld()->GetGameState<AMythicGameState>();
    checkf(GameState, TEXT("LootReward::Give - GameState is null"));

    // Get the world tier attributes
    auto WorldTierAttributes = GameState->WorldTierAttributes;
    checkf(GameState->WorldTierAttributes, TEXT("LootReward::Give - WorldTierAttributes is null"));

    auto LevelFound = false;
    auto PlayerLevel = UMythicAttributeSet_Proficiencies::GetLevel(ASC, LevelFound);
    if (!LevelFound) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Failed to get player level - Using Level 1"));
        PlayerLevel = 1;
    }

    UE_LOG(Myth, Warning, TEXT("LootReward::Give - Current Player Level: %d"), PlayerLevel);

    // Get the loot rates for the player
    // Calculate drop rates once for this request
    const float CommonRate = GameState->CommonLootChanceCurveRowHandle.Eval(PlayerLevel, "");
    const float RareRate = GameState->RareLootChanceCurveRowHandle.Eval(PlayerLevel, "");
    const float EpicRate = GameState->EpicLootChanceCurveRowHandle.Eval(PlayerLevel, "");
    const float LegendaryRate = GameState->LegendaryLootChanceCurveRowHandle.Eval(PlayerLevel, "") * WorldTierAttributes->GetLegendaryDropRateMultiplier();
    const float MythicRate = GameState->MythicLootChanceCurveRowHandle.Eval(PlayerLevel, "") * WorldTierAttributes->GetMythicDropRateMultiplier();

    UE_LOG(Myth, Log, TEXT("LootReward::Give - Rarities for Level %d = Common: %f, Rare: %f, Epic: %f, Legendary: %f, Mythic: %f"), PlayerLevel, CommonRate,
           RareRate, EpicRate, LegendaryRate, MythicRate);

    // Check if we have a loot source
    for (auto LootTable : OverridenLootSource.LootTables) {
        UE_LOG(Myth, Log, TEXT("LootReward::Give - Using loot source %s"), *LootTable->GetName());
        RequestLootFromSource(CommonRate, RareRate, EpicRate, LegendaryRate, MythicRate, PlayerController, LootContext->ItemLevel, LootTable,
                              LootContext->PutInInventory, OverridenLootSource.IsPrivate, LootContext->SpawnLocation, MythicLootManager);
    }
    if (OverridenLootSource.bSkipGlobal) {
        UE_LOG(Myth, Log, TEXT("LootReward::Give - Skipping global loot source"));
        return true;
    }

    // Settings
    auto MythicSettings = UMythicDevSettings::Get();
    if (!MythicSettings) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Mythic Settings not found"));
        return false;
    }

    auto LootTable = MythicSettings->GlobalLootTable.Get();
    if (!LootTable) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Global loot table not found"));
        return false;
    }

    // If we made it here, we didn't skip the global loot source, so we should use it
    UE_LOG(Myth, Log, TEXT("LootReward::Give - Using global loot source"));
    RequestLootFromSource(CommonRate, RareRate, EpicRate, LegendaryRate, MythicRate, PlayerController, LootContext->ItemLevel,
                          LootTable, LootContext->PutInInventory, OverridenLootSource.IsPrivate, LootContext->SpawnLocation,
                          MythicLootManager);

    return true;
}

void ULootReward::RequestLootFromSource(float CommonRate, float RareRate, float EpicRate, float LegendaryRate, float MythicRate,
                                        APlayerController *PlayerController, int32 DropLevel,
                                        UMythicLootTable *LootTable, TScriptInterface<IInventoryProviderInterface> InventoryProvider, bool isPrivate,
                                        FVector SpawnLocation,
                                        UMythicLootManagerSubsystem *MythicLootManager) {
    if (!LootTable || LootTable->Entries.Num() == 0) {
        UE_LOG(Myth, Error, TEXT("LootReward::RequestLootFromSource - Loot table is empty or invalid"));
        return;
    }

    // Table proc check
    float procRoll = FMath::FRand();
    if (procRoll > LootTable->DropChance) {
        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Table failed to proc - Required: %.2f, Rolled: %.2f"),
               LootTable->DropChance, procRoll);
        return;
    }

    // Store rarity weights
    constexpr int32 NumRarities = 5;
    const float WeightsByRarity[NumRarities] = {
        CommonRate,
        RareRate,
        EpicRate,
        LegendaryRate,
        MythicRate
    };

    // Setup recipient and location
    auto TargetRecipient = isPrivate ? PlayerController : nullptr;
    auto SpawnLoc = SpawnLocation.IsZero() ? PlayerController->GetPawn()->GetActorLocation() : SpawnLocation;

    // First, build array of valid entries that pass the drop chance check
    static TArray<int32> ValidIndices;
    ValidIndices.Reserve(LootTable->Entries.Num());

    UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Checking drop chances for items in table %s"), *LootTable->GetName());

    for (int32 i = 0; i < LootTable->Entries.Num(); ++i) {
        const auto &Entry = LootTable->Entries[i];
        if (!Entry.Item) [[unlikely]]
            continue;

        const int32 RarityIndex = static_cast<int32>(Entry.Item->Rarity);
        float DropChance = Entry.OverrideDropChance > 0 ? Entry.OverrideDropChance : WeightsByRarity[RarityIndex];

        const float RollResult = FMath::FRand();

        if (RollResult <= DropChance) {
            ValidIndices.Add(i);
            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Item %s passed drop check - Required: %.2f, Rolled: %.2f"),
                   *Entry.Item->GetName(), DropChance, RollResult);
        }
        else {
            UE_LOG(Myth, Verbose, TEXT("LootReward::RequestLootFromSource - Item %s failed drop check - Required: %.2f, Rolled: %.2f"),
                   *Entry.Item->GetName(), DropChance, RollResult);
        }
    }

    if (ValidIndices.Num() == 0) {
        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - No items passed the drop chance check"));
        return;
    }

    // Determine how many items we'll actually drop
    const int32 NumItemsToDrop = FMath::RandRange(1, FMath::Min(LootTable->MaxItems, ValidIndices.Num()));
    UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Will drop %d items from %d eligible items"),
           NumItemsToDrop, ValidIndices.Num());

    // Track used indices
    static TBitArray<> UsedIndices;
    UsedIndices.Init(false, ValidIndices.Num());

    int32 SuccessfulDrops = 0;
    for (int32 DropCount = 0; DropCount < NumItemsToDrop; ++DropCount) {
        // Reset used tracking if we've used all items
        if (UsedIndices.CountSetBits() == ValidIndices.Num()) {
            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Reset used items tracking - all items have been used"));
            UsedIndices.Init(false, ValidIndices.Num());
        }

        // Try to find an unused item
        int32 MaxAttempts = 10;
        int32 SelectedIndex = -1;
        int32 CurrentAttempt = 0;

        while (MaxAttempts-- > 0) {
            CurrentAttempt++;
            int32 RandomIdx = FMath::RandRange(0, ValidIndices.Num() - 1);

            if (!UsedIndices[RandomIdx]) {
                SelectedIndex = RandomIdx;
                UsedIndices[RandomIdx] = true;
                break;
            }
        }

        if (SelectedIndex == -1) {
            UE_LOG(Myth, Warning, TEXT("LootReward::RequestLootFromSource - Failed to find unused item after %d attempts"), CurrentAttempt);
            continue;
        }

        const auto &SelectedEntry = LootTable->Entries[ValidIndices[SelectedIndex]];

        // Calculate stack size
        int32 StackSize = SelectedEntry.Item->StackSizeMax > 1 ? FMath::RandRange(SelectedEntry.StackRange.Min, SelectedEntry.StackRange.Max) : 1;

        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Selected item: %s (Rarity: %d) with stack size: %d"),
               *SelectedEntry.Item->GetName(),
               static_cast<int32>(SelectedEntry.Item->Rarity),
               StackSize);

        // Spawn the item
        if (InventoryProvider) {
            MythicLootManager->CreateAndGive(
                SelectedEntry.Item,
                StackSize,
                InventoryProvider,
                TargetRecipient,
                DropLevel
                );

            // The item is either in the inventory or in the world, so we're done
            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Item %s spawned in inventory"), *SelectedEntry.Item->GetName());
        }
        else {
            FVector Offset(FMath::RandRange(-50.0f, 50.0f), FMath::RandRange(-50.0f, 50.0f), 0.0f);
            if (!MythicLootManager->CreateAndSpawn(SelectedEntry.Item, SpawnLoc + Offset, TargetRecipient, DropLevel, StackSize, 100)) {
                UE_LOG(Myth, Warning, TEXT("LootReward::RequestLootFromSource - Failed to spawn item %s"), *SelectedEntry.Item->GetName());
                continue;
            }

            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Item %s spawned in world"), *SelectedEntry.Item->GetName());
        }

        SuccessfulDrops++;
    }

    UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Loot generation complete. Successfully dropped %d/%d items from table %s"),
           SuccessfulDrops, NumItemsToDrop, *LootTable->GetName());
}

bool ULootReward::GiveLootReward(ULootReward *Reward, FLootRewardContext Context) {
    return Reward->Give(Context);
}
