#include "LocationEventSink.h"
#include "BanditSpawner.h"
#include "SpawnTracker.h"
#include <SKSE/SKSE.h>

LocationEventSink* LocationEventSink::GetSingleton() {
    static LocationEventSink singleton;
    return &singleton;
}

void LocationEventSink::Register() {
    auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (scriptEventSource) {
        scriptEventSource->AddEventSink<RE::TESCellAttachDetachEvent>(GetSingleton());
        SKSE::log::info("Registered LocationEventSink.");
    }
}

RE::BSEventNotifyControl LocationEventSink::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) {
    if (!a_event || !a_event->reference || !a_event->attached) return RE::BSEventNotifyControl::kContinue;
    
    if (a_event->reference.get() != RE::PlayerCharacter::GetSingleton()) return RE::BSEventNotifyControl::kContinue;

    auto cell = a_event->reference->GetParentCell();
    if(!cell) return RE::BSEventNotifyControl::kContinue;

    auto loc = cell->GetLocation();
    
    // Fallback: cell itself or location
    RE::FormID trackID = loc ? loc->GetFormID() : cell->GetFormID();

    // Lazy Evaluation: Sadece oyuncu yeni hücreye girdiğinde güncelleme yap
    SpawnTracker::GetSingleton()->Update();

    if (loc) {
        auto kwdBanditCamp = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000171DF); // LocTypeBanditCamp
        auto kwdDungeon = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00018A0E);    // LocTypeDungeon
        
        if (kwdBanditCamp && loc->HasKeyword(kwdBanditCamp) || (kwdDungeon && loc->HasKeyword(kwdDungeon))) {
            if (SpawnTracker::GetSingleton()->IsLocationReady(trackID)) {
                BanditSpawner::SpawnBandits(cell);
                SpawnTracker::GetSingleton()->RegisterSpawn(trackID, {});
            }
        }
    } else {
        // If no location, maybe checking cell name? We just check locs for now.
    }

    return RE::BSEventNotifyControl::kContinue;
}
