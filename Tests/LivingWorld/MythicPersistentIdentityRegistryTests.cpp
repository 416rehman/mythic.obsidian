#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"

namespace MythicPersistentIdentityRegistryTests {
void WriteEntityId(FArchive &Ar, const FMythicEntityId &EntityId) {
    uint8 Domain = static_cast<uint8>(EntityId.GetDomain());
    FGuid Guid = EntityId.GetAuthorityGuid();
    Ar << Domain;
    Ar << Guid.A;
    Ar << Guid.B;
    Ar << Guid.C;
    Ar << Guid.D;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPersistentIdentityAllocationUniquenessTest,
    "Mythic.LivingWorld.Identity.AllocationUniqueness",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPersistentIdentityAllocationUniquenessTest::RunTest(
    const FString &Parameters) {
    UMythicPersistentNPCRegistry *Registry =
        NewObject<UMythicPersistentNPCRegistry>();
    TSet<FMythicEntityId> Seen;

    constexpr int32 AllocationCount = 256;
    constexpr uint32 CollidingNameSeed = 0xBADC0DEu;
    for (int32 Index = 0; Index < AllocationCount; ++Index) {
        const FMythicEntityId EntityId = Registry->AllocateEntityIdentity(
            CollidingNameSeed,
            EMythicEntityIdentityProvenance::SettlementPopulation);
        TestTrue(TEXT("allocation returns a valid typed ID"), EntityId.IsValid());
        TestFalse(TEXT("canonical GUID never aliases an earlier allocation"),
                  Seen.Contains(EntityId));
        Seen.Add(EntityId);
    }

    TestEqual(TEXT("every allocation has one registry record"),
              Registry->GetIdentityCount(), AllocationCount);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPersistentIdentitySerializationRoundTripTest,
    "Mythic.LivingWorld.Identity.SerializationRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPersistentIdentitySerializationRoundTripTest::RunTest(
    const FString &Parameters) {
    UMythicPersistentNPCRegistry *Source =
        NewObject<UMythicPersistentNPCRegistry>();
    const FMythicEntityId EntityId = Source->AllocateEntityIdentity(
        77u, EMythicEntityIdentityProvenance::Encounter);
    FMythicFactionId Faction;
    Faction.Index = 3;
    TestTrue(TEXT("death tombstone accepted for registered identity"),
             Source->RegisterDeath(EntityId, Faction, FGameplayTag(),
                                   FMythicCellCoord(9, 4), 42.5, nullptr));
    TestTrue(TEXT("durable learned knowledge marks offline-safe retention"),
             Source->MarkRetainedByLearnedDossier(EntityId));
    Source->AllocateNameSeedSerial();

    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes);
        Source->Serialize(Writer);
        TestFalse(TEXT("save archive remains valid"), Writer.IsError());
    }

    UMythicPersistentNPCRegistry *Loaded =
        NewObject<UMythicPersistentNPCRegistry>();
    {
        FMemoryReader Reader(Bytes);
        Loaded->Serialize(Reader);
        TestFalse(TEXT("load archive remains valid"), Reader.IsError());
    }

    const FMythicPersistentEntityIdentityRecord *Record =
        Loaded->FindIdentityRecord(EntityId);
    TestNotNull(TEXT("canonical identity survives the round trip"), Record);
    if (Record) {
        TestEqual(TEXT("name seed survives independently"), Record->NameSeed, 77u);
        TestEqual(TEXT("provenance survives independently"), Record->Provenance,
                  EMythicEntityIdentityProvenance::Encounter);
    }
    TestTrue(TEXT("permanent-death tombstone survives"),
             Loaded->IsPermaDead(EntityId));
    TestTrue(TEXT("durable learned-dossier retention survives"),
             Loaded->IsRetainedByLearnedDossier(EntityId));
    TestEqual(TEXT("generation serial resumes after the saved value"),
              Loaded->AllocateNameSeedSerial(), 1);

