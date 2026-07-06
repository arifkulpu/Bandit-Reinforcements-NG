#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <SimpleIni.h>

namespace Settings {
    inline bool EnableLogging = true;
    inline int MaxSpawnsLevel1_10 = 3;
    inline int MaxSpawnsLevel10_25 = 6;
    inline int MaxSpawnsLevel25Plus = 10;
    inline float ClearedCooldownDays = 10.0f;
    inline float VisitedCooldownDays = 3.0f;

    inline void Load() {
        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile("Data/SKSE/Plugins/BanditReinforcementsNG.ini") == SI_OK) {
            EnableLogging = ini.GetBoolValue("Settings", "bEnableLogging", true);
            MaxSpawnsLevel1_10 = ini.GetLongValue("Spawning", "iMaxSpawnsLevel1_10", 3);
            MaxSpawnsLevel10_25 = ini.GetLongValue("Spawning", "iMaxSpawnsLevel10_25", 6);
            MaxSpawnsLevel25Plus = ini.GetLongValue("Spawning", "iMaxSpawnsLevel25Plus", 10);
            ClearedCooldownDays = (float)ini.GetDoubleValue("Cooldown", "fClearedCooldownDays", 10.0);
            VisitedCooldownDays = (float)ini.GetDoubleValue("Cooldown", "fVisitedCooldownDays", 3.0);
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
