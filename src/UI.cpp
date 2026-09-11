#include "PCH.h"
#include "UI.h"
#include "Settings.h"
#include "SKSEMenuFramework.h"
#include <cmath>

namespace ImGui = ImGuiMCP;

// ────────────────────────────────────────────────────────────────────────────
//  Yardimci makrolar:
//  Checkbox gibi anlik elementler icin: SAVE_IF_CHANGED
//  Slider gibi suruklenen elementler icin: SAVE_ON_RELEASE
// ────────────────────────────────────────────────────────────────────────────
#define SAVE_IF_CHANGED(expr) \
    do { if (expr) { Settings::Save(); } } while (0)

#define SAVE_ON_RELEASE(slider_expr) \
    do { slider_expr; if (ImGui::IsItemDeactivatedAfterEdit()) { Settings::Save(); } } while (0)

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 1 - Fraksiyonlar
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderFactions() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "Enabled Factions");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Toggle which enemy types the mod will handle.");
    ImGui::Spacing();

    SAVE_IF_CHANGED(ImGui::Checkbox("Bandits",    &Settings::EnableBandits));
    ImGui::SameLine(200);
    SAVE_IF_CHANGED(ImGui::Checkbox("Vampires",   &Settings::EnableVampires));

    SAVE_IF_CHANGED(ImGui::Checkbox("Warlocks",   &Settings::EnableWarlocks));
    ImGui::SameLine(200);
    SAVE_IF_CHANGED(ImGui::Checkbox("Forsworn",   &Settings::EnableForsworn));

    SAVE_IF_CHANGED(ImGui::Checkbox("Draugr & Skeleton##draugr", &Settings::EnableDraugr));
    ImGui::SameLine(200);
    SAVE_IF_CHANGED(ImGui::Checkbox("Animals",    &Settings::EnableAnimals));

    SAVE_IF_CHANGED(ImGui::Checkbox("Falmer",     &Settings::EnableFalmer));
    ImGui::SameLine(200);
    SAVE_IF_CHANGED(ImGui::Checkbox("Dwemer",     &Settings::EnableDwemer));
}

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 2 - Spawn Ayarlari
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderSpawning() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "Spawn Settings");
    ImGui::Separator();
    ImGui::Spacing();

    // Seviye 1'den spawn özelliği
    ImGui::Spacing();
    SAVE_IF_CHANGED(ImGui::Checkbox("Enable Spawns from Level 1##sfl1", &Settings::SpawnFromLevel1));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, players at levels 1-9 will also trigger spawns and ambushes,\n"
                          "using the same limits as the Lvl 10-24 bracket. (Default: OFF)");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(default: OFF)");

    // Max spawns per level bracket
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Max extra enemies by player level:");
    ImGui::TextDisabled("  (Lvl 1-9 uses Lvl 10-24 limits when 'Spawn from Level 1' is ON)");
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Spawns  (Lvl 10-24)##s1", &Settings::MaxSpawnsLevel10_24,  1, 20));
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Spawns  (Lvl 25-49)##s2", &Settings::MaxSpawnsLevel25_49, 1, 30));
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Spawns  (Lvl 50+)##s3",   &Settings::MaxSpawnsLevel50Plus, 1, 50));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Patrol spawn distance from player (game units):");

    float patrolDist = Settings::PatrolSpawnDistance;
    if (ImGui::SliderFloat("Patrol Distance##pd", &patrolDist, 500.0f, 5000.0f, "%.0f units")) {
        Settings::PatrolSpawnDistance = patrolDist;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Max actors allowed per cell (performance limit):");
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Actors per Cell##mac", &Settings::MaxRuntimeActorsPerCell, 5, 100));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Spawn Density (0%% = no spawns, 100%% = always maximum):");
    SAVE_ON_RELEASE(ImGui::SliderInt("Spawn Density (%%)##sdens", &Settings::SpawnDensity, 0, 100));
    ImGui::SameLine();
    // Anlık gösterge: Mevcut ayarda Lvl10-24 için beklenen aralık
    {
        int scaledMax10 = static_cast<int>(std::round(Settings::MaxSpawnsLevel10_24 * Settings::SpawnDensity / 100.0f));
        if (scaledMax10 < 1 && Settings::SpawnDensity > 0) scaledMax10 = 1;
        int scaledMax50 = static_cast<int>(std::round(Settings::MaxSpawnsLevel50Plus * Settings::SpawnDensity / 100.0f));
        if (scaledMax50 < 1 && Settings::SpawnDensity > 0) scaledMax50 = 1;
        if (Settings::SpawnDensity <= 0) {
            ImGui::TextDisabled("(No spawns)");
        } else if (Settings::SpawnDensity >= 100) {
            ImGui::TextDisabled("(Lvl10: always %d | Lvl50: always %d)", Settings::MaxSpawnsLevel10_24, Settings::MaxSpawnsLevel50Plus);
        } else {
            ImGui::TextDisabled("(Lvl10: 1-%d | Lvl50: 1-%d)", scaledMax10, scaledMax50);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 3 - Boss Ayarlari
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderBoss() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "Boss Spawn Chances");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("Probability (0-100%%) that a boss spawns with the group:");
    ImGui::Spacing();

    SAVE_ON_RELEASE(ImGui::SliderInt("Boss Chance  (Lvl 10-24)##b1", &Settings::BossChanceLevel10_24,  0, 100));
    SAVE_ON_RELEASE(ImGui::SliderInt("Boss Chance  (Lvl 25-49)##b2", &Settings::BossChanceLevel25_49, 0, 100));
    SAVE_ON_RELEASE(ImGui::SliderInt("Boss Chance  (Lvl 50+)##b3",   &Settings::BossChanceLevel50Plus, 0, 100));
}

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 4 - Pusu (Ambush) Sistemi
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderAmbush() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "Ambush System");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Chance of an ambush when leaving a dungeon or camp:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Ambush Chance (%%)##ac", &Settings::AmbushChance, 0, 100));

    ImGui::Spacing();
    ImGui::TextDisabled("Ambush group size:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Min Count##amin", &Settings::AmbushMinCount, 1, 10));
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Count##amax", &Settings::AmbushMaxCount, 1, 20));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Spawn distances (game units):");

    float dungDist = Settings::AmbushSpawnDistance;
    if (ImGui::SliderFloat("Dungeon Exit Distance##asd", &dungDist, 100.0f, 3000.0f, "%.0f")) {
        Settings::AmbushSpawnDistance = dungDist;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }

    float campDist = Settings::AmbushCampSpawnDistance;
    if (ImGui::SliderFloat("Camp Leave Distance##acsd", &campDist, 500.0f, 5000.0f, "%.0f")) {
        Settings::AmbushCampSpawnDistance = campDist;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Combat stop distance (enemies stop chasing beyond this):");
    float combatDist = Settings::CombatStopDistance;
    if (ImGui::SliderFloat("Combat Stop Distance##csd", &combatDist, 1000.0f, 15000.0f, "%.0f")) {
        Settings::CombatStopDistance = combatDist;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 5 - Destek Kuvvetler (Backup Forces)
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderBackupForces() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "Backup Forces (Interior Only)");
    ImGui::Separator();
    ImGui::Spacing();

    SAVE_IF_CHANGED(ImGui::Checkbox("Enable Backup Forces##ebf", &Settings::EnableBackupForces));
    ImGui::SameLine();
    SAVE_IF_CHANGED(ImGui::Checkbox("Show Notification##sbm", &Settings::ShowBackupMessage));

    ImGui::Spacing();
    ImGui::TextDisabled("Chance (0-100%) for backup forces to appear in a dungeon:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Backup Chance (%)##bchance", &Settings::BackupChance, 0, 100));

    ImGui::Spacing();
    ImGui::TextDisabled("Time range (seconds) before backup forces arrive:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Min Timer (sec)##btmin", &Settings::BackupTimerMin, 10, 600));
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Timer (sec)##btmax", &Settings::BackupTimerMax, 30, 3600));

    ImGui::Spacing();
    ImGui::TextDisabled("Maximum backup waves per dungeon run:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Max Waves##bmw", &Settings::BackupMaxWaves, 1, 10));
}

