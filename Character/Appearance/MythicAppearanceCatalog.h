#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "MythicAppearanceTypes.h"
#include "MythicAppearanceCatalog.generated.h"

class UAnimInstance;
class UAnimSequence;
class UItemDefinition;
class UMaterialInterface;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct FMythicAppearanceScalar {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Parameter;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value = 0;
};
USTRUCT(BlueprintType)
struct FMythicAppearanceVector {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Parameter;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Value = FLinearColor::White;
};
USTRUCT(BlueprintType)
struct FMythicAppearancePalette {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Domain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Swatch = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMythicAppearanceScalar> Scalars;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMythicAppearanceVector> Vectors;
};
USTRUCT(BlueprintType)
struct FMythicAppearanceMaterial {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Slot;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Channel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PaletteDomain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UMaterialInterface> Material;
};
USTRUCT(BlueprintType)
struct FMythicAppearancePart {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Frame;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Region;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMythicAppearanceMaterial> Materials;
    /** Socket attachments use the canonical gameplay driver; body/gear follows the fitted pose. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Socket;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHideOwnerHead = false;
};
USTRUCT(BlueprintType)
struct FMythicAppearanceFrame {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HeadId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> BaseParts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HairPart;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<UAnimInstance> BodyAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<UAnimInstance> GameplayAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UAnimSequence> PreviewIdle;
};
USTRUCT(BlueprintType)
struct FMythicAppearanceGearFit {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Frame;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> Parts;
};
USTRUCT(BlueprintType)
struct FMythicAppearanceGear {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UItemDefinition> Item;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInventorySlotType Slot = EInventorySlotType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCoversHair = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ReplacesRegions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMythicAppearanceGearFit> Fits;
};

/** Private input; never replicated as the public outfit. */
struct FMythicAppearanceEquippedItem {
    const UItemDefinition* Item = nullptr;
    EInventorySlotType Slot = EInventorySlotType::None;
    FMythicItemAppearanceData Appearance;
    FGuid ItemGuid;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicAppearanceCatalog : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMythicAppearanceRecipe DefaultRecipe;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<UAnimInstance> CopyPoseAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAppearanceFrame> Frames;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAppearancePart> Parts;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAppearanceGear> Gear;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAppearancePalette> Palettes;

    const FMythicAppearanceFrame* FindFrame(FName Id) const;
    const FMythicAppearancePart* FindPart(FName Id) const;
    const FMythicAppearanceGear* FindGear(const UItemDefinition* Item) const;
    const FMythicAppearancePalette* FindPalette(FName Id, FName Domain=NAME_None) const;
    bool ValidatePersonalChange(const FMythicAppearanceRecipe& Before, const FMythicAppearanceRecipe& After) const;
    bool ValidateItemChanges(const FMythicAppearanceGear& Adapter, const TArray<FMythicAppearanceColor>& Changes) const;
    bool ResolveOutfit(const FMythicAppearanceRecipe& Recipe, const TArray<FMythicAppearanceEquippedItem>& Equipment,
        FMythicAppearancePublicState& Out, TArray<FText>* Diagnostics=nullptr, bool bInspectHair=false) const;
    bool ValidateCatalog(TArray<FText>& Errors) const;
    static UMythicAppearanceCatalog* Get();
private:
    void Index() const;
    mutable TMap<FName,int32> PartIndex, FrameIndex, PaletteIndex;
    mutable TMap<FSoftObjectPath,int32> GearIndex;
    mutable int32 IndexedCount = INDEX_NONE;
public:
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Player Appearance"))
class MYTHIC_API UMythicAppearanceSettings : public UDeveloperSettings {
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Appearance") TSoftObjectPtr<UMythicAppearanceCatalog> Catalog;
};
