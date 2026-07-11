#include "BanditSpawner.h"
#include "Settings.h"
#include <random>
#include <cmath>
#include <SKSE/SKSE.h>

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
static RE::TESForm* LookupLeveledList(const char* editorID, FactionType faction, bool isBoss) {
    auto form = RE::TESForm::LookupByEditorID(editorID);
    if (form) {
        if (Settings::EnableLogging)
            SKSE::log::info("  LeveledList OK by EditorID: '{}'", editorID);
        return form;
    }

    // Fallback to vanilla Skyrim.esm FormIDs
    RE::FormID fallbackID = 0;
    switch (faction) {
        case FactionType::Bandit:
            fallbackID = isBoss ? 0x0003DF03 : 0x001068FF;
            break;
        case FactionType::Vampire:
            fallbackID = isBoss ? 0x0009B2C0 : 0x00014B91;
            break;
        case FactionType::Warlock:
            fallbackID = isBoss ? 0x0003DF05 : 0x0010EF01;
            break;
        case FactionType::Forsworn:
            fallbackID = isBoss ? 0x0009B2C6 : 0x00046090;
            break;
        case FactionType::Draugr:
            fallbackID = isBoss ? 0x0003DF01 : 0x0001397E;
            break;
        default:
            fallbackID = isBoss ? 0x0003DF03 : 0x001068FF;
            break;
    }

    if (fallbackID != 0) {
        form = RE::TESForm::LookupByID(fallbackID);
        if (form) {
            SKSE::log::warn("  LeveledList fallback: EditorID '{}' not found, using FormID 0x{:08X}", editorID, fallbackID);
        } else {
            SKSE::log::error("  LeveledList FAILED: EditorID '{}' AND FormID 0x{:08X} both missing!", editorID, fallbackID);
        }
    }
    return form;
}

// ── Determine faction from location keywords ─────────────────────
FactionType BanditSpawner::GetFactionFromLocation(RE::BGSLocation* loc) {
    if (!loc) return FactionType::Unknown;

    auto kwdBandit   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeBanditCamp");
    auto kwdVampire  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeVampireLair");
    auto kwdWarlock  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeWarlockLair");
    auto kwdForsworn = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeForswornCamp");
    auto kwdDraugr   = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDraugrCrypt");
    auto kwdDungeon  = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");

    // FormID fallbacks
    if (!kwdBandit)   kwdBandit   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000171DF);
    if (!kwdVampire)  kwdVampire  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DB);
    if (!kwdWarlock)  kwdWarlock  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DE);
    if (!kwdForsworn) kwdForsworn = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D5);
    if (!kwdDraugr)   kwdDraugr   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D2);
    if (!kwdDungeon)  kwdDungeon  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0E);

    if (Settings::EnableLogging) {
        SKSE::log::info("  GetFactionFromLocation: loc=0x{:08X}", loc->GetFormID());
        SKSE::log::info("    kwdBandit={}, kwdVampire={}, kwdWarlock={}, kwdForsworn={}, kwdDraugr={}, kwdDungeon={}",
            kwdBandit != nullptr, kwdVampire != nullptr, kwdWarlock != nullptr,
            kwdForsworn != nullptr, kwdDraugr != nullptr, kwdDungeon != nullptr);
        SKSE::log::info("    hasBandit={}, hasVampire={}, hasWarlock={}, hasForsworn={}, hasDraugr={}, hasDungeon={}",
            kwdBandit ? loc->HasKeyword(kwdBandit) : false,
            kwdVampire ? loc->HasKeyword(kwdVampire) : false,
            kwdWarlock ? loc->HasKeyword(kwdWarlock) : false,
            kwdForsworn ? loc->HasKeyword(kwdForsworn) : false,
            kwdDraugr ? loc->HasKeyword(kwdDraugr) : false,
            kwdDungeon ? loc->HasKeyword(kwdDungeon) : false);
    }

    if (kwdBandit   && loc->HasKeyword(kwdBandit)   && Settings::EnableBandits)   return FactionType::Bandit;
    if (kwdForsworn && loc->HasKeyword(kwdForsworn) && Settings::EnableForsworn)  return FactionType::Forsworn;
    if (kwdVampire  && loc->HasKeyword(kwdVampire)  && Settings::EnableVampires)  return FactionType::Vampire;
    if (kwdWarlock  && loc->HasKeyword(kwdWarlock)  && Settings::EnableWarlocks)  return FactionType::Warlock;
    if (kwdDraugr   && loc->HasKeyword(kwdDraugr)   && Settings::EnableDraugr)    return FactionType::Draugr;
    if (kwdDungeon  && loc->HasKeyword(kwdDungeon)  && Settings::EnableBandits)   return FactionType::Bandit;

    return FactionType::Unknown;
}

