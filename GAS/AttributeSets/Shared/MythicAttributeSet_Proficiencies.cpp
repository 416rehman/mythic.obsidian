#include "MythicAttributeSet_Proficiencies.h"

#include "Mythic.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Net/UnrealNetwork.h"
#include "GAS/MythicTags_GAS.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MythicAttributeSet_Utility.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "GameFramework/PlayerState.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Stats/MythicStatDefinition.h"
#include "World/Camping/MythicRestedXp.h"

namespace {
const UMythicStatDefinition *FindRegisteredStatDefinition(
    const UMythicAttributeSet_Proficiencies &AttributeSet,
    const FGameplayAttribute &Attribute) {
    const UWorld *World = AttributeSet.GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    return Registry && Registry->IsCoreSemanticReady()
        ? Registry->FindStat(Attribute) : nullptr;
}
}

void UMythicAttributeSet_Proficiencies::OnRep_CombatProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CombatProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_WoodcuttingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_WoodcuttingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_ConstructionProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_ConstructionProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiencyMax, OldValue);
}


void UMythicAttributeSet_Proficiencies::OnRep_TradingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_TradingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HuntingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HuntingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CraftingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CraftingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_AlchemyProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_AlchemyProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CookingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CookingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_MiningProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_MiningProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FarmingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FarmingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FishingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FishingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HarvestingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HarvestingProficiencyMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiencyMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_OverallXp(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, OverallXp, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_OverallXpMax(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, OverallXpMax, OldValue);
}

void UMythicAttributeSet_Proficiencies::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiency, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiencyMax, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, OverallXp, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, OverallXpMax, COND_OwnerOnly, REPNOTIFY_Always);
}

float UMythicAttributeSet_Proficiencies::GetMaxValueForAttribute(
    const FGameplayAttribute &Attribute) const {
    const UMythicStatDefinition *Current = FindRegisteredStatDefinition(*this, Attribute);
    const UMythicStatDefinition *Capacity = Current
        && Current->PairRole == EMythicStatPairRole::Current
        ? Current->PairedStat.GetAsset() : nullptr;
    if (Capacity && Capacity->PairRole == EMythicStatPairRole::Capacity
        && Capacity->PairedStat.GetAsset() == Current && Capacity->Attribute.IsValid()) {
        return Capacity->Attribute.GetNumericValue(this);
    }
    return -1;
}

void UMythicAttributeSet_Proficiencies::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    const UMythicStatDefinition *Definition = FindRegisteredStatDefinition(*this, Attribute);
    if (!Definition || Attribute == GetOverallXpAttribute()) {
        return;
    }

    if (Definition->PairRole == EMythicStatPairRole::Capacity) {
        NewValue = FMath::Max(NewValue, 0.0f);
        return;
    }

    float MaxValue = GetMaxValueForAttribute(Attribute);
    if (MaxValue > 0.0f) {
        NewValue = FMath::Clamp(NewValue, 0.0f, MaxValue);
    }
}

void UMythicAttributeSet_Proficiencies::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);

    const UMythicStatDefinition *Definition = FindRegisteredStatDefinition(*this, Attribute);
    if (!Definition || Definition->PairRole != EMythicStatPairRole::Current
        || Attribute == GetOverallXpAttribute()) {
        return;
    }

    const UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
    if (ASC) {
        AMythicPlayerController *MythicPC = nullptr;
        if (AActor *OwnerActor = ASC->GetOwnerActor()) {
            if (APawn *Pawn = Cast<APawn>(OwnerActor)) {
                MythicPC = Cast<AMythicPlayerController>(Pawn->GetController());
            }
            else if (APlayerState *PS = Cast<APlayerState>(OwnerActor)) {
                MythicPC = Cast<AMythicPlayerController>(PS->GetPlayerController());
            }
        }
        if (!MythicPC) {
            if (APawn *Avatar = Cast<APawn>(ASC->GetAvatarActor())) {
                MythicPC = Cast<AMythicPlayerController>(Avatar->GetController());
            }
        }
        if (MythicPC) {
            if (const UProficiencyComponent *ProfComp = MythicPC->GetProficiencyComponent()) {
                if (ProfComp->IsRestoring()) {
                    return;
                }
            }
        }
    }

    const float OldValue = Attribute.GetNumericValue(this);
    const float Delta = NewValue - OldValue;

    auto ScaledDelta= ScaleProficiencyXpGain(Delta, GetOwningAbilitySystemComponent());

    NewValue = OldValue + ScaledDelta;
}

