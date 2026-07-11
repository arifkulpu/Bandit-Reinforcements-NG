#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <SimpleIni.h>
#include <filesystem>

namespace Settings {
    // General
    inline bool EnableLogging = true;

    // Spawning (Level-based max spawns)
    inline int MaxSpawnsLevel10_24 = 5;
    inline int MaxSpawnsLevel25_49 = 10;
    inline int MaxSpawnsLevel50Plus = 15;
    inline float PatrolSpawnDistance = 2000.0f;

    // Boss (Level-based chance)
    inline int BossChanceLevel10_24 = 20;
    inline int BossChanceLevel25_49 = 30;
    inline int BossChanceLevel50Plus = 40;

    // Ambush
    inline int AmbushChance = 30;
    inline int AmbushMinCount = 2;
    inline int AmbushMaxCount = 5;
    inline float AmbushSpawnDistance = 500.0f;       // Zindan çıkışı pususu (yakın)
    inline float AmbushCampSpawnDistance = 1500.0f;  // Açık dünya kamp terk pususu (uzak)

    // Cooldown
    inline float ClearedCooldownDays = 10.0f;
    inline float VisitedCooldownDays = 3.0f;

    // Factions
    inline bool EnableBandits = true;
    inline bool EnableVampires = true;
    inline bool EnableWarlocks = true;
    inline bool EnableForsworn = true;
    inline bool EnableDraugr = true;
    inline bool EnableAnimals = true;
    inline bool EnableFalmer = true;
    inline bool EnableDwemer = true;

    // FormIDs for Leveled Lists (Hex strings)
    inline std::string FormID_Bandit = "0x0001068A";
    inline std::string FormID_BanditBoss = "0x000179C9";
    inline std::string FormID_Vampire = "0x00013589";
    inline std::string FormID_VampireBoss = "0x0001358A";
    inline std::string FormID_Warlock = "0x000130F1";
    inline std::string FormID_WarlockBoss = "0x000130F2";
    inline std::string FormID_Forsworn = "0x000130F3";
    inline std::string FormID_ForswornBoss = "0x000130F4";
    inline std::string FormID_Draugr = "0x000130F5";
    inline std::string FormID_DraugrBoss = "0x000130F6";

    inline constexpr const char* INI_PATH = "Data/SKSE/Plugins/BanditReinforcementsNG.ini";

    // If INI doesn't exist, create it with default values
    inline void CreateDefaultINI() {
        CSimpleIniA ini;
        ini.SetUnicode();

        // Settings
        ini.SetBoolValue("Settings", "bEnableLogging", true,
            "; Loglama aktif mi (1: Evet, 0: Hayir) / Enable logging (1: Yes, 0: No)");

        // Spawning
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel10_24", 5,
            "; Level 10-24 arasinda maks ek dusman (Lvl 1-9 hic dogma yapmaz) / Max spawns for Lvl 10-24 (Lvl 1-9 spawns nothing)");
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel25_49", 10);
        ini.SetLongValue("Spawning", "iMaxSpawnsLevel50Plus", 15);
        ini.SetDoubleValue("Spawning", "fPatrolSpawnDistance", 2000.0,
            "; Dusmanlar oyuncudan ne kadar uzakta dogacak (oyun birimi) / Patrol spawn distance (game units)");

        // Boss
        ini.SetLongValue("Boss", "iBossChanceLevel10_24", 20,
            "; Lvl 10-24 arasi boss dogma ihtimali (0-100) / Boss spawn chance Lvl 10-24");
        ini.SetLongValue("Boss", "iBossChanceLevel25_49", 30,
            "; Lvl 25-49 arasi boss dogma ihtimali (0-100) / Boss spawn chance Lvl 25-49");
        ini.SetLongValue("Boss", "iBossChanceLevel50Plus", 40,
            "; Lvl 50+ arasi boss dogma ihtimali (0-100) / Boss spawn chance Lvl 50+");

        // Ambush
        ini.SetLongValue("Ambush", "iAmbushChance", 30,
            "; Pusu dogma sansi (0-100) / Ambush chance on dungeon exit (0-100)");
        ini.SetLongValue("Ambush", "iAmbushMinCount", 2,
            "; Pusu grubundaki min/max dusman / Min and max enemies in ambush group");
        ini.SetLongValue("Ambush", "iAmbushMaxCount", 5);
        ini.SetDoubleValue("Ambush", "fAmbushSpawnDistance", 500.0,
            "; Zindan cikisinda pusu mesafesi (oyun birimi, ~7m) / Dungeon exit ambush distance (game units)");
        ini.SetDoubleValue("Ambush", "fAmbushCampSpawnDistance", 1500.0,
            "; Acik dunya kampi terkinde pusu mesafesi (oyun birimi, ~21m) / Outdoor camp ambush distance (game units)");

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
        ini.SetBoolValue("Factions", "bEnableAnimals", true);
        ini.SetBoolValue("Factions", "bEnableFalmer", true);
        ini.SetBoolValue("Factions", "bEnableDwemer", true);

