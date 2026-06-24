// Mythic Living World — NPC Generation Utility Implementation
// Full procedural NPC identity pipeline from faction culture data.

#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "GameplayTagContainer.h"

// ─────────────────────────────────────────────────────────────
// Syllable Tables — Faction culture-specific name components
// ─────────────────────────────────────────────────────────────

// Culture 0: Imperial/Latin (Kingdoms, Empires, Religious orders)
static const TCHAR* ImperialFirstSyllables[] = {
    TEXT("Au"), TEXT("Cas"), TEXT("Val"), TEXT("Mar"), TEXT("Tib"),
    TEXT("Jul"), TEXT("Ser"), TEXT("Cor"), TEXT("Max"), TEXT("Luc"),
    TEXT("Flav"), TEXT("Octa"), TEXT("Ant"), TEXT("Dec"), TEXT("Quint"),
    TEXT("Sul"), TEXT("Pom"), TEXT("Aur"), TEXT("Cla"), TEXT("Dom")
};
static const TCHAR* ImperialEndSyllables[] = {
    TEXT("ius"), TEXT("ian"), TEXT("us"), TEXT("ius"), TEXT("ean"),
    TEXT("ax"), TEXT("ius"), TEXT("on"), TEXT("alis"), TEXT("inus"),
    TEXT("ien"), TEXT("ras"), TEXT("tus"), TEXT("vian"), TEXT("ard"),
    TEXT("oric"), TEXT("mund"), TEXT("ston"), TEXT("wyn"), TEXT("rick")
};

// Culture 1: Nordic/Harsh (Warbands, Northern clans)
static const TCHAR* NordicFirstSyllables[] = {
    TEXT("Thor"), TEXT("Bjor"), TEXT("Sig"), TEXT("Ul"), TEXT("Hel"),
    TEXT("Frey"), TEXT("Gun"), TEXT("Rag"), TEXT("Sven"), TEXT("Brun"),
    TEXT("Eid"), TEXT("Tor"), TEXT("Halv"), TEXT("Ing"), TEXT("Arn"),
    TEXT("Grim"), TEXT("Hak"), TEXT("Kol"), TEXT("Ulf"), TEXT("Var")
};
static const TCHAR* NordicEndSyllables[] = {
    TEXT("nar"), TEXT("rik"), TEXT("mund"), TEXT("gard"), TEXT("vald"),
    TEXT("sson"), TEXT("dor"), TEXT("mar"), TEXT("rok"), TEXT("thur"),
    TEXT("bjorn"), TEXT("heim"), TEXT("olf"), TEXT("sten"), TEXT("gar"),
    TEXT("vik"), TEXT("thar"), TEXT("dak"), TEXT("ryn"), TEXT("rek")
};

// Culture 2: Elven/Flowing (Ancient races, mages, forest dwellers)
static const TCHAR* ElvenFirstSyllables[] = {
    TEXT("Ael"), TEXT("Cael"), TEXT("Lia"), TEXT("Tha"), TEXT("Eol"),
    TEXT("Gal"), TEXT("Ael"), TEXT("Sil"), TEXT("Lor"), TEXT("Ara"),
    TEXT("Elen"), TEXT("Mira"), TEXT("Fael"), TEXT("Nol"), TEXT("Tel"),
    TEXT("Ira"), TEXT("Rhi"), TEXT("Vol"), TEXT("Nal"), TEXT("Dae")
};
static const TCHAR* ElvenEndSyllables[] = {
    TEXT("wyn"), TEXT("dyn"), TEXT("iel"), TEXT("wen"), TEXT("dris"),
    TEXT("andil"), TEXT("iel"), TEXT("ador"), TEXT("anor"), TEXT("thien"),
    TEXT("ara"), TEXT("ella"), TEXT("ion"), TEXT("iel"), TEXT("ris"),
    TEXT("loth"), TEXT("nara"), TEXT("ven"), TEXT("ria"), TEXT("mir")
};

