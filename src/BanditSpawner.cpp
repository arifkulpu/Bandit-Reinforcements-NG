#include "BanditSpawner.h"
#include "Settings.h"
#include <random>

RE::TESNPC* BanditSpawner::GetRandomBanditBase(int playerLevel) {
    // 0x1068FF is a generic LCharBandit leveled list, or we can use specific IDs.
    // LCharBanditMelee: 0x1068FF
    // For simplicity, we just lookup "LCharBanditMelee" or "LCharBanditMissile" leveled list.
    auto dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return nullptr;

    // Use Skyrim.esm base forms
    // Generic melee bandit leveled list
    auto list = RE::TESForm::LookupByID<RE::TESLevCharacter>(0x001068FF); 
    if (!list) return nullptr;

    return nullptr; // We'll actually spawn via PlaceAtMe with Leveled List
}

RE::TESObjectREFR* BanditSpawner::PlaceActorAtMe(RE::TESObjectREFR* target, RE::TESBoundObject* baseObj) {
    if (!target || !baseObj) return nullptr;
    auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Actor>();
    if (factory) {
        // PlaceAtMe is usually done by Papyrus:
        // SKSE ObjectReference::PlaceAtMe
    }
    return nullptr;
}

void BanditSpawner::SpawnBandits(RE::TESObjectCELL* cell) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !cell) return;

    int pLevel = player->GetLevel();
    int maxSpawns = Settings::MaxSpawnsLevel1_10;
    if (pLevel >= 25) maxSpawns = Settings::MaxSpawnsLevel25Plus;
    else if (pLevel >= 10) maxSpawns = Settings::MaxSpawnsLevel10_25;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(1, maxSpawns);

    int count = distr(gen);
    SKSE::log::info("SpawnBandits: Spawning {} bandits in cell {}", count, cell->GetFormID());

    auto listForm = RE::TESForm::LookupByID(0x001068FF); // LCharBanditMelee
    if (!listForm) {
        SKSE::log::info("LCharBanditMelee not found");
        return;
    }

    for (int i = 0; i < count; ++i) {
        // Use Papyrus PlaceAtMe
        // We will call player->PlaceObjectAtMe
        auto spawned = player->PlaceObjectAtMe(listForm->As<RE::TESBoundObject>(), false);
        if (spawned) {
            auto spawnedActor = spawned->As<RE::Actor>();
            if (spawnedActor) {
                // Move them around a bit randomly so they don't spawn exactly on the player
                RE::NiPoint3 pos = player->GetPosition();
                std::uniform_real_distribution<float> offsetDist(-500.0f, 500.0f);
                pos.x += offsetDist(gen);
                pos.y += offsetDist(gen);
                spawnedActor->SetPosition(pos, false);
            }
        }
    }
}
