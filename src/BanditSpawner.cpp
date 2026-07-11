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

void BanditSpawner::DumpLeveledLists() {
    if (!Settings::EnableLogging) return;
    
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    SKSE::log::info("=== DUMPING ALL RELEVANT LEVELED LISTS ===");
    for (auto form : dataHandler->GetFormArray<RE::TESLevCharacter>()) {
        if (!form) continue;
        const char* eid = form->GetFormEditorID();
        if (!eid) continue;
        
        std::string editorID(eid);
        // Sadece Bandit, Vampire, Warlock, Forsworn, Draugr kelimelerini içerenleri logla
        if (editorID.find("Bandit") != std::string::npos ||
            editorID.find("Vampire") != std::string::npos ||
            editorID.find("Warlock") != std::string::npos ||
            editorID.find("Forsworn") != std::string::npos ||
            editorID.find("Draugr") != std::string::npos) 
        {
            SKSE::log::info("  Found LvlList: '{}' -> FormID: 0x{:08X}", editorID, form->GetFormID());
        }
    }
    SKSE::log::info("=== DUMP FINISHED ===");
}

// ── Dynamic Fallback Cache ────────────────────────────────────────
std::vector<RE::TESBoundObject*>& BanditSpawner::GetCacheForFaction(FactionType faction) {
    static std::vector<RE::TESBoundObject*> cacheBandit;
    static std::vector<RE::TESBoundObject*> cacheVampire;
    static std::vector<RE::TESBoundObject*> cacheWarlock;
    static std::vector<RE::TESBoundObject*> cacheForsworn;
    static std::vector<RE::TESBoundObject*> cacheDraugr;
    static std::vector<RE::TESBoundObject*> cacheUnknown;

    switch (faction) {
        case FactionType::Bandit:   return cacheBandit;
        case FactionType::Vampire:  return cacheVampire;
        case FactionType::Warlock:  return cacheWarlock;
        case FactionType::Forsworn: return cacheForsworn;
        case FactionType::Draugr:   return cacheDraugr;
        default:                    return cacheUnknown;
    }
}

void BanditSpawner::UpdateCache(RE::TESObjectCELL* cell, FactionType faction) {
    if (!cell || faction == FactionType::Unknown) return;

    auto& cache = GetCacheForFaction(faction);
    
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    if (Settings::EnableLogging) {
        SKSE::log::info("  UpdateCache: Scanning {} references in cell 0x{:08X}", cell->GetRuntimeData().references.size(), cell->GetFormID());
    }

    // Sadece uygun keyword/faction'a sahip npc'leri cache'e ekle
    bool added = false;
    for (auto& ref : cell->GetRuntimeData().references) {
        if (!ref) continue;
        auto actor = ref->As<RE::Actor>();
        if (!actor || actor == player) continue;
        
        auto baseObj = actor->GetActorBase();
        if (!baseObj) continue;
        
        // Faction kontrolü yapalım (Örn: BanditFaction = 0x1BCC0, VampireFaction = 0x27242)
        bool isCorrectFaction = false;
        actor->VisitFactions([&](RE::TESFaction* f, int8_t rank) {
            if (f) {
                RE::FormID fID = f->GetFormID();
                if (faction == FactionType::Bandit && fID == 0x0001BCC0) isCorrectFaction = true;
                else if (faction == FactionType::Vampire && fID == 0x00027242) isCorrectFaction = true;
                else if (faction == FactionType::Forsworn && fID == 0x00043599) isCorrectFaction = true;
                else if (faction == FactionType::Warlock && fID == 0x00030C66) isCorrectFaction = true; // WarlockFaction
                else if (faction == FactionType::Draugr && fID == 0x0002430D) isCorrectFaction = true; // DraugrFaction
            }
            return false;
        });

        // Fallback: Eğer faction'u kesin bilemezsek ama NPC ise ve düşmansa/ölüyse kabul et!
        if (!isCorrectFaction) {
            auto kwdNPC = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00013794); // ActorTypeNPC
            bool isNPC = (kwdNPC && actor->HasKeyword(kwdNPC));
            if (faction == FactionType::Draugr) isNPC = true; // Draugr'lar NPC degildir (Creature)

            if (isNPC && (actor->IsHostileToActor(player) || actor->IsDead())) {
                isCorrectFaction = true;
            }
        }

        if (Settings::EnableLogging) {
            SKSE::log::info("    - Found Actor: '{}' (BaseID: 0x{:08X}), isCorrectFaction={}, Dead={}", 
                baseObj->GetName(), baseObj->GetFormID(), isCorrectFaction, actor->IsDead());
        }

        if (isCorrectFaction) {
            // Düşman factionına aitse cache'e ekle
            bool exists = false;
            for (auto* cachedBase : cache) {
                if (cachedBase == baseObj) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                cache.push_back(baseObj);
                added = true;
                if (Settings::EnableLogging) {
                    SKSE::log::info("      -> CACHED!");
                }
                if (cache.size() > 50) { // Keep bounded
                    cache.erase(cache.begin());
                }
            }
        }
    }
    
    if (added && Settings::EnableLogging) {
        SKSE::log::info("  UpdateCache: Faction {} cache size is now {}", static_cast<int>(faction), cache.size());
    }
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
            return isBoss ? "LCharBanditBoss" : "LCharBanditAny";
        case FactionType::Vampire:
            return isBoss ? "LCharVampireBoss" : "LCharVampire";
        case FactionType::Warlock:
            return isBoss ? "LCharWarlockBoss" : "LCharWarlockAny";
        case FactionType::Forsworn:
            return isBoss ? "LCharForswornBoss" : "LCharForswornAny";
        case FactionType::Draugr:
            return isBoss ? "LCharDraugrBoss" : "LCharDraugrAny";
        default:
            return isBoss ? "LCharBanditBoss" : "LCharBanditAny";
    }
}

