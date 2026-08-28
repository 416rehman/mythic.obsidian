#include "FragmentTypes.h"
#include "AttributeSet.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Survival.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Mythic/Mythic.h"

namespace {
bool DecodeCanonicalUtf8(const uint8 *Bytes, const int32 ByteCount, FString &OutValue) {
    if (ByteCount == 0) {
        OutValue.Reset();
        return true;
    }
    if (!Bytes || ByteCount < 0) {
        return false;
    }
    for (int32 Index = 0; Index < ByteCount; ++Index) {
        if (Bytes[Index] == 0) {
            return false;
        }
    }

    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR *>(Bytes), ByteCount);
    OutValue = FString(Converted.Length(), Converted.Get());
    const FTCHARToUTF8 RoundTrip(*OutValue);
    return RoundTrip.Length() == ByteCount
        && FMemory::Memcmp(RoundTrip.Get(), Bytes, ByteCount) == 0;
}
}

bool MythicFragmentSerialization::SerializeBoundedUtf8(
    FArchive &Ar, FString &Value, const int32 MaxBytes, const bool bRequired) {
    if (MaxBytes <= 0) {
        Ar.SetError();
        return false;
    }

    int32 ByteCount = 0;
    if (Ar.IsSaving()) {
        const FTCHARToUTF8 Utf8(*Value);
        ByteCount = Utf8.Length();
        if ((bRequired && ByteCount == 0) || ByteCount < 0 || ByteCount > MaxBytes) {
            Ar.SetError();
            return false;
        }
        Ar << ByteCount;
        if (ByteCount > 0 && !Ar.IsError()) {
            Ar.Serialize(const_cast<ANSICHAR *>(Utf8.Get()), ByteCount);
        }
        return !Ar.IsError();
    }

    Ar << ByteCount;
    const int64 Remaining = Ar.TotalSize() >= 0 ? Ar.TotalSize() - Ar.Tell() : ByteCount;
    if (Ar.IsError() || (bRequired && ByteCount == 0) || ByteCount < 0
        || ByteCount > MaxBytes || Remaining < ByteCount) {
        Ar.SetError();
        return false;
    }
    TArray<uint8> Bytes;
    Bytes.SetNumUninitialized(ByteCount);
    if (ByteCount > 0) {
        Ar.Serialize(Bytes.GetData(), ByteCount);
    }
    if (Ar.IsError() || !DecodeCanonicalUtf8(Bytes.GetData(), ByteCount, Value)) {
        Ar.SetError();
        return false;
    }
    return !bRequired || !Value.IsEmpty();
}

bool MythicFragmentSerialization::ResolveAllowedAttribute(
    const FString &AttributeSetClassPath, const FString &AttributePropertyName,
    FGameplayAttribute &OutAttribute) {
    OutAttribute = FGameplayAttribute();
    if (AttributeSetClassPath.IsEmpty() || AttributePropertyName.IsEmpty()) {
        return false;
    }

    const UClass *AllowedClasses[] = {
        UMythicAttributeSet_Defense::StaticClass(),
        UMythicAttributeSet_Life::StaticClass(),
        UMythicAttributeSet_Offense::StaticClass(),
        UMythicAttributeSet_Proficiencies::StaticClass(),
        UMythicAttributeSet_Survival::StaticClass(),
        UMythicAttributeSet_Utility::StaticClass(),
    };
    UClass *ResolvedClass = nullptr;
    for (const UClass *AllowedClass : AllowedClasses) {
        if (AllowedClass && AllowedClass->GetPathName() == AttributeSetClassPath) {
            ResolvedClass = const_cast<UClass *>(AllowedClass);
            break;
        }
    }
    if (!ResolvedClass) {
        return false;
    }

    FProperty *Property = ResolvedClass->FindPropertyByName(FName(*AttributePropertyName));
    if (!Property || !FGameplayAttribute::IsSupportedProperty(Property)
        || Property->GetOwnerStruct() != ResolvedClass) {
        return false;
    }
    const FGameplayAttribute Candidate(Property);
    if (!Candidate.IsValid()
        || Candidate.GetAttributeSetClass() != ResolvedClass
        || Candidate.GetName() != AttributePropertyName) {
        return false;
    }
    OutAttribute = Candidate;
    return true;
}

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
    if (Ar.IsError()
        || !MythicFragmentSerialization::SerializeBoundedUtf8(
            Ar, AttributeSetClassName,
            MythicFragmentSerialization::MaxIdentityStringBytes, true)
        || !MythicFragmentSerialization::SerializeBoundedUtf8(
            Ar, AttributePropertyName,
            MythicFragmentSerialization::MaxIdentityStringBytes, true)) {
        return true;
    }
    Ar << Value;

    if (Ar.IsLoading()) {
        bIsApplied = false;

        if (Ar.IsError() || !FMath::IsFinite(Value)
            || !MythicFragmentSerialization::ResolveAllowedAttribute(
                AttributeSetClassName, AttributePropertyName, Attribute)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("FRolledAttributeSpec::Serialize - rejected unsupported attribute %s.%s"),
                   *AttributeSetClassName, *AttributePropertyName);
            Ar.SetError();
            return true;
        }
        UE_LOG(MythSaveLoad, Log, TEXT("FRolledAttributeSpec::Serialize (Load) - Restored %s.%s = %.2f"),
               *AttributeSetClassName, *AttributePropertyName, Value);
    }

    return true;
}
