#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"
#include "MythicAbilityIntentTargetData.generated.h"

/** Only bounded directions cross the ability wire. Origins, paths, targets and outcomes belong to authority. */
USTRUCT()
struct MYTHIC_API FMythicAbilityIntentTargetData : public FGameplayAbilityTargetData {
    GENERATED_BODY()
    UPROPERTY() FVector_NetQuantizeNormal AimDirection = FVector::ForwardVector;
    /** Zero means neutral movement. Otherwise this is a horizontal unit direction, independent of camera aim. */
    UPROPERTY() FVector_NetQuantizeNormal MovementDirection = FVector::ZeroVector;
    virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
    bool HasValidDirections() const {
        const FVector Aim(AimDirection),Move(MovementDirection);
        return !Aim.ContainsNaN() && !Move.ContainsNaN() && FMath::IsNearlyEqual(Aim.SizeSquared(),1.0,0.02)
            && FMath::Abs(Move.Z)<0.001 && (Move.IsNearlyZero() || FMath::IsNearlyEqual(Move.SizeSquared(),1.0,0.02));
    }
    bool NetSerialize(FArchive& Ar,UPackageMap* Map,bool& bOutSuccess) {
        bool AimSuccess=false,MoveSuccess=false;
        AimDirection.NetSerialize(Ar,Map,AimSuccess); MovementDirection.NetSerialize(Ar,Map,MoveSuccess);
        bOutSuccess=AimSuccess && MoveSuccess && !Ar.IsError() && HasValidDirections(); return true;
    }
};

template<> struct TStructOpsTypeTraits<FMythicAbilityIntentTargetData> : TStructOpsTypeTraitsBase2<FMythicAbilityIntentTargetData> {
    enum { WithNetSerializer=true, WithCopy=true };
};
