#include "Settings.h"
#include <SKSE/SKSE.h>

#include "LocationEventSink.h"

extern "C" void __std_regex_transform_primary_char() {}

void InitListener() {
    LocationEventSink::GetSingleton()->Register();
    SKSE::log::info("Location event listener initialized.");
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    Logger::Setup();
    SKSE::log::info("BanditReinforcementsNG loaded - version 2.0.0");

    Settings::Load();
    SKSE::log::info("Settings loaded from INI.");
    SKSE::log::info("  Logging={}, Bandits={}, Vampires={}, Warlocks={}, Forsworn={}, Draugr={}",
        Settings::EnableLogging, Settings::EnableBandits, Settings::EnableVampires,
        Settings::EnableWarlocks, Settings::EnableForsworn, Settings::EnableDraugr);
    SKSE::log::info("  PatrolDist={:.0f}, AmbushDist={:.0f}, AmbushChance={}%",
        Settings::PatrolSpawnDistance, Settings::AmbushSpawnDistance, Settings::AmbushChance);
    SKSE::log::info("  Boss: Chance={}%, MinLevel={}",
        Settings::BossSpawnChance, Settings::BossMinPlayerLevel);

    // TaskInterface kaydını doğrula
    auto taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        SKSE::log::info("SKSE TaskInterface: OK");
    } else {
        SKSE::log::error("SKSE TaskInterface: NOT AVAILABLE! AI fix will be skipped.");
    }

    InitListener();

    return true;
}
