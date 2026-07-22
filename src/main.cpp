#include "Settings.h"
#include <SKSE/SKSE.h>

#include "LocationEventSink.h"

extern "C" void __std_regex_transform_primary_char() {}

#include "BanditSpawner.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Tüm ESM/ESP yüklendikten sonra:
        // 1. Leveled list debug dump
        BanditSpawner::DumpLeveledLists();
        // 2. INI'den önceki oturumlarda keşfedilen NPC'leri cache'e yükle.
        //    Böylece ilk ziyaretten önce bile cache hazır olur.
        BanditSpawner::LoadCachedFormIDs();
    }
    else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        // Oyuncu kaydettiğinde: yeni keşfedilen NPC'leri INI'ye yaz (merge).
        // Bu işlem sadece kayıt sırasında çalışır, gameplay'i etkilemez.
        BanditSpawner::SaveCachedFormIDs();
    }
}


#include <thread>
#include <chrono>


#include "SpawnTracker.h"

void InitPeriodicSpawner() {
    std::thread([]() {
        RE::FormID lastCellID = 0;
        int exteriorTimer = 0;
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto task = SKSE::GetTaskInterface();
            if (task) {
                task->AddTask([&lastCellID, &exteriorTimer]() {
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (!player) return;
                    auto cell = player->GetParentCell();
                    if (!cell) return;
                    
                    RE::FormID currentCellID = cell->GetFormID();
                    bool isInterior = cell->IsInteriorCell();
                    
                    // Mesafe bazlı savaş durdurma kontrolü (saniyede 1 kez)
                    SpawnTracker::GetSingleton()->CheckAndStopCombatByDistance(player);
                    
                    // Hücre değişimi algılandı
                    if (currentCellID != lastCellID) {
                        lastCellID = currentCellID;
                        exteriorTimer = 0; // Zamanlayıcıyı sıfırla
                        
                        // İsimsiz kamp pusu kontrolü
                        auto sink = LocationEventSink::GetSingleton();
                        if (sink && sink->GetCurrentUnnamedFaction() != FactionType::Unknown) {
                            auto ambushFaction = sink->GetCurrentUnnamedFaction();
                            sink->SetCurrentUnnamedFaction(FactionType::Unknown);
                            SKSE::log::info(">> UNNAMED CAMP LEAVE DETECTED! (faction={})", static_cast<int>(ambushFaction));
                            BanditSpawner::SpawnAmbush(ambushFaction, true);
                        }
                        
                        // İÇ MEKAN ve İSİMSİZ DIŞ MEKAN KAMPLARI İÇİN SPAWN
                        if (SpawnTracker::GetSingleton()->IsLocationReady(currentCellID)) {
                            FactionType cellFaction = FactionType::Unknown;
                            RE::BGSLocation* loc = cell->GetLocation();
                            if (loc) {
                                cellFaction = BanditSpawner::GetFactionFromLocation(loc);
                            }
                            
                            // Eğer dış mekansak ve mekanın kendi factionı varsa (isimlendirilmiş lokasyon), 
                            // ActorLocationChangeEvent zaten hallediyor. Burada çift spawn olmasın diye atlıyoruz.
                            if (!isInterior && cellFaction != FactionType::Unknown) {
                                // atla
                            } else {
                                // İç mekansak VEYA isimsiz bir dış kamp ise NPC'leri tarayarak faction bulalım
                                if (cellFaction == FactionType::Unknown) {
                                    std::unordered_map<FactionType, int> factionCounts;
                                    for (auto& ref : cell->GetRuntimeData().references) {
                                        if (!ref) continue;
                                        auto actor = ref->As<RE::Actor>();
                                        if (!actor || actor == player || actor->IsDead() || !actor->IsHostileToActor(player)) continue;
                                        
                                        FactionType actFaction = FactionType::Unknown;
                                        actor->VisitFactions([&](RE::TESFaction* f, int8_t) {
                                            if (!f) return false;
                                            RE::FormID fID = f->GetFormID();
                                            if (fID == 0x0001BCC0 && Settings::EnableBandits)   { actFaction = FactionType::Bandit;   return true; }
                                            if (fID == 0x00027242 && Settings::EnableVampires)  { actFaction = FactionType::Vampire;  return true; }
                                            if (fID == 0x00030C66 && Settings::EnableWarlocks)  { actFaction = FactionType::Warlock;  return true; }
                                            if (fID == 0x00043599 && Settings::EnableForsworn)  { actFaction = FactionType::Forsworn; return true; }
                                            if (fID == 0x0002430D && Settings::EnableDraugr)    { actFaction = FactionType::Draugr;   return true; }
                                            if ((fID == 0x0001CB62 || fID == 0x00028FDF || fID == 0x0002C6C8 || fID == 0x00043596 || fID == 0x0004359A || fID == 0x00043598 || fID == 0x00043594 || fID == 0x00043595 || fID == 0x00043597) && Settings::EnableAnimals) { actFaction = FactionType::Animal; return true; }
                                            if (fID == 0x0002446A && Settings::EnableFalmer)    { actFaction = FactionType::Falmer;   return true; }
                                            if (fID == 0x0001BCC1 && Settings::EnableDwemer)    { actFaction = FactionType::Dwemer;   return true; }
                                            return false;
                                        });
                                        if (actFaction != FactionType::Unknown) {
                                            factionCounts[actFaction]++;
                                        }
                                    }
                                    
                                    // Yeterli düşman sayısına ulaşan bir faction bul
                                    int requiredCount = isInterior ? 1 : Settings::MinHostilesForUnnamedCamp;
                                    for (auto& [faction, count] : factionCounts) {
                                        if (count >= requiredCount) {
                                            cellFaction = faction;
                                            break;
                                        }
                                    }
                                }
                                
                                if (cellFaction != FactionType::Unknown) {
                                    SKSE::log::info(">> [PeriodicTask] CELL SPAWN (Interior or Unnamed Camp): Cell=0x{:08X} (faction={})", currentCellID, static_cast<int>(cellFaction));
                                    auto spawnResult = BanditSpawner::SpawnReinforcements(cell, cellFaction);
                                    if (spawnResult.count > 0) {
                                        SpawnTracker::GetSingleton()->RegisterSpawn(currentCellID, spawnResult.spawnedActors);
                                        
                                        if (!isInterior) {
                                            LocationEventSink::GetSingleton()->SetCurrentUnnamedFaction(cellFaction);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // DIŞ MEKAN: Limitsiz sürekli spawn modu (eğer açıksa)
                    if (Settings::ContinuousExteriorSpawns && !isInterior) {
                        exteriorTimer++;
                        float maxTime = Settings::ExteriorSpawnTimer;
                        if (maxTime < 10.0f) maxTime = 10.0f;
                        
                        if (exteriorTimer >= maxTime) {
                            exteriorTimer = 0;
                            // Açık dünyada tarama yapıp klonla
                            BanditSpawner::SpawnReinforcements(cell, FactionType::Unknown);
                        }
                    }
                });
            }
        }
    }).detach();
}

void InitListener() {
    LocationEventSink::GetSingleton()->Register();
    InitPeriodicSpawner();
    SKSE::log::info("Location event listener initialized.");
    
    auto messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener(OnMessage);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    Logger::Setup();
    SKSE::log::info("BanditReinforcementsNG loaded - version 2.0.0");

    Settings::Load();
    SKSE::log::info("Settings loaded from INI.");
    SKSE::log::info("  Logging={}, Bandits={}, Vampires={}, Warlocks={}, Forsworn={}, Draugr={}",
        Settings::EnableLogging, Settings::EnableBandits, Settings::EnableVampires,
        Settings::EnableWarlocks, Settings::EnableForsworn, Settings::EnableDraugr);
    SKSE::log::info("  Animals={}, Falmer={}, Dwemer={}",
        Settings::EnableAnimals, Settings::EnableFalmer, Settings::EnableDwemer);
    SKSE::log::info("  PatrolDist={:.0f}, AmbushDist={:.0f}, AmbushChance={}%",
        Settings::PatrolSpawnDistance, Settings::AmbushSpawnDistance, Settings::AmbushChance);
    SKSE::log::info("  Boss Chances: L10-24={}%, L25-49={}%, L50+={}%",
        Settings::BossChanceLevel10_24, Settings::BossChanceLevel25_49, Settings::BossChanceLevel50Plus);
    SKSE::log::info("  ContinuousExteriorSpawns={}, ExteriorSpawnTimer={:.0f}s",
        Settings::ContinuousExteriorSpawns, Settings::ExteriorSpawnTimer);

    // TaskInterface kaydını doğrula
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        SKSE::log::info("SKSE TaskInterface: OK");
    } else {
        SKSE::log::error("SKSE TaskInterface: NOT AVAILABLE! AI fix will be skipped.");
    }

    InitListener();

    return true;
}