// Culture 3: Tribal/Primal (Bandits, Creature-adjacent, Nomads)
static const TCHAR* TribalFirstSyllables[] = {
    TEXT("Gro"), TEXT("Mok"), TEXT("Zul"), TEXT("Tor"), TEXT("Dak"),
    TEXT("Gor"), TEXT("Kra"), TEXT("Bor"), TEXT("Thr"), TEXT("Nak"),
    TEXT("Rak"), TEXT("Dur"), TEXT("Var"), TEXT("Shak"), TEXT("Gul"),
    TEXT("Ash"), TEXT("Ur"), TEXT("Mur"), TEXT("Dro"), TEXT("Kag")
};
static const TCHAR* TribalEndSyllables[] = {
    TEXT("nak"), TEXT("thar"), TEXT("gash"), TEXT("rak"), TEXT("mok"),
    TEXT("dak"), TEXT("tor"), TEXT("gul"), TEXT("bash"), TEXT("urn"),
    TEXT("mar"), TEXT("dok"), TEXT("rok"), TEXT("zul"), TEXT("gar"),
    TEXT("tuk"), TEXT("gor"), TEXT("kash"), TEXT("bur"), TEXT("nak")
};

// Culture 4: Generic Fantasy (fallback)
static const TCHAR* GenericFirstSyllables[] = {
    TEXT("Al"), TEXT("Bel"), TEXT("Cor"), TEXT("Dra"), TEXT("El"),
    TEXT("Fen"), TEXT("Gar"), TEXT("Hal"), TEXT("Iro"), TEXT("Kel"),
    TEXT("Lor"), TEXT("Mal"), TEXT("Nor"), TEXT("Ora"), TEXT("Per"),
    TEXT("Quen"), TEXT("Ral"), TEXT("Sar"), TEXT("Tal"), TEXT("Ven")
};
static const TCHAR* GenericEndSyllables[] = {
    TEXT("dric"), TEXT("mund"), TEXT("wyn"), TEXT("tos"), TEXT("ric"),
    TEXT("don"), TEXT("han"), TEXT("gar"), TEXT("ven"), TEXT("ion"),
    TEXT("ard"), TEXT("lin"), TEXT("tor"), TEXT("ren"), TEXT("dan"),
    TEXT("ric"), TEXT("mos"), TEXT("lan"), TEXT("win"), TEXT("ras")
};

static constexpr int32 SyllableTableSize = 20;

// ─────────────────────────────────────────────────────────────
// Hash Utility
// ─────────────────────────────────────────────────────────────

uint32 FMythicNPCGenerator::HashStep(uint32 Seed) {
    // Wang hash — fast integer hash with good avalanche properties
    Seed = (Seed ^ 61u) ^ (Seed >> 16u);
    Seed *= 9u;
    Seed = Seed ^ (Seed >> 4u);
    Seed *= 0x27d4eb2du;
    Seed = Seed ^ (Seed >> 15u);
    return Seed;
}

// ─────────────────────────────────────────────────────────────
// Syllable Access
// ─────────────────────────────────────────────────────────────

const TCHAR* FMythicNPCGenerator::GetSyllable(uint8 CultureIndex, int32 SyllableIndex, bool bIsFirst) {
    // Clamp to available tables
    const int32 SafeIndex = FMath::Abs(SyllableIndex) % SyllableTableSize;

    switch (CultureIndex) {
    case 0: return bIsFirst ? ImperialFirstSyllables[SafeIndex] : ImperialEndSyllables[SafeIndex];
    case 1: return bIsFirst ? NordicFirstSyllables[SafeIndex] : NordicEndSyllables[SafeIndex];
    case 2: return bIsFirst ? ElvenFirstSyllables[SafeIndex] : ElvenEndSyllables[SafeIndex];
    case 3: return bIsFirst ? TribalFirstSyllables[SafeIndex] : TribalEndSyllables[SafeIndex];
    default: return bIsFirst ? GenericFirstSyllables[SafeIndex] : GenericEndSyllables[SafeIndex];
    }
}

int32 FMythicNPCGenerator::GetSyllableCount(uint8 CultureIndex, bool bIsFirst) {
    return SyllableTableSize;
}

// ─────────────────────────────────────────────────────────────
// Name Generation
// ─────────────────────────────────────────────────────────────