    const FMythicEntityId PostLoadEntityId =
        Loaded->AllocateEntityIdentity(
            88u, EMythicEntityIdentityProvenance::Runtime);
    TestTrue(TEXT("a post-load allocation remains valid"),
             PostLoadEntityId.IsValid());
    TestTrue(TEXT("a post-load allocation cannot alias the restored ID"),
             PostLoadEntityId != EntityId);
    const FMythicPersistentEntityIdentityRecord *PostLoadRecord =
        Loaded->FindIdentityRecord(PostLoadEntityId);
    TestNotNull(TEXT("the post-load allocation is registered"),
                PostLoadRecord);
    if (PostLoadRecord) {
        TestEqual(TEXT("allocation sequence resumes after the saved record"),
                  PostLoadRecord->AllocationSequence, uint64{2});
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPersistentIdentityInvalidAndCollisionTest,
    "Mythic.LivingWorld.Identity.InvalidAndCollisionRejection",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPersistentIdentityInvalidAndCollisionTest::RunTest(
    const FString &Parameters) {
    UMythicPersistentNPCRegistry *Registry =
        NewObject<UMythicPersistentNPCRegistry>();
    AddExpectedError(TEXT("Identity allocation rejected invalid provenance"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("invalid provenance cannot allocate an identity"),
              Registry->AllocateEntityIdentity(
                  12u, EMythicEntityIdentityProvenance::Invalid).IsValid());
    AddExpectedError(
        TEXT("Permanent death rejected an invalid or unregistered canonical entity ID"),
        EAutomationExpectedErrorFlags::Contains, 2);
    TestFalse(TEXT("unregistered IDs cannot create death tombstones"),
              Registry->RegisterDeath(
                  FMythicEntityId::FromAuthorityGuid(
                      EMythicEntityDomain::LivingWorld, FGuid::NewGuid()),
                  FMythicFactionId(), FGameplayTag(), FMythicCellCoord(),
                  0.0, nullptr));
    TestFalse(TEXT("foreign-domain IDs cannot enter the LivingWorld registry"),
              Registry->RegisterDeath(
                  FMythicEntityId::FromAuthorityGuid(
                      EMythicEntityDomain::PlayerCharacter,
                      FGuid::NewGuid()),
                  FMythicFactionId(), FGameplayTag(), FMythicCellCoord(),
                  0.0, nullptr));

    const FMythicEntityId DuplicateId = FMythicEntityId::FromAuthorityGuid(
        EMythicEntityDomain::LivingWorld, FGuid::NewGuid());
    TArray<uint8> MalformedBytes;
    {
        FMemoryWriter Writer(MalformedBytes);
        int32 Version = 2;
        uint32 NextNameSeedSerial = 0;
        uint64 NextIdentitySequence = 3;
        int32 IdentityCount = 2;
        Writer << Version;
        Writer << NextNameSeedSerial;
        Writer << NextIdentitySequence;
        Writer << IdentityCount;
        for (uint64 Sequence = 1; Sequence <= 2; ++Sequence) {
            MythicPersistentIdentityRegistryTests::WriteEntityId(
                Writer, DuplicateId);
            uint32 NameSeed = static_cast<uint32>(Sequence);
            uint8 Provenance = static_cast<uint8>(
                EMythicEntityIdentityProvenance::Runtime);
            Writer << NameSeed;
            Writer << Provenance;
            Writer << Sequence;
            uint8 Retention = 0;
            Writer << Retention;
        }
    }

    {
        FMemoryReader Reader(MalformedBytes);
        Registry->Serialize(Reader);
        TestTrue(TEXT("duplicate canonical IDs poison the load archive"),
                 Reader.IsError());
    }
    TestEqual(TEXT("failed collision load leaves no partial records"),
              Registry->GetIdentityCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPersistentIdentityDurableReferenceRetirementTest,
    "Mythic.LivingWorld.Identity.DurableReferenceRetirement",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPersistentIdentityDurableReferenceRetirementTest::RunTest(
    const FString &Parameters) {
    UMythicPersistentNPCRegistry *Registry =
        NewObject<UMythicPersistentNPCRegistry>();
    const FMythicEntityId FirstLeader = Registry->AllocateEntityIdentity(
        101u, EMythicEntityIdentityProvenance::SettlementPopulation);
    const FMythicEntityId Successor = Registry->AllocateEntityIdentity(
        202u, EMythicEntityIdentityProvenance::SettlementPopulation);

    UMythicFactionDatabaseSettings *Settings =
        NewObject<UMythicFactionDatabaseSettings>();
    Settings->MaxFactions = 2;
    Settings->InitialFactions.AddDefaulted();
    UMythicFactionDatabase *FactionDatabase =
        NewObject<UMythicFactionDatabase>();
    FactionDatabase->Initialize(Settings);
    FMythicFactionId Faction;
    Faction.Index = 0;

    FactionDatabase->ReportLeaderCandidate(Faction, FirstLeader, 0.5f);
    TestTrue(TEXT("the active leader is a durable identity reference"),
             FactionDatabase->ReferencesEntityIdentity(FirstLeader));
    TestFalse(TEXT("durably referenced entities cannot be logically destroyed"),
              UMythicLivingWorldSubsystem::AuthorizesLogicalEntityDestruction(
                  EMythicEntityRetirementResult::RetainedByDurableReference));

    FactionDatabase->ReportLeaderCandidate(Faction, Successor, 0.75f);
    TestFalse(TEXT("succession releases the previous leadership reference"),
              FactionDatabase->ReferencesEntityIdentity(FirstLeader));
    TestTrue(TEXT("succession transfers durable ownership to the new leader"),
             FactionDatabase->ReferencesEntityIdentity(Successor));

    const FMythicEntityId WrongDomainCandidate =
        FMythicEntityId::FromAuthorityGuid(
            EMythicEntityDomain::PlayerCharacter, FGuid::NewGuid());
    FactionDatabase->ReportLeaderCandidate(Faction, WrongDomainCandidate, 1.0f);
    TestFalse(TEXT("non-LivingWorld IDs cannot become faction leaders"),
              FactionDatabase->ReferencesEntityIdentity(
                  WrongDomainCandidate));
    TestTrue(TEXT("a rejected foreign-domain candidate cannot displace the leader"),
             FactionDatabase->ReferencesEntityIdentity(Successor));

    TSet<FMythicEntityId> RestorableLogicalPeople;
    RestorableLogicalPeople.Add(Successor);
    TestEqual(TEXT("restore preserves a leader whose logical entity is reconstructible"),
              FactionDatabase->ClearUnrestorableLeaderReferences(
                  RestorableLogicalPeople),
              0);

    TSet<FMythicEntityId> NoRestorableLogicalPeople;
    TestEqual(TEXT("restore clears a leader that has no reconstructible logical entity"),
              FactionDatabase->ClearUnrestorableLeaderReferences(
                  NoRestorableLogicalPeople),
              1);
    TestFalse(TEXT("clearing an unrestorable leader releases its durable reference"),
              FactionDatabase->ReferencesEntityIdentity(Successor));

    FactionDatabase->ReportLeaderCandidate(Faction, Successor, 0.75f);
    TestTrue(TEXT("permanent death clears the active leadership reference"),
             FactionDatabase->HandlePermanentEntityDeath(Successor));
    TestFalse(TEXT("a dead leader no longer blocks succession or retirement"),
              FactionDatabase->ReferencesEntityIdentity(Successor));

    TestTrue(TEXT("learned dossiers establish offline-safe sticky ownership"),
             Registry->MarkRetainedByLearnedDossier(FirstLeader));
    TestTrue(TEXT("sticky dossier ownership remains queryable without a connected player"),
             Registry->IsRetainedByLearnedDossier(FirstLeader));
    TestTrue(TEXT("tombstones retain records but authorize logical cleanup"),
             UMythicLivingWorldSubsystem::AuthorizesLogicalEntityDestruction(
                 EMythicEntityRetirementResult::Tombstoned));
    return true;
}

#endif
