#pragma once
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "Misc/DataValidation.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "GameplayTagContainer.h"
#include "FragmentTypes.generated.h"

struct FRolledTagSpec;
namespace MythicFragmentSerialization {
inline constexpr int32 MaxIdentityStringBytes = 4096;

/** Serializes a bounded canonical UTF-8 field as a byte count followed by bytes without a terminator. */
MYTHIC_API bool SerializeBoundedUtf8(FArchive &Ar, FString &Value, int32 MaxBytes,
                                     bool bRequired);

/** Resolves an attribute only when its native AttributeSet and property belong to the supported item-stat domain. */
MYTHIC_API bool ResolveAllowedAttribute(const FString &AttributeSetClassPath,
                                        const FString &AttributePropertyName,
                                        FGameplayAttribute &OutAttribute);
}

USTRUCT(Blueprintable, BlueprintType)
struct MYTHIC_API FRollDefinition {
    GENERATED_BODY()

    // When rolling a value, this is the minimum value that can be rolled, before any level scaling is applied
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float Min = 0;

    // When rolling a value, this is the maximum value that can be rolled, before any level scaling is applied
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float Max = 0;

    /** GAS modifier operation used when this legacy non-affix roll definition is applied. */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Itemization")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;

    // Level scaling modifier. The item level will be multiplied by this and added to the Min and Max values.
    // For example, if Min is 10 and Max is 20, and LevelScaling is 0.5, at level 10, the Min will be 15 and the Max will be 25.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float LevelScaling = 0.0f;

    // Lower is better? For example, for cooldowns, lower is better.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation")
    bool bLowerIsBetter = false;

    // Forces a percent reading on an attribute whose StatDefinition asset declares Flat presentation. The
    // data-driven stat definitions decide units for every other format, so this stays off unless a roll genuinely
    // needs to override them.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation")
    bool bIsPercentage = false;

    // Whole-number rolls: the rolled VALUE snaps to an integer at roll and reroll time, server-side.
    // For flat stats a player reads as counts (Power, Strength, Resolve, Armor) a +3.4 roll is noise
    // the UI would have to hide; authored here so itemization owns the rule, not the display layer.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    bool bWholeNumber = false;

    // Fractional digits the rich text prints the value and its range with. Zero keeps the whole-number reading
    // every talent has always had; a rune window of 0.35 s needs two.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation", meta=(ClampMin="0", ClampMax="3"))
    int32 DisplayDecimals = 0;

    // Unit printed after the value (" s", " m", "x"). bIsPercentage prints its % sign first.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation")
    FText DisplaySuffix;

    // The number as the rich text prints it. At zero decimals the value truncates to an int, which is what talents
    // have always shown; the unit only follows the rolled value, never the range, matching the % convention.
    FString FormatForDisplay(float Value, bool bWithUnit) const {
        const float Shown = bIsPercentage ? Value * 100.0f : Value;
        FString Number;
        if (DisplayDecimals > 0) {
            FNumberFormattingOptions Options;
            Options.UseGrouping = false;
            Options.MinimumFractionalDigits = DisplayDecimals;
            Options.MaximumFractionalDigits = DisplayDecimals;
            Number = FText::AsNumber(Shown, &Options).ToString();
        }
        else {
            Number = FString::Printf(TEXT("%d"), static_cast<int>(Shown));
        }
        if (bWithUnit) {
            if (bIsPercentage) {
                Number += TEXT("%");
            }
            Number += DisplaySuffix.ToString();
        }
        return Number;
    }

    bool Serialize(FArchive &Ar) {
        if (!Ar.IsSaveGame()) {
            return false;
        }
        Ar << Min;
        Ar << Max;
        uint8 ModifierByte = Ar.IsSaving() ? static_cast<uint8>(Modifier.GetValue()) : 0;
        Ar << ModifierByte;
        if (ModifierByte > static_cast<uint8>(EGameplayModOp::Override)
            && ModifierByte != static_cast<uint8>(EGameplayModOp::MultiplyCompound)) {
            Ar.SetError();
            return true;
        }
        if (Ar.IsLoading()) {
            Modifier = static_cast<EGameplayModOp::Type>(ModifierByte);
        }
        Ar << LevelScaling;
        Ar << bLowerIsBetter;
        Ar << bIsPercentage;
        Ar << bWholeNumber;
        if (Ar.IsLoading()
            && (!FMath::IsFinite(Min) || !FMath::IsFinite(Max)
                || !FMath::IsFinite(LevelScaling) || Min > Max)) {
            Ar.SetError();
        }
        return true;
    }

    bool operator==(const FRollDefinition &Other) const {
        return Min == Other.Min
            && Max == Other.Max
            && Modifier == Other.Modifier
            && bLowerIsBetter == Other.bLowerIsBetter
            && bIsPercentage == Other.bIsPercentage
            && bWholeNumber == Other.bWholeNumber;
    }

