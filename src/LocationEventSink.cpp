#include "LocationEventSink.h"
#include "BanditSpawner.h"
#include "SpawnTracker.h"
#include "Settings.h"
#include <SKSE/SKSE.h>

LocationEventSink* LocationEventSink::GetSingleton() {
    static LocationEventSink singleton;
    return &singleton;
}

void LocationEventSink::Register() {
    auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (scriptEventSource) {
        scriptEventSource->AddEventSink<RE::TESCellAttachDetachEvent>(GetSingleton());
        SKSE::log::info("LocationEventSink registered (TESCellAttachDetachEvent).");
    } else {
        SKSE::log::error("LocationEventSink: ScriptEventSourceHolder not found!");
    }
}

RE::BSEventNotifyControl LocationEventSink::ProcessEvent(
    const RE::TESCellAttachDetachEvent* a_event,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) 
{
    // Sadece "attach" (yükleme) olaylarını işle, "detach" (boşaltma) değil
    if (!a_event || !a_event->reference || !a_event->attached) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Sadece oyuncu olaylarını işle
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || a_event->reference.get() != player) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto cell = player->GetParentCell();
    if (!cell) {
        SKSE::log::warn("ProcessEvent: Player has no parent cell, skipping.");
        return RE::BSEventNotifyControl::kContinue;
    }

    // Aynı hücrede tekrar tetiklenmeyi önle
    RE::FormID currentCellID = cell->GetFormID();
    if (currentCellID == m_lastProcessedCellID) {
        return RE::BSEventNotifyControl::kContinue;
    }
    m_lastProcessedCellID = currentCellID;

    bool isInterior = cell->IsInteriorCell();

    SKSE::log::info("=== CellChange: Cell=0x{:08X}, Interior={}, WasInterior={} ===", 
                    currentCellID, isInterior, m_wasInInterior);

    // ── PUSU KONTROLÜ: İç → Dış mekan geçişi (zindan çıkışı) ──
    if (m_wasInInterior && !isInterior && m_lastInteriorCellID != 0) {
        SKSE::log::info("  >> DUNGEON EXIT! PrevInteriorCell=0x{:08X}", m_lastInteriorCellID);

        auto prevCell = RE::TESForm::LookupByID<RE::TESObjectCELL>(m_lastInteriorCellID);
        if (prevCell) {
            // Önce hücrenin kendi location'ına bak
            RE::BGSLocation* prevLoc = prevCell->GetLocation();
            
            SKSE::log::info("  PrevCell=0x{:08X}, prevLoc={}", 
                           prevCell->GetFormID(), prevLoc ? "FOUND" : "NULL");

            if (prevLoc) {
                auto faction = BanditSpawner::GetFactionFromLocation(prevLoc);
                SKSE::log::info("  Ambush faction={}", static_cast<int>(faction));

                if (faction != FactionType::Unknown) {
                    auto ambushResult = BanditSpawner::SpawnAmbush(faction);
                    if (ambushResult.count > 0) {
                        RE::FormID trackID = currentCellID;
                        SpawnTracker::GetSingleton()->RegisterSpawn(trackID, ambushResult.spawnedActors);
                    }
                } else {
                    SKSE::log::info("  No hostile faction at previous location, no ambush.");
                }
            } else {
                SKSE::log::info("  Previous interior cell had no BGSLocation, no ambush.");
            }
        }
    }

    // Durum güncellemesi
    m_wasInInterior = isInterior;
    m_lastInteriorCellID = isInterior ? currentCellID : 0;

    // ── GECİKMİŞ GÜNCELLEME: Eski spawn kayıtlarını temizle ──
    SpawnTracker::GetSingleton()->Update();

    // ── TAKVİYE KONTROLÜ: Düşman lokasyonuna giriş ──
    // Önce hücrenin location'ına bak, sonra oyuncunun location'ına
    RE::BGSLocation* loc = cell->GetLocation();
    if (!loc) {
        loc = player->GetCurrentLocation();
    }

    if (!loc) {
        SKSE::log::info("  No BGSLocation for this cell or player, skipping reinforcements.");
        return RE::BSEventNotifyControl::kContinue;
    }

    SKSE::log::info("  Location=0x{:08X}", loc->GetFormID());

    auto faction = BanditSpawner::GetFactionFromLocation(loc);
    if (faction == FactionType::Unknown) {
        SKSE::log::info("  Location is not a hostile area, skipping.");
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::FormID trackID = loc->GetFormID();

    if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
        SKSE::log::info("  Location READY → spawning reinforcements (faction={})", static_cast<int>(faction));
        auto spawnResult = BanditSpawner::SpawnReinforcements(cell, faction);
        if (spawnResult.count > 0) {
            SpawnTracker::GetSingleton()->RegisterSpawn(trackID, spawnResult.spawnedActors);
        }
    } else {
        SKSE::log::info("  Location in COOLDOWN, skipping.");
    }

    return RE::BSEventNotifyControl::kContinue;
}