// ── Post-spawn AI fix (called on game thread via TaskInterface) ───
// Skyrim'de PlaceObjectAtMe sonrası aktör tam yüklenmeden SetPosition/EvaluatePackage
// çağrılırsa animasyon glitch ve T-pose oluşur.
// Bu fonksiyon bir sonraki frame'de game thread'inde çalışarak aktörü düzgün başlatır.
void BanditSpawner::FixActorAI(RE::ObjectRefHandle handle, int retries) {
    auto taskInterface = SKSE::GetTaskInterface();
    if (!taskInterface) return;

    taskInterface->AddTask([handle, retries]() {
        auto ref = handle.get();
        if (!ref) return;

        auto actor = ref->As<RE::Actor>();
        if (!actor || actor->IsDead()) return;

        // Skyrim 3D loading is asynchronous. If we evaluate AI before 3D is ready,
        // the actor might glide without playing animations (T-pose or stuck).
        if (!actor->Is3DLoaded() && retries < 60) { // wait up to 1 second (60 frames)
            BanditSpawner::FixActorAI(handle, retries + 1);
            return;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        // 1. Move to high processing (fully active)
        actor->MoveToHigh();

        // 2. Enable AI explicitly
        actor->EnableAI(true);

        // 3. Reset and evaluate AI package so they don't just stand there
        actor->EvaluatePackage(true, true);

        // 4. Update combat state so they notice the player
        actor->UpdateCombat();

        if (Settings::EnableLogging) {
            SKSE::log::info("  FixActorAI: Actor 0x{:08X} AI initialized (retries={})", 
                            actor->GetFormID(), retries);
        }
    });
}

// ── Helper: Spawn a single actor ─────────────────────────────────
static RE::ObjectRefHandle SpawnSingleActor(RE::TESObjectREFR* anchor, FactionType faction, bool isBoss,
                                              float offsetX, float offsetY) {
    const char* listEditorID = BanditSpawner::GetLeveledListEditorID(faction, isBoss);
    auto listForm = LookupLeveledList(listEditorID, faction, isBoss);

    if (!listForm) {
        SKSE::log::error("  SPAWN FAILED: No leveled list found for faction={} boss={}", 
                        static_cast<int>(faction), isBoss);
        return RE::ObjectRefHandle();
    }

    auto baseObj = listForm->As<RE::TESBoundObject>();
    if (!baseObj) {
        SKSE::log::error("  SPAWN FAILED: Form 0x{:08X} is not TESBoundObject", listForm->GetFormID());
        return RE::ObjectRefHandle();
    }

    // PlaceObjectAtMe: aktörü anchor'ın yanında doğur, OFFSET YOK!
    // Offset'i SetPosition ile değil, MoveTo_Impl'in pos argümanıyla vereceğiz.
    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (!spawned) {
        SKSE::log::error("  SPAWN FAILED: PlaceObjectAtMe returned null");
        return RE::ObjectRefHandle();
    }

    // Aktörü doğru konuma taşı
    RE::NiPoint3 targetPos = anchor->GetPosition();
    targetPos.x += offsetX;
    targetPos.y += offsetY;
    // Z koordinatı: anchor'dan 0 fark (yükseklik sabit)
    spawned->SetPosition(targetPos);

    auto handle = spawned->GetHandle();

    if (Settings::EnableLogging) {
        SKSE::log::info("  SPAWNED: '{}' (boss={}) offset=({:.0f},{:.0f}) formID=0x{:08X}", 
                        listEditorID, isBoss, offsetX, offsetY, spawned->GetFormID());
    }

    // AI fix'i bir sonraki frame'e ertele (animasyon glitch önleme)
    BanditSpawner::FixActorAI(handle);

    return handle;
}

// ── Main spawn: Reinforcements ───────────────────────────────────
SpawnResult BanditSpawner::SpawnReinforcements(RE::TESObjectCELL* cell, FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !cell) return result;

    int count = GetSpawnCount();
    bool isInterior = cell->IsInteriorCell();

    SKSE::log::info("=== SpawnReinforcements ===");
    SKSE::log::info("  Count={}, Faction={}, Interior={}, Cell=0x{:08X}", 
                    count, static_cast<int>(faction), isInterior, cell->GetFormID());

    auto& rng = GetRNG();

    // İç mekan: 200-500 birim (mağara içinde kalır)
    // Dış mekan: PatrolSpawnDistance kadar uzak (devriye etkisi)
    float minDist, maxDist;
    if (isInterior) {
        minDist = 200.0f;
        maxDist = 500.0f;
    } else {
        float base = Settings::PatrolSpawnDistance;
        minDist = base * 0.7f;
        maxDist = base * 1.3f;
    }

    // Boss spawn kontrolü
    bool shouldSpawnBoss = false;
    int pLevel = player->GetLevel();
    if (pLevel >= Settings::BossMinPlayerLevel) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= Settings::BossSpawnChance) {
            shouldSpawnBoss = true;
            SKSE::log::info("  >> BOSS SPAWN triggered! (player level={})", pLevel);
        }
    }

    for (int i = 0; i < count; ++i) {
        bool isBoss = (shouldSpawnBoss && i == 0);

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
    SKSE::log::info("=== SpawnReinforcements DONE: {}/{} spawned ===", result.count, count);

    return result;
}

// ── Ambush spawn (dungeon exit) ──────────────────────────────────
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    auto& rng = GetRNG();

    // Şans kontrolü
    std::uniform_int_distribution<> chanceDist(1, 100);
    int roll = chanceDist(rng);
    if (roll > Settings::AmbushChance) {
        SKSE::log::info("SpawnAmbush: SKIPPED (rolled {}%, threshold {}%)", roll, Settings::AmbushChance);
        return result;
    }

    int minC = (Settings::AmbushMinCount < 1) ? 1 : Settings::AmbushMinCount;
    int maxC = (Settings::AmbushMaxCount < minC) ? minC : Settings::AmbushMaxCount;
    std::uniform_int_distribution<> countDist(minC, maxC);
    int count = countDist(rng);

    SKSE::log::info("=== SpawnAmbush ===");
    SKSE::log::info("  AMBUSH! count={}, faction={}, roll={}%", count, static_cast<int>(faction), roll);

    float ambushDist = Settings::AmbushSpawnDistance;
    float playerAngle = player->GetAngleZ();

    for (int i = 0; i < count; ++i) {
        // Oyuncunun önünde 120 derecelik bir yarım dairede yayıl
        std::uniform_real_distribution<float> arcDist(-1.05f, 1.05f);  // ±60 derece
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
    SKSE::log::info("=== SpawnAmbush DONE: {}/{} spawned ===", result.count, count);

    return result;
}
