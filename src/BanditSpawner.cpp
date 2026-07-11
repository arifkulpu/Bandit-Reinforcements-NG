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
    static std::vector<RE::TESBoundObject*> cacheAnimal;
    static std::vector<RE::TESBoundObject*> cacheFalmer;
    static std::vector<RE::TESBoundObject*> cacheDwemer;
    static std::vector<RE::TESBoundObject*> cacheUnknown;

    switch (faction) {
        case FactionType::Bandit:   return cacheBandit;
        case FactionType::Vampire:  return cacheVampire;
        case FactionType::Warlock:  return cacheWarlock;
        case FactionType::Forsworn: return cacheForsworn;
        case FactionType::Draugr:   return cacheDraugr;
        case FactionType::Animal:   return cacheAnimal;
        case FactionType::Falmer:   return cacheFalmer;
        case FactionType::Dwemer:   return cacheDwemer;
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
                else if (faction == FactionType::Animal && (fID == 0x0001CB62 || fID == 0x00028FDF)) isCorrectFaction = true; // Predator/Spider
                else if (faction == FactionType::Falmer && fID == 0x0002446A) isCorrectFaction = true; // FalmerFaction
                else if (faction == FactionType::Dwemer && fID == 0x0001BCC1) isCorrectFaction = true; // AutomatonFaction
            }
            return false;
        });

        // Fallback: Eğer faction'u kesin bilemezsek ama NPC ise ve düşmansa/ölüyse kabul et!
        if (!isCorrectFaction) {
            auto kwdNPC = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00013794); // ActorTypeNPC
            bool isNPC = (kwdNPC && actor->HasKeyword(kwdNPC));
            if (faction == FactionType::Draugr || faction == FactionType::Animal || faction == FactionType::Dwemer) {
                isNPC = true; // Bunlar Creature'dır
            }

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
    if (!player) return 0;

    int pLevel = player->GetLevel();
    if (pLevel < 10) return 0; // Lvl 1-9 hic spawn yok

    int maxSpawns = Settings::MaxSpawnsLevel10_24;
    if (pLevel >= 50)      maxSpawns = Settings::MaxSpawnsLevel50Plus;
    else if (pLevel >= 25) maxSpawns = Settings::MaxSpawnsLevel25_49;

    if (maxSpawns < 1) maxSpawns = 1;
    std::uniform_int_distribution<> dist(1, maxSpawns);
    return dist(GetRNG());
}

struct BossSpawnInfo {
    int guaranteed; // Garanti boss sayisi
    int extraChance; // Ek boss icin yuzde sans (0-100)
};

BossSpawnInfo GetBossSpawnInfo(int pLevel) {
    if (pLevel < 10)   return {0, 0};
    if (pLevel >= 50)  return {2, Settings::BossChanceLevel50Plus};  // 2 garanti + %40 sans
    if (pLevel >= 25)  return {1, Settings::BossChanceLevel25_49};   // 1 garanti + %30 sans
    return                     {0, Settings::BossChanceLevel10_24};  // 0 garanti + %20 sans
}

