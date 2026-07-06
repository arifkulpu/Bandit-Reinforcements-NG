#pragma once

#include <RE/Skyrim.h>
#include <vector>
#include <unordered_map>

enum class SpawnState {
    Active,
    Cleared,
    Expired
};

struct SpawnRecord {
    std::vector<RE::ObjectRefHandle> actors;
    float spawnTimeDays;
    SpawnState state;
};

class SpawnTracker {
public:
    static SpawnTracker* GetSingleton() {
        static SpawnTracker singleton;
        return &singleton;
    }

    bool IsLocationReady(RE::FormID locFormID);
    void RegisterSpawn(RE::FormID locFormID, const std::vector<RE::ObjectRefHandle>& spawnedActors);
    void Update();

private:
    SpawnTracker() = default;
    ~SpawnTracker() = default;
    SpawnTracker(const SpawnTracker&) = delete;
    SpawnTracker& operator=(const SpawnTracker&) = delete;

    std::unordered_map<RE::FormID, SpawnRecord> m_spawns;

    float GetCurrentGameTimeDays();
};