float UMythicAttributeSet_Proficiencies::ComposeXpMultipliers(float ProficiencyXpBonus, float EnlightenBonus, float RestedMultiplier) {
    const float Additive = 1.0f + ProficiencyXpBonus + EnlightenBonus;
    return FMath::Max(0.0f, Additive) * FMath::Max(0.0f, RestedMultiplier);
}

float UMythicAttributeSet_Proficiencies::ScaleProficiencyXpGain(
    float BaseXp,
    const UAbilitySystemComponent* ASC) const
{
    if (BaseXp <= 0.0f || !ASC)
    {
        return BaseXp;
    }

    float ProficiencyBonus = 0.0f;
    if (const UMythicAttributeSet_Utility* Util = ASC->GetSet<UMythicAttributeSet_Utility>())
    {
        ProficiencyBonus = Util->GetProficiencyXPBonus();
    }

    float EnlightenBonus = 0.0f;
    float WorldTierXpMultiplier = 1.0f;
    if (const UWorld* World = GetWorld())
    {
        if (const AMythicGameState* GS = World->GetGameState<AMythicGameState>())
        {
            if (ASC->HasMatchingGameplayTag(GAS_BUFF_ENLIGHTEN))
            {
                EnlightenBonus = GS->EnlightenProficiencyBonus;
            }
            if (const UWorldTierAttributes* WTA = GS->WorldTierAttributes)
            {
                WorldTierXpMultiplier = WTA->GetExperienceGainMultiplier();
            }
        }
    }

    // Rested lives here rather than at one caller, so every kind of work is worth more while rested instead of
    // only the kinds that happen to pay out through a reward asset.
    const float Multiplier = ComposeXpMultipliers(ProficiencyBonus, EnlightenBonus,
                                                  MythicCampsite::ReadRestedXpMultiplier(ASC));
    return ApplyWorldTierXpMultiplier(BaseXp * Multiplier, WorldTierXpMultiplier);
}

float UMythicAttributeSet_Proficiencies::ApplyWorldTierXpMultiplier(float ScaledXp, float WorldTierMultiplier)
{
    return ScaledXp * (WorldTierMultiplier > 0.0f ? WorldTierMultiplier : 1.0f);
}

void UMythicAttributeSet_Proficiencies::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) {
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    const UMythicStatDefinition *Definition = FindRegisteredStatDefinition(*this, Attribute);
    if (!Definition) {
        return;
    }
    if (Definition->PairRole == EMythicStatPairRole::Capacity
        && Attribute != GetOverallXpMaxAttribute()) {
        SetOverallXpMax(CalculateOverallXpMax());
    }
    else if (Definition->PairRole == EMythicStatPairRole::Current
             && Attribute != GetOverallXpAttribute()) {
        auto OverallLevel = CalculateOverallXp();
        SetOverallXp(OverallLevel);
    }
}

int32 UMythicAttributeSet_Proficiencies::GetLevel(const UAbilitySystemComponent *ASC, bool &found) {
    found = false;
    auto CurrentXp = ASC->GetGameplayAttributeValue(GetOverallXpAttribute(), found);
    if (!found) {
        UE_LOG(Myth, Error, TEXT("Unable to find OverallXp attribute in ASC"));
        return 0;
    }

    auto MaxXp = ASC->GetGameplayAttributeValue(GetOverallXpMaxAttribute(), found);
    if (!found) {
        UE_LOG(Myth, Error, TEXT("Unable to find OverallXpMax attribute in ASC"));
        return 0;
    }

    return LevelFromXp(CurrentXp, MaxXp);
}

