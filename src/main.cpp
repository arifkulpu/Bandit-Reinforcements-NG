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
    SKSE::log::info("BanditReinforcementsNG loaded");

    Settings::Load();
    SKSE::log::info("Settings loaded");

    InitListener();

    return true;
}
