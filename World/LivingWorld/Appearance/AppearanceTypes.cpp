// Mythic Living World — NPC Appearance implementation

#include "World/LivingWorld/Appearance/AppearanceTypes.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h" // FMythicNPCGenerator::HashStep (public static)

namespace {
    // Appearance seed salt — DISTINCT from the other generator salts (personality 0x12345678 / archetype 0xCAFEBABE /
    // demographic 0xDEADBEEF) so the wardrobe stream is decorrelated from name/personality/demographic streams.
    constexpr uint32 AppearanceSalt = 0x0A99EA00u;
} // namespace

// ─────────────────────────────────────────────────────────────
// UMythicAppearanceLibrary
// ─────────────────────────────────────────────────────────────

const FMythicOutfitSet* UMythicAppearanceLibrary::FindBestFor(const FGameplayTag& RoleTag, uint8 WealthTier) const {
    const FMythicOutfitSet* Best = nullptr;
    for (const FMythicOutfitSet& Set : OutfitSets) {
        const bool bRoleOk = !Set.RoleTag.IsValid() || RoleTag.MatchesTag(Set.RoleTag);
        const bool bWealthOk = WealthTier >= Set.MinWealthTier && WealthTier <= Set.MaxWealthTier;
        if (!bRoleOk || !bWealthOk) {
            continue;
        }
        if (!Best || Set.RelativeWeight > Best->RelativeWeight) {
            Best = &Set;
        }
    }
    return Best;
}

// ─────────────────────────────────────────────────────────────
// Code defaults
// ─────────────────────────────────────────────────────────────

namespace MythicAppearanceDefaults {

    TConstArrayView<FMythicOutfitSet> GetCodeDefaultOutfitSets() {
        // Built once, then handed out by view. Static init is thread-safe (C++11 magic statics); the data is const
        // thereafter so concurrent reads are race-free.
        static const TArray<FMythicOutfitSet> Sets = []() {
            TArray<FMythicOutfitSet> Out;

            // Per-slot option counts indexed by EMythicAppearanceSlot { Head, Hair, Torso, Legs, Feet, Hands, Back }.
            auto MakeCounts = [](uint8 Head, uint8 Hair, uint8 Torso, uint8 Legs, uint8 Feet, uint8 Hands, uint8 Back) {
                TArray<uint8> C;
                C.Reserve(static_cast<int32>(EMythicAppearanceSlot::COUNT));
                C.Add(Head);
                C.Add(Hair);
                C.Add(Torso);
                C.Add(Legs);
                C.Add(Feet);
                C.Add(Hands);
                C.Add(Back);
                return C;
            };

            // 1) Always-eligible commoner set (empty role, full wealth band). Guarantees every NPC resolves to a look.
            {
                FMythicOutfitSet& S = Out.AddDefaulted_GetRef();
                S.RoleTag = FGameplayTag();
                S.MinWealthTier = 0;
                S.MaxWealthTier = 3;
                S.PartCountPerSlot = MakeCounts(/*Head*/ 1, /*Hair*/ 4, /*Torso*/ 3, /*Legs*/ 3, /*Feet*/ 2, /*Hands*/ 1, /*Back*/ 1);
                S.BodyTypeCount = 2;
                S.RelativeWeight = 1.0f;
            }

            // 2) Ragged poor set — low-tier only (0..1). Higher weight at the bottom of the wealth band.
            {
                FMythicOutfitSet& S = Out.AddDefaulted_GetRef();
                S.RoleTag = FGameplayTag();
                S.MinWealthTier = 0;
                S.MaxWealthTier = 1;
                S.PartCountPerSlot = MakeCounts(1, 4, 2, 2, 2, 1, 1);
                S.BodyTypeCount = 2;
                S.RelativeWeight = 1.5f;
            }

            // 3) Fine attire — high-tier only (2..3). Wealthier NPCs trend to this.
            {
                FMythicOutfitSet& S = Out.AddDefaulted_GetRef();
                S.RoleTag = FGameplayTag();
                S.MinWealthTier = 2;
                S.MaxWealthTier = 3;
                S.PartCountPerSlot = MakeCounts(2, 4, 4, 3, 3, 2, 2);
                S.BodyTypeCount = 2;
                S.RelativeWeight = 1.5f;
            }

            return Out;
        }();
        return Sets;
    }