// ── Fallback FormIDs for vanilla leveled lists ───────────────────
static RE::TESBoundObject* LookupLeveledList(const char* editorID, FactionType faction, bool isBoss) {
    auto form = RE::TESForm::LookupByEditorID(editorID);
    if (form) {
        if (Settings::EnableLogging)
            SKSE::log::info("  LeveledList OK by EditorID: '{}'", editorID);
        return form->As<RE::TESBoundObject>();
    }

    // Fallback to INI FormIDs
    std::string formIdStr = "";
    switch (faction) {
        case FactionType::Bandit:   formIdStr = isBoss ? Settings::FormID_BanditBoss : Settings::FormID_Bandit; break;
        case FactionType::Vampire:  formIdStr = isBoss ? Settings::FormID_VampireBoss : Settings::FormID_Vampire; break;
        case FactionType::Warlock:  formIdStr = isBoss ? Settings::FormID_WarlockBoss : Settings::FormID_Warlock; break;
        case FactionType::Forsworn: formIdStr = isBoss ? Settings::FormID_ForswornBoss : Settings::FormID_Forsworn; break;
        case FactionType::Draugr:   formIdStr = isBoss ? Settings::FormID_DraugrBoss : Settings::FormID_Draugr; break;
        default:                    formIdStr = isBoss ? Settings::FormID_BanditBoss : Settings::FormID_Bandit; break;
    }

    RE::FormID fallbackID = 0;
    try {
        if (!formIdStr.empty()) {
            fallbackID = std::stoul(formIdStr, nullptr, 16);
        }
    } catch (...) {
        SKSE::log::error("  LeveledList FAILED: Invalid FormID string in INI: '{}'", formIdStr);
    }

    if (fallbackID != 0) {
        form = RE::TESForm::LookupByID(fallbackID);
        if (form) {
            SKSE::log::warn("  LeveledList fallback: EditorID '{}' not found, using INI FormID {}", editorID, formIdStr);
            return form->As<RE::TESBoundObject>();
        } else {
            SKSE::log::error("  LeveledList FAILED: EditorID '{}' AND INI FormID {} both missing!", editorID, formIdStr);
        }
    }
    
    // YENI: Eğer EditorID ve FormID çökerse (Örn: Lawless modu listeleri silmişse), 
    // etraftaki veya daha önce gördüğümüz aynı tip düşmanların BaseObj'lerini kopyala!
    auto& cache = BanditSpawner::GetCacheForFaction(faction);
    
    // Eğer cache boşsa, anlık olarak mevcut hücreyi tekrar tara (çünkü hücre yeni yüklenmiş olabilir)
    if (cache.empty()) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player && player->GetParentCell()) {
            BanditSpawner::UpdateCache(player->GetParentCell(), faction);
        }
    }

    if (!cache.empty()) {
        std::uniform_int_distribution<> dist(0, (int)cache.size() - 1);
        auto* cachedBase = cache[dist(BanditSpawner::GetRNG())];
        if (cachedBase) {
            SKSE::log::info("  DYNAMIC COPY: Using a cached NPC base object for Faction {} (Total cached: {})", static_cast<int>(faction), cache.size());
            return cachedBase;
        }
    }

    SKSE::log::error("  DYNAMIC COPY FAILED: No cached NPCs available for Faction {}. Cannot spawn!", static_cast<int>(faction));
    return nullptr;
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
        if (!actor->Is3DLoaded() && retries < 300) { // wait up to 5 seconds (300 frames)
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
    auto baseObj = LookupLeveledList(listEditorID, faction, isBoss);

    if (!baseObj) {
        SKSE::log::error("  SPAWN FAILED: No leveled list or cached NPC found for faction={} boss={}", 
                        static_cast<int>(faction), isBoss);
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
    if (!player) return result; // Sadece player null ise dön. Cell null olabilir!

    int count = GetSpawnCount();
    bool isInterior = cell ? cell->IsInteriorCell() : false;

    SKSE::log::info("=== SpawnReinforcements ===");
    SKSE::log::info("  Count={}, Faction={}, Interior={}, Cell=0x{:08X}", 
                    count, static_cast<int>(faction), isInterior, cell ? cell->GetFormID() : 0);

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

    // Anchor'lari topla: Hucrede halihazirda var olan hedefe ait faction dusmanlari (olu veya diri)
    std::vector<RE::TESObjectREFR*> anchors;
    if (cell) {
        for (auto& ref : cell->GetRuntimeData().references) {
            if (!ref) continue;
            auto actor = ref->As<RE::Actor>();
            if (!actor || actor == player) continue;
            
            auto baseObj = actor->GetActorBase();
            if (!baseObj) continue;

            bool isCorrectFaction = false;
            actor->VisitFactions([&](RE::TESFaction* f, int8_t rank) {
                if (f) {
                    RE::FormID fID = f->GetFormID();
                    if (faction == FactionType::Bandit && fID == 0x0001BCC0) isCorrectFaction = true;
                    else if (faction == FactionType::Vampire && fID == 0x00027242) isCorrectFaction = true;
                    else if (faction == FactionType::Forsworn && fID == 0x00043599) isCorrectFaction = true;
                    else if (faction == FactionType::Warlock && fID == 0x00030C66) isCorrectFaction = true;
                    else if (faction == FactionType::Draugr && fID == 0x0002430D) isCorrectFaction = true;
                }
                return false;
            });
            
            if (!isCorrectFaction) {
                auto kwdNPC = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00013794);
                bool isNPC = (kwdNPC && actor->HasKeyword(kwdNPC));
                if (faction == FactionType::Draugr) isNPC = true;

                if (isNPC && (actor->IsHostileToActor(player) || actor->IsDead())) {
                    isCorrectFaction = true;
                }
            }

            if (isCorrectFaction) {
                anchors.push_back(actor);
            }
        }
    }

    if (Settings::EnableLogging) {
        SKSE::log::info("  Found {} existing valid anchors in the cell.", anchors.size());
    }

    for (int i = 0; i < count; ++i) {
        bool isBoss = (shouldSpawnBoss && i == 0);

        RE::TESObjectREFR* spawnAnchor = player;
        float currentMinDist = minDist;
        float currentMaxDist = maxDist;

        // Eger hucrede halihazirda ayni tip dusman varsa, onlardan birinin yaninda dogur (cluster effect)
        if (!anchors.empty()) {
            std::uniform_int_distribution<> anchorDist(0, (int)anchors.size() - 1);
            spawnAnchor = anchors[anchorDist(rng)];
            // Düşmanın dibinde doğsun (50 ile 150 birim arası)
            currentMinDist = 50.0f;
            currentMaxDist = 150.0f;
        }

        std::uniform_real_distribution<float> angleDist(0.0f, static_cast<float>(2.0 * M_PI));
        std::uniform_real_distribution<float> radiusDist(currentMinDist, currentMaxDist);
        float angle = angleDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(spawnAnchor, faction, isBoss, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnReinforcements DONE: {}/{} spawned ===", result.count, count);

    return result;
}

// ── Ambush spawn ─────────────────────────────────────────────────
// isOutdoorCamp=false → zindan cikisi pususu (AmbushSpawnDistance ~500 birim, yakin)
// isOutdoorCamp=true  → acik dunya kamp terk pususu (AmbushCampSpawnDistance ~1500 birim, uzak)
SpawnResult BanditSpawner::SpawnAmbush(FactionType faction, bool isOutdoorCamp) {
    SpawnResult result{ {}, 0 };
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return result;

    auto& rng = GetRNG();

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

    // Mesafeyi pusu tipine göre seç
    float ambushDist = isOutdoorCamp ? Settings::AmbushCampSpawnDistance : Settings::AmbushSpawnDistance;

    SKSE::log::info("=== SpawnAmbush ===");
    SKSE::log::info("  AMBUSH! count={}, faction={}, roll={}%, type={}, dist={:.0f}",
                   count, static_cast<int>(faction), roll, isOutdoorCamp ? "CAMP" : "DUNGEON", ambushDist);

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
