// 


#include "MythicHUDLayout.h"

#include "CommonUIExtensions.h"
#include "Input/CommonUIInputTypes.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");


UMythicHUDLayout::UMythicHUDLayout(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {}

void UMythicHUDLayout::NativeOnInitialized() {
    Super::NativeOnInitialized();

    RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE), false,
                                              FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));
}

void UMythicHUDLayout::HandleEscapeAction() {
    if (ensure(!EscapeMenuClass.IsNull()) && !PreEscapeMenuOpen()) {
        UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, EscapeMenuClass);
    }
}
