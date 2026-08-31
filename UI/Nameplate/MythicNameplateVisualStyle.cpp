#include "UI/Nameplate/MythicNameplateVisualStyle.h"

#include "Components/RichTextBlockImageDecorator.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Misc/DataValidation.h"
#include "UObject/ConstructorHelpers.h"

namespace {
FMythicNameplateIconToken MakeIcon(
    const EMythicSemanticIcon SemanticIcon, UTexture2D *Texture,
    const FLinearColor Tint,
    const float Size = 16.0f) {
    FMythicNameplateIconToken Token;
    Token.SemanticIcon = SemanticIcon;
    Token.Texture = Texture;
    Token.Tint = Tint;
    Token.LogicalSize = FVector2D(Size, Size);
    return Token;
}
} // namespace

bool FMythicNameplateIconToken::IsRenderable() const {
    return SemanticIcon != EMythicSemanticIcon::None && Texture != nullptr
        && FMath::IsFinite(LogicalSize.X)
        && FMath::IsFinite(LogicalSize.Y)
        && LogicalSize.X >= 8.0f && LogicalSize.X <= 32.0f
        && LogicalSize.Y >= 8.0f && LogicalSize.Y <= 32.0f
        && FMath::IsFinite(Tint.R) && FMath::IsFinite(Tint.G)
        && FMath::IsFinite(Tint.B) && FMath::IsFinite(Tint.A)
        && Tint.A > 0.0f;
}

