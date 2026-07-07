#pragma once

#include <RE/Skyrim.h>
#include <vector>
#include <string>
#include <random>

// Supported faction types
enum class FactionType {
    Bandit,
    Vampire,
    Warlock,
    Forsworn,
    Draugr,
    Unknown
};

struct SpawnResult {
    std::vector<RE::ObjectRefHandle> spawnedActors;
    int count;
};

class BanditSpawner {
public:
    // Spawn reinforcements for a given faction at the player's location
    static SpawnResult SpawnReinforcements(RE::TESObjectCELL* cell, FactionType faction);

    // Spawn an ambush group near the player (used on dungeon exit)
    static SpawnResult SpawnAmbush(FactionType faction);

    // Determine faction type from a BGSLocation's keywords
    static FactionType GetFactionFromLocation(RE::BGSLocation* loc);

    // Get the EditorID of the leveled character list for a faction
    static const char* GetLeveledListEditorID(FactionType faction, bool isBoss);

private:
    // Calculate how many enemies to spawn based on player level
    static int GetSpawnCount();

    // Shared random engine (thread-local for safety)
    static std::mt19937& GetRNG();
};
