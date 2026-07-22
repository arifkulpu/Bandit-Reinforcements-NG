#include "SpawnTracker.h"
#include "Settings.h"
#include <SKSE/SKSE.h>

float SpawnTracker::GetCurrentGameTimeDays() {
    auto calendar = RE::Calendar::GetSingleton();
    if (calendar) {
        return calendar->GetDaysPassed();
    }
    return 0.0f;
}

bool SpawnTracker::IsLocationReady(RE::FormID locFormID) {
    auto it = m_spawns.find(locFormID);
    if (it == m_spawns.end()) {
        return true; // Never spawned here
    }

    auto& record = it->second;
    float currentTime = GetCurrentGameTimeDays();

    if (record.state == SpawnState::Cleared) {
        if (currentTime - record.spawnTimeDays >= Settings::ClearedCooldownDays) {
            m_spawns.erase(it);
            return true;
        }
        return false;
    } else if (record.state == SpawnState::Expired) {
        m_spawns.erase(it);
        return true;
    }

    // State is Active or Pending
    return false;
}

bool SpawnTracker::MarkLocationPending(RE::FormID locFormID) {
    if (!IsLocationReady(locFormID)) return false;

    SpawnRecord record;
    record.state = SpawnState::Pending;
    record.spawnTimeDays = GetCurrentGameTimeDays();
    m_spawns[locFormID] = record;
    return true;
}

void SpawnTracker::ClearPendingLocation(RE::FormID locFormID) {
    auto it = m_spawns.find(locFormID);
    if (it != m_spawns.end() && it->second.state == SpawnState::Pending) {
        m_spawns.erase(it);
        if (Settings::EnableLogging) {
            SKSE::log::info("SpawnTracker: Cleared pending state for location 0x{:08X}", locFormID);
        }
    }
}

void SpawnTracker::RegisterSpawn(RE::FormID locFormID, const std::vector<RE::ObjectRefHandle>& spawnedActors) {
    SpawnRecord record;
    record.actors = spawnedActors;
    record.spawnTimeDays = GetCurrentGameTimeDays();
    record.state = SpawnState::Active;

    m_spawns[locFormID] = record;
}

void SpawnTracker::Update() {
    float currentTime = GetCurrentGameTimeDays();

    for (auto& [locFormID, record] : m_spawns) {
        if (record.state == SpawnState::Active) {
            bool anyAlive = false;
            
            for (auto& handle : record.actors) {
                auto actor = handle.get().get();
                if (actor && !actor->IsDead()) {
                    anyAlive = true;
                    break;
                }
            }

            if (!anyAlive) {
                record.state = SpawnState::Cleared;
                record.spawnTimeDays = currentTime; // Reset time to start cooldown
                if (Settings::EnableLogging) {
                    SKSE::log::info("SpawnTracker: Location cleared, cooldown started.");
                }
            } else if (currentTime - record.spawnTimeDays >= Settings::VisitedCooldownDays) {
                for (auto& handle : record.actors) {
                    auto actor = handle.get().get();
                    if (actor) {
                        actor->Disable();
                        actor->SetDelete(true);
                    }
                }
                record.state = SpawnState::Expired;
                if (Settings::EnableLogging) {
                    SKSE::log::info("SpawnTracker: Location expired, actors deleted.");
                }
            }
        }
    }
}

void SpawnTracker::CheckAndStopCombatByDistance(RE::PlayerCharacter* player) {
    if (!player) return;
    
    int totalStopped = 0;
    float maxDistSq = Settings::CombatStopDistance * Settings::CombatStopDistance;

    for (auto& [locFormID, record] : m_spawns) {
        if (record.state != SpawnState::Active) continue;

        for (auto& handle : record.actors) {
            auto ref = handle.get().get();
            if (ref) {
                auto actor = ref->As<RE::Actor>();
                if (actor && !actor->IsDead() && actor->IsInCombat()) {
                    auto distSq = player->GetPosition().GetSquaredDistance(actor->GetPosition());
                    if (distSq > maxDistSq) {
                        actor->StopCombat();
                        totalStopped++;
                    }
                }
            }
        }
    }

    if (Settings::EnableLogging && totalStopped > 0) {
        SKSE::log::info("SpawnTracker: Stopped combat for {} actors due to distance", totalStopped);
    }
}
