#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "NativeGameplayTags.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicStatusGroupReservation.generated.h"

class UMythicAbilitySystemComponent;
class UMythicStatusEffectDefinition;
class UMythicEntityPresentationComponent;
class UAbilitySystemComponent;

MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_STATUS_GROUP_REMOVAL_PROTECTED);

/** Facts from a removed aggregate, not an entitlement to reapply damage or heal. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatusGroupRemoval {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bRemoved = false;
    UPROPERTY(BlueprintReadOnly) FGuid ReceiptId;
    UPROPERTY(BlueprintReadOnly) FGuid ActivationId;
    UPROPERTY(BlueprintReadOnly) FName ActionId;
    UPROPERTY(BlueprintReadOnly) FGuid ActionGroupId;
    UPROPERTY(BlueprintReadOnly) FGameplayTagContainer Facets;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UMythicStatusEffectDefinition> Definition;
    UPROPERTY(BlueprintReadOnly) int32 StackCount = 0;
    UPROPERTY(BlueprintReadOnly) float RemainingSeconds = 0;
    UPROPERTY(BlueprintReadOnly) float DamagePerTick = 0;
    UPROPERTY(BlueprintReadOnly) float ControlMagnitude = 0;
    // Private provenance. Do not expose a hidden applier or copy this into an offensive context.
    FGameplayEffectContextHandle RemovedContext;
};

/** Short-lived authority lease on one recipient-owned GAS aggregate. No global map, ticking or save state. */
class MYTHIC_API FMythicStatusGroupReservation : public TSharedFromThis<FMythicStatusGroupReservation> {
public:
    static TSharedPtr<FMythicStatusGroupReservation> Reserve(UMythicAbilitySystemComponent* Recipient,
        const TArray<const UMythicStatusEffectDefinition*>& Priority);
    ~FMythicStatusGroupReservation();
    FMythicStatusGroupRemoval Consume();
    bool IsCurrent() const;
    void Release();
private:
    void Invalidate() { ++Revision; }
    void OnCanonicalApply(const UMythicStatusEffectDefinition* Definition,const UAbilitySystemComponent* Source);
    void OnStackChanged(FActiveGameplayEffectHandle,int32,int32) { Invalidate(); }
    void OnTimeChanged(FActiveGameplayEffectHandle,float,float) { Invalidate(); }
    void OnRemoved(const FGameplayEffectRemovalInfo&) { Invalidate(); }
    void OnInhibited(FActiveGameplayEffectHandle,bool) { Invalidate(); }
    void OnIncarnationEnded(UMythicEntityPresentationComponent&,const FMythicEntityPresentationInstance&) { Invalidate(); }
    TWeakObjectPtr<UMythicAbilitySystemComponent> Recipient;
    TWeakObjectPtr<AActor> Avatar;
    TWeakObjectPtr<UMythicEntityPresentationComponent> Presentation;
    FMythicEntityPresentationInstance Instance;
    TWeakObjectPtr<const UMythicStatusEffectDefinition> Definition;
    TWeakObjectPtr<const UAbilitySystemComponent> Source;
    FActiveGameplayEffectHandle Handle;
    FGameplayEffectContextHandle Context;
    float StartTime = 0;
    float Duration = 0;
    float Damage = 0;
    float Control = 0;
    int32 Stacks = 0;
    uint64 Revision = 1;
    bool bReleased = false;
    bool bHadPresentation = false;
    FDelegateHandle Applying,StackChanged,TimeChanged,Removed,Inhibited,IncarnationEnded;
};
