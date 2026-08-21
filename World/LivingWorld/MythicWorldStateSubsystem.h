
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "MythicWorldStateSubsystem.generated.h"

UCLASS()
class MYTHIC_API UMythicWorldStateSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    void ServerSetFlag(FGameplayTag Flag);

    void ServerClearFlag(FGameplayTag Flag);

    bool HasFlag(FGameplayTag Flag) const { return WorldFlags.HasTag(Flag); }

    const FGameplayTagContainer &GetWorldFlags() const { return WorldFlags; }

private:
    FGameplayTagContainer WorldFlags;
};
