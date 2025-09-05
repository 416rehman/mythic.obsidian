// 


#include "FamilyDefinition.h"

#include "Mythic.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
bool UFamilyDefinition::SetFamilyForNPCDef(const TSoftObjectPtr<UNPCDefinition> MemberAsset, const UFamilyDefinition *NewFam) {
    if (!MemberAsset.IsValid()) {
        return false;
    }

    UNPCDefinition *LoadedMember = MemberAsset.LoadSynchronous();
    if (!LoadedMember) {
        return false;
    }

    // Update reference
    LoadedMember->FamilyDef = NewFam;

    // Save the modified asset
    FString PathName = LoadedMember->GetPathName();
    UPackage *Package = LoadedMember->GetOutermost();
    if (Package) {
        Package->SetDirtyFlag(true);
        FAssetRegistryModule::AssetCreated(LoadedMember);

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, LoadedMember,
                              *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), SaveArgs);
    }
    return true;
}

EDataValidationResult UFamilyDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = EDataValidationResult::Valid;
    TSet<const UNPCDefinition *> UniqueMembers;

    bool bIsAlreadyInSet = false;

    // If father is valid, add it to the set
    if (Father.IsValid()) {
        UniqueMembers.Add(Father.Get());
    }
    // If mother is valid, add it to the set
    if (Mother.IsValid()) {
        UniqueMembers.Add(Mother.Get(), &bIsAlreadyInSet);
        if (bIsAlreadyInSet) {
            Result = EDataValidationResult::Invalid;
            Context.AddError(FText::Format(
                NSLOCTEXT("FamilyDefinition", "MotherAlreadyInSet", "Mother {0} is already in the set."), FText::FromString(Mother.ToString())));
        }
    }

    // Add all children to the set
    for (const TSoftObjectPtr<UNPCDefinition> &Child : Children) {
        UniqueMembers.Add(Child.Get(), &bIsAlreadyInSet);
        if (bIsAlreadyInSet) {
            Result = EDataValidationResult::Invalid;
            Context.AddError(
                FText::Format(NSLOCTEXT("FamilyDefinition", "ChildAlreadyInSet", "Child {0} is already in the set."), FText::FromString(Child.ToString())));
        }
    }

    return Result;
}

void UFamilyDefinition::PostLoad() {
    Super::PostLoad();

    PreviousFather = Father;
    PreviousMother = Mother;
    for (const TSoftObjectPtr<UNPCDefinition> &Child : Children) {
        PreviousChildren.Add(Child);
    }
}

void UFamilyDefinition::PreSave(FObjectPreSaveContext SaveContext) {
    Super::PreSave(SaveContext);

    if (PreviousFather != Father) {
        SetFamilyForNPCDef(PreviousFather, nullptr);
    }
    if (PreviousMother != Mother) {
        SetFamilyForNPCDef(PreviousMother, nullptr);
    }
    for (TSoftObjectPtr<UNPCDefinition> &PrevChild : PreviousChildren) {
        auto Index = this->Children.Find(PrevChild);
        if (Index == INDEX_NONE) {
            SetFamilyForNPCDef(PrevChild, nullptr);
        }
    }

    ForceSetFamilyForMembers();
}

bool UFamilyDefinition::SetFamilyForUniqueMember(const TSoftObjectPtr<UNPCDefinition> &NPCSoft, const UFamilyDefinition *NewFam,
                                                 TSet<const UNPCDefinition *> &UniqueMembers) {
    if (!NPCSoft.IsValid()) {
        const UNPCDefinition *Loaded = NPCSoft.LoadSynchronous();
        if (!Loaded) {
            return false;
        }
    }

    const UNPCDefinition *NPC = NPCSoft.Get();
    if (!NPC) {
        return false;
    }

    bool bIsAlreadyInSet = false;
    UniqueMembers.Add(NPC, &bIsAlreadyInSet);

    if (bIsAlreadyInSet && NewFam) {
        UE_LOG(Mythic, Error, TEXT("Duplicate NPC '%s' found in FamilyDef %s."), *NPC->NPCId.ToString(), *StaticClass()->GetName());
    }
    else {
        SetFamilyForNPCDef(NPCSoft, NewFam);
    }

    return !bIsAlreadyInSet;
}

void UFamilyDefinition::ForceSetFamilyForMembers() {
    TSet<const UNPCDefinition *> UniqueMembers;
    SetFamilyForUniqueMember(Father, this, UniqueMembers);
    SetFamilyForUniqueMember(Mother, this, UniqueMembers);

    // Children
    for (const TSoftObjectPtr<UNPCDefinition> &MemberAsset : Children) {
        SetFamilyForNPCDef(MemberAsset, this);
    }
}

void UFamilyDefinition::ForceUnsetFamilyForMembers() {
    SetFamilyForNPCDef(Father, nullptr);
    SetFamilyForNPCDef(Mother, nullptr);

    // Children
    for (const TSoftObjectPtr<UNPCDefinition> &MemberAsset : Children) {
        SetFamilyForNPCDef(MemberAsset, nullptr);
    }
}
#endif