// ════════════════════════════════════════════════════════════════════════════
//  SEKME 6 - Genel / Diger Ayarlar
// ════════════════════════════════════════════════════════════════════════════
static void __stdcall RenderGeneral() {
    ImGui::TextColored({1.0f, 0.85f, 0.0f, 1.0f}, "General Settings");
    ImGui::Separator();
    ImGui::Spacing();

    SAVE_IF_CHANGED(ImGui::Checkbox("Enable Logging##log", &Settings::EnableLogging));
    ImGui::SameLine();
    ImGui::TextDisabled("(Creates BanditReinforcementsNG.log)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Cooldown for cleared locations (in-game days):");
    float clearedDays = Settings::ClearedCooldownDays;
    if (ImGui::SliderFloat("Cleared Cooldown##cc", &clearedDays, 1.0f, 30.0f, "%.1f days")) {
        Settings::ClearedCooldownDays = clearedDays;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }

    ImGui::TextDisabled("Cooldown for fled locations (in-game days):");
    float visitedDays = Settings::VisitedCooldownDays;
    if (ImGui::SliderFloat("Fled Cooldown##vc", &visitedDays, 0.5f, 15.0f, "%.1f days")) {
        Settings::VisitedCooldownDays = visitedDays;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Settings::Save();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Minimum hostiles for unnamed wilderness camps:");
    SAVE_ON_RELEASE(ImGui::SliderInt("Min Hostiles##mhc", &Settings::MinHostilesForUnnamedCamp, 1, 10));

    ImGui::Spacing();
    ImGui::Separator();

    // Reset butonu
    ImGui::Spacing();
    if (ImGui::Button("Reset All to Defaults", {200, 30})) {
        Settings::MaxSpawnsLevel10_24     = 5;
        Settings::MaxSpawnsLevel25_49     = 10;
        Settings::MaxSpawnsLevel50Plus    = 15;
        Settings::PatrolSpawnDistance     = 2000.0f;
        Settings::MaxRuntimeActorsPerCell = 30;
        Settings::BossChanceLevel10_24    = 20;
        Settings::BossChanceLevel25_49    = 30;
        Settings::BossChanceLevel50Plus   = 40;
        Settings::AmbushChance            = 30;
        Settings::AmbushMinCount          = 2;
        Settings::AmbushMaxCount          = 5;
        Settings::AmbushSpawnDistance     = 500.0f;
        Settings::AmbushCampSpawnDistance = 1500.0f;
        Settings::CombatStopDistance      = 5000.0f;
        Settings::EnableBackupForces      = true;
        Settings::BackupChance            = 30;
        Settings::ShowBackupMessage       = true;
        Settings::BackupTimerMin          = 60;
        Settings::BackupTimerMax          = 600;
        Settings::BackupMaxWaves          = 1;
        Settings::ClearedCooldownDays     = 10.0f;
        Settings::VisitedCooldownDays     = 3.0f;
        Settings::MinHostilesForUnnamedCamp = 2;
        Settings::EnableBandits  = true;
        Settings::EnableVampires = true;
        Settings::EnableWarlocks = true;
        Settings::EnableForsworn = true;
        Settings::EnableDraugr   = true;
        Settings::EnableAnimals  = true;
        Settings::EnableFalmer   = true;
        Settings::EnableDwemer   = true;
        Settings::Save();
    }
    ImGui::SameLine();
    ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "<- Resets everything!");
}

// ════════════════════════════════════════════════════════════════════════════
//  KAYIT
// ════════════════════════════════════════════════════════════════════════════
void UI::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        SKSE::log::info("SKSE Menu Framework not found - in-game menu disabled. Use INI for configuration.");
        return;
    }

    SKSEMenuFramework::SetSection("Bandit Reinforcements NG");
    SKSEMenuFramework::AddSectionItem("Factions",       RenderFactions);
    SKSEMenuFramework::AddSectionItem("Spawning",       RenderSpawning);
    SKSEMenuFramework::AddSectionItem("Boss Spawns",    RenderBoss);
    SKSEMenuFramework::AddSectionItem("Ambush",         RenderAmbush);
    SKSEMenuFramework::AddSectionItem("Backup Forces",  RenderBackupForces);
    SKSEMenuFramework::AddSectionItem("General",        RenderGeneral);

    SKSE::log::info("SKSE Menu Framework registered - in-game menu active.");
}
