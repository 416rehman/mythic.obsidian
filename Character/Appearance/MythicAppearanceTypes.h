#pragma once

#include "CoreMinimal.h"
#include "MythicAppearanceTypes.generated.h"

namespace MythicAppearanceLimits {
inline constexpr int32 Parts = 16;
inline constexpr int32 Channels = 8;
inline constexpr int32 ItemEdits = 8;
inline constexpr int32 Bones = 2048;
inline constexpr int32 MaterialSlots = 8;
inline constexpr int32 Materials = 64;
inline constexpr int32 IdLength = 64;
}

/** Stable authored identities, never a material index or a localized label. None means Original. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceColor {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Channel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Palette;
    bool operator==(const FMythicAppearanceColor& Other) const { return Channel==Other.Channel && Palette==Other.Palette; }
};

/** Lives on the physical item and travels through the canonical item serializer. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicItemAppearanceData {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame) TArray<FMythicAppearanceColor> Colors;
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 Revision = 0;
    bool HasSameColors(const FMythicItemAppearanceData& Other) const;
    static bool NormalizeColors(TArray<FMythicAppearanceColor>& Values);
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceRecipe {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Frame;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Head;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Hair;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SkinPalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HairPalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EyePalette;
    bool operator==(const FMythicAppearanceRecipe& Other) const;
    bool operator!=(const FMythicAppearanceRecipe& Other) const { return !(*this==Other); }
};

/** Complete render description; contains no physical item GUID or private inventory information. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearancePartState {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Part;
    UPROPERTY(BlueprintReadOnly) TArray<FMythicAppearanceColor> Colors;
    bool operator==(const FMythicAppearancePartState& Other) const { return Part==Other.Part && Colors==Other.Colors; }
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearancePublicState {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Incarnation;
    UPROPERTY(BlueprintReadOnly) FName Frame;
    UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly) TArray<FMythicAppearancePartState> Parts;
    bool SameLook(const FMythicAppearancePublicState& Other) const { return Frame==Other.Frame && Parts==Other.Parts; }
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceItemEdit {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FGuid ItemGuid;
    UPROPERTY(BlueprintReadWrite) int32 ExpectedRevision = 0;
    /** Changed channels only; None removes that override. */
    UPROPERTY(BlueprintReadWrite) TArray<FMythicAppearanceColor> Changes;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceEditRequest {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FGuid RequestId;
    UPROPERTY(BlueprintReadWrite) FGuid Session;
    UPROPERTY(BlueprintReadWrite) int32 ExpectedRevision = 0;
    UPROPERTY(BlueprintReadWrite) bool bChangePersonal = false;
    UPROPERTY(BlueprintReadWrite) FMythicAppearanceRecipe Personal;
    UPROPERTY(BlueprintReadWrite) TArray<FMythicAppearanceItemEdit> Items;
    bool IsBounded() const;
};

UENUM(BlueprintType)
enum class EMythicAppearanceResult : uint8 { Applied, Saved, SaveFailed, Superseded, NotReady, Stale, Invalid, NotOwned, Busy };

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceReceipt {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid RequestId;
    UPROPERTY(BlueprintReadOnly) FGuid Session;
    UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly) EMythicAppearanceResult Result = EMythicAppearanceResult::NotReady;
};

/** Captured beside serialized item bytes, for the exact physical save completion. */
USTRUCT()
struct MYTHIC_API FMythicAppearanceItemStamp {
    GENERATED_BODY()
    UPROPERTY() FGuid ItemGuid;
    UPROPERTY() int32 Revision = 0;
    bool operator==(const FMythicAppearanceItemStamp& Other) const { return ItemGuid==Other.ItemGuid && Revision==Other.Revision; }
};

USTRUCT()
struct MYTHIC_API FMythicAppearanceSaveStamp {
    GENERATED_BODY()
    UPROPERTY() FString Route;
    UPROPERTY() FGuid Session;
    UPROPERTY() int32 Revision = 0;
    UPROPERTY() FMythicAppearanceRecipe Recipe;
    UPROPERTY() TArray<FMythicAppearanceItemStamp> Items;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearanceOwnerState {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid Session;
    UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly) FMythicAppearanceRecipe Recipe;
};
