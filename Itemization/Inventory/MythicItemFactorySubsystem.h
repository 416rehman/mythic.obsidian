#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicItemFactoryTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicItemFactorySubsystem.generated.h"

class UAffixesFragment;
class UMythicItemizationDataRegistrySubsystem;
struct FCompiledAffixProfile;

/**
 * The sole authoritative construction boundary for new item instances.
 * Async creation loads the exact profile closure; ready creation is allocation-only and never sync-loads.
 */
UCLASS()
class MYTHIC_API UMythicItemFactorySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    void CreateItemAsync(const FMythicCreateItemRequest &Request, FOnMythicItemCreated Completion);
    FMythicCreateItemResult CreateItemReady(const FMythicCreateItemRequest &Request);

    /** Prewarms the active-ruleset profile and exact grant closure required by an item definition. */
    void RequestItemDefinitionReadyAsync(UItemDefinition *ItemDefinition,
                                         FOnMythicItemDefinitionReady Completion);

    /** Pure readiness seam used by validation and automation; it never loads. */
    static EMythicCreateItemStatus EvaluateReadyState(
        const FMythicCreateItemRequest &Request,
        const UMythicItemizationDataRegistrySubsystem *Registry,
        TSharedPtr<const FCompiledAffixProfile> &OutCompiledProfile,
        FName &OutDiagnosticCode);

    /** Definition/profile-only readiness seam; intentionally ignores authority/request transport validation. */
    static EMythicCreateItemStatus EvaluateDefinitionReadyState(
        const UItemDefinition *ItemDefinition,
        const UMythicItemizationDataRegistrySubsystem *Registry,
        TSharedPtr<const FCompiledAffixProfile> &OutCompiledProfile,
        FName &OutDiagnosticCode);

private:
    enum class EProfileRequirement : uint8 { None, Valid, Invalid };

    static EProfileRequirement ResolveProfileRequirement(
        const UItemDefinition *ItemDefinition,
        FPrimaryAssetId &OutProfileId,
        FName &OutDiagnosticCode);
};