    static float ScaleValue(float Value, float Level, float LevelScaling) {
        return Value + (Level * LevelScaling);
    }

    float GetScaledMin(int ItemLvl) const {
        return ScaleValue(Min, ItemLvl, LevelScaling);
    }

    float GetScaledMax(int ItemLvl) const {
        return ScaleValue(Max, ItemLvl, LevelScaling);
    }

    bool IsValid(FText &OutErrorMessage) const {
        if (Min == 0 || Max == 0) {
            OutErrorMessage = FText::FromString("Invalid affix range. Min and Max must be greater than 0.");
            return false;
        }
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRollDefinition> : TStructOpsTypeTraitsBase2<FRollDefinition> {
    enum {
        WithSerializer = true
    };
};

USTRUCT(BlueprintType)
struct MYTHIC_API FRolledAttributeSpec {
    GENERATED_BODY()

    // The roll definition that was used to roll this attribute
    UPROPERTY(BlueprintReadOnly)
    FRollDefinition Definition;

    // NOTE: FGameplayAttribute uses internal pointers that don't serialize properly.
    // The Serialize method handles saving/loading via AttributeSetClassName + AttributePropertyName.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayAttribute Attribute;

    FString AttributeSetClassName;

    FString AttributePropertyName;

    /** Numeric value rolled for this current serialized attribute specification. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Value = 0;

    /** True while the owning gameplay system has this rolled attribute applied. */
    UPROPERTY(BlueprintReadOnly)
    bool bIsApplied = false;

    FRolledAttributeSpec() {}

    FRolledAttributeSpec(FGameplayAttribute InAttribute, int ItemLvl, FRollDefinition &RollDef) {
        Attribute = InAttribute;
        bIsApplied = false;

        if (Attribute.IsValid()) {
            if (UStruct *AttrSet = Attribute.GetAttributeSetClass()) {
                AttributeSetClassName = AttrSet->GetPathName();
            }
            AttributePropertyName = Attribute.GetName();
        }

        auto Min = RollDef.GetScaledMin(ItemLvl);
        auto Max = RollDef.GetScaledMax(ItemLvl);

        Value = FMath::RandRange(Min, Max);
        if (RollDef.bWholeNumber) {
            Value = FMath::RoundToFloat(Value);
        }

        Definition = RollDef;
    }

    bool Serialize(FArchive &Ar);

    bool operator==(const FRolledAttributeSpec &Other) const {
        return Attribute == Other.Attribute && Value == Other.Value;
    }
};

template <>
struct TStructOpsTypeTraits<FRolledAttributeSpec> : TStructOpsTypeTraitsBase2<FRolledAttributeSpec> {
    enum {
        WithSerializer = true
    };
};

USTRUCT(BlueprintType)
struct FRolledTagSpec {
    GENERATED_BODY()

    FRolledTagSpec() : Value(0) {}

    // The gameplay tag
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Itemization", SaveGame)
    FGameplayTag Tag;

    // The rolled value
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Itemization", SaveGame)
    float Value;

    FRolledTagSpec(FGameplayTag Key, float Value) {
        this->Tag = Key;
        this->Value = Value;
    }

    bool operator==(const FRolledTagSpec &Other) const {
        return Tag == Other.Tag && Value == Other.Value;
    }
};

USTRUCT(Blueprintable, BlueprintType)
struct FAbilityDefinition {
    GENERATED_BODY()

    // The ability to roll
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, SaveGame)
    TSubclassOf<class UGameplayAbility> Ability;

    // Map of the attributes to roll.
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, SaveGame)
    TMap<FGameplayTag, FRollDefinition> ParameterRolls;

