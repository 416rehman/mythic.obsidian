#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/MythicCombatUtilityComponent.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "MythicCombatUtilityOverlay.generated.h"

class UMythicEntityAttentionSubsystem;
class UMythicEntityPresentationComponent;
class UMythicEntityPresentationRegistry;

/** Passive, exact-local-player renderer. Receives only the utility component's permitted public projection. */
UCLASS()
class MYTHIC_API UMythicCombatUtilityOverlay : public UUserWidget {
    GENERATED_BODY()
public:
    explicit UMythicCombatUtilityOverlay(const FObjectInitializer& Initializer);
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
    UFUNCTION(BlueprintPure,Category="Combat Utility") int32 GetActivePingCount() const;
    static constexpr int32 MaximumPings=32;
private:
    UFUNCTION() void RefreshProjection();
    void HandleAttentionChanged(uint64 Revision);
    void HandleRegistryChanged(const FMythicEntityPresentationInstance& Instance,UMythicEntityPresentationComponent* Component);
    void Unbind();
    double ServerTime() const;
    bool CanIdentifySubject(const FMythicEntityPresentationInstance& Instance) const;
    TWeakObjectPtr<UMythicCombatUtilityComponent> BoundUtility;
    TWeakObjectPtr<UMythicEntityAttentionSubsystem> BoundAttention;
    TWeakObjectPtr<UMythicEntityPresentationRegistry> BoundRegistry;
    FDelegateHandle AttentionHandle,RegisteredHandle,UnregisteredHandle;
    // Fixed allocation: semantic changes rewrite the pool; no child creation, actor scan or traces in paint.
    FMythicPartyPing Pings[MaximumPings];
    int32 PingCount=0;
    UPROPERTY(Transient) FSlateBrush Plate;
    UPROPERTY(Transient) FSlateFontInfo Font;
    UPROPERTY(Transient) TArray<FText> Labels;
    TArray<FString> LabelStrings;
    TArray<FVector2D> LabelSizes;
};