// ── Map faction → leveled list EditorID ──────────────────────────
const char* BanditSpawner::GetLeveledListEditorID(FactionType faction, bool isBoss) {
    switch (faction) {
        case FactionType::Bandit:   return isBoss ? "LCharBanditBoss" : "LCharBanditAny";
        case FactionType::Vampire:  return isBoss ? "LCharVampireBoss" : "LCharVampire";
        case FactionType::Warlock:  return isBoss ? "LCharWarlockBoss" : "LCharWarlock";
        case FactionType::Forsworn: return isBoss ? "LCharForswornBoss" : "LCharForsworn";
        case FactionType::Draugr:   return isBoss ? "LCharDraugrBoss" : "LCharDraugrMeleeAny";
        case FactionType::Animal:   return "LCharAnimalPredator"; // Default generic
        case FactionType::Falmer:   return isBoss ? "LCharFalmerBoss" : "LCharFalmerAny";
        case FactionType::Dwemer:   return "LCharDwarvenAutomatonMelee";
        default:                    return "LCharBanditAny";
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
        case FactionType::Animal:   formIdStr = "0x0001007E"; break; // LCharAnimalPredator
        case FactionType::Falmer:   formIdStr = isBoss ? "0x000130FA" : "0x000130F9"; break; // Generic Falmer
        case FactionType::Dwemer:   formIdStr = "0x000130F7"; break; // Generic Automaton
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
    
    // YENI FRAKSIYONLAR
    auto kwdAnimal = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeAnimalDen");
    auto kwdFalmer = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeFalmerHive");
    auto kwdDwemer = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDwarvenRuin");

    // FormID fallbacks
    if (!kwdBandit)   kwdBandit   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000171DF);
    if (!kwdVampire)  kwdVampire  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DB);
    if (!kwdWarlock)  kwdWarlock  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130DE);
    if (!kwdForsworn) kwdForsworn = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D5);
    if (!kwdDraugr)   kwdDraugr   = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D2);
    if (!kwdDungeon)  kwdDungeon  = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0E);
    
    if (!kwdAnimal) kwdAnimal = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0C);
    if (!kwdFalmer) kwdFalmer = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D4);
    if (!kwdDwemer) kwdDwemer = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000130D3);

    if (Settings::EnableLogging) {
        SKSE::log::info("  GetFactionFromLocation: loc=0x{:08X}", loc->GetFormID());
        // loglamayı kısaltalım ki kalabalık yapmasın
    }

    if (kwdBandit   && loc->HasKeyword(kwdBandit)   && Settings::EnableBandits)   return FactionType::Bandit;
    if (kwdForsworn && loc->HasKeyword(kwdForsworn) && Settings::EnableForsworn)  return FactionType::Forsworn;
    if (kwdVampire  && loc->HasKeyword(kwdVampire)  && Settings::EnableVampires)  return FactionType::Vampire;
    if (kwdWarlock  && loc->HasKeyword(kwdWarlock)  && Settings::EnableWarlocks)  return FactionType::Warlock;
    if (kwdDraugr   && loc->HasKeyword(kwdDraugr)   && Settings::EnableDraugr)    return FactionType::Draugr;
    if (kwdAnimal   && loc->HasKeyword(kwdAnimal)   && Settings::EnableAnimals)   return FactionType::Animal;
    if (kwdFalmer   && loc->HasKeyword(kwdFalmer)   && Settings::EnableFalmer)    return FactionType::Falmer;
    if (kwdDwemer   && loc->HasKeyword(kwdDwemer)   && Settings::EnableDwemer)    return FactionType::Dwemer;
    if (kwdDungeon  && loc->HasKeyword(kwdDungeon)  && Settings::EnableBandits)   return FactionType::Bandit;

    return FactionType::Unknown;
}