int32 UMythicAttributeSet_Proficiencies::LevelFromXp(const float CurrentXp, const float MaxXp) {
    auto Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings) {
        UE_LOG(Myth, Error, TEXT("Unable to find MythicDeveloperSettings"));
        return 0;
    }

    const float MaxLevel = Settings->MaxLevel;
    if (MaxXp <= 0.0f) {
        return 1;
    }
    const float Fraction = FMath::Clamp(CurrentXp / MaxXp, 0.0f, 1.0f);

    // Proficiency XP curves are geometric, so the overall pair (a weighted average of them) is too. A
    // flat ratio against the lifetime total parks a new character at level 1 for hours; inverting the
    // same authored curve shape (the combat track's growth rate, rescaled to the overall span) paces
    // overall levels like proficiency levels. Falls back to the flat ratio only when no track is
    // authored.
    const UProficiencyDefinition *CombatDef =
        UProficiencyDefinition::FindByProgressAttribute(GetCombatProficiencyAttribute());
    if (CombatDef && CombatDef->GrowthRate > 1.0f + KINDA_SMALL_NUMBER && CombatDef->MaxLevel > 1) {
        const float Growth = CombatDef->GrowthRate;
        const float TrackSpan = static_cast<float>(CombatDef->MaxLevel);
        const float GeometricSpan = FMath::Pow(Growth, TrackSpan - 1.0f) - 1.0f;
        const float TrackLevel = 1.0f + FMath::LogX(Growth, 1.0f + Fraction * GeometricSpan);
        const float Level = 1.0f + (TrackLevel - 1.0f) * (MaxLevel - 1.0f) / (TrackSpan - 1.0f);
        return FMath::FloorToInt32(FMath::Clamp(Level, 1.0f, MaxLevel));
    }
    return FMath::FloorToInt32(FMath::Clamp(1.0f + Fraction * (MaxLevel - 1.0f), 1.0f, MaxLevel));
}

void UMythicAttributeSet_Proficiencies::GetLevelXpWindow(const float CurrentXp, const float MaxXp,
                                                         int32 &OutLevel, float &OutIntoLevel, float &OutLevelSpan) {
    OutLevel = LevelFromXp(CurrentXp, MaxXp);
    OutIntoLevel = 0.0f;
    OutLevelSpan = 0.0f;
    auto Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || MaxXp <= 0.0f) {
        return;
    }
    const float MaxLevel = static_cast<float>(Settings->MaxLevel);

    // The same curve LevelFromXp inverts, run forward: the overall XP at which a given overall level
    // starts. The two must share one shape or the bar would disagree with the number beside it.
    const UProficiencyDefinition *CombatDef =
        UProficiencyDefinition::FindByProgressAttribute(GetCombatProficiencyAttribute());
    auto XpAtLevel = [&](const float Level) -> float {
        const float Clamped = FMath::Clamp(Level, 1.0f, MaxLevel);
        if (CombatDef && CombatDef->GrowthRate > 1.0f + KINDA_SMALL_NUMBER && CombatDef->MaxLevel > 1) {
            const float Growth = CombatDef->GrowthRate;
            const float TrackSpan = static_cast<float>(CombatDef->MaxLevel);
            const float TrackLevel = 1.0f + (Clamped - 1.0f) * (TrackSpan - 1.0f) / (MaxLevel - 1.0f);
            return MaxXp * (FMath::Pow(Growth, TrackLevel - 1.0f) - 1.0f) / (FMath::Pow(Growth, TrackSpan - 1.0f) - 1.0f);
        }
        return MaxXp * (Clamped - 1.0f) / (MaxLevel - 1.0f);
    };

    const float LevelStart = XpAtLevel(static_cast<float>(OutLevel));
    const float LevelEnd = OutLevel >= Settings->MaxLevel ? MaxXp : XpAtLevel(static_cast<float>(OutLevel + 1));
    OutIntoLevel = FMath::Max(0.0f, CurrentXp - LevelStart);
    OutLevelSpan = FMath::Max(0.0f, LevelEnd - LevelStart);
}

