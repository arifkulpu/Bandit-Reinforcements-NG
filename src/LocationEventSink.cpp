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
    if (!cell) {
        if (Settings::EnableLogging) {
            SKSE::log::warn("ProcessEvent: Player has no parent cell, skipping.");
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    bool isInterior = cell->IsInteriorCell();

    if (Settings::EnableLogging) {
        SKSE::log::info("--- Cell Event: Cell=0x{:08X}, Interior={}, WasInterior={} ---", 
                        cell->GetFormID(), isInterior, m_wasInInterior);
    }

    // ── AMBUSH CHECK: Interior → Exterior transition (dungeon exit) ──
    if (m_wasInInterior && !isInterior && m_lastInteriorCellID != 0) {
        if (Settings::EnableLogging) {
            SKSE::log::info("Dungeon EXIT detected! Previous interior cell: 0x{:08X}", m_lastInteriorCellID);
        }

        auto prevCell = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevCell) {
            auto prevLoc = prevCell->GetLocation();
            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                if (faction != FactionType::Unknown) {
                    if (Settings::EnableLogging) {
                        SKSE::log::info("  Ambush check for faction={}", static_cast<int>(faction));
                    }
                    auto ambushResult = BanditSpawner::SpawnAmbush(faction);
                    if (ambushResult.count > 0) {
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

    // ── REINFORCEMENT CHECK ──
    // Try multiple ways to get the location (robustness)
    RE::BGSLocation* loc = cell->GetLocation();
    
    // Fallback: try player's current location if cell has none
    if (!loc) {
        loc = player->GetCurrentLocation();
    }

    if (!loc) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  No location found for cell or player, skipping reinforcements.");
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    if (Settings::EnableLogging) {
        SKSE::log::info("  Location found: 0x{:08X}", loc->GetFormID());
    }

    auto faction = BanditSpawner::GetFactionFromLocation(loc);
    if (faction == FactionType::Unknown) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  Location has no hostile faction keyword, skipping.");
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::FormID trackID = loc->GetFormID();

    if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
        if (Settings::EnableLogging) {
            SKSE::log::info("  Location READY for reinforcements (faction={})", static_cast<int>(faction));
        }
        auto spawnResult = BanditSpawner::SpawnReinforcements(cell, faction);
        if (spawnResult.count > 0) {
            SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
        }
    } else {
        if (Settings::EnableLogging) {
            SKSE::log::info("  Location NOT ready (cooldown or already spawned).");
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}