UMythicNameplateVisualStyle::UMythicNameplateVisualStyle() {
    static ConstructorHelpers::FObjectFinder<UObject> IdentityFontObject(
        TEXT("/Game/Mythic/UI/Fonts/LeagueSpartan/LeagueSpartan-SemiBold_Font.LeagueSpartan-SemiBold_Font"));
    static ConstructorHelpers::FObjectFinder<UObject> SecondaryFontObject(
        TEXT("/Game/Mythic/UI/Fonts/LeagueSpartan/LeagueSpartan-Medium_Font.LeagueSpartan-Medium_Font"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> FactionShieldIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Cue_Faction.T_Sem_Cue_Faction"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> CharacterIcon(
        TEXT("/Game/Mythic/UI/Icons/Emblem/T_Emblem_Character.T_Emblem_Character"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> VoiceIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Cue_Talk.T_Sem_Cue_Talk"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> VendorIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Cue_Service.T_Sem_Cue_Service"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> QuestOfferIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Cue_QuestOffer.T_Sem_Cue_QuestOffer"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> QuestTurnInIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Cue_QuestTurnIn.T_Sem_Cue_QuestTurnIn"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> GuardingIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_State_Surrender.T_Sem_State_Surrender"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> FleeingIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_State_Fleeing.T_Sem_State_Fleeing"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> AttackingIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_State_Attacking.T_Sem_State_Attacking"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> DyingIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_State_Dying.T_Sem_State_Dying"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> DownedIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_State_Downed.T_Sem_State_Downed"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> HardTargetTexture(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Attention_HardTarget.T_Sem_Attention_HardTarget"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> LockedTargetTexture(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Attention_LockedTarget.T_Sem_Attention_LockedTarget"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> EliteIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Rank_Elite.T_Sem_Rank_Elite"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> SuperiorIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Rank_Superior.T_Sem_Rank_Superior"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> ChampionIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Rank_Champion.T_Sem_Rank_Champion"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> BossIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Rank_Boss.T_Sem_Rank_Boss"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> WorldBossIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Rank_WorldBoss.T_Sem_Rank_WorldBoss"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> ThreatRiskyIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Threat_Risky.T_Sem_Threat_Risky"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> ThreatDeadlyIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Threat_Deadly.T_Sem_Threat_Deadly"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> ThreatOverwhelmingIcon(
        TEXT("/Game/Mythic/UI/Icons/Semantic/T_Sem_Threat_Overwhelming.T_Sem_Threat_Overwhelming"));
    static ConstructorHelpers::FObjectFinder<UDataTable> GlobalRichTextImageSet(
        TEXT("/Game/Mythic/UI/DT_ImageRow.DT_ImageRow"));
    RichTextImageSet = GlobalRichTextImageSet.Object;
    IdentityFont.FontObject = IdentityFontObject.Object;
    IdentityFont.TypefaceFontName = TEXT("Default");
    IdentityFont.Size = 17;
    IdentityFont.OutlineSettings.OutlineSize = 1;
    IdentityFont.OutlineSettings.OutlineColor = FLinearColor(0.005f, 0.004f, 0.004f, 0.92f);

    SecondaryFont.FontObject = SecondaryFontObject.Object;
    SecondaryFont.TypefaceFontName = TEXT("Default");
    SecondaryFont.Size = 12;
    SecondaryFont.OutlineSettings.OutlineSize = 1;
    SecondaryFont.OutlineSettings.OutlineColor = FLinearColor(0.005f, 0.004f, 0.004f, 0.88f);

    IdentityProfile.MaximumSize = FVector2D(192.0f, 24.0f);
    IdentityProfile.IdentityFontSize = 14;
    IdentityProfile.HealthBandWidth = 0.0f;
    IdentityProfile.HealthBandHeight = 0.0f;

    ContextProfile.MaximumSize = FVector2D(216.0f, 42.0f);
    ContextProfile.IdentityFontSize = 15;
    ContextProfile.HealthBandWidth = 168.0f;
    ContextProfile.HealthBandHeight = 5.0f;

    FocusCombatProfile.MaximumSize = FVector2D(240.0f, 54.0f);
    FocusCombatProfile.IdentityFontSize = 16;
    FocusCombatProfile.HealthBandWidth = 184.0f;
    FocusCombatProfile.HealthBandHeight = 6.0f;

    EliteCombatProfile.MaximumSize = FVector2D(272.0f, 62.0f);
    EliteCombatProfile.IdentityFontSize = 17;
    EliteCombatProfile.HealthBandWidth = 216.0f;
    EliteCombatProfile.HealthBandHeight = 7.0f;

    BossProfile.MaximumSize = FVector2D(336.0f, 68.0f);
    BossProfile.IdentityFontSize = 18;
    BossProfile.HealthBandWidth = 304.0f;
    BossProfile.HealthBandHeight = 8.0f;

    const FLinearColor Neutral(0.88f, 0.90f, 0.92f, 1.0f);
    const FLinearColor Interaction(0.37f, 0.72f, 0.95f, 1.0f);
    const FLinearColor Quest(0.98f, 0.73f, 0.20f, 1.0f);
    const FLinearColor QuestComplete(0.35f, 0.86f, 0.54f, 1.0f);
    const FLinearColor Warning(0.96f, 0.59f, 0.16f, 1.0f);
    const FLinearColor Champion(0.72f, 0.43f, 0.96f, 1.0f);

    const auto MakeRichTextMatchedTarget =
        [this](const EMythicSemanticIcon SemanticIcon,
               UTexture2D *FallbackTexture,
               const FLinearColor FallbackTint) {
            FMythicNameplateIconToken Token = MakeIcon(
                SemanticIcon, FallbackTexture, FallbackTint, 16.0f);
            const FName RowName =
                UMythicRichTextIconLibrary::GetSemanticIconRowName(
                    SemanticIcon);
            const FRichImageRow *Row = RichTextImageSet && !RowName.IsNone()
                ? RichTextImageSet->FindRow<FRichImageRow>(
                      RowName, TEXT("Nameplate target token initialization"),
                      false)
                : nullptr;
            if (Row) {
                if (UTexture2D *RowTexture = Cast<UTexture2D>(
                        Row->Brush.GetResourceObject())) {
                    Token.Texture = RowTexture;
                }
                Token.Tint = Row->Brush.TintColor.GetSpecifiedColor();
                Token.LogicalSize = FVector2D(Row->Brush.ImageSize.X,
                                              Row->Brush.ImageSize.Y);
            }
            return Token;
        };
    HardTargetIcon = MakeRichTextMatchedTarget(
        EMythicSemanticIcon::AttentionHardTarget,
        HardTargetTexture.Object,
        FLinearColor(0.96f, 0.52f, 0.16f, 1.0f));
    LockedTargetIcon = MakeRichTextMatchedTarget(
        EMythicSemanticIcon::AttentionLockedTarget,
        LockedTargetTexture.Object,
        FLinearColor(0.98f, 0.68f, 0.20f, 1.0f));

    const auto AddCue = [this](const EMythicNameplatePrimaryCue Cue,
                               const FMythicNameplateIconToken &Icon) {
        FMythicNameplateCueIconBinding Binding;
        Binding.Cue = Cue;
        Binding.Icon = Icon;
        CueIcons.Add(Binding);
    };
    const auto AddRank = [this](const EMythicPresentedCombatRank Rank,
                               const FMythicNameplateIconToken &Icon) {
        FMythicNameplateRankIconBinding Binding;
        Binding.Rank = Rank;
        Binding.Icon = Icon;
        RankIcons.Add(Binding);
    };
    const auto AddThreat = [this](const EMythicThreatBand Threat,
                                 const FMythicNameplateIconToken &Icon) {
        FMythicNameplateThreatIconBinding Binding;
        Binding.Threat = Threat;
        Binding.Icon = Icon;
        ThreatIcons.Add(Binding);
    };

    CueIcons.Reset(11);
    AddCue(EMythicNameplatePrimaryCue::Faction,
           MakeIcon(EMythicSemanticIcon::CueFaction,
                    FactionShieldIcon.Object, Neutral));
    AddCue(EMythicNameplatePrimaryCue::Role,
           MakeIcon(EMythicSemanticIcon::CueRole,
                    CharacterIcon.Object, Neutral));
    AddCue(EMythicNameplatePrimaryCue::Service,
           MakeIcon(EMythicSemanticIcon::CueService,
                    VendorIcon.Object, Quest, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::QuestOffer,
           MakeIcon(EMythicSemanticIcon::CueQuestOffer,
                    QuestOfferIcon.Object, Quest, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::QuestTurnIn,
           MakeIcon(EMythicSemanticIcon::CueQuestTurnIn,
                    QuestTurnInIcon.Object, QuestComplete, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::DirectedTalk,
           MakeIcon(EMythicSemanticIcon::CueTalk,
                    VoiceIcon.Object, Interaction, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::Surrendering,
           MakeIcon(EMythicSemanticIcon::StateSurrender,
                    GuardingIcon.Object, Neutral, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::Fleeing,
           MakeIcon(EMythicSemanticIcon::StateFleeing,
                    FleeingIcon.Object, Warning, 17.0f));
    AddCue(EMythicNameplatePrimaryCue::AttackingViewer,
           MakeIcon(EMythicSemanticIcon::StateAttacking,
                    AttackingIcon.Object,
                    FLinearColor(0.92f, 0.15f, 0.10f, 1.0f), 17.0f));
    AddCue(EMythicNameplatePrimaryCue::Dying,
           MakeIcon(EMythicSemanticIcon::StateDying,
                    DyingIcon.Object,
                    FLinearColor(0.96f, 0.45f, 0.12f, 1.0f), 17.0f));
    AddCue(EMythicNameplatePrimaryCue::Downed,
           MakeIcon(EMythicSemanticIcon::StateDowned,
                    DownedIcon.Object,
                    FLinearColor(0.98f, 0.66f, 0.18f, 1.0f), 17.0f));
    RankIcons.Reset(5);
    AddRank(EMythicPresentedCombatRank::Superior,
            MakeIcon(EMythicSemanticIcon::RankSuperior,
                     SuperiorIcon.Object,
                     FLinearColor(0.78f, 0.72f, 0.61f, 1.0f), 16.0f));
    AddRank(EMythicPresentedCombatRank::Elite,
            MakeIcon(EMythicSemanticIcon::RankElite,
                     EliteIcon.Object,
                     FLinearColor(0.88f, 0.61f, 0.19f, 1.0f), 17.0f));
    AddRank(EMythicPresentedCombatRank::Champion,
            MakeIcon(EMythicSemanticIcon::RankChampion,
                     ChampionIcon.Object, Champion, 18.0f));
    AddRank(EMythicPresentedCombatRank::Boss,
            MakeIcon(EMythicSemanticIcon::RankBoss,
                     BossIcon.Object, FLinearColor(0.96f, 0.68f, 0.22f, 1.0f), 20.0f));
    AddRank(EMythicPresentedCombatRank::WorldBoss,
            MakeIcon(EMythicSemanticIcon::RankWorldBoss,
                     WorldBossIcon.Object, FLinearColor(0.96f, 0.84f, 0.48f, 1.0f), 22.0f));

    ThreatIcons.Reset(3);
    AddThreat(EMythicThreatBand::Risky,
              MakeIcon(EMythicSemanticIcon::ThreatRisky,
                       ThreatRiskyIcon.Object, Warning, 16.0f));
    AddThreat(EMythicThreatBand::Deadly,
              MakeIcon(EMythicSemanticIcon::ThreatDeadly,
                       ThreatDeadlyIcon.Object,
                       FLinearColor(0.96f, 0.34f, 0.10f, 1.0f), 18.0f));
    AddThreat(EMythicThreatBand::Overwhelming,
              MakeIcon(EMythicSemanticIcon::ThreatOverwhelming,
                       ThreatOverwhelmingIcon.Object,
                       FLinearColor(0.88f, 0.06f, 0.08f, 1.0f), 20.0f));
}

const FMythicNameplateProfileGeometry &
UMythicNameplateVisualStyle::ResolveGeometry(
    const EMythicNameplateDisclosureTier Disclosure,
    const EMythicNameplateVisualFamily Family,
    const EMythicPresentedCombatRank Rank) const {
    if (Family == EMythicNameplateVisualFamily::Boss) {
        return BossProfile;
    }
    if (Rank == EMythicPresentedCombatRank::Elite
        || Rank == EMythicPresentedCombatRank::Champion) {
        return EliteCombatProfile;
    }
    if (Family == EMythicNameplateVisualFamily::Combat
        || Family == EMythicNameplateVisualFamily::AllySafety
        || Disclosure == EMythicNameplateDisclosureTier::Focus) {
        return FocusCombatProfile;
    }
    return Disclosure == EMythicNameplateDisclosureTier::Context
        ? ContextProfile : IdentityProfile;
}

const FMythicNameplateIconToken *
UMythicNameplateVisualStyle::ResolveCueIcon(
    const EMythicNameplatePrimaryCue Cue) const {
    const FMythicNameplateCueIconBinding *Binding = CueIcons.FindByPredicate(
        [Cue](const FMythicNameplateCueIconBinding &Candidate) {
            return Candidate.Cue == Cue;
        });
    return Binding && Binding->Icon.IsRenderable()
        ? &Binding->Icon : nullptr;
}

const FMythicNameplateIconToken *
UMythicNameplateVisualStyle::ResolveTargetIcon(
    const EMythicNameplateAttentionState AttentionState) const {
    if (AttentionState == EMythicNameplateAttentionState::LockedCombatTarget) {
        return LockedTargetIcon.IsRenderable() ? &LockedTargetIcon : nullptr;
    }
    if (AttentionState == EMythicNameplateAttentionState::HardCombatTarget) {
        return HardTargetIcon.IsRenderable() ? &HardTargetIcon : nullptr;
    }
    return nullptr;
}

const FMythicNameplateIconToken *
UMythicNameplateVisualStyle::ResolveRankIcon(
    const EMythicPresentedCombatRank Rank) const {
    const FMythicNameplateRankIconBinding *Binding = RankIcons.FindByPredicate(
        [Rank](const FMythicNameplateRankIconBinding &Candidate) {
            return Candidate.Rank == Rank;
        });
    return Binding && Binding->Icon.IsRenderable()
        ? &Binding->Icon : nullptr;
}

const FMythicNameplateIconToken *
UMythicNameplateVisualStyle::ResolveThreatIcon(
    const EMythicThreatBand Threat) const {
    const FMythicNameplateThreatIconBinding *Binding =
        ThreatIcons.FindByPredicate(
            [Threat](const FMythicNameplateThreatIconBinding &Candidate) {
                return Candidate.Threat == Threat;
            });
    return Binding && Binding->Icon.IsRenderable()
        ? &Binding->Icon : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UMythicNameplateVisualStyle::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Reject = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    const auto ValidProfile = [](const FMythicNameplateProfileGeometry &Profile,
                                 const FVector2D HardMaximum) {
        return Profile.MaximumSize.X > 0.0f
            && Profile.MaximumSize.Y > 0.0f
            && Profile.MaximumSize.X <= HardMaximum.X
            && Profile.MaximumSize.Y <= HardMaximum.Y
            && Profile.IdentityFontSize >= 8
            && Profile.IdentityFontSize <= 32
            && Profile.HealthBandWidth >= 0.0f
            && Profile.HealthBandWidth <= HardMaximum.X
            && Profile.HealthBandHeight >= 0.0f
            && Profile.HealthBandHeight <= 12.0f;
    };

    if (!IdentityFont.HasValidFont() || !SecondaryFont.HasValidFont()) {
        Reject(NSLOCTEXT("MythicNameplateVisualStyle", "Fonts",
                        "Nameplate visual style requires valid identity and secondary fonts."));
    }
    if (!ValidProfile(IdentityProfile, FVector2D(192.0f, 24.0f))
        || !ValidProfile(ContextProfile, FVector2D(216.0f, 42.0f))
        || !ValidProfile(FocusCombatProfile, FVector2D(240.0f, 54.0f))
        || !ValidProfile(EliteCombatProfile, FVector2D(272.0f, 62.0f))
        || !ValidProfile(BossProfile, FVector2D(336.0f, 68.0f))) {
        Reject(NSLOCTEXT("MythicNameplateVisualStyle", "ProfileBounds",
                        "Nameplate profile geometry exceeds its binding 1080p budget or contains invalid values."));
    }
    if (ActionRailMaximumSize.X <= 0.0f
        || ActionRailMaximumSize.Y <= 0.0f
        || ActionRailMaximumSize.X > 280.0f
        || ActionRailMaximumSize.Y > 28.0f) {
        Reject(NSLOCTEXT("MythicNameplateVisualStyle", "ActionBounds",
                        "Action rail bounds must remain positive and no larger than 280 by 28 logical pixels."));
    }
    if (!RichTextImageSet
        || RichTextImageSet->GetRowStruct() != FRichImageRow::StaticStruct()) {
        Reject(NSLOCTEXT("MythicNameplateVisualStyle", "RichTextImageSet",
                        "Nameplate visual style requires the project Rich Image Row table."));
    }

    const auto ValidateRichTextParity =
        [this, &Reject](const FMythicNameplateIconToken &Icon) {
            if (!RichTextImageSet || !Icon.IsRenderable()) {
                return;
            }
            const FName RowName =
                UMythicRichTextIconLibrary::GetSemanticIconRowName(
                    Icon.SemanticIcon);
            const FRichImageRow *Row = RowName.IsNone()
                ? nullptr
                : RichTextImageSet->FindRow<FRichImageRow>(
                      RowName, TEXT("Nameplate visual-style validation"),
                      false);
            if (!Row) {
                Reject(FText::Format(
                    NSLOCTEXT("MythicNameplateVisualStyle", "MissingRichTextRow",
                              "Semantic icon {0} has no Rich Text image row."),
                    FText::FromName(RowName)));
                return;
            }

            const FVector2D RowSize(Row->Brush.ImageSize.X,
                                    Row->Brush.ImageSize.Y);
            const FLinearColor RowTint =
                Row->Brush.TintColor.GetSpecifiedColor();
            if (Row->Brush.GetResourceObject() != Icon.Texture
                || !RowSize.Equals(Icon.LogicalSize, KINDA_SMALL_NUMBER)
                || !RowTint.Equals(Icon.Tint, KINDA_SMALL_NUMBER)) {
                Reject(FText::Format(
                    NSLOCTEXT("MythicNameplateVisualStyle", "RichTextDrift",
                              "Semantic icon {0} differs between its fixed HUD token and Rich Text row."),
                    FText::FromName(RowName)));
            }
        };

    ValidateRichTextParity(HardTargetIcon);
    ValidateRichTextParity(LockedTargetIcon);
    if (!HardTargetIcon.IsRenderable()
        || HardTargetIcon.SemanticIcon
            != EMythicSemanticIcon::AttentionHardTarget
        || !LockedTargetIcon.IsRenderable()
        || LockedTargetIcon.SemanticIcon
            != EMythicSemanticIcon::AttentionLockedTarget) {
        Reject(NSLOCTEXT(
            "MythicNameplateVisualStyle", "TargetIcons",
            "Nameplate visual style requires resident hard-target and locked-target chevrons; ordinary focus has no target icon."));
    }

    TSet<uint8> SeenCues;
    for (const FMythicNameplateCueIconBinding &Binding : CueIcons) {
        ValidateRichTextParity(Binding.Icon);
        const uint8 Key = static_cast<uint8>(Binding.Cue);
        if (Binding.Cue == EMythicNameplatePrimaryCue::None
            || SeenCues.Contains(Key) || !Binding.Icon.IsRenderable()) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "CueIcons",
                            "Cue icon bindings require unique non-None semantics and resident bounded textures."));
            break;
        }
        SeenCues.Add(Key);
    }
    const EMythicNameplatePrimaryCue RequiredCues[] = {
        EMythicNameplatePrimaryCue::Faction,
        EMythicNameplatePrimaryCue::Role,
        EMythicNameplatePrimaryCue::Service,
        EMythicNameplatePrimaryCue::QuestOffer,
        EMythicNameplatePrimaryCue::QuestTurnIn,
        EMythicNameplatePrimaryCue::DirectedTalk,
        EMythicNameplatePrimaryCue::Surrendering,
        EMythicNameplatePrimaryCue::Fleeing,
        EMythicNameplatePrimaryCue::AttackingViewer,
        EMythicNameplatePrimaryCue::Dying,
        EMythicNameplatePrimaryCue::Downed,
    };
    for (const EMythicNameplatePrimaryCue Cue : RequiredCues) {
        if (!ResolveCueIcon(Cue)) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "CueCoverage",
                            "The visual style is missing a required contextual cue icon."));
            break;
        }
    }

    TSet<uint8> SeenRanks;
    for (const FMythicNameplateRankIconBinding &Binding : RankIcons) {
        ValidateRichTextParity(Binding.Icon);
        const uint8 Key = static_cast<uint8>(Binding.Rank);
        if (Binding.Rank == EMythicPresentedCombatRank::Unknown
            || Binding.Rank == EMythicPresentedCombatRank::Standard
            || SeenRanks.Contains(Key) || !Binding.Icon.IsRenderable()) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "RankIcons",
                            "Rank icon bindings require unique presented ranks and resident bounded textures."));
            break;
        }
        SeenRanks.Add(Key);
    }
    const EMythicPresentedCombatRank RequiredRanks[] = {
        EMythicPresentedCombatRank::Superior,
        EMythicPresentedCombatRank::Elite,
        EMythicPresentedCombatRank::Champion,
        EMythicPresentedCombatRank::Boss,
        EMythicPresentedCombatRank::WorldBoss,
    };
    for (const EMythicPresentedCombatRank Rank : RequiredRanks) {
        if (!ResolveRankIcon(Rank)) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "RankCoverage",
                            "The visual style is missing a required presented-rank emblem."));
            break;
        }
    }

    TSet<uint8> SeenThreats;
    for (const FMythicNameplateThreatIconBinding &Binding : ThreatIcons) {
        ValidateRichTextParity(Binding.Icon);
        const uint8 Key = static_cast<uint8>(Binding.Threat);
        if (Binding.Threat == EMythicThreatBand::Unknown
            || Binding.Threat == EMythicThreatBand::None
            || SeenThreats.Contains(Key) || !Binding.Icon.IsRenderable()) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "ThreatIcons",
                            "Threat icon bindings require unique warning bands and resident bounded textures."));
            break;
        }
        SeenThreats.Add(Key);
    }
    const EMythicThreatBand RequiredThreats[] = {
        EMythicThreatBand::Risky,
        EMythicThreatBand::Deadly,
        EMythicThreatBand::Overwhelming,
    };
    for (const EMythicThreatBand Threat : RequiredThreats) {
        if (!ResolveThreatIcon(Threat)) {
            Reject(NSLOCTEXT("MythicNameplateVisualStyle", "ThreatCoverage",
                            "The visual style is missing a required viewer-relative danger icon."));
            break;
        }
    }

    return Result == EDataValidationResult::Invalid
        ? Result : EDataValidationResult::Valid;
}
#endif
