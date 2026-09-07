#include "MythicControlsSettings.h"

UMythicControlsSettings::UMythicControlsSettings() {
    const auto Add = [this](EMythicControl Control, const TCHAR* Label, FKey Keyboard, FKey Gamepad = EKeys::Invalid, bool bLayer = false) {
        FMythicControlBinding Binding;
        Binding.Control = Control;
        Binding.Label = FText::FromString(Label);
        Binding.Keyboard = Keyboard;
        Binding.Gamepad = Gamepad;
        Binding.bGamepadAbilityLayer = bLayer;
        Bindings.Add(Binding);
    };
    Add(EMythicControl::MoveForward,TEXT("Move forward"),EKeys::W);
    Add(EMythicControl::MoveBackward,TEXT("Move backward"),EKeys::S);
    Add(EMythicControl::MoveLeft,TEXT("Move left"),EKeys::A);
    Add(EMythicControl::MoveRight,TEXT("Move right"),EKeys::D);
    Add(EMythicControl::Primary,TEXT("Weapon primary"),EKeys::LeftMouseButton,EKeys::Gamepad_RightTrigger);
    Add(EMythicControl::Secondary,TEXT("Weapon secondary"),EKeys::RightMouseButton,EKeys::Gamepad_LeftTrigger);
    Add(EMythicControl::Jump,TEXT("Jump / mantle"),EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Bottom);
    Add(EMythicControl::Dash,TEXT("Dash"),EKeys::LeftShift,EKeys::Gamepad_FaceButton_Right);
    Add(EMythicControl::Crouch,TEXT("Crouch / sneak"),EKeys::X,EKeys::Gamepad_DPad_Left);
    Add(EMythicControl::Interact,TEXT("Interact"),EKeys::F,EKeys::Gamepad_FaceButton_Left);
    Add(EMythicControl::SkillQ,TEXT("Ability slot 1"),EKeys::Q,EKeys::Gamepad_RightTrigger,true);
    Add(EMythicControl::SkillE,TEXT("Ability slot 2"),EKeys::E,EKeys::Gamepad_RightShoulder,true);
    Add(EMythicControl::SkillR,TEXT("Ability slot 3"),EKeys::R,EKeys::Gamepad_FaceButton_Left,true);
    Add(EMythicControl::SkillT,TEXT("Ability slot 4"),EKeys::T,EKeys::Gamepad_FaceButton_Top,true);
    Add(EMythicControl::AbilityLayer,TEXT("Ability modifier"),EKeys::Invalid,EKeys::Gamepad_LeftShoulder);
    Add(EMythicControl::Heal,TEXT("Heal / select healing item"),EKeys::Z,EKeys::Gamepad_DPad_Up);
    Add(EMythicControl::LockTarget,TEXT("Target lock"),EKeys::LeftControl,EKeys::Gamepad_RightThumbstick);
    Add(EMythicControl::Ping,TEXT("Ping / communication wheel"),EKeys::G,EKeys::Gamepad_DPad_Right);
    Add(EMythicControl::Inspect,TEXT("Inspect"),EKeys::LeftAlt);
    Add(EMythicControl::CameraMode,TEXT("Camera perspective"),EKeys::C);
    Add(EMythicControl::CharacterMenu,TEXT("Character / inventory"),EKeys::Tab,EKeys::Gamepad_Special_Left);
    Add(EMythicControl::SystemMenu,TEXT("System menu"),EKeys::Escape,EKeys::Gamepad_Special_Right);
}

const FMythicControlBinding* UMythicControlsSettings::Find(EMythicControl Control) const {
    if (!IsAvailable(Control)) return nullptr;
    return Bindings.FindByPredicate([Control](const auto& Binding) { return Binding.Control == Control; });
}

FName UMythicControlsSettings::MappingName(EMythicControl Control, bool bGamepad) {
    const FString Identity = StaticEnum<EMythicControl>()->GetNameStringByValue(static_cast<int64>(Control));
    return FName(*FString::Printf(TEXT("Mythic.Controls.%s.%s"),*Identity,bGamepad ? TEXT("Gamepad") : TEXT("Keyboard")));
}
