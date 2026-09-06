#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythicQuickUseComponent.generated.h"

class UMythicInventoryComponent;
class UMythicItemInstance;
class UTexture2D;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicQuickUseKey {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FSoftObjectPath Definition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Variant;
    bool operator==(const FMythicQuickUseKey& Other) const { return Definition==Other.Definition && Variant==Other.Variant; }
    bool IsValid() const { return Definition.IsValid(); }
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicQuickUseEntry {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FMythicQuickUseKey Key;
    UPROPERTY(BlueprintReadOnly) FText Name;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
    int32 SortPriority = 0;
};

DECLARE_MULTICAST_DELEGATE(FMythicQuickUseEntriesChanged);

/** Inventory events update a compact local catalogue. Selection never substitutes a depleted item. */
UCLASS(ClassGroup=(Input), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicQuickUseComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicQuickUseComponent();
    const TArray<FMythicQuickUseEntry>& GetEntries(bool bHealing) const { return bHealing ? HealingEntries : UtilityEntries; }
    FMythicQuickUseKey GetSelected(bool bHealing) const { return bHealing ? SelectedHealing : SelectedUtility; }
    bool Select(bool bHealing, const FMythicQuickUseKey& Key);
    void UseSelected(bool bHealing);
    void RestoreSelections(const FMythicQuickUseKey& Healing, const FMythicQuickUseKey& Utility);
    void RefreshInventories();
    FMythicQuickUseEntriesChanged OnEntriesChanged;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION(Server,Reliable) void ServerSelect(bool bHealing, FMythicQuickUseKey Key);
    UFUNCTION() void RebuildEntries();
    UFUNCTION() void HandleSlot(int32 Slot);
    UFUNCTION() void HandleSize(int32 NewSize,int32 OldSize);
    UFUNCTION() void HandlePawn(APawn* OldPawn,APawn* NewPawn);
    static bool Describe(UMythicItemInstance* Item, bool bHealing, FMythicQuickUseEntry& Out);
    UPROPERTY(ReplicatedUsing=RebuildEntries) FMythicQuickUseKey SelectedHealing;
    UPROPERTY(ReplicatedUsing=RebuildEntries) FMythicQuickUseKey SelectedUtility;
    UPROPERTY(Transient) TArray<FMythicQuickUseEntry> HealingEntries;
    UPROPERTY(Transient) TArray<FMythicQuickUseEntry> UtilityEntries;
    TArray<TWeakObjectPtr<UMythicInventoryComponent>> BoundInventories;
};
