#pragma once
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "Misc/DataValidation.h"
#include "FragmentTypes.generated.h"

struct FRolledTagSpec;
// This struct is used to define how a value can be rolled when an item is instanced
USTRUCT(Blueprintable, BlueprintType)
struct MYTHIC_API FRollDefinition {
    GENERATED_BODY()

    // When rolling a value, this is the minimum value that can be rolled, before any level scaling is applied
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float Min = 0;

    // When rolling a value, this is the maximum value that can be rolled, before any level scaling is applied
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float Max = 0;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Itemization")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;

    // Level scaling modifier. The item level will be multiplied by this and added to the Min and Max values.
    // For example, if Min is 10 and Max is 20, and LevelScaling is 0.5, at level 10, the Min will be 15 and the Max will be 25.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Itemization")
    float LevelScaling = 0.0f;

    // Lower is better? For example, for cooldowns, lower is better.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation")
    bool bLowerIsBetter = false;

    // Is percentage? Used in the GetRichText function to determine if the value should be multiplied by 100.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Presentation")
    bool bIsPercentage = true;

    // Custom serialization - only for save games
    bool Serialize(FArchive &Ar) {
        // For assets, use default serialization
        if (!Ar.IsSaveGame()) {
            return false;
        }
        // For save games, manually serialize all fields
        Ar << Min;
        Ar << Max;
        Ar << Modifier;
        Ar << LevelScaling;
        Ar << bLowerIsBetter;
        Ar << bIsPercentage;
        return true;
    }

    // == Operator
    bool operator==(const FRollDefinition &Other) const {
        return Min == Other.Min
            && Max == Other.Max
            && Modifier == Other.Modifier
            && bLowerIsBetter == Other.bLowerIsBetter
            && bIsPercentage == Other.bIsPercentage;
    }

    // The item-level based scaling formula.
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

// Enable custom serialization for FRollDefinition
template <>
struct TStructOpsTypeTraits<FRollDefinition> : TStructOpsTypeTraitsBase2<FRollDefinition> {
    enum {
        WithSerializer = true
    };
};

// This struct holds rolled gameplay attribute instances
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

    // For serialization - stores the attribute set class path
    FString AttributeSetClassName;

    // For serialization - stores the property name  
    FString AttributePropertyName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Value = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bIsApplied = false;

    FRolledAttributeSpec() {}

    FRolledAttributeSpec(FGameplayAttribute InAttribute, int ItemLvl, FRollDefinition &RollDef) {
        Attribute = InAttribute;
        bIsApplied = false;

        // Store attribute identity for serialization
        if (Attribute.IsValid()) {
            if (UStruct *AttrSet = Attribute.GetAttributeSetClass()) {
                AttributeSetClassName = AttrSet->GetPathName();
            }
            AttributePropertyName = Attribute.GetName();
        }

        // Apply level scaling
        auto Min = RollDef.GetScaledMin(ItemLvl);
        auto Max = RollDef.GetScaledMax(ItemLvl);

        // Roll the value
        Value = FMath::RandRange(Min, Max);

        // Set the roll definition
        Definition = RollDef;
    }

    // Custom serialization - handles FGameplayAttribute which can't serialize directly
    bool Serialize(FArchive &Ar);

    // == Operator
    bool operator==(const FRolledAttributeSpec &Other) const {
        return Attribute == Other.Attribute && Value == Other.Value;
    }
};

// Enable custom serialization for FRolledAttributeSpec
template <>
struct TStructOpsTypeTraits<FRolledAttributeSpec> : TStructOpsTypeTraitsBase2<FRolledAttributeSpec> {
    enum {
        WithSerializer = true
    };
};

// This struct holds rolled gameplay tag instances
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

    // Constructor
    FRolledTagSpec(FGameplayTag Key, float Value) {
        this->Tag = Key;
        this->Value = Value;
    }

    // == Operator
    bool operator==(const FRolledTagSpec &Other) const {
        return Tag == Other.Tag && Value == Other.Value;
    }
};

/// This struct defines how an ability is rolled.
/// The instanced/rolled counterpart is FAbilityRollSpec.
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

    bool GetRichText(FText &OutRichText, TArray<FRolledTagSpec> &RolledAttributes) {
        if (RichText.IsEmpty()) {
            return false;
        }

        FText SourceString = FText::FromString(RichText.ToString());

        // Find every <#...> tag in the RichText and replace it with the rolled value
        // Example: <#GAS.Damage> with min 10 and max 20, and rolled value 15 -> will become <RichText>15</RichText><Optional>[10-20]</Optional>
        for (auto &AttributeRoll : RolledAttributes) {
            FText ToReplace = FText::FromString(FString::Printf(TEXT("<#%s>"), *AttributeRoll.Tag.ToString()));

            // Check if exists
            if (SourceString.ToString().Contains(ToReplace.ToString())) {
                auto RollDef = this->ParameterRolls[AttributeRoll.Tag];
                float AttributeValue = AttributeRoll.Value;
                float MinValue = RollDef.Min;
                float MaxValue = RollDef.Max;

                if (RollDef.bIsPercentage) {
                    AttributeValue *= 100.0f;
                    MinValue *= 100.0f;
                    MaxValue *= 100.0f;
                }

                // Replace the tag with <RichText>Value</RichText><Optional>[Min-Max]</Optional>
                FText Replacement = FText::FromString(FString::Printf(
                    TEXT("<Roll>%d%hs</><RollRange>[%d-%d]</>"), static_cast<int>(AttributeValue), RollDef.bIsPercentage ? "%" : "", static_cast<int>(MinValue),
                    static_cast<int>(MaxValue)));
                SourceString = FText::FromString(SourceString.ToString().Replace(*ToReplace.ToString(), *Replacement.ToString()));
            }
            else {
                return false;
            }
        }

        OutRichText = SourceString;
        return true;
    }

    // == Operator
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

// AbilityRoll Spec - The instance of the AbilityRollDefinition that is rolled
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

    // Constructor to roll the attributes
    FAbilityRollSpec(FAbilityDefinition &AbilityRoll) {
        // Empty ability spec
        this->AbilitySpec = FGameplayAbilitySpec();

        // Roll the attributes
        for (auto &AttributeRoll : AbilityRoll.ParameterRolls) {
            this->RolledAttributes.Add(FRolledTagSpec(AttributeRoll.Key, FMath::RandRange(AttributeRoll.Value.Min, AttributeRoll.Value.Max)));
        }

        // Get the rich text
        if (!AbilityRoll.GetRichText(RichText, RolledAttributes)) {
            RichText = FText::FromString("???");
        }
    }

    // == Operator
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

/// Talent definition - an AbilityDefinition with identifying information
/// Instanced/rolled counterpart is FAbilityRollSpec
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
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
    FAbilityDefinition AbilityDef;

#if WITH_EDITOR
    // Validate the talent definition
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override {
        EDataValidationResult Result = Super::IsDataValid(Context);
        if (AbilityDef.Ability.Get() == nullptr) {
            Context.AddError(FText::FromString("Ability is not set"));
            Result = EDataValidationResult::Invalid;
        }

        // Icon must exist
        if (Icon.IsNull()) {
            Context.AddError(FText::FromString("Icon is not set"));
            Result = EDataValidationResult::Invalid;
        }

        // Name must be set
        if (Name.IsEmpty()) {
            Context.AddError(FText::FromString("Name is not set"));
            Result = EDataValidationResult::Invalid;
        }
        return Result;
    };
#endif
};
