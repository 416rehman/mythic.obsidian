// 

#pragma once

#include "CoreMinimal.h"
#include "NPCDefinition.h"
#include "Engine/DataAsset.h"
#include "FamilyDefinition.generated.h"

/**
 * Family Definition - This is used to pre-define a SPECIFIC family of NPCs. i,e. The Starks, because they should always stay the same.
 * The members of the family will automatically set their FamilyDefinition to this on save.
 */
UCLASS()
class MYTHIC_API UFamilyDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Family ID - This is used to track the family throughout the game's systems
    UPROPERTY(BlueprintReadOnly, Category = "Family Definition")
    FGuid FamilyId = FGuid::NewGuid();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Definition")
    TSoftObjectPtr<UNPCDefinition> Father;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Definition")
    TSoftObjectPtr<UNPCDefinition> Mother;

    // Children
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Definition")
    TArray<TSoftObjectPtr<UNPCDefinition>> Children;

    // On saving this asset, set the FamilyDefinition of all children to this.
#if WITH_EDITOR
    TSoftObjectPtr<UNPCDefinition> PreviousFather;
    TSoftObjectPtr<UNPCDefinition> PreviousMother;
    TArray<TSoftObjectPtr<UNPCDefinition>> PreviousChildren;

    static bool SetFamilyForUniqueMember(const TSoftObjectPtr<UNPCDefinition> &NPCSoft, const UFamilyDefinition *NewFam,
                                         TSet<const UNPCDefinition *> &UniqueMembers);
    static bool SetFamilyForNPCDef(const TSoftObjectPtr<UNPCDefinition> MemberAsset, const UFamilyDefinition *NewFamDefinition);
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
    virtual void PostLoad() override;
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;

    UFUNCTION(CallInEditor, Category = "Family Definition")
    void ForceSetFamilyForMembers();

    UFUNCTION(CallInEditor, Category = "Family Definition")
    void ForceUnsetFamilyForMembers();
#endif
};


// Family Instance
USTRUCT(BlueprintType)
struct FFamilySpec {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Instance")
    FGuid FamilyId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Instance")
    FGuid FatherId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Instance")
    FGuid MotherId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Family Instance")
    TArray<FGuid> ChildrenIds;
};