    TConstArrayView<FColor> GetCodeDefaultSkinTonePalette() {
        static const TArray<FColor> Palette = []() {
            TArray<FColor> Out;
            // A spread of plausible human skin tones (sRGB). Non-empty by construction.
            Out.Add(FColor(0xFF, 0xDB, 0xAC)); // light
            Out.Add(FColor(0xF1, 0xC2, 0x7D));
            Out.Add(FColor(0xE0, 0xAC, 0x69));
            Out.Add(FColor(0xC6, 0x8E, 0x52));
            Out.Add(FColor(0xA1, 0x66, 0x3F));
            Out.Add(FColor(0x80, 0x4A, 0x2B));
            Out.Add(FColor(0x5C, 0x33, 0x1E)); // deep
            return Out;
        }();
        return Palette;
    }

    TConstArrayView<FColor> GetCodeDefaultHairTonePalette() {
        static const TArray<FColor> Palette = []() {
            TArray<FColor> Out;
            Out.Add(FColor(0x09, 0x06, 0x06)); // black
            Out.Add(FColor(0x2C, 0x22, 0x2B)); // dark brown
            Out.Add(FColor(0x55, 0x39, 0x21)); // brown
            Out.Add(FColor(0x8D, 0x65, 0x3A)); // light brown
            Out.Add(FColor(0xB8, 0x9B, 0x5E)); // dark blonde
            Out.Add(FColor(0xDE, 0xBC, 0x99)); // blonde
            Out.Add(FColor(0x8C, 0x3B, 0x1B)); // auburn / red
            Out.Add(FColor(0xB7, 0xB7, 0xB7)); // grey
            return Out;
        }();
        return Palette;
    }

} // namespace MythicAppearanceDefaults

// ─────────────────────────────────────────────────────────────
// FMythicAppearanceResolver
// ─────────────────────────────────────────────────────────────

uint8 FMythicAppearanceResolver::WealthTierFromHash(uint32 NameHash) {
    // Stable wealth tier from NameHash only (NOT live reserves). >>8 to use a different byte of the hash than the part
    // selection below, so wealth and part choice are decorrelated for a given NPC.
    const uint32 H = FMythicNPCGenerator::HashStep(NameHash ^ AppearanceSalt);
    return static_cast<uint8>((H >> 8) % 4u);
}

