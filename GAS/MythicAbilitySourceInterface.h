// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "MythicAbilitySourceInterface.generated.h"

class UObject;
class UPhysicalMaterial;
struct FGameplayTagContainer;

UINTERFACE()
class UMythicAbilitySourceInterface : public UInterface {
    GENERATED_UINTERFACE_BODY()
};

class IMythicAbilitySourceInterface {
    GENERATED_IINTERFACE_BODY()
    virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer *SourceTags = nullptr,
                                         const FGameplayTagContainer *TargetTags = nullptr) const = 0;

    virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial *PhysicalMaterial, const FGameplayTagContainer *SourceTags = nullptr,
                                                 const FGameplayTagContainer *TargetTags = nullptr) const = 0;
};
