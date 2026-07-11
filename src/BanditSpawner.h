#pragma once

#include <RE/Skyrim.h>
#include <vector>
#include <string>
#include <random>

enum class FactionType {
    Bandit,
    Vampire,
    Warlock,
    Forsworn,
    Draugr,
    Animal,
    Falmer,
    Dwemer,
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

    // Spawn an ambush group near the player
    // isOutdoorCamp=true  → uzak mesafe (AmbushCampSpawnDistance, ~1500 birim)
    // isOutdoorCamp=false → yakin mesafe (AmbushSpawnDistance, ~500 birim, zindan cikisi)
    static SpawnResult SpawnAmbush(FactionType faction, bool isOutdoorCamp = false);

    // Determine faction type from a BGSLocation's keywords
    static FactionType GetFactionFromLocation(RE::BGSLocation* loc);

    // Get the EditorID of the leveled character list for a faction
    static const char* GetLeveledListEditorID(FactionType faction, bool isBoss);

    // Post-spawn AI fix: called on game thread with a delay, waits for 3D
    static void FixActorAI(RE::ObjectRefHandle handle, int retries = 0);

    // Dump all leveled list FormIDs to the log (call during kDataLoaded)
    static void DumpLeveledLists();

    // Scan the cell and cache enemy base objects for dynamic copying
    static void UpdateCache(RE::TESObjectCELL* cell, FactionType faction);

    static int GetSpawnCount();
    static std::mt19937& GetRNG();
    
    // Fallback dynamic cache mapping FactionType to a list of TESBoundObject*
    static std::vector<RE::TESBoundObject*>& GetCacheForFaction(FactionType faction);
};