FMythicAppearance FMythicAppearanceResolver::Resolve(
    uint32 NameHash,
    uint8 DemographicFlags,
    const FGameplayTag& RoleTag,
    uint8 FactionIndex,
    uint8 WealthTier,
    const FColor& FactionPrimary,
    const FColor& FactionSecondary,
    TConstArrayView<FMythicOutfitSet> OutfitSets,
    TConstArrayView<FColor> SkinPalette,
    TConstArrayView<FColor> HairPalette) {

    (void)FactionIndex; // reserved for future per-faction wardrobe biasing; signature kept stable.

    FMythicAppearance Out;

    // ─── Demographics unpack (matches GenerateDemographicFlags packing: [3b age @5][1b gender @4]) ───
    Out.AgeBracket = static_cast<uint8>((DemographicFlags >> 5) & 0x7u);
    Out.bIsFemale = static_cast<uint8>((DemographicFlags >> 4) & 0x1u);

    // ─── Colors (copied straight from the resolved faction colors) ───
    Out.PrimaryColor = FactionPrimary;
    Out.SecondaryColor = FactionSecondary;

    // ─── Seed (distinct salt) ───
    const uint32 Seed = FMythicNPCGenerator::HashStep(NameHash ^ AppearanceSalt);

    // ─── Weighted pick of an eligible outfit set (mirror PickTemplateIndex two-pass accumulate-then-threshold) ───
    int32 ChosenSet = INDEX_NONE;
    {
        float Total = 0.0f;
        for (const FMythicOutfitSet& S : OutfitSets) {
            const bool bRoleOk = !S.RoleTag.IsValid() || RoleTag.MatchesTag(S.RoleTag);
            const bool bWealthOk = WealthTier >= S.MinWealthTier && WealthTier <= S.MaxWealthTier;
            if (bRoleOk && bWealthOk) {
                Total += FMath::Max(0.0f, S.RelativeWeight);
            }
        }

        if (Total > UE_KINDA_SMALL_NUMBER) {
            // Use the low 24 bits of the seed as the roll fraction (same convention as PickTemplateIndex).
            const float Roll = (static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f) * Total;
            float Cumulative = 0.0f;
            int32 LastEligible = INDEX_NONE;
            for (int32 i = 0; i < OutfitSets.Num(); ++i) {
                const FMythicOutfitSet& S = OutfitSets[i];
                const bool bRoleOk = !S.RoleTag.IsValid() || RoleTag.MatchesTag(S.RoleTag);
                const bool bWealthOk = WealthTier >= S.MinWealthTier && WealthTier <= S.MaxWealthTier;
                if (!bRoleOk || !bWealthOk) {
                    continue;
                }
                const float W = FMath::Max(0.0f, S.RelativeWeight);
                if (W <= 0.0f) {
                    continue;
                }
                LastEligible = i;
                Cumulative += W;
                if (Roll < Cumulative) {
                    ChosenSet = i;
                    break;
                }
            }
            if (ChosenSet == INDEX_NONE) {
                ChosenSet = LastEligible; // FP boundary: Roll can equal Total.
            }
        }
    }

    Out.OutfitSetId = (ChosenSet == INDEX_NONE) ? 0 : static_cast<uint8>(ChosenSet);

    // ─── Per-slot part selection ───
    // If a set was chosen, count = its PartCountPerSlot[slot] (>=1); otherwise everything is a single default part.
    const FMythicOutfitSet* Set = OutfitSets.IsValidIndex(ChosenSet) ? &OutfitSets[ChosenSet] : nullptr;
    for (uint8 s = 0; s < static_cast<uint8>(EMythicAppearanceSlot::COUNT); ++s) {
        uint8 Count = 1;
        if (Set && Set->PartCountPerSlot.IsValidIndex(s)) {
            Count = FMath::Max<uint8>(1, Set->PartCountPerSlot[s]);
        }
        const uint32 SlotHash = FMythicNPCGenerator::HashStep(Seed + s + 1u);
        const EMythicAppearanceSlot Slot = static_cast<EMythicAppearanceSlot>(s);
        Out.PartForSlot(Slot) = static_cast<uint8>(SlotHash % static_cast<uint32>(Count));
    }

    // ─── Body type ───
    {
        const uint8 BodyTypeCount = Set ? FMath::Max<uint8>(1, Set->BodyTypeCount) : 1;
        const uint32 BodyHash = FMythicNPCGenerator::HashStep(Seed ^ 0xB0D17A11u);
        Out.BodyType = static_cast<uint8>(BodyHash % static_cast<uint32>(BodyTypeCount));
    }

    // ─── Skin / hair tones (clamp into the provided palette; defaults guaranteed non-empty) ───
    {
        const int32 SkinSize = FMath::Max(1, SkinPalette.Num());
        const int32 HairSize = FMath::Max(1, HairPalette.Num());
        const uint32 SkinHash = FMythicNPCGenerator::HashStep(Seed ^ 0x5C12A001u);
        const uint32 HairHash = FMythicNPCGenerator::HashStep(Seed ^ 0x4A12C010u);
        Out.SkinTone = static_cast<uint8>(SkinHash % static_cast<uint32>(SkinSize));
        Out.HairTone = static_cast<uint8>(HairHash % static_cast<uint32>(HairSize));
    }

    return Out;
}