// ── Post-spawn AI fix (called on game thread via TaskInterface) ───
// ── Post-spawn AI fix + pozisyon taşıma ─────────────────────────
// PlaceObjectAtMe sonrası 3D yüklenene kadar bekle, sonra konuma taşı ve AI başlat.
void BanditSpawner::FixActorAI(RE::ObjectRefHandle handle, RE::NiPoint3 targetPos, int retries) {
    auto taskInterface = SKSE::GetTaskInterface();
    if (!taskInterface) return;

    taskInterface->AddTask([handle, targetPos, retries]() {
        auto ref = handle.get();
        if (!ref) return;

        auto actor = ref->As<RE::Actor>();
        if (!actor || actor->IsDead()) return;

        // 3D yuklenmesini bekle (max 60 deneme = ~1 saniye)
        if (!actor->Is3DLoaded()) {
            if (retries < 60) {
                BanditSpawner::FixActorAI(handle, targetPos, retries + 1);
                return;
            }
            if (Settings::EnableLogging) {
                SKSE::log::warn("  FixActorAI: 3D not loaded after 60 retries for 0x{:08X}, proceeding anyway",
                                actor->GetFormID());
            }
        }

        // 3D yuklendi (veya timeout) - simdi dogru konuma tas
        actor->SetPosition(targetPos, true); // true = updateCharController

        // AI baslat
        actor->MoveToHigh();
        actor->EnableAI(true);
        actor->EvaluatePackage(true, true);
        actor->UpdateCombat();

        if (Settings::EnableLogging) {
            SKSE::log::info("  FixActorAI: Actor 0x{:08X} positioned & AI initialized (retries={})",
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

    // Anchor uzerinde spawn et (hicbir SetPosition yok - 3D bozulmasin)
    auto spawned = anchor->PlaceObjectAtMe(baseObj, false);
    if (!spawned) {
        SKSE::log::error("  SPAWN FAILED: PlaceObjectAtMe returned null");
        return RE::ObjectRefHandle();
    }

    // Hedef pozisyon hesapla: anchor + XY offset, anchor'in Z'si
    RE::NiPoint3 targetPos = anchor->GetPosition();
    targetPos.x += offsetX;
    targetPos.y += offsetY;
    // Z anchor'dan alinir - yere gommez

    auto handle = spawned->GetHandle();

    if (Settings::EnableLogging) {
        SKSE::log::info("  SPAWNED: '{}' (boss={}) offset=({:.0f},{:.0f}) formID=0x{:08X}",
                        listEditorID, isBoss, offsetX, offsetY, spawned->GetFormID());
    }

    // 3D yuklenince konuma tas + AI baslat
    BanditSpawner::FixActorAI(handle, targetPos);

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

    // Boss spawn kontrolü: garantili + ekstra şans
    int pLevel = player->GetLevel();
    auto bossInfo = GetBossSpawnInfo(pLevel);
    int totalBossCount = bossInfo.guaranteed;
    if (bossInfo.extraChance > 0) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= bossInfo.extraChance) {
            totalBossCount++;
        }
    }
    if (totalBossCount > 0) {
        SKSE::log::info("  >> BOSS SPAWN: {} boss (guaranteed={}, extra_chance={}%, level={})",
            totalBossCount, bossInfo.guaranteed, bossInfo.extraChance, pLevel);
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
                    else if (faction == FactionType::Animal && (fID == 0x0001CB62 || fID == 0x00028FDF)) isCorrectFaction = true;
                    else if (faction == FactionType::Falmer && fID == 0x0002446A) isCorrectFaction = true;
                    else if (faction == FactionType::Dwemer && fID == 0x0001BCC1) isCorrectFaction = true;
                }
                return false;
            });
            
            if (!isCorrectFaction) {
                auto kwdNPC = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00013794);
                bool isNPC = (kwdNPC && actor->HasKeyword(kwdNPC));
                if (faction == FactionType::Draugr || faction == FactionType::Animal || faction == FactionType::Dwemer) {
                    isNPC = true;
                }

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
        bool isBoss = (i < totalBossCount);

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

    int pLevel = player->GetLevel();
    if (pLevel < 10) {
        SKSE::log::info("SpawnAmbush: SKIPPED (player level < 10)");
        return result;
    }

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

    // Boss kontrolü
    // Boss spawn kontrolü: garantili + ekstra şans
    auto bossInfo = GetBossSpawnInfo(pLevel);
    int totalBossCount = bossInfo.guaranteed;
    if (bossInfo.extraChance > 0) {
        std::uniform_int_distribution<> bossDist(1, 100);
        if (bossDist(rng) <= bossInfo.extraChance) {
            totalBossCount++;
        }
    }

    // Mesafeyi pusu tipine göre seç
    float ambushDist = isOutdoorCamp ? Settings::AmbushCampSpawnDistance : Settings::AmbushSpawnDistance;

    SKSE::log::info("=== SpawnAmbush ===");
    SKSE::log::info("  AMBUSH! count={}, faction={}, roll={}%, type={}, dist={:.0f}",
                   count, static_cast<int>(faction), roll, isOutdoorCamp ? "CAMP" : "DUNGEON", ambushDist);
    if (totalBossCount > 0) {
        SKSE::log::info("  >> BOSS: {} adet (guaranteed={}, extra_chance={}%, level={})",
            totalBossCount, bossInfo.guaranteed, bossInfo.extraChance, pLevel);
    }

    float playerAngle = player->GetAngleZ();

    for (int i = 0; i < count; ++i) {
        bool isBoss = (i < totalBossCount);

        // Oyuncunun önünde 120 derecelik bir yarım dairede yayıl
        std::uniform_real_distribution<float> arcDist(-1.05f, 1.05f);  // ±60 derece
        
        // Zindan çıkışları için çok daha yakın spawnla
        float minR = isOutdoorCamp ? ambushDist * 0.7f : 150.0f;
        float maxR = isOutdoorCamp ? ambushDist * 1.3f : 300.0f;
        
        std::uniform_real_distribution<float> radiusDist(minR, maxR);
        
        float angle = playerAngle + arcDist(rng);
        float radius = radiusDist(rng);
        float offsetX = radius * std::cos(angle);
        float offsetY = radius * std::sin(angle);

        auto handle = SpawnSingleActor(player, faction, isBoss, offsetX, offsetY);
        if (handle) {
            result.spawnedActors.push_back(handle);
        }
    }

    result.count = static_cast<int>(result.spawnedActors.size());
    SKSE::log::info("=== SpawnAmbush DONE: {}/{} spawned ===", result.count, count);

    return result;
}
