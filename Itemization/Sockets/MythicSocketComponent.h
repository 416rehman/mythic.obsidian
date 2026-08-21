#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicSocketComponent.generated.h"

class UMythicItemInstance;
class UItemDefinition;
class USocketsFragment;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicSocketComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicSocketComponent();

    /** When true, unsocketing mints the gem back into the player's inventory (using GemReturnDefs to resolve the item
     *  definition for the removed gem-type). When false (or no mapping is found), the gem is destroyed on removal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    bool bReturnGemOnUnsocket = false;

    /** GemType -> the ItemDefinition minted when that gem is unsocketed (only used when bReturnGemOnUnsocket). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    TMap<FGameplayTag, TObjectPtr<UItemDefinition>> GemReturnDefs;


    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Sockets")
    void ServerSocketGem(UMythicItemInstance *HostItem, int32 SocketIndex, UMythicItemInstance *Gem);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Sockets")
    void ServerUnsocketGem(UMythicItemInstance *HostItem, int32 SocketIndex);

protected:
    static USocketsFragment *GetSocketsFragment(UMythicItemInstance *Item);
    class AController *GetOwningController() const;
};
