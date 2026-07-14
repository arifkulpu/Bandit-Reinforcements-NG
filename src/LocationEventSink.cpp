#include "LocationEventSink.h"
#include "BanditSpawner.h"
#include "SpawnTracker.h"
#include "Settings.h"
#include <SKSE/SKSE.h>

LocationEventSink* LocationEventSink::GetSingleton() {
    static LocationEventSink singleton;
    return &singleton;
}

void LocationEventSink::Register() {
    auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (scriptEventSource) {
        scriptEventSource->AddEventSink<RE::TESCellAttachDetachEvent>(GetSingleton());
        scriptEventSource->AddEventSink<RE::TESActorLocationChangeEvent>(GetSingleton());
        SKSE::log::info("LocationEventSink registered (CellAttach & ActorLocationChange).");
    } else {
        SKSE::log::error("LocationEventSink: ScriptEventSourceHolder not found!");
    }
}

// ── 1. CELL ATTACH EVENT (Dungeon Exit → Ambush) ──────────────────
// Zindan (iç mekan) çıkışını yakalar. Bu durum doğrudan TESActorLocationChangeEvent
// ile yakalanmaz çünkü zindanların location'ı bazen iç/dış aynı olabiliyor.
RE::BSEventNotifyControl LocationEventSink::ProcessEvent(
    const RE::TESCellAttachDetachEvent* a_event,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) 
{
    if (!a_event || !a_event->reference || !a_event->attached) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || a_event->reference.get() != player) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto cell = player->GetParentCell();
    if (!cell) return RE::BSEventNotifyControl::kContinue;

    RE::FormID currentCellID = cell->GetFormID();
    if (currentCellID == m_lastProcessedCellID) {
        return RE::BSEventNotifyControl::kContinue;
    }
    m_lastProcessedCellID = currentCellID;

    bool isInterior = cell->IsInteriorCell();

    // ── PUSU: İç mekan (Zindan) → Dış mekan geçişi ──
    if (m_wasInInterior && !isInterior && m_lastInteriorCellID != 0) {
        SKSE::log::info(">> DUNGEON EXIT! PrevInteriorCell=0x{:08X}", m_lastInteriorCellID);
        auto prevCell = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevCell) {
            RE::BGSLocation* prevLoc = prevCell->GetLocation();
            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                if (faction != FactionType::Unknown) {
                    BanditSpawner::UpdateCache(prevCell, faction);

                    SKSE::log::info("  Zindan pususu tetikleniyor (faction={})", static_cast<int>(faction));

                    auto taskInterface = SKSE::GetTaskInterface();
                    if (taskInterface) {
                        taskInterface->AddTask([currentCellID, faction]() {
                            auto ambushResult = BanditSpawner::SpawnAmbush(faction, false); // zindan cikisi
                            if (ambushResult.count > 0) {
                                SpawnTracker::GetSingleton()->RegisterSpawn(currentCellID, ambushResult.spawnedActors);
                            }
                        });
                    }
                }
            }
        }
    }

    m_wasInInterior = isInterior;
    m_lastInteriorCellID = isInterior ? currentCellID : 0;
    SpawnTracker::GetSingleton()->Update();

    // ── İSİMSİZ DIŞ MEKAN KAMPI TESPİTİ ──────────────────────────────
    // ActorLocationChangeEvent isimsiz kamplar icin tetiklenmez.
    // Dış mekan hücresine girince hücreyi tara; eğer düşman NPC varsa spawn tetikle.
    if (!isInterior) {
        FactionType detectedFaction = FactionType::Unknown;
        int hostileCount = 0;

        for (auto& ref : cell->GetRuntimeData().references) {
            if (!ref) continue;
            auto actor = ref->As<RE::Actor>();
            if (!actor || actor == player || actor->IsDead()) continue;
            if (!actor->IsHostileToActor(player)) continue;

            // Bu hücre isimsiz bir kamp — faction'ı NPC'den tahmin et
            actor->VisitFactions([&](RE::TESFaction* f, int8_t) {
                if (!f) return false;
                RE::FormID fID = f->GetFormID();
                if (fID == 0x0001BCC0 && Settings::EnableBandits)   { detectedFaction = FactionType::Bandit;   return true; }
                if (fID == 0x00027242 && Settings::EnableVampires)  { detectedFaction = FactionType::Vampire;  return true; }
                if (fID == 0x00030C66 && Settings::EnableWarlocks)  { detectedFaction = FactionType::Warlock;  return true; }
                if (fID == 0x00043599 && Settings::EnableForsworn)  { detectedFaction = FactionType::Forsworn; return true; }
                if (fID == 0x0002430D && Settings::EnableDraugr)    { detectedFaction = FactionType::Draugr;   return true; }
                if ((fID == 0x0001CB62 || fID == 0x00028FDF || fID == 0x0002C6C8 || fID == 0x00043596 || fID == 0x0004359A || fID == 0x00043598 || fID == 0x00043594 || fID == 0x00043595 || fID == 0x00043597) && Settings::EnableAnimals) { detectedFaction = FactionType::Animal; return true; }
                if (fID == 0x0002446A && Settings::EnableFalmer)    { detectedFaction = FactionType::Falmer;   return true; }
                if (fID == 0x0001BCC1 && Settings::EnableDwemer)    { detectedFaction = FactionType::Dwemer;   return true; }
                return false;
            });

            if (detectedFaction != FactionType::Unknown) {
                hostileCount++;
                break; // En az 1 düşman yeterli
            }
        }

        if (detectedFaction != FactionType::Unknown && hostileCount >= 1) {
            m_currentUnnamedFaction = detectedFaction;

            if (currentCellID != m_lastExteriorSpawnCellID && SpawnTracker::GetSingleton()->IsLocationReady(currentCellID)) {
                m_lastExteriorSpawnCellID = currentCellID;
                SKSE::log::info(">> UNNAMED CAMP DETECTED in cell 0x{:08X} (faction={}, hostiles={})",
                               currentCellID, static_cast<int>(detectedFaction), hostileCount);

                BanditSpawner::UpdateCache(cell, detectedFaction);

                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    taskInterface->AddTask([currentCellID, detectedFaction]() {
                        auto p = RE::PlayerCharacter::GetSingleton();
                        if (!p) return;
                        auto currentCell2 = p->GetParentCell();
                        if (!currentCell2) return;

                        auto spawnResult = BanditSpawner::SpawnReinforcements(currentCell2, detectedFaction);
                        if (spawnResult.count > 0) {
                            SpawnTracker::GetSingleton()->RegisterSpawn(currentCellID, spawnResult.spawnedActors);
                        }
                    });
                }
            }
        } else {
            // Hücrede düşman yok. İsmi olmayan bir kamptan mı çıktık?
            if (m_currentUnnamedFaction != FactionType::Unknown) {
                SKSE::log::info(">> UNNAMED CAMP LEAVE DETECTED! (faction={})", static_cast<int>(m_currentUnnamedFaction));
                
                auto ambushFaction = m_currentUnnamedFaction;
                m_currentUnnamedFaction = FactionType::Unknown;

                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    taskInterface->AddTask([ambushFaction]() {
                        auto p = RE::PlayerCharacter::GetSingleton();
                        if (!p) return;

                        auto ambushResult = BanditSpawner::SpawnAmbush(ambushFaction, true); // kamp terkinde uzak mesafe
                        if (ambushResult.count > 0) {
                            auto trackID = p->GetParentCell() ? p->GetParentCell()->GetFormID() : 0;
                            if (trackID != 0) {
                                SpawnTracker::GetSingleton()->RegisterSpawn(trackID, ambushResult.spawnedActors);
                            }
                        }
                    });
                }
            }
        }
    }

    // ── MEKAN İÇİ HÜCRE DEĞİŞİMİ: Başarısız spawnları tekrar dene ──
    // Eğer oyuncu bir "Location" içine girmiş ancak ilk girilen hücrede
    // (örn: dış kapı) uygun NPC bulunamadığı için spawn iptal edilmişse,
    // oyuncu mekanın başka bir hücresine (örn: içeriye) geçtiğinde tekrar deneriz.
    RE::BGSLocation* currentLoc = cell->GetLocation();
    if (currentLoc) {
        auto faction = BanditSpawner::GetFactionFromLocation(currentLoc);
        if (faction != FactionType::Unknown) {
            RE::FormID locID = currentLoc->GetFormID();
            
            if (SpawnTracker::GetSingleton()->MarkLocationPending(locID)) {
                SKSE::log::info(">> CELL ATTACH SPAWN RETRY: Loc 0x{:08X} (faction={})", locID, static_cast<int>(faction));
                
                BanditSpawner::UpdateCache(cell, faction);

                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    taskInterface->AddTask([locID, faction]() {
                        auto p = RE::PlayerCharacter::GetSingleton();
                        if (!p || !p->GetParentCell()) {
                            SpawnTracker::GetSingleton()->ClearPendingLocation(locID);
                            return;
                        }
                        
                        auto spawnResult = BanditSpawner::SpawnReinforcements(p->GetParentCell(), faction);
                        if (spawnResult.count > 0) {
                            SpawnTracker::GetSingleton()->RegisterSpawn(locID, spawnResult.spawnedActors);
                        } else {
                            // Başarısız oldu (örn: template bulunamadı). Pending durumunu sil ki
                            // bir sonraki hücrede (örneğin mağaraya girince) tekrar denesin.
                            SpawnTracker::GetSingleton()->ClearPendingLocation(locID);
                        }
                    });
                } else {
                    SpawnTracker::GetSingleton()->ClearPendingLocation(locID);
                }
            }
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

// ── 2. ACTOR LOCATION CHANGE EVENT (Kamp Girişi + Açık Dünya Pususu) ──
// Bu olay hem zindan hem dış mekan için tetiklenir:
//   - newLoc = düşman yeri  → Takviye (Reinforcement) spawn
//   - oldLoc = düşman yeri + newLoc farklı/null → Açık dünya pususu (kamp terkinde)
RE::BSEventNotifyControl LocationEventSink::ProcessEvent(
    const RE::TESActorLocationChangeEvent* a_event,
    RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) 
{
    if (!a_event || !a_event->actor) return RE::BSEventNotifyControl::kContinue;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (a_event->actor.get() != player) return RE::BSEventNotifyControl::kContinue;

    RE::BGSLocation* oldLoc = a_event->oldLoc;
    RE::BGSLocation* newLoc = a_event->newLoc;

    // ── AÇIK DÜNYA PUSUSU: Düşman kampını terk ediyoruz ──
    // Kamp içindeyken (oldLoc=düşman yeri) dışarı çıkınca (newLoc farklı) pusu tetiklenir.
    if (oldLoc) {
        auto oldFaction = BanditSpawner::GetFactionFromLocation(oldLoc);
        bool leavingHostileArea = (oldFaction != FactionType::Unknown);
        bool enteringDifferentArea = (!newLoc || newLoc != oldLoc);

        if (leavingHostileArea && enteringDifferentArea) {
            SKSE::log::info(">> CAMP LEAVE DETECTED! OldLoc=0x{:08X} (faction={})", 
                           oldLoc->GetFormID(), static_cast<int>(oldFaction));
            
            if (player && player->GetParentCell()) {
                BanditSpawner::UpdateCache(player->GetParentCell(), oldFaction);
            }

            auto taskInterface = SKSE::GetTaskInterface();
            if (taskInterface) {
                taskInterface->AddTask([oldFaction]() {
                    auto p = RE::PlayerCharacter::GetSingleton();
                    if (!p) return;

                    auto ambushResult = BanditSpawner::SpawnAmbush(oldFaction, true); // kamp terkinde uzak mesafe
                    if (ambushResult.count > 0) {
                        auto trackID = p->GetParentCell() ? p->GetParentCell()->GetFormID() : 0;
                        if (trackID != 0) {
                            SpawnTracker::GetSingleton()->RegisterSpawn(trackID, ambushResult.spawnedActors);
                        }
                    }
                });
            }
        }
    }

    // ── TAKVİYE: Düşman alanına giriyoruz ──
    if (!newLoc) {
        return RE::BSEventNotifyControl::kContinue;
    }

    SKSE::log::info("=== ActorLocationChange: NewLoc=0x{:08X} ===", newLoc->GetFormID());
    
    // We entered a named location, so we are no longer in an unnamed camp
    m_currentUnnamedFaction = FactionType::Unknown;

    auto faction = BanditSpawner::GetFactionFromLocation(newLoc);
    if (faction == FactionType::Unknown) {
        SKSE::log::info("  Location is not a hostile area, skipping reinforcements.");
        return RE::BSEventNotifyControl::kContinue;
    }

    if (player && player->GetParentCell()) {
        BanditSpawner::UpdateCache(player->GetParentCell(), faction);
    }

    RE::FormID trackID = newLoc->GetFormID();

    if (SpawnTracker::GetSingleton()->MarkLocationPending(trackID)) {
        SKSE::log::info("  Location PENDING -> spawning reinforcements (faction={})", static_cast<int>(faction));
        
        auto taskInterface = SKSE::GetTaskInterface();
        if (taskInterface) {
            taskInterface->AddTask([trackID, faction]() {
                auto p = RE::PlayerCharacter::GetSingleton();
                if (!p) {
                    SpawnTracker::GetSingleton()->ClearPendingLocation(trackID);
                    return;
                }
                
                auto currentCell = p->GetParentCell();
                if (!currentCell) {
                    SKSE::log::warn("  Task: Player still has no parent cell. Aborting spawn.");
                    SpawnTracker::GetSingleton()->ClearPendingLocation(trackID);
                    return;
                }

                auto spawnResult = BanditSpawner::SpawnReinforcements(currentCell, faction);
                if (spawnResult.count > 0) {
                    SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
                } else {
                    // Başarısız oldu. Pending'i sil, böylece mekanın diğer hücrelerinde tekrar dener.
                    SpawnTracker::GetSingleton()->ClearPendingLocation(trackID);
                }
            });
        }
    } else {
        SKSE::log::info("  Location in COOLDOWN, ACTIVE, or PENDING, skipping.");
    }

    return RE::BSEventNotifyControl::kContinue;
}
