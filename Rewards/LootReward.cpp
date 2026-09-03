

#include "LootReward.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Mythic.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Rewards/LootScaling.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/MythicTags_Inventory.h"

bool ULootReward::ResolveEntryDropChance(float OverrideDropChance, int32 RarityIndex, TConstArrayView<float> RarityWeights, float &OutChance) {
    if (OverrideDropChance > 0.0f) {
        OutChance = OverrideDropChance;
        return true;
    }
    if (RarityWeights.IsValidIndex(RarityIndex)) {
        OutChance = RarityWeights[RarityIndex];
        return true;
    }
    OutChance = 0.0f;
    return false;
}

FLootTierBonus ULootReward::PrepareLootRoll(UMythicLootManagerSubsystem *LootManager, APlayerController *PlayerController, const int32 EnemyTierInt,
                                            const float QuantityFind) {
    FLootTierBonus Bonus = FMythicLootScaling::ComputeTierLootBonus(EnemyTierInt, QuantityFind);
    if (LootManager && EnemyTierInt > 0) {
        LootManager->OnPreLootRoll.Broadcast(PlayerController, Bonus);
    }
    return Bonus;
}

bool ULootReward::Give(FRewardContext &Context) const {
    FLootRewardContext *LootContext = static_cast<FLootRewardContext *>(&Context);
    checkf(LootContext, TEXT("LootReward::Give - LootRewardContext is null"));

    auto PlayerController = LootContext->PlayerController;
    checkf(PlayerController, TEXT("LootReward::Give - PlayerController is null"));

    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    auto MythicLootManager = PlayerController->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(MythicLootManager, TEXT("LootReward::Give - MythicLootManager not found"));

    auto GameState = PlayerController->GetWorld()->GetGameState<AMythicGameState>();
    checkf(GameState, TEXT("LootReward::Give - GameState is null"));

    auto WorldTierAttributes = GameState->WorldTierAttributes;
    checkf(GameState->WorldTierAttributes, TEXT("LootReward::Give - WorldTierAttributes is null"));

    auto LevelFound = false;
    auto PlayerLevel = UMythicAttributeSet_Proficiencies::GetLevel(ASC, LevelFound);
    if (!LevelFound) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Failed to get player level - Using Level 1"));
        PlayerLevel = 1;
    }

    UE_LOG(Myth, Warning, TEXT("LootReward::Give - Current Player Level: %d"), PlayerLevel);

    const UMythicDeveloperSettings *DropSettings = GetDefault<UMythicDeveloperSettings>();
    const float GlobalDropMult = DropSettings ? FMath::Max(0.0f, DropSettings->DropRateMultiplier) : 1.0f;

    const float CommonRate = GameState->CommonLootChanceCurveRowHandle.Eval(PlayerLevel, "") * GlobalDropMult;
    const float RareRate = GameState->RareLootChanceCurveRowHandle.Eval(PlayerLevel, "") * GlobalDropMult;
    const float EpicRate = GameState->EpicLootChanceCurveRowHandle.Eval(PlayerLevel, "") * GlobalDropMult;
    const float LegendaryRate = GameState->LegendaryLootChanceCurveRowHandle.Eval(PlayerLevel, "") * WorldTierAttributes->GetLegendaryDropRateMultiplier() * GlobalDropMult;
    const float MythicRate = GameState->MythicLootChanceCurveRowHandle.Eval(PlayerLevel, "") * WorldTierAttributes->GetMythicDropRateMultiplier() * GlobalDropMult;
    const float GoldMult = WorldTierAttributes->GetGoldDropRateMultiplier();

    UE_LOG(Myth, Log, TEXT("LootReward::Give - Rarities for Level %d = Common: %f, Rare: %f, Epic: %f, Legendary: %f, Mythic: %f"), PlayerLevel, CommonRate,
           RareRate, EpicRate, LegendaryRate, MythicRate);

    bool bRarityFindFound = false;
    bool bQuantityFindFound = false;
    const float RarityFind = ASC->GetGameplayAttributeValue(UMythicAttributeSet_Utility::GetItemRarityFindAttribute(), bRarityFindFound);
    const float QuantityFind = ASC->GetGameplayAttributeValue(UMythicAttributeSet_Utility::GetItemQuantityFindAttribute(), bQuantityFindFound);
    const FLootTierBonus TierBonus = PrepareLootRoll(MythicLootManager, PlayerController, LootContext->EnemyTierInt, QuantityFind);

    for (auto LootTable : OverridenLootSource.LootTables) {
        UE_LOG(Myth, Log, TEXT("LootReward::Give - Using loot source %s"), *LootTable->GetName());
        RequestLootFromSource(CommonRate, RareRate, EpicRate, LegendaryRate, MythicRate, GoldMult, PlayerController, LootContext->ItemLevel, LootTable,
                              LootContext->PutInInventory, OverridenLootSource.IsPrivate, LootContext->SpawnLocation, MythicLootManager,
                              RarityFind, TierBonus);
    }
    if (OverridenLootSource.bSkipGlobal) {
        UE_LOG(Myth, Log, TEXT("LootReward::Give - Skipping global loot source"));
        return true;
    }

    auto MythicSettings = GetDefault<UMythicDeveloperSettings>();
    if (!MythicSettings) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Mythic Settings not found"));
        return false;
    }

    auto LootTable = MythicSettings->GlobalLootTable.Get();
    if (!LootTable) {
        UE_LOG(Myth, Error, TEXT("LootReward::Give - Global loot table not found"));
        return false;
    }

    UE_LOG(Myth, Log, TEXT("LootReward::Give - Using global loot source"));
    RequestLootFromSource(CommonRate, RareRate, EpicRate, LegendaryRate, MythicRate, GoldMult, PlayerController, LootContext->ItemLevel,
                          LootTable, LootContext->PutInInventory, OverridenLootSource.IsPrivate, LootContext->SpawnLocation,
                          MythicLootManager, RarityFind, TierBonus);

    return true;
}

