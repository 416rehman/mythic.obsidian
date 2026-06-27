// Mythic Living World — Group spawn template code defaults

#include "World/LivingWorld/Groups/GroupTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h" // NPC.Role.* + NPC.Group.* native tags

namespace MythicGroupDefaults {
    void BuildDefaultTemplates(TArray<FMythicGroupTemplate> &Out) {
        Out.Reset();

        // ─── Retinue: a Noble + 2 Guards (Subordinate guards → leader noble) ───
        // Gated to a wealthy, populous, militarized faction (a lord with a personal guard) so it only appears where it
        // makes sense. The noble is the leader; guards are Subordinate to it (edges orient toward the noble). This is
        // the path that lets a Noble — which is bAllowedAlone=false in the archetype catalog — actually appear.
        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_RETINUE;
            T.DisplayName = FText::FromString(TEXT("Noble Retinue"));
            T.IntraRelation = EMythicSocialRelation::Subordinate;
            T.IntraEdgeStrength = 0.7f;
            T.MinFactionMilitaryStrength = 0.4f;
            T.MinFactionPopulation = 30;
            T.MinReserveWealth = 50.0f;
            T.RelativeWeight = 1.0f;

            FMythicGroupMemberSpec Noble;
            Noble.RoleTag = TAG_NPC_ROLE_NOBLE;
            Noble.MinCount = 1;
            Noble.MaxCount = 1;
            Noble.bIsLeader = true;
            T.Members.Add(Noble);

            FMythicGroupMemberSpec Guards;
            Guards.RoleTag = TAG_NPC_ROLE_GUARD;
            Guards.MinCount = 2;
            Guards.MaxCount = 2;
            Guards.bIsLeader = false;
            T.Members.Add(Guards);
        }

        // ─── Barter party: a Merchant + 2-3 Civilian porters (Associate) ───
        // Gated to a faction with a little wealth + population. Merchant is the leader; porters are professional
        // Associates. Total 3-4 members.
        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_BARTER;
            T.DisplayName = FText::FromString(TEXT("Barter Party"));
            T.IntraRelation = EMythicSocialRelation::Associate;
            T.IntraEdgeStrength = 0.5f;
            T.MinFactionMilitaryStrength = 0.0f;
            T.MinFactionPopulation = 10;
            T.MinReserveWealth = 10.0f;
            T.RelativeWeight = 1.5f;

            FMythicGroupMemberSpec Merchant;
            Merchant.RoleTag = TAG_NPC_ROLE_MERCHANT;
            Merchant.MinCount = 1;
            Merchant.MaxCount = 1;
            Merchant.bIsLeader = true;
            T.Members.Add(Merchant);

            FMythicGroupMemberSpec Porters;
            Porters.RoleTag = TAG_NPC_ROLE_LABORER;
            Porters.MinCount = 2;
            Porters.MaxCount = 3;
            Porters.bIsLeader = false;
            T.Members.Add(Porters);
        }

        // ─── Friend trio: 3 Civilians out together (Friend) ───
        // Always eligible (no faction gates) so even a poor settlement gets some social colour. The first spawned member
        // is the de-facto leader (no explicit leader flag — the processor falls back to member 0).
        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_SOCIAL;
            T.DisplayName = FText::FromString(TEXT("Friends"));
            T.IntraRelation = EMythicSocialRelation::Friend;
            T.IntraEdgeStrength = 0.6f;
            T.MinFactionMilitaryStrength = 0.0f;
            T.MinFactionPopulation = 0;
            T.MinReserveWealth = 0.0f;
            T.RelativeWeight = 1.0f;

            FMythicGroupMemberSpec Friends;
            Friends.RoleTag = TAG_NPC_ROLE_CIVILIAN;
            Friends.MinCount = 3;
            Friends.MaxCount = 3;
            Friends.bIsLeader = false;
            T.Members.Add(Friends);
        }
    }
} // namespace MythicGroupDefaults
