#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythicAppearanceCatalog.h"
#include "MythicAppearanceComponent.generated.h"

class APlayerState;
class UMythicInventoryComponent;
class UMythicItemInstance;
struct FMythicDurableCharacterSaveResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicAppearanceChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicAppearanceResultEvent,const FMythicAppearanceReceipt&,Receipt);

/** PlayerState-owned canonical cosmetics. Rendering and relevant public state live on the current avatar. */
UCLASS(ClassGroup=(Mythic), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicAppearanceComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicAppearanceComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(BlueprintAssignable) FMythicAppearanceChanged OnAppearanceChanged;
    UPROPERTY(BlueprintAssignable) FMythicAppearanceResultEvent OnAppearanceResult;
    UFUNCTION(BlueprintPure) bool IsReady() const;
    UFUNCTION(BlueprintPure) FMythicAppearanceOwnerState GetState() const { return OwnerState; }
    UFUNCTION(BlueprintPure) FMythicAppearanceReceipt GetLastReceipt() const { return LastReceipt; }
    UFUNCTION(Server,Reliable,BlueprintCallable) void ServerApplyAppearance(FMythicAppearanceEditRequest Request);
    UFUNCTION(Server,Reliable,BlueprintCallable) void ServerRetryAppearanceSave(FGuid RequestId,FGuid Session,int32 Revision);
    UFUNCTION(Client,Reliable) void ClientAppearanceResult(FMythicAppearanceReceipt Receipt);
    UFUNCTION() void MarkEquipmentDirty();
    void GetEquipment(TArray<FMythicAppearanceEquippedItem>& Out) const;
    TArray<UMythicItemInstance*> GetEditableItems() const;
    FMythicAppearanceSaveStamp CaptureSaveStamp() const;
    bool CanCaptureSave() const { return RestoreDepth==0; }
    bool HasPersonalRecipe() const { return bHasPersonalRecipe; }
    void BeginRestore();
    void FinishRestore(const FMythicAppearanceRecipe* Recipe,bool bSuccess);
    static FText DescribeResult(EMythicAppearanceResult Result);
private:
    UFUNCTION() void HandleSlotChanged(int32 Slot);
    UFUNCTION() void HandleSizeChanged(int32 NewSize,int32 OldSize);
    UFUNCTION() void HandlePawnSet(APlayerState* State,APawn* NewPawn,APawn* OldPawn);
    UFUNCTION() void OnRep_State();
    void RebuildPublic();
    void BindInventories();
    void UnbindInventories();
    void SendResult(FGuid Request,EMythicAppearanceResult Result,const FMythicAppearanceSaveStamp* Accepted=nullptr);
    void RequestSave(FGuid Request,const FMythicAppearanceSaveStamp& Wanted);
    void SaveCompleted(FGuid Request,FMythicAppearanceSaveStamp Wanted,const FMythicDurableCharacterSaveResult& Result);
    UMythicItemInstance* FindEquippedItem(FGuid Guid) const;
    TArray<UMythicInventoryComponent*> Inventories() const;
    UPROPERTY(ReplicatedUsing=OnRep_State) FMythicAppearanceOwnerState OwnerState;
    UPROPERTY(Transient) FMythicAppearanceReceipt LastReceipt;
    UPROPERTY(Transient) TObjectPtr<UMythicAppearanceCatalog> Catalog;
    TArray<TWeakObjectPtr<UMythicInventoryComponent>> BoundInventories;
    TWeakObjectPtr<APawn> BoundPawn;
    FGuid PublicIncarnation;
    FString EquipmentSignature;
    TMap<FGuid,FMythicAppearanceReceipt> Replays;
    TArray<FGuid> ReplayOrder;
    TMap<FGuid,TArray<FMythicAppearanceItemStamp>> PendingSaves;
    TMap<FGuid,FMythicAppearanceSaveStamp> FailedSaves;
    FTimerHandle RebuildTimer;
    double LastRequestTime = -1;
    int32 RestoreDepth = 0;
    bool bDirty = false;
    bool bCommittedRevision = false;
    bool bCatalogReady = false;
    bool bHasPersonalRecipe = false;
};
