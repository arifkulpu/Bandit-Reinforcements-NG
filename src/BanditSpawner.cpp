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

    if (maxSpawns < 1) maxSpawns = 1;
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

// ── Fallback FormIDs for vanilla leveled lists ───────────────────
// Used when EditorID lookup fails (more reliable across SE/AE/VR)
static RE::TESForm* LookupLeveledList(const char* editorID, FactionType faction, bool isBoss) {
    auto form = RE::TESForm::LookupByEditorID(editorID);
    if (form) return form;

    // Fallback to vanilla Skyrim.esm FormIDs
    RE::FormID fallbackID = 0;
    switch (faction) {
        case FactionType::Bandit:
            fallbackID = isBoss ? 0x0003DF03 : 0x001068FF; // LCharBanditBoss / LCharBandit (LCharBanditMelee)
            break;
        case FactionType::Vampire:
            fallbackID = isBoss ? 0x0009B2C0 : 0x00014B91; // LCharVampireBoss / LCharVampire
            break;
        case FactionType::Warlock:
            fallbackID = isBoss ? 0x0003DF05 : 0x0010EF01; // LCharWarlockBoss / LCharWarlock
            break;
        case FactionType::Forsworn:
            fallbackID = isBoss ? 0x0009B2C6 : 0x00046090; // LCharForswornBoss / LCharForsworn
            break;
        case FactionType::Draugr:
            fallbackID = isBoss ? 0x0003DF01 : 0x0001397E; // LCharDraugrBoss / LCharDraugr (LCharDraugrMelee)
            break;
        default:
            fallbackID = isBoss ? 0x0003DF03 : 0x001068FF;
            break;
    }

    if (fallbackID != 0) {
        form = RE::TESForm::LookupByID(fallbackID);
        if (form && Settings::EnableLogging) {
            SKSE::log::info("  EditorID '{}' not found, using fallback FormID 0x{:08X}", editorID, fallbackID);
        }
    }
    return form;
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
    auto kwdDungeon  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");

    // Fallback: hardcoded FormIDs from Skyrim.esm
    if (!kwdBandit)   kwdBandit   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000171DF);
    if (!kwdVampire)  kwdVampire  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DB);
    if (!kwdWarlock)  kwdWarlock  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DE);
    if (!kwdForsworn) kwdForsworn = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D5);
    if (!kwdDraugr)   kwdDraugr   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D2);
    if (!kwdDungeon)  kwdDungeon  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0E);

    if (kwdBandit   && loc->HasKeyword(kwdBandit)   && Settings::EnableBandits)   return FactionType::Bandit;
    if (kwdForsworn && loc->HasKeyword(kwdForsworn) && Settings::EnableForsworn)  return FactionType::Forsworn;
    if (kwdVampire  && loc->HasKeyword(kwdVampire)  && Settings::EnableVampires)  return FactionType::Vampire;
    if (kwdWarlock  && loc->HasKeyword(kwdWarlock)  && Settings::EnableWarlocks)  return FactionType::Warlock;
    if (kwdDraugr   && loc->HasKeyword(kwdDraugr)   && Settings::EnableDraugr)    return FactionType::Draugr;

    // If it has a dungeon keyword but no specific faction, default to Bandit
    if (kwdDungeon && loc->HasKeyword(kwdDungeon) && Settings::EnableBandits) return FactionType::Bandit;

    return FactionType::Unknown;
}

// ── Helper: Spawn a single actor ─────────────────────────────────
static RE::ObjectRefHandle SpawnSingleActor(RE::TESObjectREFR* anchor, FactionType faction, bool isBoss,
                                              float offsetX, float offsetY) {
    const char* listEditorID = BanditSpawner::GetLeveledListEditorID(faction, isBoss);
    auto listForm = LookupLeveledList(listEditorID, faction, isBoss);

    if (!listForm) {
        if (Settings::EnableLogging) {
            SKSE::log::warn("  FAILED: Could not find leveled list for faction={} boss={}", 
                           static_cast<int>(faction), isBoss);
        }
        return RE::ObjectRefHandle();
    }

    auto baseObj = listForm->As<RE::TESBoundObject>();
    if (!baseObj) {
        if (Settings::EnableLogging) {
            SKSE::log::warn("  FAILED: Form is not TESBoundObject");
        }
        return RE::ObjectRefHandle();
    }

    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (spawned) {
        RE::NiPoint3 pos = anchor->GetPosition();
        pos.x += offsetX;
        pos.y += offsetY;
        spawned->SetPosition(pos);

        if (Settings::EnableLogging) {
            SKSE::log::info("  OK: Spawned '{}' (boss={}) at offset ({:.0f}, {:.0f})", 
                           listEditorID, isBoss, offsetX, offsetY);
        }
        return spawned->GetHandle();
    }

    if (Settings::EnableLogging) {
        SKSE::log::warn("  FAILED: PlaceObjectAtMe returned null");
    }
    return RE::ObjectRefHandle();
}

