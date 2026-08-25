#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Net/UnrealNetwork.h"
#include "SocketsFragment.generated.h"

class UAbilitySystemComponent;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API USocketsFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Sockets)

    /** The item's sockets (count rolled in OnInstanced). Replicated to the owning client + persisted. */
    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame, Category = "Sockets")
    TArray<FMythicSocketSlot> Sockets;

    /** OPTIONAL per-item override of the socket-count table. When left default (empty Rules) the code-default table is
     *  used. Authoring a table here lets a specific item roll a different socket distribution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FMythicSocketCountTable CountTableOverride;

    /** OPTIONAL color assigned to every rolled socket (empty = universal). Lets an item roll e.g. ruby-only sockets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets", meta = (Categories = "Itemization.Gem"))
    FGameplayTag RolledSocketColor;

    UFUNCTION(BlueprintPure, Category = "Sockets")
    int32 GetSocketCount() const { return Sockets.Num(); }

    UFUNCTION(BlueprintPure, Category = "Sockets")
    int32 GetFilledSocketCount() const;

    void ServerSocketGem(int32 SocketIndex, const FGameplayTag &GemType, const TArray<FRolledAffix> &GemAffixes);

    FGameplayTag ServerUnsocketGem(int32 SocketIndex);

    bool ServerAddSocket();

    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, Sockets, COND_InitialOrOwner);
    }

protected:
    UPROPERTY(Transient)
    TObjectPtr<UAbilitySystemComponent> ActiveASC = nullptr;

    void ApplyToActiveASC();
    void RemoveFromActiveASC();

};
