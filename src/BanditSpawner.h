#pragma once

#include <RE/Skyrim.h>
#include <vector>

class BanditSpawner {
public:
    static void SpawnBandits(RE::TESObjectCELL* cell);
private:
    static RE::TESNPC* GetRandomBanditBase(int playerLevel);
    static RE::TESObjectREFR* PlaceActorAtMe(RE::TESObjectREFR* target, RE::TESBoundObject* baseObj);
};
