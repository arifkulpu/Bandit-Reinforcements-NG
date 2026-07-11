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
                    SKSE::log::info("  Zindan pususu tetikleniyor (faction={})", static_cast<int>(faction));
                    auto ambushResult = BanditSpawner::SpawnAmbush(faction);
                    if (ambushResult.count > 0) {
                        SpawnTracker::GetSingleton()->RegisterSpawn(currentCellID, ambushResult.spawnedActors);
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
            
            auto ambushResult = BanditSpawner::SpawnAmbush(oldFaction);
            if (ambushResult.count > 0) {
                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    auto handles = ambushResult.spawnedActors;
                    taskInterface->AddTask([handles]() {
                        auto p = RE::PlayerCharacter::GetSingleton();
                        if (!p) return;
                        auto trackID = p->GetParentCell() ? p->GetParentCell()->GetFormID() : 0;
                        if (trackID != 0) {
                            SpawnTracker::GetSingleton()->RegisterSpawn(trackID, handles);
                        }
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

    auto faction = BanditSpawner::GetFactionFromLocation(newLoc);
    if (faction == FactionType::Unknown) {
        SKSE::log::info("  Location is not a hostile area, skipping reinforcements.");
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::FormID trackID = newLoc->GetFormID();

    if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
        SKSE::log::info("  Location READY -> spawning reinforcements (faction={})", static_cast<int>(faction));
        
        auto taskInterface = SKSE::GetTaskInterface();
        if (taskInterface) {
            taskInterface->AddTask([trackID, faction]() {
                auto p = RE::PlayerCharacter::GetSingleton();
                if (!p) return;
                
                auto currentCell = p->GetParentCell();
                if (!currentCell) {
                    SKSE::log::warn("  Task: Player still has no parent cell. Aborting spawn.");
                    return;
                }

                auto spawnResult = BanditSpawner::SpawnReinforcements(currentCell, faction);
                if (spawnResult.count > 0) {
                    SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
                }
            });
        }
    } else {
        SKSE::log::info("  Location in COOLDOWN or already spawned, skipping.");
    }

    return RE::BSEventNotifyControl::kContinue;
}
