#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <SimpleIni.h>

namespace Settings {
    // General
    inline bool EnableLogging = true;

    // Spawning
    inline int MaxSpawnsLevel1_10 = 3;
    inline int MaxSpawnsLevel10_25 = 6;
    inline int MaxSpawnsLevel25Plus = 10;
    inline float PatrolSpawnDistance = 2000.0f;

    // Boss
    inline int BossSpawnChance = 15;
    inline int BossMinPlayerLevel = 10;

    // Ambush
    inline int AmbushChance = 30;
    inline int AmbushMinCount = 2;
    inline int AmbushMaxCount = 5;

    // Cooldown
    inline float ClearedCooldownDays = 10.0f;
    inline float VisitedCooldownDays = 3.0f;

    // Factions
    inline bool EnableBandits = true;
    inline bool EnableVampires = true;
    inline bool EnableWarlocks = true;
    inline bool EnableForsworn = true;
    inline bool EnableDraugr = true;

    inline void Load() {
        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile("Data/SKSE/Plugins/BanditReinforcementsNG.ini") == SI_OK) {
            // General
            EnableLogging = ini.GetBoolValue("Settings", "bEnableLogging", true);

            // Spawning
            MaxSpawnsLevel1_10 = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel1_10", 3));
            MaxSpawnsLevel10_25 = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel10_25", 6));
            MaxSpawnsLevel25Plus = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel25Plus", 10));
            PatrolSpawnDistance = static_cast<float>(ini.GetDoubleValue("Spawning", "fPatrolSpawnDistance", 2000.0));

            // Boss
            BossSpawnChance = static_cast<int>(ini.GetLongValue("Boss", "iBossSpawnChance", 15));
            BossMinPlayerLevel = static_cast<int>(ini.GetLongValue("Boss", "iBossMinPlayerLevel", 10));

            // Ambush
            AmbushChance = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushChance", 30));
            AmbushMinCount = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushMinCount", 2));
            AmbushMaxCount = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushMaxCount", 5));

            // Cooldown
            ClearedCooldownDays = static_cast<float>(ini.GetDoubleValue("Cooldown", "fClearedCooldownDays", 10.0));
            VisitedCooldownDays = static_cast<float>(ini.GetDoubleValue("Cooldown", "fVisitedCooldownDays", 3.0));

            // Factions
            EnableBandits = ini.GetBoolValue("Factions", "bEnableBandits", true);
            EnableVampires = ini.GetBoolValue("Factions", "bEnableVampires", true);
            EnableWarlocks = ini.GetBoolValue("Factions", "bEnableWarlocks", true);
            EnableForsworn = ini.GetBoolValue("Factions", "bEnableForsworn", true);
            EnableDraugr = ini.GetBoolValue("Factions", "bEnableDraugr", true);
        }
    }
}

namespace Logger {
    inline void Setup() {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }

        *path /= "BanditReinforcementsNG.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    }
}
