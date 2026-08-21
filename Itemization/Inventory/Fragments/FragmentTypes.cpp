#include "FragmentTypes.h"
#include "AttributeSet.h"
#include "Mythic/Mythic.h"

bool FRolledAttributeSpec::Serialize(FArchive &Ar) {
    if (!Ar.IsSaveGame()) {
        return false;
    }

    if (Ar.IsSaving()) {
        if (Attribute.IsValid()) {
            if (UStruct *AttrSet = Attribute.GetAttributeSetClass()) {
                AttributeSetClassName = AttrSet->GetPathName();
            }
            AttributePropertyName = Attribute.GetName();
        }
        UE_LOG(MythSaveLoad, Log, TEXT("FRolledAttributeSpec::Serialize (Save) - %s.%s = %.2f"),
               *AttributeSetClassName, *AttributePropertyName, Value);
    }

    Definition.Serialize(Ar);
    Ar << AttributeSetClassName;
    Ar << AttributePropertyName;
    Ar << Value;

    if (Ar.IsLoading()) {
        bIsApplied = false;

        if (!AttributeSetClassName.IsEmpty() && !AttributePropertyName.IsEmpty()) {
            UClass *SetClass = LoadClass<UAttributeSet>(nullptr, *AttributeSetClassName);
            if (SetClass) {
                FProperty *Prop = SetClass->FindPropertyByName(FName(*AttributePropertyName));
                if (Prop) {
                    Attribute = FGameplayAttribute(Prop);
                    UE_LOG(MythSaveLoad, Log, TEXT("FRolledAttributeSpec::Serialize (Load) - Restored %s.%s = %.2f"),
                           *AttributeSetClassName, *AttributePropertyName, Value);
                }
                else {
                    UE_LOG(MythSaveLoad, Error, TEXT("FRolledAttributeSpec::Serialize - Property not found: %s in %s"),
                           *AttributePropertyName, *AttributeSetClassName);
                }
            }
            else {
                UE_LOG(MythSaveLoad, Error, TEXT("FRolledAttributeSpec::Serialize - Class not found: %s"),
                       *AttributeSetClassName);
            }
        }
    }

    return true;
}