// ── Main spawn: Reinforcements ───────────────────────────────────
SpawnResult BanditSpawner::SpawnReinforcements(RE::TESObjectCELL* cell, FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !cell) return result;

    int count = GetSpawnCount();
    bool isInterior = cell->IsInteriorCell();

    if (Settings::EnableLogging) {
        SKSE::log::info("=== SpawnReinforcements ===");
        SKSE::log::info("  Count={}, Faction={}, Interior={}, Cell=0x{:08X}", 
                        count, static_cast<int>(faction), isInterior, cell->GetFormID());
    }

    auto& rng = GetRNG();

    // ── KEY FIX: Interior vs Exterior spawn distance ──
    // Interior (cave/dungeon): spawn close (200-500 units) so they stay inside the geometry
    // Exterior (camp): spawn far (patrol distance) so they walk towards the player
    float minDist, maxDist;
    if (isInterior) {
        minDist = 200.0f;
        maxDist = 500.0f;
    } else {
        float base = Settings::PatrolSpawnDistance;
        minDist = base * 0.7f;
        maxDist = base * 1.3f;
    }

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
        bool isBoss = (shouldSpawnBoss && i == 0);

        if (isBoss && Settings::EnableLogging) {
            SKSE::log::info("  >> BOSS SPAWN triggered!");
        }

        // Spawn in a ring around the player
        std::uniform_real_distribution<float> angleDist(0.0f, static_cast<float>(2.0 * M_PI));
        std::uniform_real_distribution<float> radiusDist(minDist, maxDist);
        float angle = angleDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, faction, isBoss, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());

    if (Settings::EnableLogging) {
        SKSE::log::info("=== SpawnReinforcements DONE: {}/{} spawned ===", result.count, count);
    }

    return result;
}

// ── Ambush spawn (dungeon exit) ──────────────────────────────────
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    auto& rng = GetRNG();

    // Roll ambush chance
    std::uniform_int_distribution<> chanceDist(1, 100);
    int roll = chanceDist(rng);
    if (roll > Settings::AmbushChance) {
        if (Settings::EnableLogging) {
            SKSE::log::info("SpawnAmbush: Chance FAILED (rolled {} > {}%)", roll, Settings::AmbushChance);
        }
        return result;
    }

    int minC = Settings::AmbushMinCount;
    int maxC = Settings::AmbushMaxCount;
    if (minC < 1) minC = 1;
    if (maxC < minC) maxC = minC;
    std::uniform_int_distribution<> countDist(minC, maxC);
    int count = countDist(rng);

    if (Settings::EnableLogging) {
        SKSE::log::info("=== SpawnAmbush ===");
        SKSE::log::info("  AMBUSH TRIGGERED! {} enemies, faction={}", count, static_cast<int>(faction));
    }

    // Ambush spawns closer, in a semicircle ahead of the player
    float ambushDist = Settings::AmbushSpawnDistance;
    float playerAngle = player->GetAngleZ();

    for (int i = 0; i < count; ++i) {
        std::uniform_real_distribution<float> arcDist(-1.05f, 1.05f);
        std::uniform_real_distribution<float> radiusDist(ambushDist * 0.7f, ambushDist * 1.3f);
        float angle = playerAngle + arcDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, faction, false, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());

    if (Settings::EnableLogging) {
        SKSE::log::info("=== SpawnAmbush DONE: {}/{} spawned ===", result.count, count);
    }

    return result;
}