uint32 FMythicNPCGenerator::GenerateNameHash(uint8 FactionIndex, const FMythicCellCoord& Cell, int32 SpawnIndex) {
    // Combine all inputs into a single deterministic hash
    uint32 Hash = static_cast<uint32>(FactionIndex);
    Hash = HashCombine(Hash, GetTypeHash(Cell));
    Hash = HashCombine(Hash, static_cast<uint32>(SpawnIndex));
    // Additional mixing for better distribution
    Hash = HashStep(Hash);
    return Hash;
}

FName FMythicNPCGenerator::ReconstructNameFromHash(uint32 NameHash, uint8 FactionIndex) {
    // Determine culture from faction index (maps to syllable table)
    // In a full implementation, this would look up the faction's culture index
    // from the FactionDatabase. For now, faction index % 5 selects culture.
    const uint8 CultureIndex = FactionIndex % 5;

    // Derive syllable indices deterministically from hash
    uint32 H = NameHash;

    const int32 FirstSyllableIdx = static_cast<int32>(H % SyllableTableSize);
    H = HashStep(H);
    const int32 EndSyllableIdx = static_cast<int32>(H % SyllableTableSize);
    H = HashStep(H);

    // 30% chance of a middle syllable (for longer names)
    const bool bHasMiddle = (H % 10) < 3;
    H = HashStep(H);
    const int32 MiddleSyllableIdx = static_cast<int32>(H % SyllableTableSize);

    // Build the name string
    FString Name;
    Name.Reserve(24);
    Name.Append(GetSyllable(CultureIndex, FirstSyllableIdx, true));

    if (bHasMiddle) {
        // Use an end syllable from a different culture for variety
        const TCHAR* MiddleSyl = GetSyllable((CultureIndex + 1) % 5, MiddleSyllableIdx, false);
        // Take only first 2-3 chars of the middle syllable
        const int32 MiddleLen = FMath::Min(3, FCString::Strlen(MiddleSyl));
        Name.Append(MiddleSyl, MiddleLen);
    }

    Name.Append(GetSyllable(CultureIndex, EndSyllableIdx, false));

    return FName(*Name);
}

// ─────────────────────────────────────────────────────────────
// Demographics
// ─────────────────────────────────────────────────────────────

uint8 FMythicNPCGenerator::GenerateDemographicFlags(uint32 NameHash, bool bHasCivilians) {
    uint32 H = HashStep(NameHash ^ 0xDEADBEEF);

    // ─── Gender (50/50) ───
    const uint8 Gender = static_cast<uint8>(H & 1u);
    H >>= 1;

    // ─── Age bracket ───
    // Distribution weighted toward adults for populated factions
    uint8 AgeBracket;
    if (bHasCivilians) {
        // Civilian population: 5% child, 15% young, 40% adult, 25% middle, 15% elder
        const int32 Roll = static_cast<int32>(H % 100);
        if (Roll < 5)        AgeBracket = 0; // Child
        else if (Roll < 20)  AgeBracket = 1; // Young
        else if (Roll < 60)  AgeBracket = 2; // Adult
        else if (Roll < 85)  AgeBracket = 3; // Middle-aged
        else                 AgeBracket = 4; // Elder
    } else {
        // Military/non-civilian: 0% child, 20% young, 60% adult, 15% middle, 5% elder
        const int32 Roll = static_cast<int32>(H % 100);
        if (Roll < 20)       AgeBracket = 1; // Young
        else if (Roll < 80)  AgeBracket = 2; // Adult
        else if (Roll < 95)  AgeBracket = 3; // Middle-aged
        else                 AgeBracket = 4; // Elder
    }

    // Pack: [3 bits: age] [1 bit: gender] [4 bits: reserved]
    return static_cast<uint8>((AgeBracket << 5) | (Gender << 4));
}

uint8 FMythicNPCGenerator::GenerateVisualArchetype(uint32 NameHash, uint8 MaxArchetypes) {
    if (MaxArchetypes == 0) return 0;
    const uint32 H = HashStep(NameHash ^ 0xCAFEBABE);
    return static_cast<uint8>(H % MaxArchetypes);
}

// ─────────────────────────────────────────────────────────────
// Personality Generation
// ─────────────────────────────────────────────────────────────