        // FormIDs
        ini.SetValue("FormIDs", "sBandit", "0x0001068A",
            "; FormIDs for the leveled lists (You MUST check these in xEdit if they don't spawn)");
        ini.SetValue("FormIDs", "sBanditBoss", "0x000179C9");
        ini.SetValue("FormIDs", "sVampire", "0x00013589");
        ini.SetValue("FormIDs", "sVampireBoss", "0x0001358A");
        ini.SetValue("FormIDs", "sWarlock", "0x000130F1");
        ini.SetValue("FormIDs", "sWarlockBoss", "0x000130F2");
        ini.SetValue("FormIDs", "sForsworn", "0x000130F3");
        ini.SetValue("FormIDs", "sForswornBoss", "0x000130F4");
        ini.SetValue("FormIDs", "sDraugr", "0x000130F5");
        ini.SetValue("FormIDs", "sDraugrBoss", "0x000130F6");

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
            MaxSpawnsLevel10_24 = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel10_24", 5));
            MaxSpawnsLevel25_49 = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel25_49", 10));
            MaxSpawnsLevel50Plus = static_cast<int>(ini.GetLongValue("Spawning", "iMaxSpawnsLevel50Plus", 15));
            PatrolSpawnDistance = static_cast<float>(ini.GetDoubleValue("Spawning", "fPatrolSpawnDistance", 2000.0));

            // Boss
            BossChanceLevel10_24 = static_cast<int>(ini.GetLongValue("Boss", "iBossChanceLevel10_24", 20));
            BossChanceLevel25_49 = static_cast<int>(ini.GetLongValue("Boss", "iBossChanceLevel25_49", 30));
            BossChanceLevel50Plus = static_cast<int>(ini.GetLongValue("Boss", "iBossChanceLevel50Plus", 40));

            // Ambush
            AmbushChance = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushChance", 30));
            AmbushMinCount = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushMinCount", 2));
            AmbushMaxCount = static_cast<int>(ini.GetLongValue("Ambush", "iAmbushMaxCount", 5));
            AmbushSpawnDistance = static_cast<float>(ini.GetDoubleValue("Ambush", "fAmbushSpawnDistance", 500.0));
            AmbushCampSpawnDistance = static_cast<float>(ini.GetDoubleValue("Ambush", "fAmbushCampSpawnDistance", 1500.0));

            // Cooldown
            ClearedCooldownDays = static_cast<float>(ini.GetDoubleValue("Cooldown", "fClearedCooldownDays", 10.0));
            VisitedCooldownDays = static_cast<float>(ini.GetDoubleValue("Cooldown", "fVisitedCooldownDays", 3.0));

            // Factions
            EnableBandits = ini.GetBoolValue("Factions", "bEnableBandits", true);
            EnableVampires = ini.GetBoolValue("Factions", "bEnableVampires", true);
            EnableWarlocks = ini.GetBoolValue("Factions", "bEnableWarlocks", true);
            EnableForsworn = ini.GetBoolValue("Factions", "bEnableForsworn", true);
            EnableDraugr = ini.GetBoolValue("Factions", "bEnableDraugr", true);
            EnableAnimals = ini.GetBoolValue("Factions", "bEnableAnimals", true);
            EnableFalmer = ini.GetBoolValue("Factions", "bEnableFalmer", true);
            EnableDwemer = ini.GetBoolValue("Factions", "bEnableDwemer", true);

            // FormIDs
            FormID_Bandit = ini.GetValue("FormIDs", "sBandit", "0x0001068A");
            FormID_BanditBoss = ini.GetValue("FormIDs", "sBanditBoss", "0x000179C9");
            FormID_Vampire = ini.GetValue("FormIDs", "sVampire", "0x00013589");
            FormID_VampireBoss = ini.GetValue("FormIDs", "sVampireBoss", "0x0001358A");
            FormID_Warlock = ini.GetValue("FormIDs", "sWarlock", "0x000130F1");
            FormID_WarlockBoss = ini.GetValue("FormIDs", "sWarlockBoss", "0x000130F2");
            FormID_Forsworn = ini.GetValue("FormIDs", "sForsworn", "0x000130F3");
            FormID_ForswornBoss = ini.GetValue("FormIDs", "sForswornBoss", "0x000130F4");
            FormID_Draugr = ini.GetValue("FormIDs", "sDraugr", "0x000130F5");
            FormID_DraugrBoss = ini.GetValue("FormIDs", "sDraugrBoss", "0x000130F6");

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
