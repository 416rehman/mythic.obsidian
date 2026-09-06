#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "AttributeSet.h"
#include "MythicAbilityHudWidget.generated.h"

class UImage;
class UHorizontalBox;
class UMythicControlsComponent;
class UBorder;
class UCommonTextBlock;
class UMaterialInstanceDynamic;
class UMythicControlGlyph;
class UMythicSkillComponent;
class UMythicSkillDefinition;
class UMythicAbilitySystemComponent;
class UGameplayAbility;
class UAbilitySystemComponent;
struct FStreamableHandle;
struct FOnAttributeChangeData;

USTRUCT()
struct FMythicAbilityHudCell {
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UImage> Icon;
    UPROPERTY() TObjectPtr<UImage> Sweep;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SweepMaterial;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Countdown;
    UPROPERTY() TObjectPtr<UCommonTextBlock> State;
    UPROPERTY() TObjectPtr<UMythicControlGlyph> Glyph;
    UPROPERTY() TObjectPtr<UMythicSkillDefinition> Definition;
    FGameplayAbilitySpecHandle SpecHandle;
    FSoftObjectPath IconPath;
    TSharedPtr<FStreamableHandle> IconLoad;
    double CooldownEnds=0;
    float CooldownDuration=0;
    int32 ShownSeconds=INDEX_NONE;
    uint32 IconRevision=0;
};

/** Owner-only ability readouts. Displays current input mappings and never consumes gameplay input. */
UCLASS(BlueprintType,Blueprintable)
class MYTHIC_API UMythicAbilityHudWidget : public UUserWidget {
    GENERATED_BODY()
public:
    UMythicAbilityHudWidget(const FObjectInitializer& Initializer);
    UFUNCTION(BlueprintCallable,Category="Mythic|HUD") bool BindToLocalPlayer();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    void BuildCells();
    void Unbind();
    void RetryBinding();
    void BindController(APlayerController* Controller);
    void HandleOwnerChanged();
    void HandleCameraAdjustmentChanged(bool bOpen);
    UFUNCTION() void QueueRefresh();
    UFUNCTION() void HandlePawnChanged(APawn* OldPawn,APawn* NewPawn);
    void Refresh();
    void AdvanceCooldowns();
    void RefreshIcon(int32 Index,const FSoftObjectPath& Path);
    void HandleAbilityChanged(UGameplayAbility* Ability);
    void HandleEffectAdded(UAbilitySystemComponent*,const FGameplayEffectSpec&,FActiveGameplayEffectHandle);
    void HandleEffectRemoved(const FActiveGameplayEffect&);
    void HandleTagChanged(FGameplayTag,int32);
    void HandleAttributeChanged(const FOnAttributeChangeData&);
    bool IsCooldownTag(const FGameplayTagContainer& Tags) const;
    UPROPERTY(Transient) TArray<FMythicAbilityHudCell> Cells;
    TWeakObjectPtr<UMythicSkillComponent> Skills;
    TWeakObjectPtr<UMythicAbilitySystemComponent> ASC;
    TWeakObjectPtr<APlayerController> BoundController;
    FDelegateHandle Activated,Ended,Added,Removed,TagsChanged,ProjectionChanged;
    FDelegateHandle OwnerChanged;
    FDelegateHandle CameraAdjustmentChanged;
    TWeakObjectPtr<UMythicControlsComponent> BoundControls;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CameraAdjustmentHint;
    TArray<TPair<FGameplayAttribute,FDelegateHandle>> AttributeListeners;
    TMap<FActiveGameplayEffectHandle,FDelegateHandle> CooldownTimeListeners;
    FTimerHandle BindRetry,RefreshTimer,CooldownTimer;
    int32 BindAttempts=0;
    bool bConstructed=false;
    bool bRefreshQueued=false;
};