FMythicPersonalityFragment FMythicNPCGenerator::GeneratePersonality(
    uint32 NameHash,
    const FMythicIdeologyProfile& Ideology,
    const FGameplayTag& RoleTag) {

    FMythicPersonalityFragment Personality;
    uint32 H = HashStep(NameHash ^ 0x12345678);

    // ─── Step 1: Faction ideology baseline ───
    // Map each ideology axis to the vent channels it influences.
    // High Violence → higher Fight weight
    // High Authority → higher Enforce weight
    // High Mercy → higher Tend weight
    // High Theft → higher Exploit weight
    // High Loyalty → higher Submit weight (obedient)
    // Low Authority → higher Rally weight (anti-authority = rebels)

    constexpr float BaseWeight = 0.3f; // All channels start with a baseline

    // Fight: boosted by Violence tolerance, reduced by Mercy
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] =
        BaseWeight + Ideology.Violence * 0.25f - Ideology.Mercy * 0.1f;

    // Flee: inversely related to Violence (pacifists flee more readily)
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] =
        BaseWeight - Ideology.Violence * 0.15f + (1.0f - Ideology.Loyalty) * 0.1f;

    // Enforce: heavily boosted by Authority
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)] =
        BaseWeight + Ideology.Authority * 0.3f;

    // Report: boosted by Authority and low Deception (honest factions report crimes)
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)] =
        BaseWeight + Ideology.Authority * 0.15f - Ideology.Deception * 0.15f;

    // Exploit: boosted by Theft and Deception
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)] =
        BaseWeight + Ideology.Theft * 0.2f + Ideology.Deception * 0.15f;

    // Tend: boosted by Mercy and Sanctity (compassionate/religious factions help others)
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)] =
        BaseWeight + Ideology.Mercy * 0.25f + Ideology.Sanctity * 0.1f;

    // Rally: boosted by negative Authority (rebels rally), and high Loyalty (loyal leaders rally)
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)] =
        BaseWeight - Ideology.Authority * 0.1f + Ideology.Loyalty * 0.15f;

    // Submit: boosted by high Authority (followers of authority submit)
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Submit)] =
        BaseWeight + Ideology.Authority * 0.2f - Ideology.Violence * 0.1f;

    // ─── Step 2: Role modifier ───
    // Guards get boosted Enforce/Fight, merchants get boosted Tend/Exploit
    if (RoleTag.IsValid()) {
        const FString RoleName = RoleTag.ToString();
        if (RoleName.Contains(TEXT("Guard")) || RoleName.Contains(TEXT("Soldier"))) {
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)] += 0.2f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] += 0.15f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] -= 0.1f;
        } else if (RoleName.Contains(TEXT("Merchant")) || RoleName.Contains(TEXT("Trader"))) {
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)] += 0.1f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)] += 0.1f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] -= 0.15f;
        } else if (RoleName.Contains(TEXT("Priest")) || RoleName.Contains(TEXT("Healer"))) {
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)] += 0.25f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] -= 0.2f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] += 0.1f;
        } else if (RoleName.Contains(TEXT("Leader")) || RoleName.Contains(TEXT("Captain"))) {
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)] += 0.25f;
            Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)] += 0.1f;
        }
    }

    // ─── Step 3: Random variance (±0.15 per channel, seeded by hash) ───
    constexpr int32 VentCount = static_cast<int32>(EMythicVentChannel::COUNT);
    for (int32 i = 0; i < VentCount; ++i) {
        H = HashStep(H);
        // Convert to [-0.15, +0.15] range
        const float Variance = (static_cast<float>(H % 300) / 1000.0f) - 0.15f;
        Personality.VentWeights[i] += Variance;
    }

    // ─── Step 4: Outlier chance (5% per channel — extreme personality trait) ───
    H = HashStep(H);
    for (int32 i = 0; i < VentCount; ++i) {
        H = HashStep(H);
        if ((H % 100) < 5) {
            // Outlier: set this channel to an extreme value
            const bool bHighOutlier = (H % 2) == 0;
            Personality.VentWeights[i] = bHighOutlier ? 0.9f : 0.05f;
        }
    }

    // ─── Step 5: Clamp all weights to [0.0, 1.0] ───
    for (int32 i = 0; i < VentCount; ++i) {
        Personality.VentWeights[i] = FMath::Clamp(Personality.VentWeights[i], 0.0f, 1.0f);
    }

    return Personality;
}