float UMythicAttributeSet_Proficiencies::CalculateOverallXpMax() {
    struct ProficiencyWeight {
        float Value;
        float Weight;
        float Contribution() const { return Value * Weight; }
    };

    TArray<ProficiencyWeight> WeightedMaxProficiencies = {
        {GetCombatProficiencyMax(), COMBAT_WEIGHT},
        {GetMiningProficiencyMax(), MINING_WEIGHT},
        {GetWoodcuttingProficiencyMax(), WOODCUTTING_WEIGHT},
        {GetHuntingProficiencyMax(), HUNTING_WEIGHT},
        {GetFarmingProficiencyMax(), FARMING_WEIGHT},
        {GetFishingProficiencyMax(), FISHING_WEIGHT},
        {GetHarvestingProficiencyMax(), HARVESTING_WEIGHT},
        {GetTradingProficiencyMax(), TRADING_WEIGHT},
        {GetCraftingProficiencyMax(), CRAFTING_WEIGHT},
        {GetConstructionProficiencyMax(), CONSTRUCTION_WEIGHT},
        {GetAlchemyProficiencyMax(), ALCHEMY_WEIGHT},
        {GetCookingProficiencyMax(), COOKING_WEIGHT}
    };

    float WeightedSum = 0.0f;
    float TotalWeight = 0.0f;

    for (const auto &Prof : WeightedMaxProficiencies) {
        WeightedSum += Prof.Contribution();
        TotalWeight += Prof.Weight;
    }

    float WeightedAverage = WeightedSum / TotalWeight;

    float MaxProgressionMultiplier = 1.0f + HIGH_SKILL_MULTIPLIER + MEDIUM_SKILL_MULTIPLIER + LOW_SKILL_MULTIPLIER;

    float CalculatedOverallXpMax = WeightedAverage * MaxProgressionMultiplier;

    return CalculatedOverallXpMax;
}

float UMythicAttributeSet_Proficiencies::CalculateOverallXp() {
    struct ProficiencyWeight {
        float Value;
        float Weight;
        float MaxValue;
        float Contribution() const { return Value * Weight; }
        float GetPercentage() const { return MaxValue > 0 ? Value / MaxValue : 0.0f; }
    };
    TArray<ProficiencyWeight> WeightedProficiencies = {
        {GetCombatProficiency(), COMBAT_WEIGHT, GetCombatProficiencyMax()},

        {GetMiningProficiency(), MINING_WEIGHT, GetMiningProficiencyMax()},
        {GetWoodcuttingProficiency(), WOODCUTTING_WEIGHT, GetWoodcuttingProficiencyMax()},
        {GetHuntingProficiency(), HUNTING_WEIGHT, GetHuntingProficiencyMax()},
        {GetFarmingProficiency(), FARMING_WEIGHT, GetFarmingProficiencyMax()},

        {GetFishingProficiency(), FISHING_WEIGHT, GetFishingProficiencyMax()},
        {GetHarvestingProficiency(), HARVESTING_WEIGHT, GetHarvestingProficiencyMax()},
        {GetTradingProficiency(), TRADING_WEIGHT, GetTradingProficiencyMax()},

        {GetCraftingProficiency(), CRAFTING_WEIGHT, GetCraftingProficiencyMax()},
        {GetConstructionProficiency(), CONSTRUCTION_WEIGHT, GetConstructionProficiencyMax()},
        {GetAlchemyProficiency(), ALCHEMY_WEIGHT, GetAlchemyProficiencyMax()},
        {GetCookingProficiency(), COOKING_WEIGHT, GetCookingProficiencyMax()}
    };

    float WeightedSum = 0.0f;
    float TotalWeight = 0.0f;

    for (const auto &Prof : WeightedProficiencies) {
        WeightedSum += Prof.Contribution();
        TotalWeight += Prof.Weight;
    }

    float WeightedAverage = WeightedSum / TotalWeight;

    float ProgressionMultiplier = 1.0f;

    int32 HighSkills = 0;
    int32 MediumSkills = 0;
    int32 LowSkills = 0;

    for (const auto &Prof : WeightedProficiencies) {
        float Percentage = Prof.GetPercentage();
        if (Percentage >= 0.75f)
            HighSkills++;
        else if (Percentage >= 0.50f)
            MediumSkills++;
        else if (Percentage >= 0.25f)
            LowSkills++;
    }

    if (HighSkills >= 3)
        ProgressionMultiplier += HIGH_SKILL_MULTIPLIER;
    if (MediumSkills >= 6)
        ProgressionMultiplier += MEDIUM_SKILL_MULTIPLIER;
    if (LowSkills >= 9)
        ProgressionMultiplier += LOW_SKILL_MULTIPLIER;

    if (HighSkills == 1 && MediumSkills <= 2) {
        ProgressionMultiplier -= 0.20f;
    }

    float CalculatedOverallXp = WeightedAverage * ProgressionMultiplier;

    float MaxPossible = GetOverallXpMax();
    if (MaxPossible > 0) {
        CalculatedOverallXp = FMath::Min(CalculatedOverallXp, MaxPossible);
    }

    return FMath::Max(CalculatedOverallXp, 0.0f);
}
