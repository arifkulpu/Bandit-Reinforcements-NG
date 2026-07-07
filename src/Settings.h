#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <SimpleIni.h>
#include <filesystem>

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

    inline constexpr const char* INI_PATH = "Data/SKSE/Plugins/BanditReinforcementsNG.ini";

    // If INI doesn't exist, create it with default values
    inline void CreateDefaultINI() {
        CSimpleIniA ini;
        ini.SetUnicode();

        // Settings
        ini.SetBoolValue("Settings", "bEnableLogging", true,
            "; Loglama aktif mi (1: Evet, 0: Hayir) / Enable logging (1: Yes, 0: No)");

        // Spawning
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel1_10", 3,
            "; Oyuncunun seviyesine gore maksimum ek dusman sayisi / Max extra enemies by player level");
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel10_25", 6);
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel25Plus", 10);
        ini.SetDoubleValue("Spawning", "fPatrolSpawnDistance", 2000.0,
            "; Dusmanlar oyuncudan ne kadar uzakta dogacak (oyun birimi) / Patrol spawn distance (game units)");

        // Boss
        ini.SetLongValue("Boss", "iBossSpawnChance", 15,
            "; Boss dogma sansi (0-100) / Boss spawn chance (0-100 percent)");
        ini.SetLongValue("Boss", "iBossMinPlayerLevel", 10,
            "; Boss icin minimum oyuncu seviyesi / Min player level for boss spawns");

        // Ambush
        ini.SetLongValue("Ambush", "iAmbushChance", 30,
            "; Pusu dogma sansi (0-100) / Ambush chance on dungeon exit (0-100)");
        ini.SetLongValue("Ambush", "iAmbushMinCount", 2,
            "; Pusu grubundaki min/max dusman / Min and max enemies in ambush group");
        ini.SetLongValue("Ambush", "iAmbushMaxCount", 5);

        // Cooldown
        ini.SetDoubleValue("Cooldown", "fClearedCooldownDays", 10.0,
            "; Temizlenmis bolgeler icin bekleme (gun) / Cooldown for cleared locations (days)");
        ini.SetDoubleValue("Cooldown", "fVisitedCooldownDays", 3.0,
            "; Kacilan bolgeler icin bekleme (gun) / Cooldown for fled locations (days)");

        // Factions
        ini.SetBoolValue("Factions", "bEnableBandits", true,
            "; Hangi fraksiyonlar aktif? (1: Aktif, 0: Pasif) / Which factions are enabled?");
        ini.SetBoolValue("Factions", "bEnableVampires", true);
        ini.SetBoolValue("Factions", "bEnableWarlocks", true);
        ini.SetBoolValue("Factions", "bEnableForsworn", true);
        ini.SetBoolValue("Factions", "bEnableDraugr", true);

        ini.SaveFile(INI_PATH);
    }

    inline void Load() {
        // If INI file doesn't exist, create it with defaults
        if (!std::filesystem::exists(INI_PATH)) {
            CreateDefaultINI();
            SKSE::log::info("INI file not found. Created default: {}", INI_PATH);
        }

        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile(INI_PATH) == SI_OK) {
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

            SKSE::log::info("INI loaded successfully from {}", INI_PATH);
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
