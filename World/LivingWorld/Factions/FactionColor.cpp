
#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

namespace MythicFactionColor {
    FColor DeterministicColorForId(uint8 FactionIndex) {
        const float HueFraction = FMath::Frac(static_cast<float>(FactionIndex) * 0.61803398875f);
        const uint8 HueByte = static_cast<uint8>(HueFraction * 255.0f);

        constexpr uint8 Saturation = 220;
        constexpr uint8 Value = 235;

        const FLinearColor Linear = FLinearColor::MakeFromHSV8(HueByte, Saturation, Value);
        FColor Out = Linear.ToFColor(true);
        Out.A = 255;
        return Out;
    }

    FColor GetFactionColor(const FMythicFactionData& Data, uint8 FactionIndex) {
        if (Data.bOverrideFactionColor) {
            return Data.FactionColor;
        }
        return DeterministicColorForId(FactionIndex);
    }
}
