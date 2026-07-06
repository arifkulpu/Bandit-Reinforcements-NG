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
        SKSE::log::info("LocationEventSink registered.");
    }
}

RE::BSEventNotifyControl LocationEventSink::ProcessEvent(
    const RE::TESCellAttachDetachEvent* a_event,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) 
{
    if (!a_event || !a_event->reference || !a_event->attached) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Only process player events
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || a_event->reference.get() != player) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto cell = player->GetParentCell();
    if (!cell) return RE::BSEventNotifyControl::kContinue;

    bool isInterior = cell->IsInteriorCell();

    // ── AMBUSH CHECK: Interior → Exterior transition (dungeon exit) ──
    if (m_wasInInterior && !isInterior && m_lastInteriorCellID != 0) {
        // Player just exited an interior cell (dungeon exit)
        // Check if the previous interior had a hostile faction location
        auto prevForm = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevForm) {
            auto prevLoc = prevForm->GetLocation();
            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                if (faction != FactionType::Unknown) {
                    if (Settings::EnableLogging) {
                        SKSE::log::info("Dungeon exit detected! Checking ambush for faction {}...", 
                                        static_cast<int>(faction));
                    }
                    // Ambush has its own internal chance roll
                    auto ambushResult = BanditSpawner::SpawnAmbush(faction);
                    if (ambushResult.count > 0) {
                        // Register ambush spawns under the exterior cell for tracking
                        RE::FormID trackID = cell->GetFormID();
                        SpawnTracker::GetSingleton()->RegisterSpawn(trackID, ambushResult.spawnedActors);
                    }
                }
            }
        }
    }

    // Update interior tracking state
    m_wasInInterior = isInterior;
    m_lastInteriorCellID = isInterior ? cell->GetFormID() : 0;

    // ── Lazy Update: clean up expired/cleared spawns ──
    SpawnTracker::GetSingleton()->Update();

    // ── REINFORCEMENT CHECK: Entering a hostile location ──
    auto loc = cell->GetLocation();
    if (!loc) return RE::BSEventNotifyControl::kContinue;

    auto faction = BanditSpawner::GetFactionFromLocation(loc);
    if (faction == FactionType::Unknown) return RE::BSEventNotifyControl::kContinue;

    RE::FormID trackID = loc->GetFormID();

    if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
        auto spawnResult = BanditSpawner::SpawnReinforcements(cell, faction);
        if (spawnResult.count > 0) {
            SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}
