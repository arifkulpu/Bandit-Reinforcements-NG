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

// ── 1. CELL ATTACH EVENT (For Ambush on Dungeon Exit) ──
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

    // ── AMBUSH CHECK: Interior → Exterior transition ──
    if (m_wasInInterior && !isInterior && m_lastInteriorCellID != 0) {
        SKSE::log::info(">> DUNGEON EXIT DETECTED! PrevInteriorCell=0x{:08X}", m_lastInteriorCellID);
        auto prevCell = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevCell) {
            RE::BGSLocation* prevLoc = prevCell->GetLocation();
            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                if (faction != FactionType::Unknown) {
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

    // Trigger cleanup
    SpawnTracker::GetSingleton()->Update();

    return RE::BSEventNotifyControl::kContinue;
}

// ── 2. ACTOR LOCATION CHANGE EVENT (For Camp Reinforcements) ──
RE::BSEventNotifyControl LocationEventSink::ProcessEvent(
    const RE::TESActorLocationChangeEvent* a_event,
    RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) 
{
    if (!a_event || !a_event->actor) return RE::BSEventNotifyControl::kContinue;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (a_event->actor.get() != player) return RE::BSEventNotifyControl::kContinue;

    RE::BGSLocation* newLoc = a_event->newLoc;
    
    // YENİ KONUM YOKSA ATLA (örn. açık dünya hücreleri bazen locationsuzdur)
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
        SKSE::log::info("  Location READY → spawning reinforcements (faction={})", static_cast<int>(faction));
        
        auto taskInterface = SKSE::GetTaskInterface();
        if (taskInterface) {
            taskInterface->AddTask([trackID, faction]() {
                auto player = RE::PlayerCharacter::GetSingleton();
                if (!player) return;
                
                auto currentCell = player->GetParentCell();
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
