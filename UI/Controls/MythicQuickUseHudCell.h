#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/MythicQuickUseComponent.h"
#include "MythicQuickUseHudCell.generated.h"

class UImage;
class UCommonTextBlock;
class UMythicControlGlyph;
struct FStreamableHandle;

/** Passive view of the owner's current eligible inventory projection, never a use/admission test. */
UCLASS(meta=(DisableNativeTick))
class MYTHIC_API UMythicQuickUseHudCell : public UUserWidget {
    GENERATED_BODY()
public:
    UMythicQuickUseHudCell(const FObjectInitializer& Initializer);
    void InitializePurpose(bool bInHealing);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    void Build();
    void Refresh();
    void ClearLoads();
    void ResolveSelectedDefinition(uint32 Revision,FMythicQuickUseKey Key);
    void RefreshIcon(const FSoftObjectPath& Path);
    UPROPERTY() TObjectPtr<UImage> Icon;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Count;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Empty;
    UPROPERTY() TObjectPtr<UMythicControlGlyph> Glyph;
    TWeakObjectPtr<UMythicQuickUseComponent> QuickUse;
    FMythicQuickUseKey ShownKey;
    FSoftObjectPath IconPath;
    TSharedPtr<FStreamableHandle> DefinitionLoad,IconLoad;
    FDelegateHandle EntriesChanged;
    uint32 SelectionRevision=0,IconRevision=0;
    bool bHealing=true,bConstructed=false;
};
