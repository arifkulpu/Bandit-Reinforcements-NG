#include "LocationEventSink.h"
#include "BanditSpawner.h"
#include "SpawnTracker.h"
#include "Settings.h"
#include <SKSE/SKSE.h>
#include <chrono>

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

        // Cooldown kontrolü: Aynı zindandan 30 saniye içinde tekrar pusu çıkmasın
        auto now = std::chrono::steady_clock::now();
        if (m_lastDungeonAmbushCellID == m_lastInteriorCellID) {
            float elapsed = std::chrono::duration<float>(now - m_lastDungeonAmbushTime).count();
            if (elapsed < kAmbushCooldownSec) {
                SKSE::log::info("  Dungeon ambush COOLDOWN ({:.1f}s < {:.0f}s), skipping.", elapsed, kAmbushCooldownSec);
                m_wasInInterior = isInterior;
                m_lastInteriorCellID = isInterior ? currentCellID : 0;
                SpawnTracker::GetSingleton()->Update();
                return RE::BSEventNotifyControl::kContinue;
            }
        }

        auto prevCell = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevCell) {
            RE::BGSLocation* prevLoc = prevCell->GetLocation();
            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                if (faction != FactionType::Unknown) {
                    BanditSpawner::UpdateCache(prevCell, faction);

                    SKSE::log::info("  Zindan pususu tetikleniyor (faction={})", static_cast<int>(faction));

                    // Cooldown zamanını güncelle
                    m_lastDungeonAmbushCellID = m_lastInteriorCellID;
                    m_lastDungeonAmbushTime = now;

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
            
            // Cooldown kontrolü: Aynı kamptan 30 saniye içinde tekrar pusu çıkmasın
            auto now = std::chrono::steady_clock::now();
            RE::FormID oldLocID = oldLoc->GetFormID();
            bool cooldownOk = true;
            auto it = m_lastAmbushTime.find(oldLocID);
            if (it != m_lastAmbushTime.end()) {
                float elapsed = std::chrono::duration<float>(now - it->second).count();
                if (elapsed < kAmbushCooldownSec) {
                    SKSE::log::info("  Camp ambush COOLDOWN ({:.1f}s < {:.0f}s), skipping.", elapsed, kAmbushCooldownSec);
                    cooldownOk = false;
                }
            }

            if (cooldownOk) {
                m_lastAmbushTime[oldLocID] = now;

                if (player && player->GetParentCell()) {
                    BanditSpawner::UpdateCache(player->GetParentCell(), oldFaction);
                }

                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    taskInterface->AddTask([oldFaction]() {
                        auto p = RE::PlayerCharacter::GetSingleton();
                        if (!p) return;

                        auto ambushResult = BanditSpawner::SpawnAmbush(oldFaction, true);
                    });
                }
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









