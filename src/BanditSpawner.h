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
    // Helper: Determine faction by cell dominance or fallback to location keywords
    static FactionType DetermineFactionForCell(RE::TESObjectCELL* cell, RE::BGSLocation* loc);
    static FactionType ScanCellForDominantFaction(RE::TESObjectCELL* cell);
    static FactionType GetFactionFromLocation(RE::BGSLocation* loc);

    // Common faction classification helper
    static bool ClassifyActorFaction(RE::Actor* actor, FactionType targetFaction, bool& outIsKnownOtherFaction);
    static FactionType DetectActorFaction(RE::Actor* actor);

    // Standardized tracking ID (Location FormID if present, otherwise Cell FormID)
    static RE::FormID GetTrackingID(RE::TESObjectCELL* cell, RE::BGSLocation* loc);

    // INI'ye keşfedilen NPC FormID'lerini kaydet (kSaveGame'de çağır)
    // Mevcut kayıtla merge eder; aynı ID'ler tekrar eklenmez.
    static void SaveCachedFormIDs();

    // INI'den kayıtlı NPC FormID'lerini cache'e yükle (kDataLoaded'da çağır)
    // Böylece oyun başında hücre taranmadan önce bile template'lar hazır olur.
    static void LoadCachedFormIDs();

    // Dump all leveled list FormIDs to the log (call during kDataLoaded)
    static void DumpLeveledLists();

    // Scan the cell and cache enemy base objects for dynamic copying
    static void UpdateCache(RE::TESObjectCELL* cell, FactionType faction);

    // ── Elite / Boss Cache ──────────────────────────────────────────
    // Her faction için ayrı elit NPC cache'i döner (boss spawn'ları için)
    static std::vector<RE::TESBoundObject*>& GetBossCacheForFaction(FactionType faction);

    // NPC'nin elit (boss) olup olmadığını isim + seviye eşiğiyle saptar
    static bool IsEliteNPC(RE::Actor* actor, RE::TESNPC* npc);

    // INI [CachedBossNPCs] section'ına elite NPC FormID'lerini kaydet
    static void SaveCachedBossFormIDs();

    // INI [CachedBossNPCs] section'ından elite NPC FormID'lerini yükle
    static void LoadCachedBossFormIDs();

    static int GetSpawnCount();
    static std::mt19937& GetRNG();
    
    // Fallback dynamic cache mapping FactionType to a list of TESBoundObject*
    static std::vector<RE::TESBoundObject*>& GetCacheForFaction(FactionType faction);
};
