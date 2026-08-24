// Copyright Stellar Games. All Rights Reserved.

#include "MythicStatDisplay.h"
#include "Internationalization/Text.h"

namespace MythicStatDisplay {
    static TMap<FString, FMythicStatRule> GRules;
    static bool GBuilt = false;

    static void Add(TMap<FString, FMythicStatRule> &Map, const TCHAR *Attribute, const TCHAR *Label,
                    EMythicStatCategory Category, EMythicStatFormat Format, int32 SortOrder,
                    const TCHAR *MaxAttribute = TEXT(""), bool bHidden = false) {
        FMythicStatRule R;
        R.Attribute = Attribute;
        R.Label = Label;
        R.Category = Category;
        R.Format = Format;
        R.SortOrder = SortOrder;
        R.MaxAttribute = MaxAttribute;
        R.bHidden = bHidden;
        Map.Add(R.Attribute, R);
    }

    static void Hide(TMap<FString, FMythicStatRule> &Map, const TCHAR *Attribute) {
        FMythicStatRule R;
        R.Attribute = Attribute;
        R.bHidden = true;
        R.Category = EMythicStatCategory::Hidden;
        Map.Add(R.Attribute, R);
    }

    static void BuildTable() {
        TMap<FString, FMythicStatRule> &M = GRules;
        M.Reset();

        Add(M, TEXT("Health"), TEXT("Health"), EMythicStatCategory::Vitality, EMythicStatFormat::Integer, 10, TEXT("MaxHealth"));
        Add(M, TEXT("Shield"), TEXT("Shield"), EMythicStatCategory::Vitality, EMythicStatFormat::Integer, 20, TEXT("MaxShield"));
        Add(M, TEXT("CurrentStamina"), TEXT("Stamina"), EMythicStatCategory::Vitality, EMythicStatFormat::Integer, 30, TEXT("MaxStamina"));
        Add(M, TEXT("HealthRegenRate"), TEXT("Health Regeneration"), EMythicStatCategory::Vitality, EMythicStatFormat::PerSecond, 40);
        Add(M, TEXT("ShieldRegenRate"), TEXT("Shield Regeneration"), EMythicStatCategory::Vitality, EMythicStatFormat::PerSecond, 50);
        Add(M, TEXT("StaminaRegenRate"), TEXT("Stamina Regeneration"), EMythicStatCategory::Vitality, EMythicStatFormat::PerSecond, 60);
        Hide(M, TEXT("MaxHealth"));
        Hide(M, TEXT("MaxShield"));
        Hide(M, TEXT("MaxStamina"));

        // Power and Strength are the two the player invests in; everything else on the sheet is
        // downstream of them. The tier is what makes that causality visible instead of presenting
        // one flat list, so they sit in their own category rather than among their own outputs.
        Add(M, TEXT("Power"), TEXT("Power"), EMythicStatCategory::Primary, EMythicStatFormat::Flat, 10);
        Add(M, TEXT("Strength"), TEXT("Strength"), EMythicStatCategory::Primary, EMythicStatFormat::Flat, 20);
        Add(M, TEXT("DamagePerHit"), TEXT("Damage Per Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Flat, 20);
        Add(M, TEXT("AttackSpeed"), TEXT("Attack Speed"), EMythicStatCategory::Offense, EMythicStatFormat::Multiplier, 30);
        Add(M, TEXT("CriticalHitChance"), TEXT("Critical Hit Chance"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 40);
        Add(M, TEXT("CriticalHitDamage"), TEXT("Critical Hit Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 50);
        Add(M, TEXT("OutgoingDamageMultiplier"), TEXT("Outgoing Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Multiplier, 60);
        Add(M, TEXT("BonusSkillDamage"), TEXT("Skill Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 70);
        Add(M, TEXT("ControlPotency"), TEXT("Control Potency"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 71);
        Add(M, TEXT("IncreasedDamageToEnemiesUnderStatusEffects"), TEXT("Damage vs Afflicted"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 80);
        Add(M, TEXT("BonusDamageToSuperiorEnemies"), TEXT("Damage vs Superior"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 90);

        Add(M, TEXT("ApplyBurnOnHitChance"), TEXT("Burn on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 100);
        Add(M, TEXT("ApplyBleedOnHitChance"), TEXT("Bleed on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 110);
        Add(M, TEXT("ApplyPoisonOnHitChance"), TEXT("Poison on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 120);
        Add(M, TEXT("ApplyFreezeOnHitChance"), TEXT("Freeze on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 130);
        Add(M, TEXT("ApplySlowOnHitChance"), TEXT("Slow on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 140);
        Add(M, TEXT("ApplyStunOnHitChance"), TEXT("Stun on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 150);
        Add(M, TEXT("ApplyWeakenOnHitChance"), TEXT("Weaken on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 160);
        Add(M, TEXT("ApplyTerrifyOnHitChance"), TEXT("Terrify on Hit"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 170);

        Add(M, TEXT("BonusSwordDamage"), TEXT("Sword Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 200);
        Add(M, TEXT("BonusAxeDamage"), TEXT("Axe Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 210);
        Add(M, TEXT("BonusDaggerDamage"), TEXT("Dagger Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 220);
        Add(M, TEXT("BonusSickleDamage"), TEXT("Sickle Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 230);
        Add(M, TEXT("BonusSpearDamage"), TEXT("Spear Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 240);
        Add(M, TEXT("BonusHammerDamage"), TEXT("Hammer Damage"), EMythicStatCategory::Offense, EMythicStatFormat::Percent, 250);

        Add(M, TEXT("Armor"), TEXT("Armor"), EMythicStatCategory::Defense, EMythicStatFormat::Flat, 10);
        Add(M, TEXT("DodgeChance"), TEXT("Dodge Chance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 20);
        Add(M, TEXT("IncomingDamageMultiplier"), TEXT("Incoming Damage"), EMythicStatCategory::Defense, EMythicStatFormat::Multiplier, 30);
        Add(M, TEXT("DecreasedDamageFromEnemiesUnderStatusEffects"), TEXT("Reduction vs Afflicted"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 40);
        Add(M, TEXT("LifePerHit"), TEXT("Life per Hit"), EMythicStatCategory::Defense, EMythicStatFormat::Flat, 50);
        Add(M, TEXT("LifePerKill"), TEXT("Life per Kill"), EMythicStatCategory::Defense, EMythicStatFormat::Flat, 60);
        Add(M, TEXT("BurnResistance"), TEXT("Burn Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 100);
        Add(M, TEXT("BleedResistance"), TEXT("Bleed Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 110);
        Add(M, TEXT("PoisonResistance"), TEXT("Poison Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 120);
        Add(M, TEXT("FreezeResistance"), TEXT("Freeze Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 130);
        Add(M, TEXT("SlowResistance"), TEXT("Slow Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 140);
        Add(M, TEXT("StunResistance"), TEXT("Stun Resistance"), EMythicStatCategory::Defense, EMythicStatFormat::Percent, 150);

        Add(M, TEXT("Resolve"), TEXT("Resolve"), EMythicStatCategory::Utility, EMythicStatFormat::Integer, 10);
        Add(M, TEXT("CooldownReduction"), TEXT("Cooldown Reduction"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 20);
        Add(M, TEXT("MaxCooldownReduction"), TEXT("Cooldown Reduction Cap"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 30);
        Add(M, TEXT("StaminaCostReduction"), TEXT("Stamina Cost Reduction"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 40);
        Add(M, TEXT("BonusSprintSpeed"), TEXT("Sprint Speed"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 50);
        Add(M, TEXT("ProficiencyXPBonus"), TEXT("Proficiency XP"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 60);
        Add(M, TEXT("ItemRarityFind"), TEXT("Item Rarity Find"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 70);
        Add(M, TEXT("ItemQuantityFind"), TEXT("Extra Item Drops"), EMythicStatCategory::Utility, EMythicStatFormat::Percent, 80);

        Add(M, TEXT("Nourishment"), TEXT("Nourishment"), EMythicStatCategory::Survival, EMythicStatFormat::Integer, 10, TEXT("MaxNourishment"));
        Add(M, TEXT("Hydration"), TEXT("Hydration"), EMythicStatCategory::Survival, EMythicStatFormat::Integer, 20, TEXT("MaxHydration"));
        Add(M, TEXT("Warmth"), TEXT("Warmth"), EMythicStatCategory::Survival, EMythicStatFormat::Bipolar, 30, TEXT("MaxWarmth"));
        Add(M, TEXT("Wetness"), TEXT("Wetness"), EMythicStatCategory::Survival, EMythicStatFormat::Integer, 40, TEXT("MaxWetness"));
        Hide(M, TEXT("MaxNourishment"));
        Hide(M, TEXT("MaxHydration"));
        Hide(M, TEXT("MaxWarmth"));
        Hide(M, TEXT("MaxWetness"));

        Add(M, TEXT("OverallXp"), TEXT("Overall"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 10, TEXT("OverallXpMax"));
        Add(M, TEXT("CombatProficiency"), TEXT("Combat"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 20, TEXT("CombatProficiencyMax"));
        Add(M, TEXT("MiningProficiency"), TEXT("Mining"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 30, TEXT("MiningProficiencyMax"));
        Add(M, TEXT("WoodcuttingProficiency"), TEXT("Woodcutting"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 40, TEXT("WoodcuttingProficiencyMax"));
        Add(M, TEXT("HarvestingProficiency"), TEXT("Harvesting"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 50, TEXT("HarvestingProficiencyMax"));
        Add(M, TEXT("HuntingProficiency"), TEXT("Hunting"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 60, TEXT("HuntingProficiencyMax"));
        Add(M, TEXT("FishingProficiency"), TEXT("Fishing"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 70, TEXT("FishingProficiencyMax"));
        Add(M, TEXT("FarmingProficiency"), TEXT("Farming"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 80, TEXT("FarmingProficiencyMax"));
        Add(M, TEXT("CookingProficiency"), TEXT("Cooking"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 90, TEXT("CookingProficiencyMax"));
        Add(M, TEXT("AlchemyProficiency"), TEXT("Alchemy"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 100, TEXT("AlchemyProficiencyMax"));
        Add(M, TEXT("CraftingProficiency"), TEXT("Crafting"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 110, TEXT("CraftingProficiencyMax"));
        Add(M, TEXT("ConstructionProficiency"), TEXT("Construction"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 120, TEXT("ConstructionProficiencyMax"));
        Add(M, TEXT("TradingProficiency"), TEXT("Trading"), EMythicStatCategory::Proficiency, EMythicStatFormat::Integer, 130, TEXT("TradingProficiencyMax"));
        const TCHAR *ProficiencyMaxes[] = {
            TEXT("OverallXpMax"), TEXT("CombatProficiencyMax"), TEXT("MiningProficiencyMax"), TEXT("WoodcuttingProficiencyMax"),
            TEXT("HarvestingProficiencyMax"), TEXT("HuntingProficiencyMax"), TEXT("FishingProficiencyMax"), TEXT("FarmingProficiencyMax"),
            TEXT("CookingProficiencyMax"), TEXT("AlchemyProficiencyMax"), TEXT("CraftingProficiencyMax"), TEXT("ConstructionProficiencyMax"),
            TEXT("TradingProficiencyMax")};
        for (const TCHAR *Name : ProficiencyMaxes) {
            Hide(M, Name);
        }

        Hide(M, TEXT("Damage"));
        Hide(M, TEXT("Healing"));
        const TCHAR *Buildups[] = {TEXT("BurnBuildup"), TEXT("BleedBuildup"), TEXT("PoisonBuildup"),
                                   TEXT("SlowBuildup"), TEXT("FreezeBuildup"), TEXT("StunBuildup")};
        for (const TCHAR *Name : Buildups) {
            Hide(M, Name);
        }

        if (const UMythicStatDisplaySettings *Settings = GetDefault<UMythicStatDisplaySettings>()) {
            for (const FMythicStatRule &Override : Settings->Overrides) {
                if (!Override.Attribute.IsEmpty()) {
                    M.Add(Override.Attribute, Override);
                }
            }
        }

        GBuilt = true;
    }

    void InvalidateCache() {
        GBuilt = false;
        GRules.Reset();
    }

    FString MakeFriendlyLabel(const FString &PropertyName) {
        FString Out;
        Out.Reserve(PropertyName.Len() + 8);
        const int32 Len = PropertyName.Len();
        for (int32 i = 0; i < Len; ++i) {
            const TCHAR C = PropertyName[i];
            if (i > 0 && FChar::IsUpper(C)) {
                const TCHAR Prev = PropertyName[i - 1];
                const bool bPrevLower = FChar::IsLower(Prev) || FChar::IsDigit(Prev);
                const bool bAcronymEnd = FChar::IsUpper(Prev) && (i + 1 < Len) && FChar::IsLower(PropertyName[i + 1]);
                if (bPrevLower || bAcronymEnd) {
                    Out.AppendChar(TEXT(' '));
                }
            }
            Out.AppendChar(C);
        }
        return Out;
    }

    FMythicStatRule GetRule(const FGameplayAttribute &Attribute) {
        if (!GBuilt) {
            BuildTable();
        }
        const FString Name = Attribute.GetName();
        if (const FMythicStatRule *Found = GRules.Find(Name)) {
            FMythicStatRule R = *Found;
            if (R.Label.IsEmpty()) {
                R.Label = MakeFriendlyLabel(Name);
            }
            return R;
        }

        FMythicStatRule R;
        R.Attribute = Name;
        R.Label = MakeFriendlyLabel(Name);
        R.Category = EMythicStatCategory::Utility;
        R.SortOrder = 9000;
        if (Name.EndsWith(TEXT("Chance")) || Name.EndsWith(TEXT("Resistance")) || Name.EndsWith(TEXT("Reduction"))) {
            R.Format = EMythicStatFormat::Percent;
        }
        else if (Name.EndsWith(TEXT("Multiplier"))) {
            R.Format = EMythicStatFormat::Multiplier;
        }
        else if (Name.EndsWith(TEXT("RegenRate")) || Name.EndsWith(TEXT("PerSecond"))) {
            R.Format = EMythicStatFormat::PerSecond;
        }
        else {
            R.Format = EMythicStatFormat::Flat;
        }
        return R;
    }

    static FString TrimNumber(float Value, int32 MaxDecimals) {
        FString S = FString::SanitizeFloat(Value, 0);
        if (MaxDecimals > 0) {
            S = FString::Printf(TEXT("%.*f"), MaxDecimals, Value);
            if (S.Contains(TEXT("."))) {
                S.RemoveFromEnd(TEXT("0"));
                S.RemoveFromEnd(TEXT("."));
            }
        }
        else {
            S = FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
        }
        return S;
    }

    FText FormatValue(float Value, EMythicStatFormat Format) {
        switch (Format) {
            case EMythicStatFormat::Integer:
            case EMythicStatFormat::Bipolar:
                return FText::FromString(TrimNumber(Value, 0));
            case EMythicStatFormat::Percent:
                return FText::FromString(TrimNumber(Value * 100.0f, 1) + TEXT("%"));
            case EMythicStatFormat::Multiplier: {
                const float Delta = (Value - 1.0f) * 100.0f;
                const TCHAR *Sign = Delta >= 0.0f ? TEXT("+") : TEXT("");
                return FText::FromString(FString::Printf(TEXT("%s%s%%"), Sign, *TrimNumber(Delta, 1)));
            }
            case EMythicStatFormat::PerSecond:
                return FText::FromString(TrimNumber(Value, 1) + TEXT("/s"));
            case EMythicStatFormat::Flat:
            default:
                return FText::FromString(TrimNumber(Value, 1));
        }
    }

    FText FormatBonus(float Delta, EMythicStatFormat Format) {
        if (FMath::IsNearlyZero(Delta, 0.001f)) {
            return FText::GetEmpty();
        }
        const TCHAR *Sign = Delta > 0.0f ? TEXT("+") : TEXT("-");
        const float Abs = FMath::Abs(Delta);
        switch (Format) {
            case EMythicStatFormat::Percent:
                return FText::FromString(FString::Printf(TEXT("%s%s%%"), Sign, *TrimNumber(Abs * 100.0f, 1)));
            case EMythicStatFormat::Multiplier:
                return FText::FromString(FString::Printf(TEXT("%s%s%%"), Sign, *TrimNumber(Abs * 100.0f, 1)));
            case EMythicStatFormat::PerSecond:
                return FText::FromString(FString::Printf(TEXT("%s%s/s"), Sign, *TrimNumber(Abs, 1)));
            case EMythicStatFormat::Integer:
            case EMythicStatFormat::Bipolar:
                return FText::FromString(FString::Printf(TEXT("%s%s"), Sign, *TrimNumber(Abs, 0)));
            case EMythicStatFormat::Flat:
            default:
                return FText::FromString(FString::Printf(TEXT("%s%s"), Sign, *TrimNumber(Abs, 1)));
        }
    }

    FText GetCategoryLabel(EMythicStatCategory Category) {
        switch (Category) {
            case EMythicStatCategory::Primary:
                return NSLOCTEXT("MythicStats", "Cat_Primary", "Primary");
            case EMythicStatCategory::Vitality:
                return NSLOCTEXT("MythicStats", "Cat_Vitality", "Vitality");
            case EMythicStatCategory::Offense:
                return NSLOCTEXT("MythicStats", "Cat_Offense", "Offense");
            case EMythicStatCategory::Defense:
                return NSLOCTEXT("MythicStats", "Cat_Defense", "Defense");
            case EMythicStatCategory::Utility:
                return NSLOCTEXT("MythicStats", "Cat_Utility", "Utility");
            case EMythicStatCategory::Proficiency:
                return NSLOCTEXT("MythicStats", "Cat_Proficiency", "Proficiencies");
            case EMythicStatCategory::Survival:
                return NSLOCTEXT("MythicStats", "Cat_Survival", "Survival");
            default:
                return FText::GetEmpty();
        }
    }
}
