// Mythic Living World — Faction Color implementation

#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

namespace MythicFactionColor {

    FColor DeterministicColorForId(uint8 FactionIndex) {
        // Golden-ratio conjugate hue walk: hue = frac(idx * 0.61803398875) * 360 (degrees). Consecutive indices land
        // ~137.5 degrees apart on the wheel, so even adjacent factions are easy to tell apart. MakeFromHSV8 takes the
        // hue as a 0..255 byte that maps to 0..360 degrees, so convert the normalized fraction directly to that byte.
        const float HueFraction = FMath::Frac(static_cast<float>(FactionIndex) * 0.61803398875f); // [0,1)
        const uint8 HueByte = static_cast<uint8>(HueFraction * 255.0f);

        // Full saturation, high (not max) value so the swatch reads as a solid faction color rather than a harsh primary.
        constexpr uint8 Saturation = 220;
        constexpr uint8 Value = 235;

        const FLinearColor Linear = FLinearColor::MakeFromHSV8(HueByte, Saturation, Value);
        FColor Out = Linear.ToFColor(true); // sRGB — these colors are shown in UI / on meshes
        Out.A = 255;
        return Out;
    }

    FColor GetFactionColor(const FMythicFactionData& Data, uint8 FactionIndex) {
        if (Data.bOverrideFactionColor) {
            return Data.FactionColor;
        }
        return DeterministicColorForId(FactionIndex);
    }

} // namespace MythicFactionColor
