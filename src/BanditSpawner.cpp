#include "BanditSpawner.h"
#include "Settings.h"
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ── Thread-local RNG ──────────────────────────────────────────────
std::mt19937& BanditSpawner::GetRNG() {
    static thread_local std::mt19937 rng{ std::random_device{}() };
    return rng;
}

// ── Spawn count based on player level ────────────────────────────
int BanditSpawner::GetSpawnCount() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return 1;

    int pLevel = player->GetLevel();
    int maxSpawns = Settings::MaxSpawnsLevel1_10;
    if (pLevel >= 25)      maxSpawns = Settings::MaxSpawnsLevel25Plus;
    else if (pLevel >= 10) maxSpawns = Settings::MaxSpawnsLevel10_25;

    std::uniform_int_distribution<> dist(1, maxSpawns);
    return dist(GetRNG());
}

// ── Map faction → leveled list EditorID ──────────────────────────
const char* BanditSpawner::GetLeveledListEditorID(FactionType faction, bool isBoss) {
    switch (faction) {
        case FactionType::Bandit:
            return isBoss ? "LCharBanditBoss" : "LCharBandit";
        case FactionType::Vampire:
            return isBoss ? "LCharVampireBoss" : "LCharVampire";
        case FactionType::Warlock:
            return isBoss ? "LCharWarlockBoss" : "LCharWarlock";
        case FactionType::Forsworn:
            return isBoss ? "LCharForswornBoss" : "LCharForsworn";
        case FactionType::Draugr:
            return isBoss ? "LCharDraugrBoss" : "LCharDraugr";
        default:
            return isBoss ? "LCharBanditBoss" : "LCharBandit";
    }
}

// ── Determine faction from location keywords ─────────────────────
FactionType BanditSpawner::GetFactionFromLocation(RE::BGSLocation* loc) {
    if (!loc) return FactionType::Unknown;

    // Look up keywords by EditorID (safe across SE/AE/VR)
    auto kwdBandit   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeBanditCamp");
    auto kwdVampire  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeVampireLair");
    auto kwdWarlock  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeWarlockLair");
    auto kwdForsworn = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeForswornCamp");
    auto kwdDraugr   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDraugrCrypt");

    // Also check dungeon keyword as fallback
    auto kwdDungeon  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");

    if (kwdBandit   && loc->HasKeyword(kwdBandit)   && Settings::EnableBandits)   return FactionType::Bandit;
    if (kwdForsworn && loc->HasKeyword(kwdForsworn) && Settings::EnableForsworn)  return FactionType::Forsworn;
    if (kwdVampire  && loc->HasKeyword(kwdVampire)  && Settings::EnableVampires)  return FactionType::Vampire;
    if (kwdWarlock  && loc->HasKeyword(kwdWarlock)  && Settings::EnableWarlocks)  return FactionType::Warlock;
    if (kwdDraugr   && loc->HasKeyword(kwdDraugr)   && Settings::EnableDraugr)    return FactionType::Draugr;

    // If it has a dungeon keyword but no specific faction, default to Bandit if enabled
    if (kwdDungeon && loc->HasKeyword(kwdDungeon) && Settings::EnableBandits) return FactionType::Bandit;

    return FactionType::Unknown;
}

// ── Helper: Spawn a single actor from a leveled list ─────────────
static RE::ObjectRefHandle SpawnSingleActor(RE::TESObjectREFR* anchor, const char* listEditorID, 
                                              float offsetX, float offsetY) {
    auto listForm = RE::TESForm::LookupByEditorID(listEditorID);
    if (!listForm) {
        // Fallback: try LCharBandit
        listForm = RE::TESForm::LookupByEditorID("LCharBandit");
    }
    if (!listForm) return RE::ObjectRefHandle();

    auto baseObj = listForm->As<RE::TESBoundObject>();
    if (!baseObj) return RE::ObjectRefHandle();

    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (spawned) {
        RE::NiPoint3 pos = anchor->GetPosition();
        pos.x += offsetX;
        pos.y += offsetY;
        spawned->SetPosition(pos);

        if (Settings::EnableLogging) {
            SKSE::log::info("  Spawned actor from '{}' at offset ({:.0f}, {:.0f})", listEditorID, offsetX, offsetY);
        }
        return spawned->GetHandle();
    }
    return RE::ObjectRefHandle();
}

