#include "Settings.h"
#include <SKSE/SKSE.h>
#include <random>

#include "LocationEventSink.h"
#include "UI.h"

extern "C" void __std_regex_transform_primary_char() {}

#include "BanditSpawner.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Tüm ESM/ESP yüklendikten sonra:
        // 1. Leveled list debug dump
        BanditSpawner::DumpLeveledLists();
        // 2. INI'den önceki oturumlarda keşfedilen normal NPC'leri cache'e yükle.
        BanditSpawner::LoadCachedFormIDs();
        // 3. INI'den önceki oturumlarda keşfedilen ELITE (boss) NPC'leri boss cache'e yükle.
        BanditSpawner::LoadCachedBossFormIDs();
    }
    else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        // Oyuncu kaydettiğinde: yeni keşfedilen NPC'leri INI'ye yaz (merge).
        BanditSpawner::SaveCachedFormIDs();
        // Yeni keşfedilen ELITE NPC'leri [CachedBossNPCs] section'ına yaz.
        BanditSpawner::SaveCachedBossFormIDs();
    }
}


#include <thread>
#include <chrono>


#include "SpawnTracker.h"

void InitPeriodicSpawner() {
    std::thread([]() {
        RE::FormID lastCellID = 0;
        RE::FormID lastLocationID = 0;
        int exteriorTimer = 0;
        int interiorTimer = 0;
        int backupWavesTriggered = 0;
        int currentBackupTarget = 300;
        
        std::mt19937 rng(std::random_device{}());
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto task = SKSE::GetTaskInterface();
            if (task) {
                task->AddTask([&lastCellID, &lastLocationID, &exteriorTimer, &interiorTimer, &backupWavesTriggered, &currentBackupTarget, &rng]() {
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
                        interiorTimer = 0;

                        RE::BGSLocation* loc = cell->GetLocation();
                        RE::FormID currentLocID = loc ? loc->GetFormID() : currentCellID;

                        // Yeni bir zindana/lokasyona girildiyse Destek Kuvvet şansı (%) rulo yap
                        if (isInterior && currentLocID != lastLocationID) {
                            lastLocationID = currentLocID;
                            backupWavesTriggered = 0;

                            std::uniform_int_distribution<int> chanceDist(1, 100);
                            int roll = chanceDist(rng);
                            if (roll > Settings::BackupChance) {
                                backupWavesTriggered = Settings::BackupMaxWaves; // Bu zindanda destek hakkını kapat
                                SKSE::log::info(">> [PeriodicTask] Backup forces SKIPPED for dungeon 0x{:08X} (rolled {}% > threshold {}%)",
                                               currentLocID, roll, Settings::BackupChance);
                            } else {
                                SKSE::log::info(">> [PeriodicTask] Backup forces ENABLED for dungeon 0x{:08X} (rolled {}% <= threshold {}%)",
                                               currentLocID, roll, Settings::BackupChance);
                            }
                        }
                        
                        std::uniform_int_distribution<int> dist(Settings::BackupTimerMin, Settings::BackupTimerMax);
                        currentBackupTarget = dist(rng);
                        
                        // İsimsiz kamp pusu kontrolü: önceki dış mekan hücresinde unnamed faction set edilmişse ambush at
                        auto sink = LocationEventSink::GetSingleton();
                        if (sink && sink->GetCurrentUnnamedFaction() != FactionType::Unknown) {
                            auto ambushFaction = sink->GetCurrentUnnamedFaction();
                            sink->SetCurrentUnnamedFaction(FactionType::Unknown);
                            
                            // Eğer yeni girdiğimiz lokasyon güvenli bir alansa (köy/han vb.) veya cooldown dolmamışsa pusu atma
                            if (!BanditSpawner::IsSafeLocation(loc) && sink->CanTriggerAmbush(currentCellID)) {
                                SKSE::log::info(">> UNNAMED CAMP LEAVE DETECTED! (faction={})", static_cast<int>(ambushFaction));
                                BanditSpawner::SpawnAmbush(ambushFaction, true);
                            } else {
                                SKSE::log::info(">> UNNAMED CAMP LEAVE SKIPPED (safe location or ambush cooldown)");
                            }
                        }

                        // İÇ MEKAN: Hücre bazlı spawn (zindan/mağara)
                        if (isInterior) {
                            FactionType cellFaction = BanditSpawner::DetermineFactionForCell(cell, loc);
                            
                            if (cellFaction != FactionType::Unknown) {
                                // Mağarada yeni bir alt alana/hücreye her girdiğimizde o hücredeki NPC'leri tara ve kopyala!
                                BanditSpawner::UpdateCache(cell, cellFaction);

                                RE::FormID trackID = BanditSpawner::GetTrackingID(cell, loc);
                                if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
                                    SKSE::log::info(">> [PeriodicTask] CELL SPAWN (Interior): TrackID=0x{:08X} Cell=0x{:08X} (faction={})", trackID, currentCellID, static_cast<int>(cellFaction));
                                    auto spawnResult = BanditSpawner::SpawnReinforcements(cell, cellFaction);
                                    if (spawnResult.count > 0) {
                                        SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
                                    }
                                }
                            }
                        }
                        // DIŞ MEKAN: İsimsiz kamp hücre değişiminde takviye spawn'u
                        // TESActorLocationChangeEvent isimsiz bölgelerde güvenilir tetiklenmediğinden,
                        // hücre değişimini yakalayıp burada da spawn yapıyoruz.
                        else {
                            FactionType cellFaction = BanditSpawner::DetermineFactionForCell(cell, loc);

                            if (cellFaction != FactionType::Unknown) {
                                BanditSpawner::UpdateCache(cell, cellFaction);

                                // Eğer isimsiz alan ise unnamed faction'ı set et (çıkışta ambush için)
                                if (!loc && sink) {
                                    sink->SetCurrentUnnamedFaction(cellFaction);
                                    SKSE::log::info(">> [PeriodicTask] Entered UNNAMED exterior hostile area (faction={})", static_cast<int>(cellFaction));
                                }

                                RE::FormID trackID = BanditSpawner::GetTrackingID(cell, loc);
                                if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
                                    SKSE::log::info(">> [PeriodicTask] CELL SPAWN (Exterior): TrackID=0x{:08X} Cell=0x{:08X} (faction={})", trackID, currentCellID, static_cast<int>(cellFaction));
                                    auto spawnResult = BanditSpawner::SpawnReinforcements(cell, cellFaction);
                                    if (spawnResult.count > 0) {
                                        SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
                                    }
                                } else {
                                    SKSE::log::info(">> [PeriodicTask] Exterior location 0x{:08X} in COOLDOWN, skipping.", trackID);
                                }
                            }
                        }
                    }

                    // DESTEK KUVVETLER (BACKUP FORCES) SİSTEMİ
                    if (isInterior && backupWavesTriggered < Settings::BackupMaxWaves && Settings::EnableBackupForces) {
                        interiorTimer++;
                        if (interiorTimer >= currentBackupTarget) {
                            backupWavesTriggered++; 
                            interiorTimer = 0; // Sonraki dalga için sıfırla
                            
                            std::uniform_int_distribution<int> dist(Settings::BackupTimerMin, Settings::BackupTimerMax);
                            currentBackupTarget = dist(rng); // Yeni dalga için rastgele hedef
                            
                            RE::BGSLocation* loc = cell->GetLocation();
                            FactionType backupFaction = BanditSpawner::DetermineFactionForCell(cell, loc);
                            
                            if (backupFaction != FactionType::Unknown) {
                                SKSE::log::info(">> [PeriodicTask] DESTEK KUVVETLER GELDİ! Cell=0x{:08X}, Wave {}/{}", currentCellID, backupWavesTriggered, Settings::BackupMaxWaves);
                                
                                if (Settings::ShowBackupMessage) {
                                    RE::DebugNotification("Backup forces have arrived!");
                                }
                                
                                // Zindan içi pusu mekaniğini kullanarak arkadan spawn at
                                auto spawnResult = BanditSpawner::SpawnAmbush(backupFaction, false);
                                if (spawnResult.count > 0) {
                                    SpawnTracker::GetSingleton()->RegisterSpawn(currentCellID, spawnResult.spawnedActors);
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
                            // Açık dünyada tarama yapıp dominant faction'ı bul ve klonla
                            RE::BGSLocation* loc = cell->GetLocation();
                            FactionType extFaction = BanditSpawner::DetermineFactionForCell(cell, loc);
                            if (extFaction != FactionType::Unknown) {
                                BanditSpawner::SpawnReinforcements(cell, extFaction);
                            }
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
    SKSE::log::info("BanditReinforcementsNG loaded - version 2.1.0");

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
    UI::Register();

    return true;
}