void ULootReward::RequestLootFromSource(float CommonRate, float RareRate, float EpicRate, float LegendaryRate, float MythicRate,
                                        float GoldMultiplier,
                                        APlayerController *PlayerController, int32 DropLevel,
                                        UMythicLootTable *LootTable, TScriptInterface<IInventoryProviderInterface> InventoryProvider, bool isPrivate,
                                        FVector SpawnLocation,
                                        UMythicLootManagerSubsystem *MythicLootManager,
                                        float RarityFind, const FLootTierBonus &TierBonus) {
    if (!LootTable || LootTable->Entries.Num() == 0) {
        UE_LOG(Myth, Error, TEXT("LootReward::RequestLootFromSource - Loot table is empty or invalid"));
        return;
    }

    float procRoll = FMath::FRand();
    if (procRoll > LootTable->DropChance) {
        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Table failed to proc - Required: %.2f, Rolled: %.2f"),
               LootTable->DropChance, procRoll);
        return;
    }

    constexpr int32 NumRarities = 5;
    float WeightsByRarity[NumRarities] = {
        CommonRate,
        RareRate,
        EpicRate,
        LegendaryRate,
        MythicRate
    };

    const float EffectiveRarityFind = FMath::Max(0.0f, RarityFind) + (TierBonus.RarityMult - 1.0f);
    FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(WeightsByRarity, NumRarities), EffectiveRarityFind);

    auto TargetRecipient = isPrivate ? PlayerController : nullptr;

    FVector SpawnLoc = SpawnLocation;
    if (SpawnLoc.IsZero() && !InventoryProvider) {
        auto RecipientPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
        if (!IsValid(RecipientPawn)) {
            UE_LOG(Myth, Warning,
                   TEXT("LootReward::RequestLootFromSource - ZeroVector spawn location, no inventory provider, and no "
                        "pawn to resolve a drop spot (recipient is pawn-less); skipping world-drop from table %s."),
                   *GetNameSafe(LootTable));
            return;
        }
        SpawnLoc = RecipientPawn->GetActorLocation();
    }

    TArray<int32> ValidIndices;
    ValidIndices.Reserve(LootTable->Entries.Num());

    UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Checking drop chances for items in table %s"), *LootTable->GetName());

    for (int32 i = 0; i < LootTable->Entries.Num(); ++i) {
        const auto &Entry = LootTable->Entries[i];
        if (!Entry.Item) [[unlikely]]
            continue;

        const int32 RarityIndex = static_cast<int32>(Entry.Item->Rarity);
        float DropChance = 0.0f;
        if (!ResolveEntryDropChance(Entry.OverrideDropChance, RarityIndex, MakeArrayView(WeightsByRarity, NumRarities), DropChance)) {
            UE_LOG(Myth, Warning,
                   TEXT("LootReward::RequestLootFromSource - Item %s has out-of-range rarity %d with no OverrideDropChance — skipping"),
                   *Entry.Item->GetName(), RarityIndex);
            continue;
        }

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

    int32 GuaranteedValidPos = -1;
    if (TierBonus.GuaranteedMinRarity > 0) {
        int32 BestEntry = -1;
        int32 BestRarity = -1;
        for (int32 i = 0; i < LootTable->Entries.Num(); ++i) {
            const auto &Entry = LootTable->Entries[i];
            if (!Entry.Item) {
                continue;
            }
            const int32 R = static_cast<int32>(Entry.Item->Rarity);
            if (R >= TierBonus.GuaranteedMinRarity && R > BestRarity) {
                BestRarity = R;
                BestEntry = i;
            }
        }
        if (BestEntry >= 0) {
            int32 Pos = ValidIndices.IndexOfByKey(BestEntry);
            if (Pos == INDEX_NONE) {
                ValidIndices.Add(BestEntry);
                Pos = ValidIndices.Num() - 1;
            }
            GuaranteedValidPos = Pos;
            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Boss floor: guaranteeing rarity>=%d drop (%s)"),
                   TierBonus.GuaranteedMinRarity, *LootTable->Entries[BestEntry].Item->GetName());
        }
    }

    if (ValidIndices.Num() == 0) {
        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - No items passed the drop chance check"));
        return;
    }

    const int32 NumItemsToDrop = FMythicLootScaling::ResolveDropCount(
        FMath::RandRange(1, FMath::Min(LootTable->MaxItems, ValidIndices.Num())), TierBonus, FMath::FRand());
    if (NumItemsToDrop <= 0) {
        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - The pre-loot roll suppressed every drop from table %s"),
               *LootTable->GetName());
        return;
    }
    UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Will drop %d items from %d eligible items (tier extra: %d, scale: %.2f)"),
           NumItemsToDrop, ValidIndices.Num(), TierBonus.ExtraDropCount, TierBonus.DropCountScale);

    TBitArray<> UsedIndices;
    UsedIndices.Init(false, ValidIndices.Num());

    int32 SuccessfulDrops = 0;
    for (int32 DropCount = 0; DropCount < NumItemsToDrop; ++DropCount) {
        if (UsedIndices.CountSetBits() == ValidIndices.Num()) {
            UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Reset used items tracking - all items have been used"));
            UsedIndices.Init(false, ValidIndices.Num());
        }

        int32 MaxAttempts = 10;
        int32 SelectedIndex = -1;
        int32 CurrentAttempt = 0;

        if (DropCount == 0 && GuaranteedValidPos >= 0 && ValidIndices.IsValidIndex(GuaranteedValidPos)) {
            SelectedIndex = GuaranteedValidPos;
            UsedIndices[GuaranteedValidPos] = true;
        }

        while (SelectedIndex == -1 && MaxAttempts-- > 0) {
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

        int32 StackSize = SelectedEntry.Item->StackSizeMax > 1 ? FMath::RandRange(SelectedEntry.StackRange.Min, SelectedEntry.StackRange.Max) : 1;

        if (SelectedEntry.Item->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            const int32 ScaledStack = FMath::RoundToInt(StackSize * GoldMultiplier);
            const int32 MaxStack = FMath::Max(1, SelectedEntry.Item->StackSizeMax);
            if (ScaledStack > MaxStack) {
                UE_LOG(Myth, Warning,
                       TEXT("LootReward: currency %s scaled to %d exceeds StackSizeMax %d; clamping (excess discarded). "
                           "Raise its StackSizeMax to capture the full gold drop."),
                       *SelectedEntry.Item->GetName(), ScaledStack, MaxStack);
            }
            StackSize = FMath::Clamp(ScaledStack, 1, MaxStack);
        }

        UE_LOG(Myth, Log, TEXT("LootReward::RequestLootFromSource - Selected item: %s (Rarity: %d) with stack size: %d"),
               *SelectedEntry.Item->GetName(),
               static_cast<int32>(SelectedEntry.Item->Rarity),
               StackSize);

        if (InventoryProvider) {
            MythicLootManager->CreateAndGive(
                SelectedEntry.Item,
                StackSize,
                InventoryProvider,
                TargetRecipient,
                DropLevel
                );

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