// ── Main spawn: Reinforcements (patrol-style offset) ─────────────
SpawnResult BanditSpawner::SpawnReinforcements(RE::TESObjectCELL* cell, FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !cell) return result;

    int count = GetSpawnCount();
    const char* listID = GetLeveledListEditorID(faction, false);

    if (Settings::EnableLogging) {
        SKSE::log::info("SpawnReinforcements: {} enemies, faction={}, cell=0x{:08X}", 
                        count, static_cast<int>(faction), cell->GetFormID());
    }

    auto& rng = GetRNG();
    float spawnDist = Settings::PatrolSpawnDistance;

    // Check boss spawn chance
    bool shouldSpawnBoss = false;
    int pLevel = player->GetLevel();
    if (pLevel >= Settings::BossMinPlayerLevel) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= Settings::BossSpawnChance) {
            shouldSpawnBoss = true;
        }
    }

    for (int i = 0; i < count; ++i) {
        // Determine which list to use (boss for the first one if boss roll succeeded)
        const char* currentList = listID;
        if (shouldSpawnBoss && i == 0) {
            currentList = GetLeveledListEditorID(faction, true);
            if (Settings::EnableLogging) {
                SKSE::log::info("  >> BOSS spawning! Using '{}'", currentList);
            }
        }

        // Patrol-style: spawn in a ring around the player at spawnDist
        std::uniform_real_distribution<float> angleDist(0.0f, static_cast<float>(2.0 * M_PI));
        std::uniform_real_distribution<float> radiusDist(spawnDist * 0.7f, spawnDist * 1.3f);
        float angle = angleDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, currentList, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());

    if (Settings::EnableLogging) {
        SKSE::log::info("SpawnReinforcements: Successfully spawned {}/{} actors", result.count, count);
    }

    return result;
}

// ── Ambush spawn (dungeon exit pusu) ─────────────────────────────
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    auto& rng = GetRNG();

    // Roll ambush chance
    std::uniform_int_distribution<> chanceDist(1, 100);
    if (chanceDist(rng) > Settings::AmbushChance) {
        if (Settings::EnableLogging) {
            SKSE::log::info("SpawnAmbush: Chance roll failed (rolled > {}%)", Settings::AmbushChance);
        }
        return result; // No ambush this time
    }

    std::uniform_int_distribution<> countDist(Settings::AmbushMinCount, Settings::AmbushMaxCount);
    int count = countDist(rng);

    const char* listID = GetLeveledListEditorID(faction, false);

    if (Settings::EnableLogging) {
        SKSE::log::info("SpawnAmbush: AMBUSH! {} enemies, faction={}", count, static_cast<int>(faction));
    }

    // Ambush spawns closer than patrol (half distance, in a semicircle ahead)
    float ambushDist = Settings::PatrolSpawnDistance * 0.5f;
    float playerAngle = player->GetAngleZ(); // Player's facing direction

    for (int i = 0; i < count; ++i) {
        // Spread in a 120-degree arc ahead of the player
        std::uniform_real_distribution<float> arcDist(-1.05f, 1.05f); // ~60 degrees each side
        std::uniform_real_distribution<float> radiusDist(ambushDist * 0.6f, ambushDist * 1.2f);
        float angle = playerAngle + arcDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, listID, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());

    if (Settings::EnableLogging) {
        SKSE::log::info("SpawnAmbush: Successfully spawned {}/{} ambush actors", result.count, count);
    }

    return result;
}
