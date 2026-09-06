#pragma once
#include "CoreMinimal.h"
#include "UObject/CoreNet.h"
#include "MythicConsumableWireTestTypes.generated.h"

/** Deterministic class-reference transport fixture for the production legacy NetSerialize function. */
UCLASS(NotBlueprintable, Hidden)
class UMythicConsumableWireTestPackageMap : public UPackageMap {
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UClass> KnownClass;
    bool bResolveClass = true;
    virtual bool SerializeObject(FArchive& Ar, UClass* RequiredClass, UObject*& Object, FNetworkGUID* OutGuid = nullptr) override {
        uint8 Id = Object ? 1 : 0;
        if (Ar.IsSaving() && Object && Object != KnownClass) { Ar.SetError(); return true; }
        Ar.SerializeBits(&Id, 2);
        if (Ar.IsError() || Id > 1) { Ar.SetError(); return true; }
        if (Ar.IsLoading()) Object = Id && bResolveClass ? KnownClass.Get() : nullptr;
        if (Object && !Object->IsA(RequiredClass)) { Ar.SetError(); return true; }
        return !Id || bResolveClass;
    }
};