    // Rich text representation of the ability and its rolled stats. To calculate the rolled value, the following <> tags can be used:
    //
    // Example: "<#GAS.Damage> Damage dealt is returned as health" -> "<RichText>10%</RichText><Optional>[Min-Max]</Optional> Damage is returned as health"
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, SaveGame)
    FText RichText = FText::FromString("???");

    // False when the text is empty, mentions no placeholder for a rolled tag, or rolls a tag it has no range for.
    bool GetRichText(FText &OutRichText, const TArray<FRolledTagSpec> &RolledAttributes) const {
        if (RichText.IsEmpty()) {
            return false;
        }

        FString Source = RichText.ToString();

        for (const FRolledTagSpec &AttributeRoll : RolledAttributes) {
            const FString ToReplace = FString::Printf(TEXT("<#%s>"), *AttributeRoll.Tag.ToString());
            const FRollDefinition *RollDef = ParameterRolls.Find(AttributeRoll.Tag);
            if (!RollDef || !Source.Contains(ToReplace)) {
                return false;
            }

            const FString Replacement = FString::Printf(TEXT("<Roll>%s</><Context>[%s-%s]</>"),
                                                        *RollDef->FormatForDisplay(AttributeRoll.Value, true),
                                                        *RollDef->FormatForDisplay(RollDef->Min, false),
                                                        *RollDef->FormatForDisplay(RollDef->Max, false));
            Source = Source.Replace(*ToReplace, *Replacement);
        }

        OutRichText = FText::FromString(Source);
        return true;
    }

    bool operator==(const FAbilityDefinition &Other) const {
        if (ParameterRolls.Num() != Other.ParameterRolls.Num()) {
            return false;
        }

        for (auto &Roll : ParameterRolls) {
            if (!Other.ParameterRolls.Contains(Roll.Key) || Roll.Value != Other.ParameterRolls[Roll.Key]) {
                return false;
            }
        }

        return Ability == Other.Ability && RichText.EqualTo(Other.RichText);
    }

    bool IsValid(FText &OutErrorMessage) const {
        if (!Ability) {
            OutErrorMessage = FText::FromString("Ability is not set");
            return false;
        }

        for (auto &Roll : ParameterRolls) {
            if (!Roll.Key.IsValid()) {
                OutErrorMessage = FText::FromString("Invalid gameplay tag in roll definition");
                return false;
            }

            if (!Roll.Value.IsValid(OutErrorMessage)) {
                return false;
            }
        }

        if (RichText.IsEmptyOrWhitespace()) {
            OutErrorMessage = FText::FromString("Rich text is empty");
            return false;
        }

        return true;
    }
};

USTRUCT(BlueprintType)
struct FAbilityRollSpec {
    GENERATED_BODY()

    FAbilityRollSpec() {}

    // Granted ability spec
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FGameplayAbilitySpec AbilitySpec;

    // Attributes used in the ability
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, SaveGame)
    TArray<FRolledTagSpec> RolledAttributes;

    // The Rich text for this spec
    UPROPERTY(BlueprintReadOnly, SaveGame)
    FText RichText;

    FAbilityRollSpec(FAbilityDefinition &AbilityRoll) {
        this->AbilitySpec = FGameplayAbilitySpec();

        for (auto &AttributeRoll : AbilityRoll.ParameterRolls) {
            this->RolledAttributes.Add(FRolledTagSpec(AttributeRoll.Key, FMath::RandRange(AttributeRoll.Value.Min, AttributeRoll.Value.Max)));
        }

        if (!AbilityRoll.GetRichText(RichText, RolledAttributes)) {
            RichText = FText::FromString("???");
        }
    }

    bool operator==(const FAbilityRollSpec &Other) const {
        if (RolledAttributes.Num() != Other.RolledAttributes.Num()) {
            return false;
        }

        for (int i = 0; i < RolledAttributes.Num(); i++) {
            if (RolledAttributes[i] != Other.RolledAttributes[i]) {
                return false;
            }
        }

        return true;
    }
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, meta=(ShowOnlyInnerProperties))
class MYTHIC_API UTalentDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Icon for the talent - Provide Icon and Name for the talent
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    // Name of the talent - Provide Icon and Name for the talent
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FText Name;

    // Ability to grant. The ability MUST be a passive ability (activated at grant time)
    // The source object of the ability will be the owning TalentFragment.
    //
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FAbilityDefinition AbilityDef;

    bool HasAnyPayload() const {
        return AbilityDef.Ability != nullptr;
    }

    // Minimum item rarity this talent is eligible to roll on. UTalentFragment::RollTalents only considers defs whose
    // MinRarity <= the item's rarity, so a designer can reserve strong talents for higher tiers. Default Common (lowest
    // rarity) = eligible on every item, so this is additive / back-compat: existing pools behave exactly as before.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    TEnumAsByte<EItemRarity> MinRarity = Common;

    /** Which item TYPES this talent may roll on, matched over the item's GetTypeProbe tags — the same mechanism
     *  UMythicAspectDefinition::AllowedItemTypes uses. An EMPTY query
     *  is UNIVERSAL (rollable on anything), so every existing talent is unaffected. Use it to stop a family-specific
     *  talent landing on the wrong weapon; leave it empty for anything that should stay cross-build, since a wide
     *  talent space is the point. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FGameplayTagQuery AllowedItemTypes;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override {
        EDataValidationResult Result = Super::IsDataValid(Context);
        if (!HasAnyPayload()) {
            Context.AddError(FText::FromString("Talent does nothing: set AbilityDef.Ability."));
            Result = EDataValidationResult::Invalid;
        }

        if (Icon.IsNull()) {
            Context.AddError(FText::FromString("Icon is not set"));
            Result = EDataValidationResult::Invalid;
        }

        if (Name.IsEmpty()) {
            Context.AddError(FText::FromString("Name is not set"));
            Result = EDataValidationResult::Invalid;
        }
        return Result;
    };
#endif
};
